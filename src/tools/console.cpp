#include "bmboot_master.hpp"

#include <sstream>
#include <thread>

using namespace std::chrono_literals;

static int usage() {
    fprintf(stderr, "usage: console\n");
    return -1;
}

int main(int argc, char** argv) {
    if (argc != 1) {
        return usage();
    }

    printf("assuming domain 'cpu1'\n");

    auto maybe_domain = bmboot_m::open_domain(bmboot::Domain::cpu1);

    if (!std::holds_alternative<bmboot_m::DomainHandle>(maybe_domain)) {
        fprintf(stderr, "open_domain: error: %s\n",
                to_string(std::get<bmboot::ErrorCode>(maybe_domain)).c_str());
        return -1;
    }

    auto domain = std::get<bmboot_m::DomainHandle>(maybe_domain);

    if (bmboot_m::get_domain_state(domain) == bmboot::DomainState::inReset) {
        fprintf(stderr, "console: implicitly intializing domain\n");

        auto err = bmboot_m::startup_domain(domain);

        if (err.has_value()) {
            fprintf(stderr, "startup_domain: error: %s\n", to_string(*err).c_str());
            return -1;
        }
    }

    if (bmboot_m::get_domain_state(domain) == bmboot::DomainState::monitorReady) {
        fprintf(stderr, "console: starting dummy payload to enable hot-reload\n");

        // we _know_ that this will time out, don't bother checking the result
        bmboot_m::start_payload_at(domain, 0xbaadf00d);
    }

    std::stringstream stdout_accum;

    for (;;) {
        int c = bmboot_m::stdout_getchar(domain);
        if (c >= 0) {
            if (c == '\n') {
                printf("%s\n", stdout_accum.str().c_str());
                std::stringstream().swap(stdout_accum);         // https://stackoverflow.com/a/23266418
            }
            else {
                stdout_accum << (char)c;
            }
        }
        else {
            std::this_thread::sleep_for(1ms);
        }
    }
}
