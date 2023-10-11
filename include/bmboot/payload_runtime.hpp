//! @file
//! @brief  Runtime functions for the payload
//! @author Martin Cejp

#pragma once

#include "bmboot.hpp"

namespace bmboot
{

// higher value = lower priority
// the resolution is implementation-defined, on the Zynq the upper 4 bits of the byte should be taken into consideration
enum class PayloadInterruptPriority
{
    p7_max = 0x80,
    p6 = 0x90,
    p5 = 0xA0,
    p4 = 0xB0,
    p3 = 0xC0,
    p2 = 0xD0,
    p1 = 0xE0,
    p0_min = 0xF0,
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

void configureAndEnableInterrupt(int interruptId, PayloadInterruptPriority priority, InterruptHandler handler);

//! Write to the standard output.
//!
//! @param data Data to write (normally in ASCII encoding)
//! @param size Number of bytes to written
//! @return Number of bytes actually written, which might be limited by available buffer space
int writeToStdout(void const* data, size_t size);

// TODO: can have some IPC here too

}
