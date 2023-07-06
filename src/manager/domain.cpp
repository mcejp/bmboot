#include "../mach/mach_linux.hpp"
#include "../bmboot_internal.hpp"
#include "bmboot/domain.hpp"
#include "coredump_linux.hpp"
#include "../utility/mmap.hpp"

#include "monitor_zynqmp_cpu1.hpp"

#include <cstring>
#include <variant>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

namespace bmboot {

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

// Maybe we should have a full state automaton on the manager side as well -- instead of just a boolean:
//  - unprobed
//  - probed, not running
//  - probed, not running, startup requested
//  - probed, running or started by us; defer to domain-reported state
static DomainGeneralState domain_general_state[DomainIndex::max_domain];

static std::variant<int, ErrorCode> get_devmem_handle() {
    if (s_devmem_handle < 0) {
        s_devmem_handle = open("/dev/mem", O_RDWR);

        if (s_devmem_handle < 0) {
            return ErrorCode::dev_mem_access_failed;
        }
    }

    return s_devmem_handle;
}

class Domain : public IDomain {
public:
    Domain(DomainIndex domain, bmboot_internal::IpcBlock& ipc_block) : m_domain(domain), m_ipc_block(ipc_block) {}

    MaybeError dumpCore(char const* filename) final;
    void dumpDebugInfo() final;
    MaybeError loadAndStartPayload(std::span<uint8_t const> payload_binary) final;
    CrashInfo getCrashInfo() final;
    DomainState getState() final;
    MaybeError terminatePayload() final;
    MaybeError startup() final;
    int getchar() final;

    void startDummyPayload() final {
        // we _know_ that this will time out, don't bother checking the result
        start_payload_at(0xbaadf00d);
    }

private:
    MaybeError await_monitor_startup();
    MaybeError start_payload_at(uintptr_t entry_address);
    MaybeError startup(std::span<uint8_t const> monitor_binary);

    volatile bmboot_internal::IpcBlock& get_ipc_block() {
        return (volatile bmboot_internal::IpcBlock&) m_ipc_block;
    }

    bmboot_internal::IpcBlock& get_ipc_block_nonvol() {
        return m_ipc_block;
    }

    DomainIndex m_domain;
    bmboot_internal::IpcBlock& m_ipc_block;
};

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
        return ErrorCode::mmap_failed;
    }

    memcpy(code_area + 0, binary.data(), binary.size());

    __clear_cache(code_area + 0, code_area + bmboot_internal::MONITOR_CODE_SIZE);

    code_area.unmap();

    return {};
}

MaybeError Domain::await_monitor_startup() {
    // up to 0.5sec for monitor to come to life; should normally take around 130 ms
    for (int i = 0; i < 50; i++) {
        usleep(10'000);

        if (getState() == DomainState::monitor_ready) {
            return {};
        }
    }

    return ErrorCode::monitor_start_timed_out;
}

MaybeError Domain::dumpCore(char const* filename) {
    auto state = getState();

    if (state != DomainState::crashed_payload) {
        return ErrorCode::bad_domain_state;
    }

    auto devmem = get_devmem_handle();
    if (std::holds_alternative<ErrorCode>(devmem)) {
        return std::get<ErrorCode>(devmem);
    }

    Mmap code_area(nullptr,
                   bmboot_internal::PAYLOAD_MAX_SIZE,
                   PROT_READ | PROT_WRITE,
                   MAP_SHARED,
                   std::get<int>(devmem),
                   bmboot_internal::PAYLOAD_START);

    if (!code_area) {
        return ErrorCode::mmap_failed;
    }

    auto& ipc_vol = get_ipc_block_nonvol();

    static const MemorySegment segments[] {
            { bmboot_internal::PAYLOAD_START, bmboot_internal::PAYLOAD_MAX_SIZE, code_area.data() },
    };

    write_core_dump(filename,
                    segments,
                    ipc_vol.dom_regs,
                    ipc_vol.dom_fpregs);

    return {};
}

DomainInstanceOrErrorCode IDomain::open(DomainIndex domain) {
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
        return ErrorCode::mmap_failed;
    }

    auto code_area = (uint8_t*) mmap(nullptr,
                                     bmboot_internal::MONITOR_CODE_SIZE,
                                     PROT_READ | PROT_WRITE,
                                     MAP_SHARED,
                                     std::get<int>(devmem),
                                     bmboot_internal::MONITOR_CODE_START);
    if (code_area == nullptr) {
        return ErrorCode::mmap_failed;
    }

    // Check if domain has been started up by looking for the bmboot cookie.
    bmboot_internal::Cookie cookie = -1;
    memcpy(&cookie, code_area + bmboot_internal::MONITOR_CODE_SIZE - sizeof(cookie), sizeof(cookie));

    if (cookie != bmboot_internal::MONITOR_CODE_COOKIE) {
        if (mach::is_zynq_cpu1_in_reset(std::get<int>(devmem))) {
            domain_general_state[domain] = DomainGeneralState::inReset;
        }
        else {
            // Core has been started, but not by us...
            domain_general_state[domain] = DomainGeneralState::unavailable;
        }
    }
    else {
        // Cookie found -> assume bmboot running (potentially with user payload)
        //
        // This can give a false positive if the startup failed or if the monitor crashed... tough luck.
        // A reboot is probably the only way out in that case, anyway.

        domain_general_state[domain] = DomainGeneralState::monitorStarted;
    }

    munmap(code_area, bmboot_internal::MONITOR_CODE_SIZE);

    return std::make_unique<Domain>(domain, *ipc_block);
}

