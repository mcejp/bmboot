//! @file
//! @brief  Machine-specific functions, bare metal
//! @author Martin Cejp

#include "bmboot_internal.hpp"
#include "executor.hpp"
#include "platform_interrupt_controller.hpp"
#include "zynqmp_executor.hpp"

#include "bspconfig.h"
#include "xil_cache.h"
#include "xpseudo_asm.h"
#include "xreg_cortexa53.h"
#include "xscugic.h"

// TODO: NO MAGIC NUMBERS

using namespace bmboot::internal;
using namespace bmboot::mach;
using namespace bmboot::platform;

static bool interrupt_routed_to_el1[(GIC_MAX_USER_INTERRUPT_ID + 1) - GIC_MIN_USER_INTERRUPT_ID];

// ************************************************************

static uint32_t read32(size_t offset) {
    return *(uint32_t volatile*)offset;
}

static void write32(size_t offset, uint32_t value) {
    *(uint32_t volatile*)offset = value;
}

// ************************************************************

int bmboot::mach::getInterruptIdForIpi(IpiChannel ipi_channel)
{
    // Mapping of IPI channels to interrupt IDs can be found in UG1085, Table 13-1: System Interrupts
    switch (ipi_channel)
    {
        case IpiChannel::ch0: return 67;
        case IpiChannel::ch1: return 65;
        case IpiChannel::ch2: return 66;
        case IpiChannel::ch7: return 61;
        case IpiChannel::ch8: return 62;
        case IpiChannel::ch9: return 63;
        case IpiChannel::ch10: return 64;
    }
}

// ************************************************************

static void enableCpuInterrupts() {
    // Upper 4 bits of priority value determine priority for preemption purposes
    XScuGic_WriteReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_BIN_PT_OFFSET, 0x03);
    // Priority Mask Register: maximum value
    XScuGic_WriteReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_CPU_PRIOR_OFFSET, 0xFF);

    // Note that the meaning of the bits in this register changes based on S/NS access
    // We execute this in EL3 aka Secure
    XScuGic_WriteReg(XPAR_SCUGIC_0_CPU_BASEADDR,
                     XSCUGIC_CONTROL_OFFSET,
                     (XSCUGIC_CNTR_FIQEN_MASK |
                      XSCUGIC_CNTR_ACKCTL_MASK |
                      XSCUGIC_CNTR_EN_NS_MASK |
                      XSCUGIC_CNTR_EN_S_MASK));

    // Enable FIQ & IRQ.
    // By design, in the monitor we only receive FIQ, but since all IRQ handler code is already in place, just in case
    // we mess up and get one, we might as well report it, instead of falling into some kind of stub handler.

    // very misleading -- these are in fact *mask* bits, not *enable* bits
    mtcpsr(mfcpsr() & ~XREG_CPSR_IRQ_ENABLE & ~XREG_CPSR_FIQ_ENABLE);
}

// ************************************************************

static void enableIpiReception(IpiChannel src_channel, IpiChannel dst_channel)
{
    uintptr_t base_address = getIpiBaseAddress(dst_channel);

    // clear any pending request
    write32(base_address + 0x10, getIpiPeerMask(src_channel));

    // enable channel
    write32(base_address + 0x18, getIpiPeerMask(src_channel));
}

// ************************************************************

void bmboot::platform::flushICache() {
    Xil_ICacheInvalidate();
}

// ************************************************************

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

void bmboot::platform::configurePrivatePeripheralInterrupt(int ch, InterruptGroup group, MonitorInterruptPriority priority) {
    // TODO: should maybe just use Xilinx SDK functions

    setGroupForInterruptChannel(ch, group);

    // TODO: apparently this register can be byte-addressed
    const auto gic_dist_IPRIORITYRn = 0xF9010400;
    auto reg = gic_dist_IPRIORITYRn + ch / 4 * 4;
    auto shift = (ch % 4) * 8;
    auto mask = 0xff << shift;
    write32(reg, (read32(reg) & ~mask) | ((int)priority << shift));
}

// ************************************************************

void bmboot::platform::configureSharedPeripheralInterruptAndRouteToCpu(int ch,
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
}

// ************************************************************

void bmboot::platform::disableInterrupt(int interrupt_id)
{
    const auto gic_dist_ICENABLERn = 0xF9010180;
    auto reg = gic_dist_ICENABLERn + interrupt_id / 32 * 4;
    auto mask = (1 << (interrupt_id % 32));
    write32(reg, mask);
}

// ************************************************************

void bmboot::platform::enableInterrupt(int interrupt_id)
{
    // Clear pending and active statuses.
    // We don't know what happened in the past, a previous payload might have been terminated during the handling this
    // interrupt, in which case the interrupt would remain in an Active state in the GIC.

    const auto gic_dist_ICPENDRn = 0xF9010280;
    auto reg = gic_dist_ICPENDRn + interrupt_id / 32 * 4;
    auto mask = (1 << (interrupt_id % 32));
    write32(reg, mask);

    const auto gic_dist_ICACTIVERn = 0xF9010380;
    reg = gic_dist_ICACTIVERn + interrupt_id / 32 * 4;
    mask = (1 << (interrupt_id % 32));
    write32(reg, mask);

    const auto gic_dist_ISENABLERn = 0xF9010100;
    reg = gic_dist_ISENABLERn + interrupt_id / 32 * 4;
    mask = (1 << (interrupt_id % 32));
    write32(reg, mask);
}

// ************************************************************

void bmboot::platform::teardownEl1Interrupts()
{
    platform::disableInterrupt(mach::CNTPNS_IRQ_CHANNEL);

    for (int int_id = GIC_MIN_USER_INTERRUPT_ID; int_id <= GIC_MAX_USER_INTERRUPT_ID; int_id++)
    {
        if (interrupt_routed_to_el1[int_id - GIC_MIN_USER_INTERRUPT_ID])
        {
            disableInterrupt(int_id);

            interrupt_routed_to_el1[int_id - GIC_MIN_USER_INTERRUPT_ID] = false;
        }
    }
}

// ************************************************************

void bmboot::platform::setupInterrupts()
{
    enableCpuInterrupts();

    // Enable IPI reception
    const auto my_ipi_channel = getIpiChannelForCpu(getCpuIndex());
    const auto ipi_interrupt_id = getInterruptIdForIpi(my_ipi_channel);

    platform::configureSharedPeripheralInterruptAndRouteToCpu(ipi_interrupt_id,
                                                              platform::InterruptTrigger::unchanged,
                                                              getCpuIndex(),
                                                              platform::InterruptGroup::group0_fiq_el3,
                                                              platform::MonitorInterruptPriority::m7_max);

    platform::enableInterrupt(ipi_interrupt_id);

    enableIpiReception(IPI_SRC_BMBOOT_MANAGER, my_ipi_channel);
}
