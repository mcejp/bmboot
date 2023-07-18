//! @file
//! @brief  bmboot common definitions
//! @author Martin Cejp

#pragma once

#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>

//! Namespace containing common definitions
namespace bmboot
{

//! State of a domain. If a domain is being controlled by bmboot, it self-reports its state;
//! for the remaining states, the manager estimates it based on the available evidence.
enum DomainState
{
    in_reset,                   //!< The CPU core is in power-on reset
    monitor_ready,              //!< The monitor is ready to start a new payload
    starting_payload,           //!< A payload has been loaded and is initializing
    running_payload,            //!< A payload is executing
    crashed_payload,            //!< A payload has encountered an unrecoverable error
    unavailable,                //!< The CPU core is running but the bmboot monitor is not present
    invalid_state,              //!< The executors reports an invalid state
};

enum DomainIndex
{
    cpu1,
    max_domain,
};

enum ErrorCode
{
    bad_domain_state,                   //!< The requested operation is not permitted in the current state
    payload_start_timed_out,            //!< The payload failed to confirm a successful start-up within the timeout
    payload_checksum_mismatch,          //!< Payload checksum failed to validate
    payload_crashed_during_startup,     //!< The payload crashed before confirming a successful start-up
    program_too_large,                  //!< The provided program is too large
    monitor_start_timed_out,            //!< The monitor failed to confirm a successful start-up within the timeout

    // TODO: might want to just propagate the OS error for these?
    dev_mem_access_failed,              //!< Failed to access the @c /dev/mem special device
    mmap_failed,                        //!< The @c mmap function returned an error
};

//! Parse a domain index from its string representation
std::optional<DomainIndex> parseDomainIndex(std::string_view const& str);

//! Convert a domain state into its string representation
std::string toString(bmboot::DomainState state);

//! Convert an error code into its string representation
std::string toString(bmboot::ErrorCode err);

}
