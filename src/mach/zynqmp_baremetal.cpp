#include "../bmboot_internal.hpp"

#include "xil_cache.h"
#include "xpseudo_asm.h"
#include "xreg_cortexa53.h"

namespace bmboot::mach {

static uint32_t read32(size_t offset) {
    return *(uint32_t volatile*)offset;
}

static void write32(size_t offset, uint32_t value) {
    *(uint32_t volatile*)offset = value;
}

void enable_CPU_interrupts() {
    // per function CPUInitialize in BSP
    write32(0xF9020004, 0xF0);
    write32(0xF9020000, 7);

    mtcpsr(mfcpsr() & ~XREG_CPSR_IRQ_ENABLE);
}

void enable_IPI_reception(int src_channel)
{
    // clear any pending request
    write32(0xFF300010, 1<<src_channel);

    // enable channel
    write32(0xFF300018, 1<<src_channel);
}

void flush_icache() {
    Xil_ICacheInvalidate();
}

void setup_interrupt(int ch, int target_cpu) {
    // TODO: should maybe just use Xilinx SDK functions

    const auto gic_dist_ISENABLERn = 0xF9010100;
    auto reg = gic_dist_ISENABLERn + ch / 32 * 4;
    auto mask = (1 << (ch % 32));
    write32(reg, mask);

    const auto gic_dist_ITARGETSRn = 0xF9010800;
    reg = gic_dist_ITARGETSRn + ch / 4 * 4;
    auto shift = (ch % 4) * 8;
    mask = 0xff << shift;
    write32(reg, (read32(reg) & ~mask) | ((1 << target_cpu) << shift));
}

}
