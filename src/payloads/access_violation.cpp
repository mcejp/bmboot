#include <bmboot_slave.hpp>

#include "sleep.h"

int main(int argc, char** argv) {
    bmboot_s::notify_payload_started();

    // must not be too fast, otherwise it triggers payloadCrashedDuringStartup
    usleep(10'000);

    // BOOM
    *(volatile int*)0x00F0000000 = 123;

    for (;;) {}
}
