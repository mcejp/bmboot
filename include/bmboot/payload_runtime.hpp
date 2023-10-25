//! @file
//! @brief  Runtime functions for the payload
//! @author Martin Cejp

#pragma once

#include "bmboot.hpp"

namespace bmboot
{

//! User interrupt priority.
//!
//! Higher numerical value corresponds to lower priority.
//! The resolution is implementation-defined, on the Zynq the upper 4 bits of the byte matter for preemption purposes.
enum class PayloadInterruptPriority
{
    p7_max = 0x80,         //!< Highest priority available to payload code (0x80)
    p6 = 0x90,             //!< Intermediate priority (0x90)
    p5 = 0xA0,             //!< Intermediate priority (0xA0)
    p4 = 0xB0,             //!< Intermediate priority (0xB0)
    p3 = 0xC0,             //!< Intermediate priority (0xC0)
    p2 = 0xD0,             //!< Intermediate priority (0xD0)
    p1 = 0xE0,             //!< Intermediate priority (0xE0)
    p0_min = 0xF0,         //!< Lowest priority (0xF0)
};

//! Callback function for the periodic interrupt
using InterruptHandler = void (*)();

//! Escalate to the monitor after a crash has been detected.
//! This would not normally be called by user code.
//!
//! @param desc A textual description of the error (will be trimmed to 32 characters)
//! @param address Code address of the crash
void notifyPayloadCrashed(const char* desc, uintptr_t address);

//! Notify the manager that the payload has started successfully.
void notifyPayloadStarted();

//! Start a periodic interrupt.
//!
//! \param period_us Interrupt period in microseconds
//! \param handler Funcion to be called
void startPeriodicInterrupt(int period_us, InterruptHandler handler);

//! Stop the periodic interrupt, if it is running.
void stopPeriodicInterrupt();

//! Configure and enable the reception of a peripheral interrupt.
//!
//! @param interruptId Platform-specific interrupt ID
//! @param priority Interrupt priority. A high-priority interrupt may preempt a low priority one.
//! @param handler Callback function
void configureAndEnableInterrupt(int interruptId, PayloadInterruptPriority priority, InterruptHandler handler);

//! Write to the standard output.
//!
//! @param data Data to write (normally in ASCII encoding)
//! @param size Number of bytes to written
//! @return Number of bytes actually written, which might be limited by available buffer space
int writeToStdout(void const* data, size_t size);

// TODO: can have some IPC here too

}
