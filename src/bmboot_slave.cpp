#include "bmboot_internal.hpp"
#include "bmboot_slave.hpp"

#include <cstring>

#include "xpseudo_asm.h"

namespace bmboot_s {

static auto& ipc_block = *(bmboot_internal::IpcBlock*) bmboot_internal::MONITOR_IPC_START;

void notify_payload_crashed(const char* desc, uintptr_t address) {
    ipc_block.dom_fault_pc = address;
    ipc_block.dom_fault_el = mfcp(currentEL);
    strncpy(ipc_block.dom_fault_desc, desc, sizeof(ipc_block.dom_fault_desc));
    ipc_block.dom_state = DomainState::crashedPayload;
}

void notify_payload_started() {
    ipc_block.dom_state = DomainState::runningPayload;
}

int write_stdout(void const* data, size_t size) {
    auto data_bytes = static_cast<uint8_t const*>(data);

    size_t wrote = 0;

    if (ipc_block.dom_stdout_wrpos >= sizeof(ipc_block.dom_stdout_buf)) {
        auto weird = ipc_block.dom_stdout_wrpos;
        ipc_block.dom_stdout_wrpos = 0;
        printf("unexpected dom_stdout_wrpos %zx, reset to 0\n", weird);
    }

    // TODO: can be replaced with a more efficient implementation instead of writing one byte at a time
    while (size > 0) {
        auto wrpos_new = (ipc_block.dom_stdout_wrpos + 1) % sizeof(ipc_block.dom_stdout_buf);
        if (wrpos_new == ipc_block.mst_stdout_rdpos) {
            // buffer full, abort
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

}
