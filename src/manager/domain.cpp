//! @file
//! @brief  Domain management
//! @author Martin Cejp

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

using namespace bmboot;
using namespace bmboot::internal;

static int s_devmem_handle = -1;

// TODO: need a really good explanation of this enum and its relation to DomainState
// roughly speaking, this state that cannot change autonomously (e.g., the domain will not start itself...)
// this is in contrast to DomainState proper, which can for example go from runningPayload to crashedPayload
enum class DomainGeneralState
{
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

// ************************************************************

class Domain : public IDomain
{
public:
    Domain(DomainIndex domain, IpcBlock& ipc_block) : m_domain(domain), m_ipc_block(ipc_block) {}

    MaybeError dumpCore(char const* filename) final;
    void dumpDebugInfo() final;
    MaybeError ensureReadyToLoadPayload() final;
    MaybeError loadAndStartPayload(std::span<uint8_t const> payload_binary, uint32_t payload_crc32) final;
    int getchar() final;
    CrashInfo getCrashInfo() final;
    DomainState getState() final;
    MaybeError terminatePayload() final;
    MaybeError startup() final;

    void startDummyPayload() final
    {
        // we _know_ that this will time out, don't bother checking the result
        startPayloadAt(0xbaadf00d, 0, 0);
    }

private:
    MaybeError awaitMonitorStartup();
    MaybeError startPayloadAt(uintptr_t entry_address, size_t payload_size, uint32_t payload_crc32);
    MaybeError startup(std::span<uint8_t const> monitor_binary);

//    volatile IpcBlock& getIpcBlock()
//    {
//        return (volatile IpcBlock&) m_ipc_block;
//    }

    volatile const auto& getInbox()
    {
        return m_ipc_block.executor_to_manager;
    }

    volatile auto& getOutbox()
    {
        return m_ipc_block.manager_to_executor;
    }

    auto& getInboxNonvolatile()
    {
        return m_ipc_block.executor_to_manager;
    }

