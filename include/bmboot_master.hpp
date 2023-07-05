#pragma once

#include "bmboot.hpp"

#include <cstdint>
#include <cstdlib>

#include <optional>
#include <span>
#include <variant>

namespace bmboot_m {

using namespace bmboot;

struct DomainHandle {
    Domain domain;
    void* ipc_block;
};

using MaybeError = std::optional<ErrorCode>;
using DomainHandleOrErrorCode = std::variant<DomainHandle, ErrorCode>;

DomainHandleOrErrorCode open_domain(Domain domain);

// Load the monitor code at the appropriate address + take the CPU out of reset
// TODO: will there be separate builds for different domains?
MaybeError startup_domain(DomainHandle const& domain);
MaybeError startup_domain(DomainHandle const& domain, std::span<uint8_t const> bmboot_slave_binary);

CrashInfo get_crash_info(DomainHandle const& domain);
DomainState get_domain_state(DomainHandle const& domain);
//MaybeError set_domain_state(Domain domain, DomainState state);
MaybeError reset_domain(DomainHandle const& domain);

MaybeError load_and_start_payload(DomainHandle const& domain, std::span<uint8_t const> payload_binary);
MaybeError start_payload_at(DomainHandle const& domain, uintptr_t entry_address);

int stdout_getchar(DomainHandle const& domain);

MaybeError dump_core(DomainHandle const& domain, char const* filename);
void dump_debug_info(Domain domain);

}
