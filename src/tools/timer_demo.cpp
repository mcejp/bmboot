//! @file
//! @brief  bmboot timers & IRQ demo
//! @author Martin Cejp

#include "bmboot/domain.hpp"
#include "bmboot/domain_helpers.hpp"

using namespace bmboot;

// ************************************************************

int main(int argc, char** argv)
{
    auto domain = throwOnError(IDomain::open(DomainIndex::cpu1), "IDomain::open");
    throwOnError(domain->ensureReadyToLoadPayload(), "ensureReadyToLoadPayload");
    loadPayloadFromFileOrThrow(*domain, "payload_timer_demo.bin");

    fprintf(stderr, "Program started; ready for output (Ctrl-C to quit)\n");

    displayOutputContinuously(*domain);
}
