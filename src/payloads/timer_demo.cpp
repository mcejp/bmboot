#include <bmboot/payload_runtime.hpp>

static void myHandler();

int main(int argc, char** argv)
{
    bmboot::notifyPayloadStarted();

    printf("hello from payload\n");

    bmboot::setupPeriodicInterrupt(1'000'000, myHandler);
    bmboot::startPeriodicInterrupt();

    for (;;) {}
}

static void myHandler()
{
    static int cnt = 0;
    printf("%dth event\n", ++cnt);

    if (cnt == 5) {
        bmboot::stopPeriodicInterrupt();
    }
}