MaybeError Domain::terminatePayload() {
    if (domain_general_state[m_domain] != DomainGeneralState::monitorStarted) {
        return ErrorCode::bad_domain_state;
    }

    auto& ipc_vol = get_ipc_block();

    auto devmem = get_devmem_handle();
    if (std::holds_alternative<ErrorCode>(devmem)) {
        return std::get<ErrorCode>(devmem);
    }

    ipc_vol.mst_requested_state = DomainState::monitor_ready;

    // TODO: clean up a bit
    uint8_t message[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18 , 19, 20};
    mach::send_ipi_message(std::get<int>(devmem), message);

    return await_monitor_startup();
}

MaybeError Domain::startup() {
    return startup(monitor_zynqmp_cpu1_payload);
}

MaybeError Domain::startup(std::span<uint8_t const> monitor_binary) {
    if (domain_general_state[m_domain] != DomainGeneralState::inReset) {
        return ErrorCode::bad_domain_state;
    }

    auto& ipc_vol = get_ipc_block();

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
        return ErrorCode::mmap_failed;
    }

    const auto cookie = bmboot_internal::MONITOR_CODE_COOKIE;

    if (monitor_binary.size() > bmboot_internal::MONITOR_CODE_SIZE - sizeof(cookie)) {
        return ErrorCode::program_too_large;
    }

    memcpy(code_area, monitor_binary.data(), monitor_binary.size());      // won't work without cache flush to L2/DDR
    memcpy(code_area + bmboot_internal::MONITOR_CODE_SIZE - sizeof(cookie), &cookie, sizeof(cookie));

    // flush the newly written code from L1 through to DDR (since CPUn will come up in uncached mode)
    __clear_cache(code_area, code_area + bmboot_internal::MONITOR_CODE_SIZE);

    munmap(code_area, bmboot_internal::MONITOR_CODE_SIZE);

    // initialize IPC block
    memset((void*) &ipc_vol, 0, sizeof(ipc_vol));
    ipc_vol.mst_requested_state = DomainState::monitor_ready;

    // flush the IPC region to DDR (since the SCU is not in effect yet and CPUn will come up with cold caches)
    __clear_cache(&m_ipc_block, (uint8_t*) &m_ipc_block + bmboot_internal::MONITOR_IPC_SIZE);

    mach::boot_zynq_cpu1(std::get<int>(devmem), bmboot_internal::MONITOR_CODE_START);

    // TODO: maybe we should only do this after state goes to ready
    domain_general_state[m_domain] = DomainGeneralState::monitorStarted;

    return await_monitor_startup();
}

CrashInfo Domain::getCrashInfo() {
    auto& ipc_vol = get_ipc_block();

    return CrashInfo { .pc = ipc_vol.dom_fault_pc, .desc = std::string((char const*) ipc_vol.dom_fault_desc, sizeof(ipc_vol.dom_fault_desc)) };
}

DomainState Domain::getState() {
    auto& ipc_vol = get_ipc_block();

    auto state_raw = ipc_vol.dom_state;

    if (state_raw <= (int)DomainState::invalid_state) {
        return (DomainState) state_raw;
    }
    else {
        return DomainState::invalid_state;
    }
}

MaybeError Domain::loadAndStartPayload(std::span<uint8_t const> payload_binary) {
    // First, ensure we are in 'ready' state
    if (getState() != DomainState::monitor_ready) {
        return ErrorCode::bad_domain_state;
    }

    load_to_physical_memory(bmboot_internal::PAYLOAD_START, payload_binary);

    return start_payload_at(bmboot_internal::PAYLOAD_START);
}

MaybeError Domain::start_payload_at(uintptr_t entry_address) {
    // First, ensure we are in 'ready' state
    if (getState() != DomainState::monitor_ready) {
        return ErrorCode::bad_domain_state;
    }

    auto& ipc_vol = get_ipc_block();

    // flush any residual content of the stdout buffer by setting our read position equal to the write position
    ipc_vol.mst_stdout_rdpos = ipc_vol.dom_stdout_wrpos;

    ipc_vol.mst_payload_entry_address = entry_address;
    ipc_vol.mst_requested_state = DomainState::running_payload;

    // up to 1sec for domain to come to life
    for (int i = 0; i < 100; i++) {
        usleep(10'000);

        auto state = getState();

        if (state == DomainState::running_payload) {
            return {};
        }
        else if (state == DomainState::crashed_payload) {
            return ErrorCode::payload_crashed_during_startup;
        }
    }

    // TODO: might want to latch DomainState::crashedPayload when this happens?
    return ErrorCode::payload_start_timed_out;
}

int Domain::getchar() {
    auto& ipc_vol = get_ipc_block();

    if (ipc_vol.mst_stdout_rdpos >= sizeof(ipc_vol.dom_stdout_buf)) {
        printf("bmboot: unexpected mst_stdout_rdpos %zx, resetting to 0\n", ipc_vol.mst_stdout_rdpos);
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

void Domain::dumpDebugInfo() {
    auto& ipc_vol = get_ipc_block();

    fprintf(stderr, "debug: rdpos=%3zu wrpos=%3zu\n", ipc_vol.mst_stdout_rdpos, ipc_vol.dom_stdout_wrpos);
}

}
