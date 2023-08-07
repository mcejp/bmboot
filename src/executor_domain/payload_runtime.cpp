//! @file
//! @brief  Runtime functions for the payload
//! @author Martin Cejp

#include <bmboot/payload_runtime.hpp>

#include "executor_lowlevel.hpp"
#include "../bmboot_internal.hpp"
#include "../mach/mach_baremetal.hpp"

#include <cstring>

#include "bspconfig.h"
#include "xpseudo_asm.h"

using namespace bmboot;
using namespace bmboot::internal;

static auto& ipc_block = *(IpcBlock*) MONITOR_IPC_START;

static uint64_t timer_period_ticks;
static InterruptHandler timer_irq_handler;

#if EL1_NONSECURE
void bmboot::startPeriodicInterrupt(int period_us, InterruptHandler handler)
{
    // Perhaps this could be refactored so that only the IRQ setup is done via SMC and we configure the timer directly

    // ticks = duration_us * timer_freq_Hz / 1e6
    timer_period_ticks = (uint64_t) period_us * mfcp(CNTFRQ_EL0) / 1'000'000;
    timer_irq_handler = handler;

    smc(SMC_START_PERIODIC_INTERRUPT, timer_period_ticks);
}

void bmboot::stopPeriodicInterrupt()
{
    smc(SMC_STOP_PERIODIC_INTERRUPT);
}
#endif

void bmboot::mach::handleTimerIrq()
{
    if (timer_irq_handler)
    {
        timer_irq_handler();
    }
    else
    {
        // TBD: spurious interrupt -- log a warning?
    }

    // must either set new deadline...
    mtcp(CNTP_CVAL_EL0, mfcp(CNTP_CVAL_EL0) + timer_period_ticks);
    // ...or disable the interrupt
//    mtcp(CNTP_CTL_EL0, 0);
}

void bmboot::notifyPayloadCrashed(const char* desc, uintptr_t address) {
#if EL3
    auto& outbox = ipc_block.executor_to_manager;

    outbox.fault_pc = address;
    outbox.fault_el = mfcp(currentEL);
    strncpy(outbox.fault_desc, desc, sizeof(outbox.fault_desc));
    outbox.state = DomainState::crashed_payload;

    // Force data propagation
    memory_write_reorder_barrier();
#else
    smc(SMC_NOTIFY_PAYLOAD_CRASHED, desc, address);
#endif
}

void bmboot::notifyPayloadStarted() {
#if EL3
    ipc_block.executor_to_manager.state = DomainState::running_payload;
#else
    smc(SMC_NOTIFY_PAYLOAD_STARTED);
#endif
}

int bmboot::writeToStdout(void const* data, size_t size) {
#if EL3
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
#else
    return smc(SMC_WRITE_STDOUT, data, size);
#endif
}