    DomainIndex m_domain;
    IpcBlock& m_ipc_block;
};

// ************************************************************

static std::variant<int, ErrorCode> get_devmem_handle()
{
    if (s_devmem_handle < 0)
    {
        s_devmem_handle = open("/dev/mem", O_RDWR);

        if (s_devmem_handle < 0)
        {
            return ErrorCode::dev_mem_access_failed;
        }
    }

    return s_devmem_handle;
}

static MaybeError load_to_physical_memory(uintptr_t address, std::span<uint8_t const> binary)
{
    auto devmem = get_devmem_handle();
    if (std::holds_alternative<ErrorCode>(devmem))
    {
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

    if (!code_area)
    {
        return ErrorCode::mmap_failed;
    }

    memcpy(code_area.data(), binary.data(), binary.size());

    __clear_cache(code_area.data(), (uint8_t*) code_area.data() + MONITOR_CODE_SIZE);

    code_area.unmap();

    return {};
}

// ************************************************************

MaybeError Domain::awaitMonitorStartup()
{
    // wait up to 0.5sec for monitor to come to life; should normally take around 130 ms
    constexpr int timeout_msec = 500;
    constexpr int poll_period_msec = 10;

    for (int i = 0; i < timeout_msec / poll_period_msec; i++)
    {
        usleep(poll_period_msec * 1000);

        if (getState() == DomainState::monitor_ready)
        {
            return {};
        }
    }

    return ErrorCode::monitor_start_timed_out;
}

// ************************************************************

MaybeError Domain::dumpCore(char const* filename)
{
    auto state = getState();

    if (state != DomainState::crashed_payload)
    {
        return ErrorCode::bad_domain_state;
    }

    auto devmem = get_devmem_handle();
    if (std::holds_alternative<ErrorCode>(devmem))
    {
        return std::get<ErrorCode>(devmem);
    }

    Mmap code_area(nullptr,
                   PAYLOAD_MAX_SIZE,
                   PROT_READ | PROT_WRITE,
                   MAP_SHARED,
                   std::get<int>(devmem),
                   PAYLOAD_START);

    if (!code_area)
    {
        return ErrorCode::mmap_failed;
    }

    auto& inbox = getInboxNonvolatile();

    static const MemorySegment segments[]
    {
            { PAYLOAD_START, PAYLOAD_MAX_SIZE, code_area.data() },
    };

    writeCoreDump(filename,
                  segments,
                  inbox.regs,
                  inbox.fpregs);

    return {};
}

// ************************************************************

void Domain::dumpDebugInfo()
{
    fprintf(stderr, "debug: rdpos=%3zu wrpos=%3zu\n",
            getOutbox().stdout_rdpos,
            getInbox().stdout_wrpos);
}

// ************************************************************

MaybeError Domain::ensureReadyToLoadPayload()
{
    auto state = getState();

    if (state == DomainState::in_reset)
    {
        return startup();
    }
    else if (state == DomainState::monitor_ready)
    {
        return {};
    }
    else if (state == DomainState::crashed_payload ||
             state == DomainState::running_payload ||
             state == DomainState::starting_payload)
    {
        return terminatePayload();
    }
    else
    {
        return ErrorCode::bad_domain_state;
    }
}

// ************************************************************

CrashInfo Domain::getCrashInfo()
{
    auto& inbox = getInbox();

    return CrashInfo { .pc = inbox.fault_pc, .desc = std::string((char const*) inbox.fault_desc, sizeof(inbox.fault_desc)) };
}

// ************************************************************

DomainState Domain::getState()
{
    // FIXME: domain_general_state must take precedence
    // otherwise, for example after a failed monitor start-up, in_reset state is being reported

    if (domain_general_state[m_domain] == DomainGeneralState::inReset)
    {
        return DomainState::in_reset;
    }

    auto state_raw = getInbox().state;

    if (state_raw <= (int)DomainState::invalid_state)
    {
        return (DomainState) state_raw;
    }
    else
    {
        return DomainState::invalid_state;
    }
}

// ************************************************************

int Domain::getchar()
{
    auto const& inbox = getInbox();
    auto& outbox = getOutbox();

    if (outbox.stdout_rdpos >= sizeof(inbox.stdout_buf))
    {
        printf("bmboot: unexpected mst_stdout_rdpos %zx, resetting to 0\n", outbox.stdout_rdpos);
        outbox.stdout_rdpos = 0;
    }

    if (outbox.stdout_rdpos != inbox.stdout_wrpos)
    {
        char c = inbox.stdout_buf[outbox.stdout_rdpos];
        outbox.stdout_rdpos = (outbox.stdout_rdpos + 1) % sizeof(inbox.stdout_buf);
        return c;
    }
    else
    {
        return -1;
    }
}

// ************************************************************

MaybeError Domain::loadAndStartPayload(std::span<uint8_t const> payload_binary, uint32_t payload_crc32)
{
    // First, ensure we are in 'ready' state
    if (getState() != DomainState::monitor_ready)
    {
        return ErrorCode::bad_domain_state;
    }

    load_to_physical_memory(PAYLOAD_START, payload_binary);

    return startPayloadAt(PAYLOAD_START, payload_binary.size(), payload_crc32);
}

// ************************************************************

DomainInstanceOrErrorCode IDomain::open(DomainIndex domain)
{
    auto devmem = get_devmem_handle();
    if (std::holds_alternative<ErrorCode>(devmem))
    {
        return std::get<ErrorCode>(devmem);
    }

    auto ipc_block = (IpcBlock*) mmap(nullptr,
                                      MONITOR_IPC_SIZE,
                                      PROT_READ | PROT_WRITE,
                                      MAP_SHARED,
                                      std::get<int>(devmem),
                                      MONITOR_IPC_START);
    if (ipc_block == nullptr)
    {
        return ErrorCode::mmap_failed;
    }

    auto code_area = (uint8_t*) mmap(nullptr,
                                     MONITOR_CODE_SIZE,
                                     PROT_READ | PROT_WRITE,
                                     MAP_SHARED,
                                     std::get<int>(devmem),
                                     MONITOR_CODE_START);
    if (code_area == nullptr)
    {
        return ErrorCode::mmap_failed;
    }

    // It is not obvious how to determine whether the bmboot monitor is running on a given CPU core
    // We solve this by placing a special value -- a *cookie* at a fixed memory location when starting the monitor.
    // If this value is found there, we assume the monitor has been started up previously.

    // Look for cookie in the specified location
    Cookie cookie = -1;
    memcpy(&cookie, code_area + MONITOR_CODE_SIZE - sizeof(cookie), sizeof(cookie));

    if (cookie != MONITOR_CODE_COOKIE)
    {
        if (mach::isZynqCpu1InReset(std::get<int>(devmem)))
        {
            domain_general_state[domain] = DomainGeneralState::inReset;
        }
        else
        {
            // Core has been started, but not by us...
            domain_general_state[domain] = DomainGeneralState::unavailable;
        }
    }
    else
    {
        // Cookie found -> assume bmboot running (potentially with user payload)
        //
        // This can give a false positive if the startup failed or if the monitor crashed... tough luck.
        // A reboot is probably the only way out in that case, anyway.

        domain_general_state[domain] = DomainGeneralState::monitorStarted;
    }

    munmap(code_area, MONITOR_CODE_SIZE);

    return std::make_unique<Domain>(domain, *ipc_block);
}

// ************************************************************

MaybeError Domain::startPayloadAt(uintptr_t entry_address, size_t payload_size, uint32_t payload_crc32)
{
    // First, ensure we are in 'ready' state
    if (getState() != DomainState::monitor_ready)
    {
        return ErrorCode::bad_domain_state;
    }

    auto const& inbox = getInbox();
    auto& outbox = getOutbox();

    if (inbox.cmd_ack != outbox.cmd_seq)
    {
        return ErrorCode::bad_domain_state;
    }

    // flush any residual content of the stdout buffer by setting our read position equal to the write position
    outbox.stdout_rdpos = inbox.stdout_wrpos;

    outbox.payload_entry_address = entry_address;
    outbox.payload_size = payload_size;
    outbox.payload_crc = payload_crc32;
    outbox.cmd = Command::start_payload;
    memory_write_reorder_barrier();
    outbox.cmd_seq = (outbox.cmd_seq + 1);

    // wait up to 1sec for domain to come to life
    constexpr int timeout_msec = 1000;
    constexpr int poll_period_msec = 10;

    for (int i = 0; i < timeout_msec / poll_period_msec; i++)
    {
        usleep(poll_period_msec * 1000);

        if (inbox.cmd_ack == outbox.cmd_seq)
        {
            if (inbox.cmd_resp == Response::crc_mismatched)
            {
                return ErrorCode::payload_checksum_mismatch;
            }
            else
            {
                // Otherwise we expect Response::crc_ok, but we are waiting for DomainState::running_payload anyway
            }
        }

        auto state = getState();

        if (state == DomainState::running_payload)
        {
            return {};
        }
        else if (state == DomainState::crashed_payload)
        {
            return ErrorCode::payload_crashed_during_startup;
        }
    }

    // TODO: might want to latch DomainState::crashedPayload when this happens?
    return ErrorCode::payload_start_timed_out;
}

// ************************************************************

MaybeError Domain::startup()
{
    return startup(monitor_zynqmp_cpu1_payload);
}

// ************************************************************

MaybeError Domain::startup(std::span<uint8_t const> monitor_binary)
{
    if (domain_general_state[m_domain] != DomainGeneralState::inReset)
    {
        return ErrorCode::bad_domain_state;
    }

    auto devmem = get_devmem_handle();
    if (std::holds_alternative<ErrorCode>(devmem))
    {
        return std::get<ErrorCode>(devmem);
    }

    auto code_area = (uint8_t*) mmap(nullptr,
                                     MONITOR_CODE_SIZE,
                                     PROT_READ | PROT_WRITE,
                                     MAP_SHARED,
                                     std::get<int>(devmem),
                                     MONITOR_CODE_START);
    if (code_area == nullptr)
    {
        return ErrorCode::mmap_failed;
    }

    const auto cookie = MONITOR_CODE_COOKIE;

    if (monitor_binary.size() > MONITOR_CODE_SIZE - sizeof(cookie))
    {
        return ErrorCode::program_too_large;
    }

    memcpy(code_area, monitor_binary.data(), monitor_binary.size());      // won't work without cache flush to L2/DDR
    memcpy(code_area + MONITOR_CODE_SIZE - sizeof(cookie), &cookie, sizeof(cookie));

    // flush the newly written code from L1 through to DDR (since CPUn will come up in uncached mode)
    __clear_cache(code_area, code_area + MONITOR_CODE_SIZE);

    munmap(code_area, MONITOR_CODE_SIZE);

    // initialize IPC block
    memset((void*) &m_ipc_block, 0, MONITOR_IPC_SIZE);

    // flush the IPC region to DDR (since the SCU is not in effect yet and CPUn will come up with cold caches)
    __clear_cache(&m_ipc_block, (uint8_t*) &m_ipc_block + MONITOR_IPC_SIZE);

    mach::bootZynqCpu1(std::get<int>(devmem), MONITOR_CODE_START);

    // TODO: maybe we should only do this after state goes to ready
    domain_general_state[m_domain] = DomainGeneralState::monitorStarted;

    return awaitMonitorStartup();
}

// ************************************************************

MaybeError Domain::terminatePayload()
{
    if (domain_general_state[m_domain] != DomainGeneralState::monitorStarted)
    {
        return ErrorCode::bad_domain_state;
    }

    auto devmem = get_devmem_handle();
    if (std::holds_alternative<ErrorCode>(devmem))
    {
        return std::get<ErrorCode>(devmem);
    }

    // Clear any pending command (although none should have been sent in the current state)
    getOutbox().cmd = Command::noop;

    // TODO: Clean up a bit; these values have no meaning, but if/when we have multiple different IPIs, we will use
    //       this buffer to signal which one is being invoked.
    uint8_t message[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    mach::sendIpiMessage(std::get<int>(devmem), message);

    return awaitMonitorStartup();
}