//! @file
//! @brief  Runtime functions for the payload
//! @author Martin Cejp

#include <bmboot/payload_runtime.hpp>

#include "executor.hpp"
#include "executor_asm.hpp"
#include "payload_runtime_internal.hpp"

#include "bspconfig.h"
#include "xpseudo_asm.h"

using namespace bmboot;
using namespace bmboot::internal;

static uint64_t timer_period_ticks;
static InterruptHandler timer_irq_handler;

InterruptHandler internal::user_interrupt_handlers[(GIC_MAX_USER_INTERRUPT_ID + 1) - GIC_MIN_USER_INTERRUPT_ID];

void bmboot::configureAndEnableInterrupt(int interruptId, PayloadInterruptPriority priority, InterruptHandler handler)
{
    if (interruptId < GIC_MIN_USER_INTERRUPT_ID || interruptId > GIC_MAX_USER_INTERRUPT_ID)
    {
        // TODO: signal error
        return;
    }

    user_interrupt_handlers[interruptId - GIC_MIN_USER_INTERRUPT_ID] = handler;

    smc(SMC_ZYNQMP_GIC_SPI_CONFIGURE_AND_ENABLE, interruptId, (int) priority);
}

void bmboot::startPeriodicInterrupt(int period_us, InterruptHandler handler)
{
    // Perhaps this could be refactored so that only the IRQ setup is done via SMC and we configure the timer directly

    // ticks = duration_us * timer_freq_Hz / 1e6
    timer_period_ticks = (uint64_t) period_us * mfcp(CNTFRQ_EL0) / 1'000'000;
    timer_irq_handler = handler;

    smc(SMC_START_PERIODIC_INTERRUPT, timer_period_ticks);
}

void bmboot::stopPeriodicInterrupt()
{
    smc(SMC_STOP_PERIODIC_INTERRUPT);
}

void internal::handleTimerIrq()
{
    if (timer_irq_handler)
    {
        timer_irq_handler();
    }
    else
    {
        // TBD: spurious interrupt -- log a warning?
    }

    // must either set new deadline...
    mtcp(CNTP_CVAL_EL0, mfcp(CNTP_CVAL_EL0) + timer_period_ticks);
    // ...or disable the interrupt
//    mtcp(CNTP_CTL_EL0, 0);
}

int bmboot::getCpuIndex()
{
    return internal::getCpuIndex();
}

void bmboot::notifyPayloadCrashed(const char* desc, uintptr_t address)
{
    smc(SMC_NOTIFY_PAYLOAD_CRASHED, desc, address);
}

void bmboot::notifyPayloadStarted()
{
    smc(SMC_NOTIFY_PAYLOAD_STARTED);
}

int bmboot::writeToStdout(void const* data, size_t size)
{
    return smc(SMC_WRITE_STDOUT, data, size);
}
