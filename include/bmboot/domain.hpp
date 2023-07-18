//! @file
//! @brief  Domain management
//! @author Martin Cejp

#pragma once

#include "bmboot.hpp"

#include <cstdint>
#include <cstdlib>

#include <memory>
#include <optional>
#include <span>
#include <variant>

namespace bmboot
{

class IDomain;
using MaybeError = std::optional<ErrorCode>;
using DomainInstanceOrErrorCode = std::variant<std::unique_ptr<IDomain>, ErrorCode>;

struct CrashInfo
{
    uintptr_t pc;
    std::string desc;
};

//! An abstract class representing an executor domain
class IDomain
{
public:
    //! Open an executor domain.
    //!
    //! @param index Domain selector
    //! @return
    static DomainInstanceOrErrorCode open(DomainIndex index);

    virtual ~IDomain() {}

    //! Start the bmboot monitor in this domain.
    //! The monitor must be running before a user payload can be executed.
    //!
    //! This operation is permissible only when the domain state is @link bmboot::in_reset in_reset@endlink.
    virtual MaybeError startup() = 0;

    //! Query current state of the domain
    virtual DomainState getState() = 0;

    //! Terminate the payload, returning control to the monitor.
    virtual MaybeError terminatePayload() = 0;

    //! A shortcut function to call #startup or #terminatePayload, if necessary
    //!
    //! If the function returns with success, the domain state will be @link bmboot::monitor_ready monitor_ready@endlink.
    //!
    //! @return
    virtual MaybeError ensureReadyToLoadPayload() = 0;

    //! Load and execute the given payload.
    //!
    //! This operation is permissible only when the domain state is @link bmboot::monitor_ready monitor_ready@endlink.
    virtual MaybeError loadAndStartPayload(std::span<uint8_t const> payload_binary, uint32_t payload_crc32) = 0;

    //! Read a character from the executor's standard output. This function should be polled on a regular basis.
    //!
    //! @return The character read, or -1 if no output is pending.
    virtual int getchar() = 0;

    //! Produce a Linux-compatible core dump for a crashed executor.
    //!
    //! @param filename Name of the file to be generated
    //! @return
    virtual MaybeError dumpCore(char const* filename) = 0;

    //! Write miscellaneous debug information to the standard output. No guarantees are made about the content.
    virtual void dumpDebugInfo() = 0;

    //! Return some information about a crash of the executor
    virtual CrashInfo getCrashInfo() = 0;

    //! Start an idle payload. This mechanism is used to enable payloads to be started from Vitis.
    virtual void startDummyPayload() = 0;
};

}
