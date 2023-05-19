#include "mach/mach_linux.hpp"
#include "bmboot_internal.hpp"
#include "bmboot_master.hpp"
#include "utility/mmap.hpp"

#include "bmboot_slave_zynqmp_cpu1.hpp"

#include <cstring>
#include <variant>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace bmboot_m {

static int s_devmem_handle = -1;

// TODO: need a really good explanation of this enum and its relation to DomainState
// roughly speaking, this state that cannot change autonomously (e.g., the domain will not start itself...)
// this is in contrast to DomainState proper, which can for example go from runningPayload to crashedPayload
enum class DomainGeneralState {
    inReset,
    unavailable,
//    monitorStartupRequested,
    monitorStarted,                 // *only in this case* is DomainState meaningful
};

// Maybe we should have a full state automaton on the master side as well -- instead of just a boolean:
//  - unprobed
//  - probed, not running
//  - probed, not running, startup requested
//  - probed, running or started by us; defer to domain-reported state
static DomainGeneralState domain_general_state[Domain::max_domain];

static std::variant<int, ErrorCode> get_devmem_handle() {
    if (s_devmem_handle < 0) {
        s_devmem_handle = open("/dev/mem", O_RDWR);

        if (s_devmem_handle < 0) {
            return ErrorCode::devMemAccessFailed;
        }
    }

    return s_devmem_handle;
}

static volatile bmboot_internal::IpcBlock& get_ipc_block(DomainHandle const& domain) {
    return *(volatile bmboot_internal::IpcBlock*) domain.ipc_block;
}

static MaybeError load_to_physical_memory(uintptr_t address, std::span<uint8_t const> binary) {
    auto devmem = get_devmem_handle();
    if (std::holds_alternative<ErrorCode>(devmem)) {
        return std::get<ErrorCode>(devmem);
    }

    // TODO: assert that address is aligned

    // FIXME: assuming 4k pages
    const auto alignment = 4096;
    auto size_aligned = (binary.size() + alignment - 1) & ~(alignment - 1);

    Mmap code_area(nullptr,
                   size_aligned,
                   PROT_READ | PROT_WRITE,
                   MAP_SHARED,
                   std::get<int>(devmem),
                   address);

    if (!code_area) {
        return ErrorCode::mmapFailed;
    }

    memcpy(code_area + 0, binary.data(), binary.size());

    __clear_cache(code_area + 0, code_area + bmboot_internal::MONITOR_CODE_SIZE);

    code_area.unmap();

    return {};
}

