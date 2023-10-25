#include "bmboot/payload_runtime.hpp"
#include "executor.hpp"
#include "executor_asm.hpp"
#include "payload_runtime_internal.hpp"
#include "zynqmp_executor.hpp"

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

    if (int_id >= GIC_MIN_USER_INTERRUPT_ID &&
        int_id <= GIC_MAX_USER_INTERRUPT_ID &&
        user_interrupt_handlers[int_id - GIC_MIN_USER_INTERRUPT_ID])
    {
        // Back up SPSR and ELR before re-enabling interrupts
        //
        // Equivalent macro in Xilinx SDK (but looks quite sketchy with the stack usage):
        // https://github.com/Xilinx/embeddedsw/blob/8fca1ac929453ba06613b5417141483b4c2d8cf3/lib/bsp/standalone/src/arm/common/xil_exception.h#L371
        uint64_t spsr = mfcp(SPSR_EL1);
        uint64_t elr = mfcp(ELR_EL1);
        mtcpsr(mfcpsr() & ~XREG_CPSR_IRQ_ENABLE);           // clear IRQ *mask* bit (mis-named constant)

        user_interrupt_handlers[int_id - GIC_MIN_USER_INTERRUPT_ID]();

        mtcpsr(mfcpsr() | XREG_CPSR_IRQ_ENABLE);           // set IRQ *mask* bit (mis-named constant)
        mtcp(SPSR_EL1, spsr);
        mtcp(ELR_EL1, elr);

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

    auto& ipc_block = getIpcBlock();
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
