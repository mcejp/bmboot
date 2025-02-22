//! @file
//! @brief  Internal definitons
//! @author Martin Cejp

#pragma once

#include <cstdint>
#include <cstdlib>

#include "bmboot_memmap.hpp"
#include "cpu_state.hpp"

namespace bmboot::internal
{

// placed in the last 4 bytes of MONITOR_CODE area
using Cookie = uint32_t;
constexpr inline Cookie MONITOR_CODE_COOKIE = 0x7150ABCD;

constexpr inline int GIC_MIN_USER_INTERRUPT_ID = 0;
constexpr inline int GIC_MAX_USER_INTERRUPT_ID = 128;     // inclusive

enum
{
    IPI_REQ_KILL = 0x01,            // request to kill the payload & return to 'ready' state
};

enum {
    SMC_GET_ABI_VERSION = 0xF2000000,
    SMC_NOTIFY_PAYLOAD_STARTED,
    SMC_NOTIFY_PAYLOAD_CRASHED,
    SMC_WRITE_STDOUT,

    SMC_ZYNQMP_GIC_IRQ_CONFIGURE = 0xF2000080,
    SMC_ZYNQMP_GIC_IRQ_ENABLE,
    SMC_ZYNQMP_GIC_IRQ_DISABLE,
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
    image_malformed,
    abi_incompatible,
};

// zeroed in bmboot::startup_domain
struct IpcBlock
{
    struct
    {
        Command cmd;
        uint32_t cmd_seq;

        uint32_t cntfrq;            // CNTFRQ_EL0 is supposed to be set by firmware -- but there is no firmware running under Bmboot
                                    // (maybe there could be if we used PSCI to boot the monitor?)

        uintptr_t payload_entry_address;
        size_t payload_size;
        uint32_t payload_crc;
        uintptr_t payload_argument;

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

static_assert(sizeof(IpcBlock) <= bmboot_cpu1_monitor_ipc_SIZE);
static_assert(sizeof(IpcBlock) <= bmboot_cpu2_monitor_ipc_SIZE);
static_assert(sizeof(IpcBlock) <= bmboot_cpu3_monitor_ipc_SIZE);

}
