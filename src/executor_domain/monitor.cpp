//! @file
//! @brief  Implements the bmboot monitor
//! @author Martin Cejp

#include "../mach/mach_baremetal.hpp"
#include "../bmboot_internal.hpp"
#include "bmboot/payload_runtime.hpp"
#include "executor_lowlevel.hpp"

using namespace bmboot;
using namespace bmboot::internal;

// ************************************************************

static void dummy_payload();

extern "C" int main()
{
    auto ipc_block = (IpcBlock *) MONITOR_IPC_START;
    volatile auto& ipc_vol = *ipc_block;

    // hardcoded ZynqMP CPU1
    mach::enableCpuInterrupts();
    mach::setupInterrupt(bmboot::mach::IPI_CURRENT_CPU_GIC_CHANNEL, bmboot::mach::SELF_CPU_INDEX);

    mach::enableIpiReception(0);

    ipc_vol.dom_state = DomainState::monitor_ready;

    for (;;)
    {
        if (ipc_vol.dom_state == DomainState::monitor_ready && ipc_vol.mst_requested_state == DomainState::running_payload)
        {
            ipc_vol.dom_state = DomainState::starting_payload;

            // FLush all I-cache. Overkill? Should also flush D-cache?
            mach::flushICache();

            // TODO: legitimize this h_a_c_k
            if (ipc_vol.mst_payload_entry_address == 0xbaadf00d)
            {
                enterEL1Payload((uintptr_t) &dummy_payload);
            }
            else
            {
                enterEL1Payload(ipc_vol.mst_payload_entry_address);
            }

            ipc_vol.dom_state = DomainState::monitor_ready;
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
