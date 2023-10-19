#include "platform_interrupt_controller.hpp"
#include "executor.hpp"
#include "executor_asm.hpp"
#include "monitor_asm.hpp"
#include "zynqmp_executor.hpp"

#include "bspconfig.h"
#include "xipipsu.h"
#include "xscugic.h"
#include "xpseudo_asm.h"

// This is a hack to get access to notifyPayloadCrashed
#include "bmboot/payload_runtime.hpp"

// ************************************************************

#define EL_STRING "EL3"

enum {
    EC_SMC = 0b010111,
};

using namespace bmboot;
using namespace bmboot::internal;

// FIXME: we call notifyPayloadCrashed even if it is the *monitor* crashing

// ************************************************************

extern "C" void FIQInterrupt(void)
{
    auto iar = XScuGic_ReadReg(XPAR_SCUGIC_0_CPU_BASEADDR, XSCUGIC_INT_ACK_OFFSET);

    auto my_ipi = mach::getIpiChannelForCpu(internal::getCpuIndex());

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
extern "C" void SynchronousInterrupt(Aarch64_Regs& saved_regs)
{
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

                // set to Group1 (routed to IRQ, therefore EL1)
                platform::enablePrivatePeripheralInterrupt(mach::CNTPNS_IRQ_CHANNEL,
                                                           platform::InterruptGroup::group1_irq_el1,
                                                           platform::MonitorInterruptPriority::payloadMaxPriorityValue);

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

            case SMC_ZYNQMP_GIC_SPI_CONFIGURE_AND_ENABLE: {
                int requestedPriority = saved_regs.regs[2];

                if (requestedPriority < (int) platform::MonitorInterruptPriority::payloadMinPriorityValue ||
                    requestedPriority > (int) platform::MonitorInterruptPriority::payloadMaxPriorityValue) {
                    break;
                }

                platform::enableSharedPeripheralInterruptAndRouteToCpu(saved_regs.regs[1],
                                                                       platform::InterruptTrigger::edge,
                                                                       internal::getCpuIndex(),
                                                                       platform::InterruptGroup::group1_irq_el1,
                                                                        (platform::MonitorInterruptPriority) requestedPriority);
                break;
            }

            default:
                notifyPayloadCrashed("SMC " EL_STRING, saved_regs.regs[0]);
                for (;;) {}
        }

        return;
    }

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
