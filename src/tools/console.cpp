//! @file
//! @brief  bmboot serial console
//! @author Martin Cejp

#include "bmboot/domain.hpp"
#include "bmboot/domain_helpers.hpp"

using namespace bmboot;

// ************************************************************

static int usage()
{
    fprintf(stderr, "usage: console <domain>\n");
    return -1;
}

// ************************************************************

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return usage();
    }

    auto domain_index = parseDomainIndex(argv[1]);

    if (!domain_index.has_value())
    {
        fprintf(stderr, "console: unknown domain '%s'\n", argv[1]);
        return -1;
    }

    auto domain = throwOnError(IDomain::open(*domain_index), "IDomain::open");

    if (domain->getState() == DomainState::in_reset)
    {
        fprintf(stderr, "console: implicitly intializing domain\n");

        auto err = domain->startup();

        if (err.has_value())
        {
            fprintf(stderr, "startup_domain: error: %s\n", toString(*err).c_str());
            return -1;
        }
    }

    if (domain->getState() == DomainState::monitor_ready)
    {
        fprintf(stderr, "console: starting dummy payload to enable hot-reload\n");

        domain->startDummyPayload();
    }

    displayOutputContinuously(*domain);
}
