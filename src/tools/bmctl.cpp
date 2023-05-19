#include "bmboot_master.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

static int usage() {
    fprintf(stderr, "usage: bmctl boot <payload>\n");
    fprintf(stderr, "usage: bmctl reset\n");
    fprintf(stderr, "usage: bmctl status\n");
    fprintf(stderr, "usage: bmctl startup\n");
    return -1;
}

static void display_domain_state(bmboot_m::DomainHandle const& domain) {
    auto state = bmboot_m::get_domain_state(domain);

    puts(to_string(state).c_str());

    if (state == bmboot::DomainState::crashedPayload) {
        auto error_info = bmboot_m::get_crash_info(domain);
        printf("(at address 0x%zx)\n", error_info.pc);
        printf("(description %s)\n", error_info.desc.c_str());
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        return usage();
    }

    printf("assuming domain 'cpu1'\n");
    auto which_domain = bmboot::Domain::cpu1;

    auto maybe_domain = bmboot_m::open_domain(which_domain);

    if (!std::holds_alternative<bmboot_m::DomainHandle>(maybe_domain)) {
        fprintf(stderr, "open_domain: error: %s\n",
                to_string(std::get<bmboot::ErrorCode>(maybe_domain)).c_str());
        return -1;
    }

    auto domain = std::get<bmboot_m::DomainHandle>(maybe_domain);

    if (strcmp(argv[1], "exec") == 0) {
        if (argc != 3) {
            return usage();
        }

        auto payload_filename = argv[2];

        auto state = bmboot_m::get_domain_state(domain);

        if (state != bmboot::DomainState::monitorReady) {
            fprintf(stderr, "cannot execute payload: domain state %s != monitorReady\n", bmboot::to_string(state).c_str());
        }
        else {
            std::ifstream file(payload_filename, std::ios::binary);

            if (!file) {
                fprintf(stderr, "failed to open file\n");
                return -1;
            }

            std::vector<uint8_t> program((std::istreambuf_iterator<char>(file)),
                                         std::istreambuf_iterator<char>());

            auto err = bmboot_m::load_and_start_payload(domain, program);

            if (err.has_value()) {
                fprintf(stderr, "startup_domain: error: %s\n", to_string(*err).c_str());
                return -1;
            }
        }
    }
    else if (strcmp(argv[1], "reset") == 0) {
        auto err = bmboot_m::reset_domain(domain);

        if (err.has_value()) {
            fprintf(stderr, "reset_domain: error: %s\n", to_string(*err).c_str());
            return -1;
        }
    }
    else if (strcmp(argv[1], "startup") == 0) {
        auto state = bmboot_m::get_domain_state(domain);

        if (state == bmboot::DomainState::inReset) {
            auto err = bmboot_m::startup_domain(domain);

            if (err.has_value()) {
                fprintf(stderr, "startup_domain: error: %s\n", to_string(*err).c_str());
                return -1;
            }

            auto state = bmboot_m::get_domain_state(domain);

            printf("domain state: %s\n", to_string(state).c_str());
        }
        else {
            fprintf(stderr, "cannot start domain up: domain state %s != inReset\n", bmboot::to_string(state).c_str());
        }
    }
    else if (strcmp(argv[1], "status") == 0) {
        display_domain_state(domain);
    }
    else {
        usage();
        return -1;
    }
}
