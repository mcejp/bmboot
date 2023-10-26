#include <bmboot/payload_runtime.hpp>

#include <unistd.h>

#define INTERRUPT_ID_ADRIAN_100US   (121 + 4)
#define INTERRUPT_ID_ADRIAN_1000US  (121 + 5)

static void on100us();
static void on1000us();

static volatile bool high_active = 0;
static volatile bool low_active = 0;

static volatile int high_count = 0;
static volatile int low_count = 0;
static volatile int high_nested = 0;
static volatile int low_nested = 0;

int main(int argc, char** argv)
{
    bmboot::notifyPayloadStarted();

    printf("Adrian IRQ demo\nNOTE: This code only works with a specific, unofficial gateware build.\n\n");

    bmboot::setupInterruptHandling(INTERRUPT_ID_ADRIAN_100US, bmboot::PayloadInterruptPriority::p1, on100us);
    bmboot::setupInterruptHandling(INTERRUPT_ID_ADRIAN_1000US, bmboot::PayloadInterruptPriority::p0_min, on1000us);
    bmboot::enableInterruptHandling(INTERRUPT_ID_ADRIAN_100US);
    bmboot::enableInterruptHandling(INTERRUPT_ID_ADRIAN_1000US);

    for (;;) {
        sleep(1);

        // expectations:
        //  high increasing by 10k every second
        //  low increasing by 1k every second
        //  high-nested increasing by ~3k every second depending on the delay of the low-priority interrupt
        //  low-nested should stay zero, otherwise it indicates a priority inversion
        printf("high=%d, low=%d low-nested=%d high-nested=%d\n",
               high_count, low_count, low_nested, high_nested);
    }
}

static void on100us()
{
    high_active = true;

    if (low_active) {
        high_nested++;
    }

    high_count++;

//    if (++cnt == 10000) {
//        printf("10000x INTERRUPT_ID_ADRIAN_100US\n");
//        cnt = 0;
//    }

    high_active = false;
}

static void on1000us()
{
    low_active = true;

    if (high_active) {
        low_nested++;
    }

//    if (++cnt == 1000) {
//        printf("1000x INTERRUPT_ID_ADRIAN_1000US\n");
//        cnt = 0;
//    }

    low_count++;

    // block for a while to trigger preemption by higher-priority interrupt
    usleep(300);

    low_active = false;
}
