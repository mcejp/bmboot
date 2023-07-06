#include "bmboot/domain.hpp"

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

    auto maybe_domain = bmboot::IDomain::open(bmboot::DomainIndex::cpu1);

    if (!std::holds_alternative<std::unique_ptr<bmboot::IDomain>>(maybe_domain)) {
        fprintf(stderr, "open_domain: error: %s\n",
                toString(std::get<bmboot::ErrorCode>(maybe_domain)).c_str());
        return -1;
    }

    auto domain = std::move(std::get<std::unique_ptr<bmboot::IDomain>>(maybe_domain));

    if (domain->getState() == bmboot::DomainState::in_reset) {
        fprintf(stderr, "console: implicitly intializing domain\n");

        auto err = domain->startup();

        if (err.has_value()) {
            fprintf(stderr, "startup_domain: error: %s\n", toString(*err).c_str());
            return -1;
        }
    }

    if (domain->getState() == bmboot::DomainState::monitor_ready) {
        fprintf(stderr, "console: starting dummy payload to enable hot-reload\n");

        domain->startDummyPayload();
    }

    std::stringstream stdout_accum;

    for (;;) {
        int c = domain->getchar();
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
