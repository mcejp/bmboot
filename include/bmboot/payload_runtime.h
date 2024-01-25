//! @file
//! @brief  Runtime functions for the payload
//! @author Martin Cejp

#pragma once

#include <stdint.h>

inline uint32_t bmGetBuiltinTimerFrequency()
{
    uint64_t freq;
    asm("mrs %0, CNTFRQ_EL0" : "=r" (freq));
    return freq;
}

inline uint64_t bmGetBuiltinTimerValue()
{
    // NOTE: should perhaps use CNTPCTSS_EL0 & ISB; see D10.2.1 in AArch64 ARM
    uint64_t cntval;
    asm volatile("mrs %0, CNTPCT_EL0" : "=r" (cntval));
    return cntval;
}

void bmNotifyPayloadStarted();
