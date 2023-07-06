#include <bmboot/payload_runtime.hpp>

int main(int argc, char** argv) {
    *(int*)0xFF000030 = ':';

    bmboot::notifyPayloadStarted();

    for (;;) {}
}
