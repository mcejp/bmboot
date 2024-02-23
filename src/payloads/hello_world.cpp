#include <bmboot/payload_runtime.hpp>

extern "C" void _vector_table();

int main(int argc, char** argv)
{
    bmboot::notifyPayloadStarted();

    auto abi_version = bmboot::getMonitorAbiVersion();

    printf("Hello world from CPU%d! Our base address is %p and the Payload Argument is %ld. ABI version %d.%d.\n",
           bmboot::getCpuIndex(),
           (void*) &_vector_table,
           bmboot::getPayloadArgument(),
           abi_version.major, abi_version.minor);
}
