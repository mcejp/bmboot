#include <bmboot/payload_runtime.hpp>

#define INTERRUPT_ID_ADRIAN_100US   (121 + 4)
#define INTERRUPT_ID_ADRIAN_1000US  (121 + 5)

static void on100us();
static void on1000us();

int main(int argc, char** argv)
{
    bmboot::notifyPayloadStarted();

    printf("Adrian IRQ demo\n");

    bmboot::configureAndEnableInterrupt(INTERRUPT_ID_ADRIAN_100US, on100us);
    bmboot::configureAndEnableInterrupt(INTERRUPT_ID_ADRIAN_1000US, on1000us);

    for (;;) {}
}

static void on100us()
{
    static int cnt = 0;

    if (++cnt == 10000) {
        printf("10000x INTERRUPT_ID_ADRIAN_100US\n");
        cnt = 0;
    }
}

static void on1000us()
{
    static int cnt = 0;

    if (++cnt == 1000) {
        printf("1000x INTERRUPT_ID_ADRIAN_1000US\n");
        cnt = 0;
    }
}
