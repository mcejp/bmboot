//! @file
//! @brief  Machine-specific functions, bare metal
//! @author Martin Cejp

#include "mach_baremetal.hpp"

#include "bspconfig.h"
#include "xil_cache.h"
#include "xpseudo_asm.h"
#include "xreg_cortexa53.h"
#include "xscugic.h"

// TODO: NO MAGIC NUMBERS

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
    write32(0xF9020004, 0xF0);
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
void bmboot::mach::enablePrivatePeripheralInterrupt(int ch) {
    // TODO: should maybe just use Xilinx SDK functions

    const int priority = 0x80;      // higher value = lower priority

    // set to Group1 (routed to IRQ, therefore EL1)
    const auto gic_dist_IGROUPRn = 0xF9010080;
    auto reg = gic_dist_IGROUPRn + ch / 32 * 4;
    auto mask = (1 << (ch % 32));
    write32(reg, read32(reg) | mask);

    // TODO: apparently this register can be byte-addressed
    const auto gic_dist_IPRIORITYRn = 0xF9010400;
    reg = gic_dist_IPRIORITYRn + ch / 4 * 4;
    auto shift = (ch % 4) * 8;
    mask = 0xff << shift;
    write32(reg, (read32(reg) & ~mask) | (priority << shift));

    const auto gic_dist_ISENABLERn = 0xF9010100;
    reg = gic_dist_ISENABLERn;
    mask = (1 << ch);
    write32(reg, mask);
}
#endif
// ************************************************************
#if EL3
void bmboot::mach::enableSharedPeripheralInterruptAndRouteToCpu(int ch, int target_cpu) {
    // TODO: should maybe just use Xilinx SDK functions

    // set to Group0 (routed to FIQ, therefore EL3)
    // apparently this is not the default even though ARM docs would make it seem so
    const auto gic_dist_IGROUPRn = 0xF9010080;
    auto reg = gic_dist_IGROUPRn + ch / 32 * 4;
    auto mask = (1 << (ch % 32));
    write32(reg, read32(reg) & ~mask);

    // TODO: apparently these can be byte-accessed
    const auto gic_dist_IPRIORITYRn = 0xF9010400;
    reg = gic_dist_IPRIORITYRn + ch / 4 * 4;
    auto shift = (ch % 4) * 8;
    mask = 0xff << shift;
    auto prio = 0x20;
    write32(reg, (read32(reg) & ~mask) | ((prio) << shift));

    const auto gic_dist_ITARGETSRn = 0xF9010800;
    reg = gic_dist_ITARGETSRn + ch / 4 * 4;
    shift = (ch % 4) * 8;
    mask = 0xff << shift;
    write32(reg, (read32(reg) & ~mask) | ((1 << target_cpu) << shift));

    // This does not seem necessary, the bit reads as 1 anyway -- at least for the IPI (SPI #67)... curious
    const auto gic_dist_ISENABLERn = 0xF9010100;
    reg = gic_dist_ISENABLERn + ch / 32 * 4;
    mask = (1 << (ch % 32));
    write32(reg, mask);
}
#endif
