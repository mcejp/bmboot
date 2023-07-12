//! @file
//! @brief  Implementation of toString function
//! @author Martin Cejp

#include "bmboot.hpp"

using namespace bmboot;
using namespace std::string_view_literals;

// ************************************************************

std::optional<DomainIndex> bmboot::parseDomainIndex(std::string_view const& str) {
    if (str == "cpu1"sv) {
        return DomainIndex::cpu1;
    }
    else {
        return {};
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
        case DomainState::unavailable: return "unavailable";
        case DomainState::invalid_state: return "invalid_state";
        default: return "unknown state " + std::to_string((int) state);
    }
}

// ************************************************************

std::string bmboot::toString(ErrorCode err) {
    switch (err) {
        case ErrorCode::bad_domain_state: return "bad domain state";
        case ErrorCode::payload_crashed_during_startup: return "payload crashed during startup";
        case ErrorCode::payload_start_timed_out: return "payload startup timed out";
        case ErrorCode::monitor_start_timed_out: return "monitor startup timed out";
        default: return "error " + std::to_string((int) err);
    }
}
