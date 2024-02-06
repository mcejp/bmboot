#include "armv8a.hpp"
#include "executor.hpp"
#include "executor_asm.hpp"
#include "monitor_internal.hpp"
#include "platform_interrupt_controller.hpp"
#include "zynqmp.hpp"
#include "zynqmp_executor.hpp"

// ************************************************************

enum {
    EC_SMC = 0b010111,
};

using namespace bmboot;
using namespace bmboot::internal;
using namespace zynqmp;

// ************************************************************

extern "C" void FIQInterrupt(void)
{
    auto iar = scugic::GICC->IAR;
    auto interrupt_id = (iar & arm::gicv2::GICC::IAR_INTERRUPT_ID_MASK);

    auto my_ipi = mach::getIpiChannelForCpu(getCpuIndex());

    if (interrupt_id == mach::getInterruptIdForIpi(my_ipi)) {
        auto ipi = mach::getIpi(my_ipi);

        // acknowledge IPI
        auto source_mask = ipi->ISR;
        ipi->ISR = source_mask;

        // acknowledge interrupt
        scugic::GICC->EOIR = iar;

        // IRQ triggered by APU?
        if (source_mask & mach::getIpiPeerMask(mach::IPI_SRC_BMBOOT_MANAGER)) {
            platform::teardownEl1Interrupts();

            // reset monitor by jumping to entry point
            _boot();
        }
    }

    auto fault_address = iar; //get_ELR();
    reportCrash(CrashingEntity::monitor, "Monitor FIQInterrupt", fault_address);

    // Even if we're crashing, we acknowledge the interrupt to not upset the GIC which is shared by the entire CPU
    scugic::GICC->EOIR = iar;
    for (;;) {}
}

// ************************************************************

// Should never arrive to EL3
extern "C" void IRQInterrupt(void)
{
    auto iar = scugic::GICC->IAR;

    auto fault_address = iar; //get_ELR();
    reportCrash(CrashingEntity::monitor, "Monitor IRQInterrupt", fault_address);

    // Even if we're crashing, we acknowledge the interrupt to not upset the GIC which is shared by the entire CPU
    scugic::GICC->EOIR = iar;
    for (;;) {}
}

// ************************************************************

extern "C" void SynchronousInterrupt(Aarch64_Regs& saved_regs)
{
    auto ec = (readSysReg(ESR_EL3) >> 26) & 0b111111;

    if (ec == EC_SMC)
    {
        handleSmc(saved_regs);
        return;
    }

    auto fault_address = readSysReg(ELR_EL3);

    auto& ipc_block = getIpcBlock();
    saveFpuState(ipc_block.executor_to_manager.fpregs);

    ipc_block.executor_to_manager.regs = saved_regs;
    reportCrash(CrashingEntity::monitor, "Monitor SynchronousInterrupt", fault_address);
    for (;;) {}
}

// ************************************************************

extern "C" void SErrorInterrupt(void)
{
    auto fault_address = readSysReg(ELR_EL3);
    reportCrash(CrashingEntity::monitor, "Monitor SErrorInterrupt", fault_address);
    for (;;) {}
}
