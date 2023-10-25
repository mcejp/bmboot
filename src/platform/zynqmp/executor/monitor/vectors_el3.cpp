#include "executor.hpp"
#include "executor_asm.hpp"
#include "monitor_internal.hpp"
#include "platform_interrupt_controller.hpp"
#include "zynqmp_executor.hpp"

#include "xipipsu.h"
#include "xscugic.h"

// ************************************************************

enum {
    EC_SMC = 0b010111,
};

using namespace bmboot;
using namespace bmboot::internal;

// ************************************************************

extern "C" void FIQInterrupt(void)
{
    auto iar = XScuGic_ReadReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_INT_ACK_OFFSET);

    auto my_ipi = mach::getIpiChannelForCpu(getCpuIndex());

    if ((iar & XSCUGIC_ACK_INTID_MASK) == mach::getInterruptIdForIpi(my_ipi)) {
        // acknowledge IPI
        auto ipi_base = mach::getIpiBaseAddress(my_ipi);
        auto source_mask = XIpiPsu_ReadReg(ipi_base, XIPIPSU_ISR_OFFSET);
        XIpiPsu_WriteReg(ipi_base, XIPIPSU_ISR_OFFSET, source_mask);

        XScuGic_WriteReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_EOI_OFFSET, iar);

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
    XScuGic_WriteReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_EOI_OFFSET, iar);
    for (;;) {}
}

// ************************************************************

// Should never arrive to EL3
extern "C" void IRQInterrupt(void)
{
    auto iar = XScuGic_ReadReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_INT_ACK_OFFSET);

    auto fault_address = iar; //get_ELR();
    reportCrash(CrashingEntity::monitor, "Monitor IRQInterrupt", fault_address);

    // Even if we're crashing, we acknowledge the interrupt to not upset the GIC which is shared by the entire CPU
    XScuGic_WriteReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_EOI_OFFSET, iar);
    for (;;) {}
}

// ************************************************************

// rethink: currently this gets compiled separately for EL1 and EL3. but do we want that?
// shouldn't all EL1 errors trigger a switch into EL3 and be handled there?
//
// also because we can end up here EITHER due to an EL3 issue or an EL1 issue that trapped to EL3
extern "C" void SynchronousInterrupt(Aarch64_Regs& saved_regs)
{
    auto ec = (mfcp(ESR_EL3) >> 26) & 0b111111;

    if (ec == EC_SMC)
    {
        handleSmc(saved_regs);
        return;
    }

    auto fault_address = get_ELR();

    auto& ipc_block = getIpcBlock();
    saveFpuState(ipc_block.executor_to_manager.fpregs);

    ipc_block.executor_to_manager.regs = saved_regs;
    reportCrash(CrashingEntity::monitor, "Monitor SynchronousInterrupt", fault_address);
    for (;;) {}
}

// ************************************************************

extern "C" void SErrorInterrupt(void)
{
    auto fault_address = get_ELR();
    reportCrash(CrashingEntity::monitor, "Monitor SErrorInterrupt", fault_address);
    for (;;) {}
}
