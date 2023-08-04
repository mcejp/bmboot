#include <bmboot/payload_runtime.hpp>

int main(int argc, char** argv)
{
    bmboot::notifyPayloadStarted();

    printf("Hello, world!\n");

    for (;;) {}
}
