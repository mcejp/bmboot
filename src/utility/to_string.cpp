#include "bmboot.hpp"

namespace bmboot {

std::string toString(bmboot::DomainState state) {
    switch (state) {
        case bmboot::DomainState::in_reset: return "in_reset";
        case bmboot::DomainState::monitor_ready: return "monitor_ready";
        case bmboot::DomainState::starting_payload: return "starting_payload";
        case bmboot::DomainState::running_payload: return "running_payload";
        case bmboot::DomainState::crashed_payload: return "crashed_payload";
        case bmboot::DomainState::unavailable: return "unavailable";
        case bmboot::DomainState::invalid_state: return "invalid_state";
        default: return "unknown state " + std::to_string((int) state);
    }
}

std::string toString(bmboot::ErrorCode err) {
    switch (err) {
        case bmboot::ErrorCode::bad_domain_state: return "bad domain state";
        case bmboot::ErrorCode::payload_crashed_during_startup: return "payload crashed during startup";
        case bmboot::ErrorCode::payload_start_timed_out: return "payload startup timed out";
        case bmboot::ErrorCode::monitor_start_timed_out: return "monitor startup timed out";
        default: return "error " + std::to_string((int) err);
    }
}

}
