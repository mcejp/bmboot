#include <stdexcept>

#include <bmboot/payload_runtime.hpp>

int main(int argc, char** argv)
{
    bmboot::notifyPayloadStarted();

    try
    {
      throw std::runtime_error("Runtime exception thrown");
    }
    catch (std::exception& e)
    {
      printf("Caught exception: %s.\n", e.what());
    }
    printf("Application still responsive after exception handling.\n");

    for (;;) {}
}
