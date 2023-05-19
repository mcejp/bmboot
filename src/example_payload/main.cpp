#include <bmboot_slave.hpp>

int main(int argc, char** argv) {
    *(int*)0xFF000030 = ':';

    bmboot_s::notify_payload_started();

    for (;;) {}
}
