//! @file
//! @brief  Implements the bmboot monitor
//! @author Martin Cejp

#include "bmboot_internal.hpp"
#include "executor.hpp"
#include "executor_asm.hpp"
#include "monitor_internal.hpp"
#include "platform_interrupt_controller.hpp"
#include "utility/crc32.hpp"

using namespace bmboot;
using namespace bmboot::internal;

// ************************************************************

static void dummy_payload();

extern "C" int main()
{
    auto& ipc_block = (volatile IpcBlock &) getIpcBlock();
    volatile const auto& inbox = ipc_block.manager_to_executor;
    volatile auto& outbox = ipc_block.executor_to_manager;

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
                platform::flushICache();

                // TODO: legitimize this h_a_c_k
                if (inbox.payload_entry_address == 0xbaadf00d)
                {
                    enterEL1Payload((uintptr_t) &dummy_payload);
                }
                else
                {
                    // Validate CRC-32
                    auto crc_gotten = crc32(0, (void const*) inbox.payload_entry_address, inbox.payload_size);

                    bool crc_match = (crc_gotten == inbox.payload_crc);

                    outbox.cmd_resp = crc_match ? Response::crc_ok : Response::crc_mismatched;
                    memory_write_reorder_barrier();
                    outbox.cmd_ack = (outbox.cmd_ack + 1);

                    if (crc_match)
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

// this exists so that we have *something* to jump to in EL1 when the real payload is to be hot-loaded by a debugger
static void dummy_payload()
{
    for (;;)
    {
        __asm__ volatile("nop");
    }
}
