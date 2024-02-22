//! @file
//! @brief  Runtime functions for the payload, C version. This should be done in a more systematic way.
//! @author Martin Cejp

#pragma once

#include <stdint.h>

// For documentation, see the corresponding functions in paylaod_runtime.hpp. Sorry.

inline uint32_t bmGetBuiltinTimerFrequency()
{
    uint64_t freq;
    asm("mrs %0, CNTFRQ_EL0" : "=r" (freq));
    return freq;
}

inline uint64_t bmGetBuiltinTimerValue()
{
    uint64_t cntval;
    asm volatile("isb; mrs %0, CNTPCT_EL0" : "=r" (cntval));
    return cntval;
}

inline uint64_t bmGetCycleCounterValue()
{
    uint64_t cntval;
    asm volatile("isb; mrs %0, PMCCNTR_EL0" : "=r" (cntval));
    return cntval;
}

void bmNotifyPayloadStarted();
void bmStartCycleCounter();
