//! @file
//! @brief  Runtime functions for the payload
//! @author Martin Cejp

#include "../bmboot_internal.hpp"
#include "bmboot/payload_runtime.hpp"
#include "../mach/mach_baremetal_defs.hpp"

#include <cstring>

#include "xpseudo_asm.h"

using namespace bmboot;
using namespace bmboot::internal;

static auto& ipc_block = *(IpcBlock*) MONITOR_IPC_START;

void bmboot::notifyPayloadCrashed(const char* desc, uintptr_t address) {
    ipc_block.dom_fault_pc = address;
    ipc_block.dom_fault_el = mfcp(currentEL);
    strncpy(ipc_block.dom_fault_desc, desc, sizeof(ipc_block.dom_fault_desc));
    ipc_block.dom_state = DomainState::crashed_payload;

    // Force data propagation
    memory_write_reorder_barrier();
}

void bmboot::notifyPayloadStarted() {
    ipc_block.dom_state = DomainState::running_payload;
}

int bmboot::writeToStdout(void const* data, size_t size) {
    auto data_bytes = static_cast<uint8_t const*>(data);

    size_t wrote = 0;

    // Here we want to be very conservative to be absolutely certain we will not overflow the buffer
    if (ipc_block.dom_stdout_wrpos >= sizeof(ipc_block.dom_stdout_buf)) {
        auto weird = ipc_block.dom_stdout_wrpos;
        ipc_block.dom_stdout_wrpos = 0;
        printf("unexpected dom_stdout_wrpos %zx, reset to 0\n", weird);
    }

    // TODO: can be replaced with a more efficient implementation instead of writing one byte at a time
    while (size > 0) {
        auto wrpos_new = (ipc_block.dom_stdout_wrpos + 1) % sizeof(ipc_block.dom_stdout_buf);
        if (wrpos_new == ipc_block.mst_stdout_rdpos) {
            // Buffer full, abort!
            // However, we must lie about number of characters written, otherwise stdout error flag will be set and
            // printf will refuse to print any more
            // (on a non-rt OS, a write to clogged stdout would just block instead)

            wrote += size;
            break;
        }

        ipc_block.dom_stdout_buf[ipc_block.dom_stdout_wrpos] = *data_bytes;
        ipc_block.dom_stdout_wrpos = wrpos_new;

        wrote++;
        data_bytes++;
        size--;
    }

    return wrote;
}
