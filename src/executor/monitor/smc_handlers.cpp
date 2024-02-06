#include "armv8a.hpp"
#include "bmboot.hpp"
#include "executor.hpp"
#include "executor_asm.hpp"
#include "monitor_internal.hpp"
#include "platform_interrupt_controller.hpp"

#include <cstring>

// ************************************************************

using namespace bmboot;
using namespace bmboot::internal;

// ************************************************************

static void reportPayloadStarted()
{
    getIpcBlock().executor_to_manager.state = DomainState::running_payload;
}

// ************************************************************

static int writeToStdout(void const* data, size_t size)
{
    auto& ipc_block = getIpcBlock();
    auto& outbox = ipc_block.executor_to_manager;

    // Here we want to be very conservative to be absolutely certain we will not overflow the buffer
    if (outbox.stdout_wrpos >= sizeof(outbox.stdout_buf)) {
        auto weird = outbox.stdout_wrpos;
        outbox.stdout_wrpos = 0;
//        printf("unexpected dom_stdout_wrpos %zx, reset to 0\n", weird);

        // printf bloats the monitor binary too much, so we cannot easily report the observed value
        char const message[] = "unexpected dom_stdout_wrpos, reset to 0\n";
        writeToStdout(message, sizeof(message) - 1);
    }

    auto data_bytes = static_cast<uint8_t const*>(data);
    size_t wrote = 0;

    // TODO: can be replaced with a more efficient implementation instead of writing one byte at a time
    while (size > 0) {
        auto wrpos_new = (outbox.stdout_wrpos + 1) % sizeof(outbox.stdout_buf);
        if (wrpos_new == ipc_block.manager_to_executor.stdout_rdpos) {
            // Buffer full, abort!
            // However, we must lie about number of characters written, otherwise stdout error flag will be set and
            // printf will refuse to print any more
            // (on a non-rt OS, a write to clogged stdout would just block instead)

            wrote += size;
            break;
        }

        outbox.stdout_buf[outbox.stdout_wrpos] = *data_bytes;
        outbox.stdout_wrpos = wrpos_new;

        wrote++;
        data_bytes++;
        size--;
    }

    return wrote;
}

// ************************************************************

void internal::handleSmc(Aarch64_Regs& saved_regs)
{
    switch (saved_regs.regs[0])
    {
        case SMC_NOTIFY_PAYLOAD_STARTED:
            reportPayloadStarted();
            break;

        case SMC_NOTIFY_PAYLOAD_CRASHED:
            reportCrash(CrashingEntity::payload, (char const*) saved_regs.regs[1], (uintptr_t) saved_regs.regs[2]);
            // TODO: returns to EL1 so that we can be interrupted by IPI. should it be like that, though?
            break;

        case SMC_WRITE_STDOUT:
            saved_regs.regs[0] = writeToStdout((void const*) saved_regs.regs[1], (size_t) saved_regs.regs[2]);
            break;

        case SMC_ZYNQMP_GIC_IRQ_CONFIGURE: {
            int interruptId = saved_regs.regs[1];
            int requestedPriority = saved_regs.regs[2];

            if (requestedPriority < (int) platform::MonitorInterruptPriority::payloadMinPriorityValue ||
                requestedPriority > (int) platform::MonitorInterruptPriority::payloadMaxPriorityValue) {
                break;
            }

            if (interruptId >= 16 && interruptId < 32)
            {
                // PPI
                platform::configurePrivatePeripheralInterrupt(interruptId,
                                                              platform::InterruptGroup::group1_irq_el1,
                                                              (platform::MonitorInterruptPriority) requestedPriority);
            }
            else if (interruptId >= 32)
            {
                // SPI
                platform::configureSharedPeripheralInterruptAndRouteToCpu(interruptId,
                                                                          platform::InterruptTrigger::edge,
                                                                          internal::getCpuIndex(),
                                                                          platform::InterruptGroup::group1_irq_el1,
                                                                          (platform::MonitorInterruptPriority) requestedPriority);
            }

            break;
        }

        case SMC_ZYNQMP_GIC_IRQ_DISABLE: {
            int interruptId = saved_regs.regs[1];

            platform::disableInterrupt(interruptId);
            break;
        }

        case SMC_ZYNQMP_GIC_IRQ_ENABLE: {
            int interruptId = saved_regs.regs[1];

            platform::enableInterrupt(interruptId);
            break;
        }

        default:
            // TODO: this should crash the payload, not the monitor...
            reportCrash(CrashingEntity::monitor, "Unhandled SMC", saved_regs.regs[0]);
            for (;;) {}
    }
}

// ************************************************************

void internal::reportCrash(CrashingEntity who, const char* desc, uintptr_t address)
{
    auto& outbox = getIpcBlock().executor_to_manager;

    outbox.fault_pc = address;
    outbox.fault_el = readSysReg(currentEL);
    strncpy(outbox.fault_desc, desc, sizeof(outbox.fault_desc));
    outbox.state = (who == CrashingEntity::payload) ? DomainState::crashed_payload : DomainState::crashed_monitor;

    // Force data propagation
    memory_write_reorder_barrier();
}
