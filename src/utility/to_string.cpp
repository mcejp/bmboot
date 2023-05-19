#include "bmboot.hpp"

namespace bmboot {

std::string to_string(bmboot::DomainState state) {
    switch (state) {
        case bmboot::DomainState::inReset: return "inReset";
        case bmboot::DomainState::monitorReady: return "monitorReady";
        case bmboot::DomainState::startingPayload: return "startingPayload";
        case bmboot::DomainState::runningPayload: return "runningPayload";
        case bmboot::DomainState::stoppingPayload: return "stoppingPayload";
        case bmboot::DomainState::crashedPayload: return "crashedPayload";
        case bmboot::DomainState::unavailable: return "unavailable";
        case bmboot::DomainState::invalidState: return "invalidState";
        default: return "unknown state " + std::to_string((int) state);
    }
}

std::string to_string(bmboot::ErrorCode err) {
    switch (err) {
        case bmboot::ErrorCode::badDomainState: return "bad domain state";
        case bmboot::ErrorCode::payloadCrashedDuringStartup: return "payload crashed during startup";
        case bmboot::ErrorCode::payloadStartTimedOut: return "payload startup timed out";
        case bmboot::ErrorCode::monitorStartTimedOut: return "monitor startup timed out";
        default: return "error " + std::to_string((int) err);
    }
}

}
