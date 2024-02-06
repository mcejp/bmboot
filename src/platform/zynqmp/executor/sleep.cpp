//! @file
//! @brief  Implementation of sleep & usleep from unistd.h
//! @author Martin Cejp

#include <unistd.h>

#include <bmboot/payload_runtime.hpp>

using namespace bmboot;

// ************************************************************

static uint64_t divideRoundingUp(uint64_t dividend, uint64_t divisor)
{
    return (dividend + divisor - 1) / divisor;
}

// ************************************************************

extern "C" unsigned int sleep(unsigned int seconds)
{
    usleep(seconds * 1'000'000UL);
    return 0;
}

extern "C" int usleep(unsigned long useconds)
{
    auto start = getBuiltinTimerValue();

    // Multiply microseconds by number of ticks per microsecond, rounding up
    // (The philosophy is to never sleep shorter than requested)
    auto end = start + divideRoundingUp(useconds * getBuiltinTimerFrequency(), 1'000'000);

    while (getBuiltinTimerValue() < end)
    {
    }

    return 0;
}
