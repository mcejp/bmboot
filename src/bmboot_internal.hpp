//! @file
//! @brief  Internal definitons
//! @author Martin Cejp

#pragma once

#include <cstdint>
#include <cstdlib>

#include "cpu_state.hpp"

namespace bmboot::internal
{

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

enum
{
    IPI_REQ_KILL = 0x01,            // request to kill the payload & return to 'ready' state
};

enum Command
{
    noop = 0x00,
    start_payload = 0x01,
};

enum Response
{
    crc_ok,
    crc_mismatched,
};

// TODO: add assertions for sizeof(IpcBlock) vs mmap sizes
// TODO: instead of mst_ and dom_ prefixes, use sub-structures
//
// zeroed in bmboot::startup_domain
struct IpcBlock
{
    struct
    {
        Command cmd;
        uint32_t cmd_seq;

        uintptr_t payload_entry_address;
        size_t payload_size;
        uint32_t payload_crc;

        size_t stdout_rdpos;
    }
    manager_to_executor;

    struct
    {
        uint32_t state;

        uint32_t cmd_ack;
        Response cmd_resp;

        uint32_t fault_el;
        uintptr_t fault_pc;     // code address of fault
        char fault_desc[32];

        Aarch64_Regs regs;
        Aarch64_FpRegs fpregs;

        // standard output (circular buffer)
        size_t stdout_wrpos;
        char stdout_buf[1024];
    }
    executor_to_manager;
};

}
