#pragma once

#include <cstdint>
#include <cstdlib>
#include <string>

namespace bmboot {

enum DomainState {
    inReset,
    monitorReady,
    startingPayload,
    runningPayload,
    stoppingPayload,
    crashedPayload,
    unavailable,
    invalidState,
};

// unclear if we want any kind of run-time discovery
enum Domain {
    cpu1,
    max_domain,
};

enum ErrorCode {
    badDomainState,
    payloadStartTimedOut,
    payloadCrashedDuringStartup,
    programTooLarge,
//    slaveCodeRequired,
    monitorStartTimedOut,

    // TODO: might want to just propagate the OS error for these?
    devMemAccessFailed,
    mmapFailed,
};

struct CrashInfo {
    uintptr_t pc;
    std::string desc;
};

std::string to_string(bmboot::DomainState state);
std::string to_string(bmboot::ErrorCode err);

}
