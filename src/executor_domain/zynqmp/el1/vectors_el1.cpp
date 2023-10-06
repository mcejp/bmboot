#include "bmboot/payload_runtime.hpp"
#include "payload_runtime_internal.hpp"
#include "../../executor_lowlevel.hpp"
#include "../../../mach/mach_baremetal.hpp"

#include "bspconfig.h"
#include "xipipsu.h"
#include "xscugic.h"
#include "xpseudo_asm.h"

// ************************************************************

#if not(EL1_NONSECURE)
#error This file is only relevant when building the payload runtime
#endif

#define get_ELR() mfcp(ELR_EL1)
#define EL_STRING "EL1"

using namespace bmboot;
using namespace bmboot::internal;

static auto& ipc_block = *(IpcBlock*) MONITOR_IPC_START;

// ************************************************************

extern "C" void FIQInterrupt(void)
{
    auto iar = XScuGic_ReadReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_INT_ACK_OFFSET);

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
    auto int_id = (iar & XSCUGIC_ACK_INTID_MASK);

    if (int_id == mach::CNTPNS_IRQ_CHANNEL) {
        // Private timer interrupt

        if ((mfcp(CNTP_CTL_EL0) & 4) != 0)  // check that the timer is really signalled -- just for good measure
        {
            mach::handleTimerIrq();
        }

        XScuGic_WriteReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_EOI_OFFSET, iar);
        return;
    }
    else if (int_id >= GIC_MIN_USER_INTERRUPT_ID &&
             int_id <= GIC_MAX_USER_INTERRUPT_ID &&
             user_interrupt_handlers[int_id - GIC_MIN_USER_INTERRUPT_ID])
    {
        user_interrupt_handlers[int_id - GIC_MIN_USER_INTERRUPT_ID]();

        XScuGic_WriteReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_EOI_OFFSET, iar);
        return;
    }

    // Q: What to do if we get IRQInterrupt EL1 0x3ff?
    // A: It means that an interrupt was signalled, but by the time we got around to checking it, it was gone.
    //    For example, it might have been configured as level sensitive, while it was meant to be edge-sensitive

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
extern "C" void SynchronousInterrupt(Aarch64_Regs& saved_regs)
{
    auto fault_address = get_ELR();

    saveFpuState(ipc_block.executor_to_manager.fpregs);

    ipc_block.executor_to_manager.regs = saved_regs;
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
