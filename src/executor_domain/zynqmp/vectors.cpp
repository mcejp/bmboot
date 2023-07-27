#include "bmboot/payload_runtime.hpp"
#include "../executor_lowlevel.hpp"
#include "../../mach/mach_baremetal_defs.hpp"

#include "bspconfig.h"
#include "xipipsu.h"
#include "xscugic.h"
#include "xpseudo_asm.h"

// ************************************************************

#if EL3
#define get_ELR() mfcp(ELR_EL3)
#define get_SPSR() mfcp(SPSR_EL3)
#define EL_STRING "EL3"
#elif EL1_NONSECURE
#define get_ELR() mfcp(ELR_EL1)
#define get_SPSR() mfcp(SPSR_EL1)
#define EL_STRING "EL1"
#endif

using namespace bmboot;
using namespace bmboot::internal;

static auto& ipc_block = *(IpcBlock*) MONITOR_IPC_START;

// ************************************************************

static void fill_crash_info(uintptr_t pc, uintptr_t sp, uint64_t* regs) {
    auto& outbox = ipc_block.executor_to_manager;

    outbox.regs.pc = pc;
    outbox.regs.sp = sp;
    outbox.regs.regs[29] = *regs++;
    outbox.regs.regs[30] = *regs++;

    for (int i = 18; i >= 0; i -= 2) {
        outbox.regs.regs[i] = *regs++;
        outbox.regs.regs[i + 1] = *regs++;
    }
}

// ************************************************************

extern "C" void FIQInterrupt(void)
{
    auto iar = XScuGic_ReadReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_INT_ACK_OFFSET);

#if EL3
    if ((iar & XSCUGIC_ACK_INTID_MASK) == mach::IPI_CH0_GIC_CHANNEL) {
        // acknowledge IPI
        auto source_mask = XIpiPsu_ReadReg(XPAR_PSU_IPI_0_S_AXI_BASEADDR, XIPIPSU_ISR_OFFSET);
        XIpiPsu_WriteReg(XPAR_PSU_IPI_0_S_AXI_BASEADDR, XIPIPSU_ISR_OFFSET, source_mask);

        XScuGic_WriteReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_EOI_OFFSET, iar);

        // IRQ triggered by APU?
        if (source_mask & (1 << mach::IPI_SRC_APU)) {
            // reset monitor -- jump to entry
            _boot();
        }
    }
#endif

    auto fault_address = iar; //get_ELR();
    notifyPayloadCrashed("FIQInterrupt " EL_STRING, fault_address);

    // Even if we're crashing, we acknowledge the interrupt to not upset the GIC which is shared by the entire CPU
    XScuGic_WriteReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_EOI_OFFSET, iar);
    for (;;) {}
}

// ************************************************************

extern "C" void IRQInterrupt(void)
{
    auto iar = XScuGic_ReadReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_INT_ACK_OFFSET);

    auto fault_address = iar; //get_ELR();
    notifyPayloadCrashed("IRQInterrupt " EL_STRING, fault_address);

    // Even if we're crashing, we acknowledge the interrupt to not upset the GIC which is shared by the entire CPU
    XScuGic_WriteReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_EOI_OFFSET, iar);
    for (;;) {}
}

// ************************************************************

// rethink: currently this gets compiled separately for EL1 and EL3. but do we want that?
// shouldn't all EL1 errors trigger a switch into EL3 and be handled there?
//
// also because we can end up here EITHER due to an EL3 issue or an EL1 issue that trapped to EL3
extern "C" void SynchronousInterrupt(uint64_t* the_sp)
{
    auto fault_address = get_ELR();

    saveFpuState(ipc_block.executor_to_manager.fpregs);

    ipc_block.executor_to_manager.regs.pstate = get_SPSR();

    // TODO: specify the layout of saved registers
    // BUG: this does not save all GPRs!
    fill_crash_info(fault_address, (uintptr_t)(the_sp + 32), the_sp);

    notifyPayloadCrashed("SynchronousInterrupt " EL_STRING, fault_address);
    for (;;) {}
}

// ************************************************************

extern "C" void SErrorInterrupt(void)
{
    auto fault_address = get_ELR();
    notifyPayloadCrashed("SErrorInterrupt " EL_STRING, fault_address);
    for (;;) {}
}
