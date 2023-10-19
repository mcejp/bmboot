#include <bmboot/payload_runtime.hpp>

extern "C" void _vector_table();

int main(int argc, char** argv)
{
    bmboot::notifyPayloadStarted();

    printf("Hello world from CPU%d! Our base address is %p.\n", bmboot::getCpuIndex(), &_vector_table);

    for (;;) {}
}