static MaybeError await_monitor_startup(DomainHandle const& domain) {
    // up to 0.5sec for monitor to come to life; should normally take around 130 ms
    for (int i = 0; i < 50; i++) {
        usleep(10'000);

        if (get_domain_state(domain) == DomainState::monitorReady) {
            return {};
        }
    }

    return ErrorCode::monitorStartTimedOut;
}

DomainHandleOrErrorCode open_domain(Domain domain) {
    auto devmem = get_devmem_handle();
    if (std::holds_alternative<ErrorCode>(devmem)) {
        return std::get<ErrorCode>(devmem);
    }

    auto ipc_block = (bmboot_internal::IpcBlock*) mmap(nullptr,
                                                       bmboot_internal::MONITOR_IPC_SIZE,
                                                       PROT_READ | PROT_WRITE,
                                                       MAP_SHARED,
                                                       std::get<int>(devmem),
                                                       bmboot_internal::MONITOR_IPC_START);
    if (ipc_block == nullptr) {
        return ErrorCode::mmapFailed;
    }

    auto code_area = (uint8_t*) mmap(nullptr,
                                     bmboot_internal::MONITOR_CODE_SIZE,
                                     PROT_READ | PROT_WRITE,
                                     MAP_SHARED,
                                     std::get<int>(devmem),
                                     bmboot_internal::MONITOR_CODE_START);
    if (code_area == nullptr) {
        return ErrorCode::mmapFailed;
    }

    // check if domain has been started
    // TODO: can probe it with IPI? probably not, because it will break real-time code.
    //       but we *should* have some mechanism of checking it is alive. perhaps we can require the payload to feed a soft watchdog
    bmboot_internal::Cookie cookie = -1;
    memcpy(&cookie, code_area + bmboot_internal::MONITOR_CODE_SIZE - sizeof(cookie), sizeof(cookie));

    if (cookie != bmboot_internal::MONITOR_CODE_COOKIE) {
        if (mach::is_zynq_cpu1_in_reset(std::get<int>(devmem))) {
            domain_general_state[domain] = DomainGeneralState::inReset;
        }
        else {
            domain_general_state[domain] = DomainGeneralState::unavailable;
        }
    }
    else {
        // This is a bit of assumption
        domain_general_state[domain] = DomainGeneralState::monitorStarted;
    }

    munmap(code_area, bmboot_internal::MONITOR_CODE_SIZE);

    return DomainHandle {domain, ipc_block};
}

MaybeError reset_domain(DomainHandle const& domain) {
    if (domain_general_state[domain.domain] != DomainGeneralState::monitorStarted) {
        return ErrorCode::badDomainState;
    }

    auto& ipc_vol = get_ipc_block(domain);

    auto devmem = get_devmem_handle();
    if (std::holds_alternative<ErrorCode>(devmem)) {
        return std::get<ErrorCode>(devmem);
    }

    ipc_vol.mst_requested_state = DomainState::monitorReady;

    uint8_t message[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18 , 19, 20};
    mach::send_ipi_message(std::get<int>(devmem), message);

    return await_monitor_startup(domain);
}

MaybeError startup_domain(DomainHandle const& domain) {
    return startup_domain(domain, slave_zynqmp_cpu1_payload);
}

MaybeError startup_domain(DomainHandle const& domain, std::span<uint8_t const> bmboot_slave_binary) {
    if (domain_general_state[domain.domain] != DomainGeneralState::inReset) {
        return ErrorCode::badDomainState;
    }

    auto& ipc_vol = get_ipc_block(domain);

    auto devmem = get_devmem_handle();
    if (std::holds_alternative<ErrorCode>(devmem)) {
        return std::get<ErrorCode>(devmem);
    }

    auto code_area = (uint8_t*) mmap(nullptr,
                                     bmboot_internal::MONITOR_CODE_SIZE,
                                     PROT_READ | PROT_WRITE,
                                     MAP_SHARED,
                                     std::get<int>(devmem),
                                     bmboot_internal::MONITOR_CODE_START);
    if (code_area == nullptr) {
        return ErrorCode::mmapFailed;
    }

    const auto cookie = bmboot_internal::MONITOR_CODE_COOKIE;

    if (bmboot_slave_binary.size() > bmboot_internal::MONITOR_CODE_SIZE - sizeof(cookie)) {
        return ErrorCode::programTooLarge;
    }

    memcpy(code_area, bmboot_slave_binary.data(), bmboot_slave_binary.size());      // won't work without cache flush to L2/DDR
    memcpy(code_area + bmboot_internal::MONITOR_CODE_SIZE - sizeof(cookie), &cookie, sizeof(cookie));

    // flush the newly written code from L1 through to DDR (since CPUn will come up in uncached mode)
    __clear_cache(code_area, code_area + bmboot_internal::MONITOR_CODE_SIZE);

    munmap(code_area, bmboot_internal::MONITOR_CODE_SIZE);

    ipc_vol.mst_requested_state = DomainState::monitorReady;
    ipc_vol.mst_stdout_rdpos = 0;
    ipc_vol.dom_stdout_wrpos = 0;

    // flush the IPC region to DDR (since the SCU is not in effect yet and CPUn will come up with cold caches)
    __clear_cache(domain.ipc_block, (uint8_t*) domain.ipc_block + bmboot_internal::MONITOR_IPC_SIZE);

    mach::boot_zynq_cpu1(std::get<int>(devmem), bmboot_internal::MONITOR_CODE_START);

    // TODO: maybe we should only do this after state goes to ready
    domain_general_state[domain.domain] = DomainGeneralState::monitorStarted;

    return await_monitor_startup(domain);
}

CrashInfo get_crash_info(DomainHandle const& domain) {
    auto& ipc_vol = get_ipc_block(domain);

    return CrashInfo { .pc = ipc_vol.dom_fault_pc, .desc = std::string((char const*) ipc_vol.dom_fault_desc, sizeof(ipc_vol.dom_fault_desc)) };
}

DomainState get_domain_state(DomainHandle const& domain) {
    auto& ipc_vol = get_ipc_block(domain);

    auto state_raw = ipc_vol.dom_state;

    if (state_raw <= (int)DomainState::invalidState) {
        return (DomainState) state_raw;
    }
    else {
        return DomainState::invalidState;
    }
}

MaybeError load_and_start_payload(DomainHandle const& domain, std::span<uint8_t const> payload_binary) {
    // First, ensure we are in 'ready' state
    if (get_domain_state(domain) != DomainState::monitorReady) {
        return ErrorCode::badDomainState;
    }

    load_to_physical_memory(bmboot_internal::PAYLOAD_START, payload_binary);

    return start_payload_at(domain, bmboot_internal::PAYLOAD_START);
}

MaybeError start_payload_at(DomainHandle const& domain, uintptr_t entry_address) {
    // First, ensure we are in 'ready' state
    if (get_domain_state(domain) != DomainState::monitorReady) {
        return ErrorCode::badDomainState;
    }

    auto& ipc_vol = get_ipc_block(domain);

    ipc_vol.mst_payload_entry_address = entry_address;
    ipc_vol.mst_requested_state = DomainState::runningPayload;

    // up to 1sec for domain to come to life
    for (int i = 0; i < 100; i++) {
        usleep(10'000);

        if (get_domain_state(domain) == DomainState::runningPayload) {
            return {};
        }
        else if (get_domain_state(domain) == DomainState::crashedPayload) {
            return ErrorCode::payloadCrashedDuringStartup;
        }
    }

    // TODO: might want to latch DomainState::crashedPayload when this happens?
    return ErrorCode::payloadStartTimedOut;
}

int stdout_getchar(DomainHandle const& domain) {
    auto& ipc_vol = get_ipc_block(domain);

    if (ipc_vol.mst_stdout_rdpos >= sizeof(ipc_vol.dom_stdout_buf)) {
        printf("bmboot_m: unexpected mst_stdout_rdpos %zx, resetting to 0\n", ipc_vol.mst_stdout_rdpos);
        ipc_vol.mst_stdout_rdpos = 0;
    }

    if (ipc_vol.mst_stdout_rdpos != ipc_vol.dom_stdout_wrpos) {
        char c = ipc_vol.dom_stdout_buf[ipc_vol.mst_stdout_rdpos];
        ipc_vol.mst_stdout_rdpos = (ipc_vol.mst_stdout_rdpos + 1) % sizeof(ipc_vol.dom_stdout_buf);
        return c;
    }
    else {
        return -1;
    }
}

void dump_debug_info(DomainHandle const& domain) {
    auto& ipc_vol = get_ipc_block(domain);

    fprintf(stderr, "debug: rdpos=%3zu wrpos=%3zu\n", ipc_vol.mst_stdout_rdpos, ipc_vol.dom_stdout_wrpos);
}

}
