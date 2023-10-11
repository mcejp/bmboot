//! @file
//! @brief  Implements the bmboot monitor
//! @author Martin Cejp

#include "../mach/mach_baremetal.hpp"
#include "../bmboot_internal.hpp"
#include "bmboot/payload_runtime.hpp"
#include "executor_lowlevel.hpp"
#include "../utility/crc32.hpp"

#include "xpseudo_asm.h"

using namespace bmboot;
using namespace bmboot::internal;

// ************************************************************

static void dummy_payload();

extern "C" int main()
{
    auto ipc_block = (volatile IpcBlock *) MONITOR_IPC_START;
    volatile const auto& inbox = ipc_block->manager_to_executor;
    volatile auto& outbox = ipc_block->executor_to_manager;

    // FIXME: there is no need to enable IRQs as we always route those to EL1.
    mach::enableCpuInterrupts();
    // IPI: set to Group0 (routed to FIQ, therefore EL3)
    // apparently this is not the default even though ARM docs would make it seem so
    mach::enableSharedPeripheralInterruptAndRouteToCpu(mach::IPI_CURRENT_CPU_GIC_CHANNEL,
                                                       mach::InterruptTrigger::unchanged,
                                                       mach::SELF_CPU_INDEX,
                                                       mach::InterruptGroup::group0_fiq_el3,
                                                       mach::MonitorInterruptPriority::m7_max);

    mach::enableIpiReception(0);

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

                // FLush all I-cache. Overkill? Should also flush D-cache?
                mach::flushICache();

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
