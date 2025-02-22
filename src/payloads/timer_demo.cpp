#include <chrono>

#include "../executor/armv8a.hpp"
#include <bmboot/payload_runtime.hpp>

static void myHandler();

int main(int argc, char** argv)
{
    bmboot::notifyPayloadStarted();

    printf("hello from payload\n");

    bmboot::setupPeriodicInterrupt(std::chrono::microseconds(1'000'000), myHandler);
    bmboot::startPeriodicInterrupt();

    // do not exit the program while interrupt is active
    for (;;) {
        arm::armv8a::waitForInterrupt();
    }
}

static void myHandler()
{
    static int cnt = 0;
    printf("%dth event\n", ++cnt);

    if (cnt == 5) {
        bmboot::stopPeriodicInterrupt();
    }
}
