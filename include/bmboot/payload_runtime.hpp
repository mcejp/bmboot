//! @file
//! @brief  Runtime functions for the payload
//! @author Martin Cejp

#pragma once

// workaround to allow including sstream indirectly via chrono: macro 'str' has been (re-)defined by Xilinx engineers
// in their infinite wisdom in xpseudo_asm_gcc.h and clashes with standard library's definition
#undef str
#include <chrono>

#include "bmboot.hpp"

#include <functional>

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
using InterruptHandler = std::function<void()>;

//! Get the frequency of the built-in timer.
//!
//! Per document 102379_0100_02_en (<em>Learn the architecture - Generic Timer</em>), this frequency should typically
//! be in the range of 1MHz to 50MHz. Yet, the default on ZCU102 is actually close to 100MHz.
//!
//! \return Frequency in Hz
inline uint32_t getBuiltinTimerFrequency()
{
    uint64_t freq;
    asm("mrs %0, CNTFRQ_EL0" : "=r" (freq));
    return freq;
}

//! Get the current value of the built-in (always running) timer.
//!
//! Implementations of this function typically include an Instruction Barrier (ISB), see e.g.
//! https://github.com/qemu/u-boot/blob/c84a00a647057d83ecfb081ca03c4865e4c1c1be/arch/arm/cpu/armv8/generic_timer.c.
//! This is to avoid misleading results due to out-of-order execution, particularly when using the timer to benchmark
//! program runtime (example: https://stackoverflow.com/q/58586320).
//! As the Cortex-A53 is an in-order architecture with limited prefetch/speculation capabilities, this should
//! be less of a concern, but we err on the side of caution.
//!
//! In Armv8.6 and later, there is a "self-synchronizing" version of this register (CNTPCTSS_EL0) which can be used
//! instead.
//!
//! \return Timer value
inline uint64_t getBuiltinTimerValue()
{
    uint64_t cntval;
    asm volatile("isb; mrs %0, CNTPCT_EL0" : "=r" (cntval));
    return cntval;
}

//! Retrieve the argument provided when calling bmboot::IDomain::loadAndStartPayload
//!
//! \return Argument value
uintptr_t getPayloadArgument();

//! Get the CPU core on which the program is executing
//!
//! \return CPU core number, counted from 0
int getCpuIndex();

//! Escalate to the monitor after a crash has been detected.
//! This would not normally be called by user code.
//!
//! @param desc A textual description of the error (will be trimmed to 32 characters)
//! @param address Code address of the crash
void notifyPayloadCrashed(const char* desc, uintptr_t address);

//! Notify the manager that the payload has started successfully.
void notifyPayloadStarted();

//! Configure the built-in periodic interrupt.
//!
//! \param period_us Interrupt period in microseconds
//! \param handler Funcion to be called
void setupPeriodicInterrupt(std::chrono::microseconds period_us, InterruptHandler handler);

//! Start the built-in periodic interrupt.
//!
//! @link bmboot::setupPeriodicInterrupt @endlink must be called first to configure the interrupt handler and period.
//! Otherwise, the call will fail.
void startPeriodicInterrupt();

//! Stop the periodic interrupt, if it is running.
void stopPeriodicInterrupt();

//! Configure the reception of a peripheral interrupt.
//!
//! @param interruptId Platform-specific interrupt ID
//! @param priority Interrupt priority. A high-priority interrupt may preempt a low priority one.
//! @param handler Callback function
void setupInterruptHandling(int interrupt_id, PayloadInterruptPriority priority, InterruptHandler handler);

//! Enable the reception of a peripheral interrupt.
//!
//! @link bmboot::setupInterruptHandling @endlink must be called first to configure the interrupt handler and priority.
//! Otherwise, the behavior is unpredictable.
//!
//! @param interruptId Platform-specific interrupt ID
void enableInterruptHandling(int interruptId);

//! Disable the reception of a peripheral interrupt.
//!
//! @param interruptId Platform-specific interrupt ID
void disableInterruptHandling(int interruptId);

//! Write to the standard output.
//!
//! @param data Data to write (normally in ASCII encoding)
//! @param size Number of bytes to written
//! @return Number of bytes actually written, which might be limited by available buffer space
int writeToStdout(void const* data, size_t size);

// TODO: can have some IPC here too

}
