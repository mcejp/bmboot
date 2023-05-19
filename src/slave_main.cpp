#include "mach/mach_baremetal.hpp"
#include "bmboot_internal.hpp"
#include "bmboot_slave.hpp"

extern "C" void enter_el1_payload(uintptr_t address);

namespace bmboot_s {

static void dummy_payload();

extern "C" int main() {
    auto ipc_block = (bmboot_internal::IpcBlock *) bmboot_internal::MONITOR_IPC_START;
    volatile auto& ipc_vol = *ipc_block;

    // hardcoded ZynqMP CPU1
    mach::enable_CPU_interrupts();
    mach::setup_interrupt(bmboot::mach::IPI_CURRENT_CPU_GIC_CHANNEL, bmboot::mach::SELF_CPU_INDEX);

    mach::enable_IPI_reception(0);

    ipc_vol.dom_state = DomainState::monitorReady;

    for (;;) {
        if (ipc_vol.dom_state == DomainState::monitorReady && ipc_vol.mst_requested_state == DomainState::runningPayload) {
            ipc_vol.dom_state = DomainState::startingPayload;

            // FLush all I-cache. Overkill? Should also flush D-cache?
            mach::flush_icache();

            // TODO: legitimize this h_a_c_k
            if (ipc_vol.mst_payload_entry_address == 0xbaadf00d) {
                enter_el1_payload((uintptr_t) &dummy_payload);
            }
            else {
                enter_el1_payload(ipc_vol.mst_payload_entry_address);
            }

            ipc_vol.dom_state = DomainState::monitorReady;
        }
    }
}

// this exists so that we have *something* to jump to in EL1 when the real payload is to be hot-loaded by a debugger
static void dummy_payload() {
    for (;;) {
        __asm__ volatile("nop");
    }
}

}
