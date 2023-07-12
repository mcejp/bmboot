#include <bmboot/payload_runtime.hpp>

#include "sleep.h"

[[gnu::noinline]] [[noreturn]]
static void access_invalid_memory()
{
    *(volatile int*)0x00F0000000 = 123;
}

int main(int argc, char** argv)
{
    bmboot::notifyPayloadStarted();

    // must not be too fast, otherwise it triggers payloadCrashedDuringStartup
    usleep(10'000);

    // something to find in subsequent core dump
    __asm__ volatile ("fmov d0, #0.5");
    __asm__ volatile ("fmov d1, #0.25");

    // BOOM
    access_invalid_memory();
}
