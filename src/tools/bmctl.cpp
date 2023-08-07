//! @file
//! @brief  bmctl utility
//! @author Martin Cejp

#include "bmboot/domain.hpp"
#include "bmboot/domain_helpers.hpp"

#include <cstdio>
#include <cstring>

using namespace bmboot;

// ************************************************************

static int usage()
{
    fprintf(stderr, "usage: bmctl core <domain>\n");
    fprintf(stderr, "usage: bmctl exec <domain> <payload>\n");
    fprintf(stderr, "usage: bmctl reset <domain>\n");
    fprintf(stderr, "usage: bmctl status <domain>\n");
    fprintf(stderr, "usage: bmctl startup <domain>\n");
    return -1;
}

// ************************************************************

static void display_domain_state(IDomain& domain)
{
    auto state = domain.getState();

    puts(toString(state).c_str());

    if (state == DomainState::crashed_payload)
    {
        auto error_info = domain.getCrashInfo();
        printf("(at address 0x%zx)\n", error_info.pc);
        printf("(description %s)\n", error_info.desc.c_str());
    }
}

// ************************************************************

int main(int argc, char** argv)
{
    // each sub-command takes domain as 1st parameter
    if (argc < 3)
    {
        return usage();
    }

    auto domain_index = parseDomainIndex(argv[2]);

    if (!domain_index.has_value())
    {
        fprintf(stderr, "bmctl: unknown domain '%s'\n", argv[2]);
        return -1;
    }

    auto domain = throwOnError(IDomain::open(*domain_index), "IDomain::open");

    if (strcmp(argv[1], "core") == 0)
    {
        auto err = domain->dumpCore("core");

        if (err.has_value())
        {
            fprintf(stderr, "IDomain::dumpCore: error: %s\n", toString(*err).c_str());
            return -1;
        }
    }
    else if (strcmp(argv[1], "exec") == 0)
    {
        if (argc != 4)
        {
            return usage();
        }

        auto payload_filename = argv[3];

        auto state = domain->getState();

        if (state != DomainState::monitor_ready)
        {
            fprintf(stderr, "cannot execute payload: domain state %s != monitorReady\n", toString(state).c_str());
        }
        else
        {
            loadPayloadFromFileOrThrow(*domain, payload_filename);
        }
    }
    else if (strcmp(argv[1], "reset") == 0)
    {
        auto err = domain->terminatePayload();

        if (err.has_value())
        {
            fprintf(stderr, "IDomain::reset_domain: error: %s\n", toString(*err).c_str());
            return -1;
        }
    }
    else if (strcmp(argv[1], "startup") == 0)
    {
        auto state = domain->getState();

        if (state == DomainState::in_reset)
        {
            auto err = domain->startup();

            if (err.has_value())
            {
                fprintf(stderr, "IDomain::startup_domain: error: %s\n", toString(*err).c_str());
                return -1;
            }

            state = domain->getState();

            printf("domain state: %s\n", toString(state).c_str());
        }
        else
        {
            fprintf(stderr, "cannot start domain up: domain state %s != inReset\n", toString(state).c_str());
        }
    }
    else if (strcmp(argv[1], "status") == 0)
    {
        display_domain_state(*domain);
    }
    else
    {
        usage();
        return -1;
    }
}
