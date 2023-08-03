//! @file
//! @brief  Runtime functions for the payload
//! @author Martin Cejp

#include <bmboot/payload_runtime.hpp>

#include "executor_lowlevel.hpp"
#include "../bmboot_internal.hpp"
#include "../mach/mach_baremetal_defs.hpp"

#include <cstring>

#include "bspconfig.h"
#include "xpseudo_asm.h"

using namespace bmboot;
using namespace bmboot::internal;

static auto& ipc_block = *(IpcBlock*) MONITOR_IPC_START;

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
