#include "bmboot/payload_runtime.hpp"
#include "../executor_lowlevel.hpp"
#include "../../mach/mach_baremetal.hpp"

#include "bspconfig.h"
#include "xipipsu.h"
#include "xscugic.h"
#include "xpseudo_asm.h"

// ************************************************************

#if EL3
#define get_ELR() mfcp(ELR_EL3)
#define get_SPSR() mfcp(SPSR_EL3)
#define EL_STRING "EL3"

enum {
    EC_SMC = 0b010111,
};
#elif EL1_NONSECURE
#define get_ELR() mfcp(ELR_EL1)
#define get_SPSR() mfcp(SPSR_EL1)
#define EL_STRING "EL1"
#endif

using namespace bmboot;
using namespace bmboot::internal;

static auto& ipc_block = *(IpcBlock*) MONITOR_IPC_START;

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

#if EL1_NONSECURE
    if ((iar & XSCUGIC_ACK_INTID_MASK) == mach::CNTPNS_IRQ_CHANNEL) {
        // Private timer interrupt

        if ((mfcp(CNTP_CTL_EL0) & 4) != 0)  // check that the timer is really signalled -- just for good measure
        {
            mach::handleTimerIrq();
        }

        XScuGic_WriteReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_EOI_OFFSET, iar);
        return;
    }
#endif

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
#if EL3
    auto ec = (mfcp(ESR_EL3) >> 26) & 0b111111;

    if (ec == EC_SMC)
    {
        switch (saved_regs.regs[0])
        {
            case SMC_NOTIFY_PAYLOAD_STARTED:
                notifyPayloadStarted();
                break;

            case SMC_NOTIFY_PAYLOAD_CRASHED:
                // TODO: this call shall not return back to EL1
                notifyPayloadCrashed((char const*) saved_regs.regs[1], (uintptr_t) saved_regs.regs[2]);
                break;

            case SMC_START_PERIODIC_INTERRUPT:
                // stop timer if already running
                mtcp(CNTP_CTL_EL0, 0);

                mach::enablePrivatePeripheralInterrupt(mach::CNTPNS_IRQ_CHANNEL);

                // configure timer & start it
                mtcp(CNTP_TVAL_EL0, saved_regs.regs[1]);
                mtcp(CNTP_CTL_EL0, 1);          // TODO: magic number!
                break;

            case SMC_STOP_PERIODIC_INTERRUPT:
                // TOOD: disable IRQ etc.
                mtcp(CNTP_CTL_EL0, 0);
                break;

            case SMC_WRITE_STDOUT:
                saved_regs.regs[0] = writeToStdout((void const*) saved_regs.regs[1], (size_t) saved_regs.regs[2]);
                break;

            default:
                notifyPayloadCrashed("SMC " EL_STRING, saved_regs.regs[0]);
                for (;;) {}
        }

        return;
    }
#endif

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
