//! @file
//! @brief  Machine-specific functions, bare metal
//! @author Martin Cejp

#include "mach_baremetal.hpp"
#include "../bmboot_internal.hpp"

#include "bspconfig.h"
#include "xil_cache.h"
#include "xpseudo_asm.h"
#include "xreg_cortexa53.h"
#include "xscugic.h"

// TODO: NO MAGIC NUMBERS

using namespace bmboot::internal;
using namespace bmboot::mach;

static bool interrupt_routed_to_el1[(GIC_MAX_USER_INTERRUPT_ID + 1) - GIC_MIN_USER_INTERRUPT_ID];

// ************************************************************

static uint32_t read32(size_t offset) {
    return *(uint32_t volatile*)offset;
}

static void write32(size_t offset, uint32_t value) {
    *(uint32_t volatile*)offset = value;
}

// ************************************************************
#if EL3
void bmboot::mach::disablePrivatePeripheralInterrupt(int ch) {
    // TODO: should maybe just use Xilinx SDK functions

    const auto gic_dist_ICENABLERn = 0xF9010180;
    auto reg = gic_dist_ICENABLERn;
    auto mask = (1 << ch);
    write32(reg, mask);
}
#endif
// ************************************************************

#if EL3
void bmboot::mach::enableCpuInterrupts() {
    // per function CPUInitialize in BSP
    write32(0xF9020004, 0xFF);                          // Priority Mask Register: maximum value
    write32(0xF9020000, XSCUGIC_CNTR_FIQEN_MASK |
                        XSCUGIC_CNTR_ACKCTL_MASK |
                        XSCUGIC_CNTR_EN_NS_MASK |
                        XSCUGIC_CNTR_EN_S_MASK);
    // note that the meaning of the bits in this register changes based on S/NS access

    // very misleading -- these are in fact *mask* bits, not *enable* bits
    mtcpsr(mfcpsr() & ~XREG_CPSR_IRQ_ENABLE & ~XREG_CPSR_FIQ_ENABLE);
}
#endif

// ************************************************************
#if EL3
void bmboot::mach::enableIpiReception(int src_channel)
{
    // clear any pending request
    write32(0xFF300010, 1<<src_channel);

    // enable channel
    write32(0xFF300018, 1<<src_channel);
}
#endif
// ************************************************************

void bmboot::mach::flushICache() {
    Xil_ICacheInvalidate();
}

// ************************************************************
#if EL3
static void setGroupForInterruptChannel(int int_id, InterruptGroup group)
{
    const auto gic_dist_IGROUPRn = 0xF9010080;
    auto reg = gic_dist_IGROUPRn + int_id / 32 * 4;
    auto mask = (1 << (int_id % 32));

    if (group == InterruptGroup::group0_fiq_el3)
    {
        write32(reg, read32(reg) & ~mask);
    }
    else
    {
        write32(reg, read32(reg) | mask);
    }

    // Update interrupt_routed_to_el1 if the interrupt ID falls in its range
    if (int_id >= GIC_MIN_USER_INTERRUPT_ID && int_id <= GIC_MAX_USER_INTERRUPT_ID)
    {
        interrupt_routed_to_el1[int_id - GIC_MIN_USER_INTERRUPT_ID] = (group == InterruptGroup::group1_irq_el1);
    }
}

void bmboot::mach::enablePrivatePeripheralInterrupt(int ch, InterruptGroup group, MonitorInterruptPriority priority) {
    // TODO: should maybe just use Xilinx SDK functions

    setGroupForInterruptChannel(ch, group);

    // TODO: apparently this register can be byte-addressed
    const auto gic_dist_IPRIORITYRn = 0xF9010400;
    auto reg = gic_dist_IPRIORITYRn + ch / 4 * 4;
    auto shift = (ch % 4) * 8;
    auto mask = 0xff << shift;
    write32(reg, (read32(reg) & ~mask) | ((int)priority << shift));

    // TODO: should clear interrupt pending flag, just for a good measure?

    const auto gic_dist_ISENABLERn = 0xF9010100;
    reg = gic_dist_ISENABLERn;
    mask = (1 << ch);
    write32(reg, mask);
}
#endif
// ************************************************************
#if EL3
void bmboot::mach::enableSharedPeripheralInterruptAndRouteToCpu(int ch,
                                                                InterruptTrigger trigger,
                                                                int target_cpu,
                                                                InterruptGroup group,
                                                                MonitorInterruptPriority priority) {
    // TODO: should maybe just use Xilinx SDK functions

    setGroupForInterruptChannel(ch, group);

    // TODO: apparently these can be byte-accessed
    const auto gic_dist_IPRIORITYRn = 0xF9010400;
    auto reg = gic_dist_IPRIORITYRn + ch / 4 * 4;
    auto shift = (ch % 4) * 8;
    auto mask = 0xff << shift;
    write32(reg, (read32(reg) & ~mask) | ((int)priority << shift));

    const auto gic_dist_ITARGETSRn = 0xF9010800;
    reg = gic_dist_ITARGETSRn + ch / 4 * 4;
    shift = (ch % 4) * 8;
    mask = 0xff << shift;
    write32(reg, (read32(reg) & ~mask) | ((1 << target_cpu) << shift));

    // ARM IHI 0048B.b; 4.3.13 Interrupt Configuration Registers, GICD_ICFGRn
    const auto gic_dist_ICFGRn = 0xF9010C00;
    reg = gic_dist_ICFGRn + ch / 16 * 4;
    shift = (ch % 16) * 2;
    mask = 0b10 << shift;       // only touching the MSB -- LSB is reserved

    if (trigger == InterruptTrigger::level)
    {
        write32(reg, (read32(reg) & ~mask) | (0b00 << shift));
    }
    else if (trigger == InterruptTrigger::edge)
    {
        write32(reg, (read32(reg) & ~mask) | (0b10 << shift));
    }

    // Clear pending and active statuses.
    // We don't know what happened in the past, a previous payload might have been terminated during the handling this
    // interrupt, in which case the interrupt would remain in an Active state in the GIC.

    const auto gic_dist_ICPENDRn = 0xF9010280;
    reg = gic_dist_ICPENDRn + ch / 32 * 4;
    mask = (1 << (ch % 32));
    write32(reg, mask);

    const auto gic_dist_ICACTIVERn = 0xF9010380;
    reg = gic_dist_ICACTIVERn + ch / 32 * 4;
    mask = (1 << (ch % 32));
    write32(reg, mask);

    // This does not seem necessary, the bit reads as 1 anyway -- at least for the IPI (SPI #67)... curious
    const auto gic_dist_ISENABLERn = 0xF9010100;
    reg = gic_dist_ISENABLERn + ch / 32 * 4;
    mask = (1 << (ch % 32));
    write32(reg, mask);
}
#endif
// ************************************************************
#if EL3
void bmboot::mach::teardownEl1Interrupts()
{
    mach::disablePrivatePeripheralInterrupt(mach::CNTPNS_IRQ_CHANNEL);

    for (int int_id = GIC_MIN_USER_INTERRUPT_ID; int_id <= GIC_MAX_USER_INTERRUPT_ID; int_id++)
    {
        if (interrupt_routed_to_el1[int_id - GIC_MIN_USER_INTERRUPT_ID])
        {
            const auto gic_dist_ICENABLERn = 0xF9010180;
            auto reg = gic_dist_ICENABLERn + int_id / 32 * 4;
            auto mask = (1 << (int_id % 32));
            write32(reg, mask);

            interrupt_routed_to_el1[int_id - GIC_MIN_USER_INTERRUPT_ID] = false;
        }
    }
}
#endif
