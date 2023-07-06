#pragma once

#include <cstdint>
#include <cstdlib>

#include "cpu_state.hpp"

namespace bmboot_internal {

// We are assuming a coherent memory system
// TODO: multiple definitions will be needed for different domains
constexpr inline uintptr_t  MONITOR_CODE_START =    0x7800'0000;
constexpr inline size_t     MONITOR_CODE_SIZE =     0x0001'0000;
constexpr inline uintptr_t  MONITOR_IPC_START =     0x7801'0000;
constexpr inline size_t     MONITOR_IPC_SIZE =      0x0001'0000;
constexpr inline uintptr_t  PAYLOAD_START =         0x7802'0000;
constexpr inline uintptr_t  PAYLOAD_MAX_SIZE =      0x01FE'0000;        // code, data, stack, everything

// placed in the last 4 bytes of MONITOR_CODE area
using Cookie = uint32_t;
constexpr inline Cookie MONITOR_CODE_COOKIE = 0x7150ABCD;

enum {
    IPI_REQ_KILL = 0x01,            // request to kill the payload & return to 'ready' state
};

// TODO: add assertions for sizeof(IpcBlock) vs mmap sizes
// TODO: instead of mst_ and dom_ prefixes, use sub-structures
//
// zeroed in bmboot::startup_domain
struct IpcBlock {
    // mst_ prefix -> written by manager
    uint32_t mst_requested_state;
    uintptr_t mst_payload_entry_address;

    // dom_ prefix -> written by executor domain
    uint32_t dom_state;
    uint32_t dom_fault_el;
    uintptr_t dom_fault_pc;     // code address of fault
    char dom_fault_desc[32];

    Aarch64_Regs dom_regs;
    Aarch64_FpRegs dom_fpregs;

    // standard output dom->mst
    size_t mst_stdout_rdpos;
    size_t dom_stdout_wrpos;
    char dom_stdout_buf[1024];
};

}
