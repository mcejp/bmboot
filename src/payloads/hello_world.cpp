#include <bmboot/payload_runtime.hpp>

#define ZYNQMP_UART_PUTCHAR (*(volatile uint32_t*) 0xFF000030)

int main(int argc, char** argv)
{
    ZYNQMP_UART_PUTCHAR = ':';

    bmboot::notifyPayloadStarted();

    for (;;) {}
}
