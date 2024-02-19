#include <bmboot/payload_runtime.hpp>
#include <unistd.h>

#include "../executor/armv8a.hpp"

static void myHandler();

int main(int argc, char** argv)
{
    bmboot::notifyPayloadStarted();

    printf("Performance Monitor Unit demo\n");
    printf("PMCR_EL0 =     0x%08X\n", readSysReg(PMCR_EL0));
    printf("PMCCFLTR_EL0 = 0x%08X\n", readSysReg(PMCCFILTR_EL0));
    printf("PMCCNTR_EL0 =  %ld\n", readSysReg(PMCCNTR_EL0));

    bmboot::startCycleCounter();

    auto before = bmboot::getCycleCounterValue();
    usleep(10);
    auto after = bmboot::getCycleCounterValue();

    printf("before: %ld, after: %ld, delta: %ld\n", before, after, after - before);

    for (;;) {}
}
