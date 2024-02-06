#include "bmboot/payload_runtime.hpp"
#include "executor.hpp"
#include "executor_asm.hpp"
#include "payload_runtime_internal.hpp"
#include "zynqmp.hpp"
#include "zynqmp_executor.hpp"

#include "xpseudo_asm.h"

// ************************************************************

using namespace bmboot;
using namespace bmboot::internal;
using namespace zynqmp;

// ************************************************************

extern "C" void FIQInterrupt(void)
{
    auto iar = scugic::GICC->IAR;

    auto fault_address = iar; //get_ELR();
    notifyPayloadCrashed("EL1 FIQInterrupt", fault_address);

    // Even if we're crashing, we acknowledge the interrupt to not upset the GIC which is shared by the entire CPU
    scugic::GICC->EOIR = iar;
    for (;;) {}
}

// ************************************************************

extern "C" void IRQInterrupt(void)
{
    auto iar = scugic::GICC->IAR;
    auto interrupt_id = (iar & arm::gicv2::GICC::IAR_INTERRUPT_ID_MASK);

    if (interrupt_id >= GIC_MIN_USER_INTERRUPT_ID &&
            interrupt_id <= GIC_MAX_USER_INTERRUPT_ID &&
        user_interrupt_handlers[interrupt_id - GIC_MIN_USER_INTERRUPT_ID])
    {
        // Back up SPSR and ELR before re-enabling interrupts
        //
        // Equivalent macro in Xilinx SDK (but looks quite sketchy with the stack usage):
        // https://github.com/Xilinx/embeddedsw/blob/8fca1ac929453ba06613b5417141483b4c2d8cf3/lib/bsp/standalone/src/arm/common/xil_exception.h#L371
        uint64_t spsr = readSysReg(SPSR_EL1);
        uint64_t elr = readSysReg(ELR_EL1);
        mtcpsr(mfcpsr() & ~XREG_CPSR_IRQ_ENABLE);           // clear IRQ *mask* bit (mis-named constant)

        user_interrupt_handlers[interrupt_id - GIC_MIN_USER_INTERRUPT_ID]();

        mtcpsr(mfcpsr() | XREG_CPSR_IRQ_ENABLE);           // set IRQ *mask* bit (mis-named constant)
        writeSysReg(SPSR_EL1, spsr);
        writeSysReg(ELR_EL1, elr);

        scugic::GICC->EOIR = iar;
        return;
    }

    // Q: What to do if we get IRQInterrupt EL1 0x3ff?
    // A: It means that an interrupt was signalled, but by the time we got around to checking it, it was gone.
    //    For example, it might have been configured as level sensitive, while it was meant to be edge-sensitive

    auto fault_address = iar; //get_ELR();
    notifyPayloadCrashed("EL1 IRQInterrupt", fault_address);

    // Even if we're crashing, we acknowledge the interrupt to not upset the GIC which is shared by the entire CPU
    scugic::GICC->EOIR = iar;
    for (;;) {}
}

// ************************************************************

extern "C" void SynchronousInterrupt(Aarch64_Regs& saved_regs)
{
    auto fault_address = readSysReg(ELR_EL1);

    auto& ipc_block = getIpcBlock();
    saveFpuState(ipc_block.executor_to_manager.fpregs);

    ipc_block.executor_to_manager.regs = saved_regs;
    notifyPayloadCrashed("EL1 SynchronousInterrupt", fault_address);
    for (;;) {}
}

// ************************************************************

extern "C" void SErrorInterrupt(void)
{
    auto fault_address = readSysReg(ELR_EL1);
    notifyPayloadCrashed("EL1 SErrorInterrupt", fault_address);
    for (;;) {}
}
