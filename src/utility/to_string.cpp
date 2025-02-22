//! @file
//! @brief  Implementation of toString function
//! @author Martin Cejp

#include "bmboot.hpp"

using namespace bmboot;
using namespace std::string_view_literals;

// ************************************************************

std::optional<DomainIndex> bmboot::parseDomainIndex(std::string_view const& str)
{
    if (str == "cpu1"sv)
    {
        return DomainIndex::cpu1;
    }
    else if (str == "cpu2"sv)
    {
        return DomainIndex::cpu2;
    }
    else if (str == "cpu3"sv)
    {
        return DomainIndex::cpu3;
    }
    else {
        return {};
    }
}

// ************************************************************

std::string bmboot::toString(DomainIndex index)
{
    switch (index)
    {
        case DomainIndex::cpu1: return "cpu1";
        case DomainIndex::cpu2: return "cpu2";
        case DomainIndex::cpu3: return "cpu3";
        default:                return "invalid";
    }
}

// ************************************************************

std::string bmboot::toString(DomainState state) {
    switch (state) {
        case DomainState::in_reset: return "in_reset";
        case DomainState::monitor_ready: return "monitor_ready";
        case DomainState::starting_payload: return "starting_payload";
        case DomainState::running_payload: return "running_payload";
        case DomainState::crashed_payload: return "crashed_payload";
        case DomainState::crashed_monitor: return "crashed_monitor";
        case DomainState::unavailable: return "unavailable";
        case DomainState::invalid_state: return "invalid_state";
        default: return "unknown state " + std::to_string((int) state);
    }
}

// ************************************************************

std::string bmboot::toString(ErrorCode err) {
    switch (err) {
        case ErrorCode::bad_domain_state: return "bad domain state";
        case ErrorCode::configuration_file_error: return "/etc/bmboot.conf not found or malformed (see docs)";
        case ErrorCode::hw_resource_unavailable: return "a requested hardware resource is not available";
        case ErrorCode::payload_abi_incompatible: return "payload was built against an incompatible ABI version";
        case ErrorCode::payload_crashed_during_startup: return "payload crashed during startup";
        case ErrorCode::payload_image_malformed: return "provided file is not a valid Bmboot payload";
        case ErrorCode::payload_start_timed_out: return "payload startup timed out";
        case ErrorCode::program_too_large: return "program too large, or wrong load address";
        case ErrorCode::monitor_start_timed_out: return "monitor startup timed out";
        case ErrorCode::unknown_error: return "unknown error";
        default: return "error " + std::to_string((int) err);
    }
}
