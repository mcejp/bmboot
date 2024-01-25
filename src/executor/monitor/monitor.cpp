//! @file
//! @brief  Implements the bmboot monitor
//! @author Martin Cejp

#include "bmboot_internal.hpp"
#include "executor.hpp"
#include "executor_asm.hpp"
#include "monitor_internal.hpp"
#include "platform_interrupt_controller.hpp"
#include "utility/crc32.hpp"

#include <string.h>

using namespace bmboot;
using namespace bmboot::internal;

// ************************************************************

static void dummy_payload();
static Response validatePayload(void const* image, size_t image_size, uint32_t crc_expected);

// ************************************************************

extern "C" int main()
{
    auto& ipc_block = (volatile IpcBlock &) getIpcBlock();
    volatile const auto& inbox = ipc_block.manager_to_executor;
    volatile auto& outbox = ipc_block.executor_to_manager;

    // This is normally set by the firmware... plot twist -- we're the firmware now.
    writeSysReg(CNTFRQ_EL0, inbox.cntfrq);

    platform::setupInterrupts();

    outbox.state = DomainState::monitor_ready;

    for (;;)
    {
        if (inbox.cmd_seq != outbox.cmd_ack)
        {
            // TODO: must check for sequence breaks

            switch (inbox.cmd)
            {
            case Command::noop:
                outbox.cmd_ack = (outbox.cmd_ack + 1);
                break;

            case Command::start_payload:
                outbox.state = DomainState::starting_payload;

                // FLush all I-cache. Overkill?
                // FIXME: Must also flush D-cache, since the binary contains data
                //        Ah, but what if we have dirty lines? Should flush when payload exits, now's too late!
                platform::flushICache();

                // TODO: legitimize this h_a_c_k
                if (inbox.payload_entry_address == 0xbaadf00d)
                {
                    enterEL1Payload((uintptr_t) &dummy_payload);
                }
                else
                {
                    auto resp = validatePayload((void const*) inbox.payload_entry_address,
                                                inbox.payload_size,
                                                inbox.payload_crc);

                    outbox.cmd_resp = resp;
                    memory_write_reorder_barrier();
                    outbox.cmd_ack = (outbox.cmd_ack + 1);

                    if (resp == Response::crc_ok)
                    {
                        enterEL1Payload(inbox.payload_entry_address);
                    }
                }

                outbox.state = DomainState::monitor_ready;
                break;
            }
        }
    }
}

// ************************************************************

struct PayloadImageHeader
{
    uint8_t thunk[8];

    uint32_t magic;
    uint8_t abi_major;
    uint8_t abi_minor;
    uint8_t res0[2];

    uint64_t load_address;
    uint64_t program_size;
};

static_assert(sizeof(PayloadImageHeader) == 32);  // Not that the exact size matters so much. We just want to be sure about it.

static Response validatePayload(void const* image, size_t image_size, uint32_t crc_expected)
{
    // Validate CRC-32
    auto crc_gotten = crc32(0, image, image_size);

    if (crc_gotten != crc_expected)
    {
        return Response::crc_mismatched;
    }

    PayloadImageHeader hdr;
    memcpy(&hdr, image, sizeof(hdr));

    constexpr uint32_t MAGIC = 0x6f626d42;
    constexpr uint8_t ABI_MAJOR = 0x01;
    constexpr uint8_t ABI_MINOR = 0x01;

    if (hdr.magic != MAGIC)
    {
        return Response::image_malformed;
    }

    if (hdr.abi_major != ABI_MAJOR || hdr.abi_minor > ABI_MINOR)
    {
        return Response::abi_incompatible;
    }

    // TODO: hdr.load_address
    // TODO: hdr.program_size

    return Response::crc_ok;
}

// ************************************************************

// this exists so that we have *something* to jump to in EL1 when the real payload is to be hot-loaded by a debugger
static void dummy_payload()
{
    for (;;)
    {
        __asm__ volatile("nop");
    }
}
