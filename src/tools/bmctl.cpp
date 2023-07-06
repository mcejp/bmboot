#include "bmboot/domain.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

static int usage() {
    fprintf(stderr, "usage: bmctl core\n");
    fprintf(stderr, "usage: bmctl exec <payload>\n");
    fprintf(stderr, "usage: bmctl reset\n");
    fprintf(stderr, "usage: bmctl status\n");
    fprintf(stderr, "usage: bmctl startup\n");
    return -1;
}

static void display_domain_state(bmboot::IDomain& domain) {
    auto state = domain.getState();

    puts(toString(state).c_str());

    if (state == bmboot::DomainState::crashed_payload) {
        auto error_info = domain.getCrashInfo();
        printf("(at address 0x%zx)\n", error_info.pc);
        printf("(description %s)\n", error_info.desc.c_str());
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        return usage();
    }

    printf("assuming domain 'cpu1'\n");
    auto which_domain = bmboot::DomainIndex::cpu1;

    auto maybe_domain = bmboot::IDomain::open(which_domain);

    if (!std::holds_alternative<std::unique_ptr<bmboot::IDomain>>(maybe_domain)) {
        fprintf(stderr, "open_domain: error: %s\n",
                toString(std::get<bmboot::ErrorCode>(maybe_domain)).c_str());
        return -1;
    }

    auto domain = std::move(std::get<std::unique_ptr<bmboot::IDomain>>(maybe_domain));

    if (strcmp(argv[1], "core") == 0) {
        auto err = domain->dumpCore("core");

        if (err.has_value()) {
            fprintf(stderr, "IDomain::dumpCore: error: %s\n", toString(*err).c_str());
            return -1;
        }
    }
    else if (strcmp(argv[1], "exec") == 0) {
        if (argc != 3) {
            return usage();
        }

        auto payload_filename = argv[2];

        auto state = domain->getState();

        if (state != bmboot::DomainState::monitor_ready) {
            fprintf(stderr, "cannot execute payload: domain state %s != monitorReady\n", bmboot::toString(state).c_str());
        }
        else {
            std::ifstream file(payload_filename, std::ios::binary);

            if (!file) {
                fprintf(stderr, "failed to open file\n");
                return -1;
            }

            std::vector<uint8_t> program((std::istreambuf_iterator<char>(file)),
                                         std::istreambuf_iterator<char>());

            auto err = domain->loadAndStartPayload(program);

            if (err.has_value()) {
                fprintf(stderr, "IDomain::loadAndStartPayload: error: %s\n", toString(*err).c_str());
                return -1;
            }
        }
    }
    else if (strcmp(argv[1], "reset") == 0) {
        auto err = domain->terminatePayload();

        if (err.has_value()) {
            fprintf(stderr, "IDomain::reset_domain: error: %s\n", toString(*err).c_str());
            return -1;
        }
    }
    else if (strcmp(argv[1], "startup") == 0) {
        auto state = domain->getState();

        if (state == bmboot::DomainState::in_reset) {
            auto err = domain->startup();

            if (err.has_value()) {
                fprintf(stderr, "IDomain::startup_domain: error: %s\n", toString(*err).c_str());
                return -1;
            }

            state = domain->getState();

            printf("domain state: %s\n", bmboot::toString(state).c_str());
        }
        else {
            fprintf(stderr, "cannot start domain up: domain state %s != inReset\n", bmboot::toString(state).c_str());
        }
    }
    else if (strcmp(argv[1], "status") == 0) {
        display_domain_state(*domain);
    }
    else {
        usage();
        return -1;
    }
}
