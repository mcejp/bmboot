#include "bmboot_master.hpp"

#include <gtest/gtest.h>

#include <fstream>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace bmboot_m;
using namespace std::chrono_literals;

struct BmbootFixture : public ::testing::Test
{
    void SetUp() override
    {
        auto which_domain = bmboot::Domain::cpu1;

        auto maybe_domain = open_domain(which_domain);

        if (!std::holds_alternative<DomainHandle>(maybe_domain)) {
            throw std::runtime_error("open_domain: error: " + to_string(std::get<bmboot::ErrorCode>(maybe_domain)));
        }

        this->domain = std::get<DomainHandle>(maybe_domain);

        auto state = get_domain_state(domain);

        if (state == DomainState::inReset) {
            auto err = bmboot_m::startup_domain(domain);

            if (err.has_value()) {
                throw std::runtime_error("startup_domain: error: " + to_string(*err));
            }

            state = get_domain_state(domain);
        }
        else if (state != DomainState::monitorReady) {
            // attempt to reset it

            auto err = bmboot_m::reset_domain(domain);

            if (err.has_value()) {
                throw std::runtime_error("reset_domain: error: " + to_string(*err));
            }

            state = get_domain_state(domain);
        }

        if (state != DomainState::monitorReady) {
            throw std::runtime_error("ensure_monitor_ready: bad state " + to_string(state));
        }
    }

    void TearDown() override
    {
    }

    static void throw_for_err(MaybeError err) {
        if (err.has_value()) {
            throw std::runtime_error(to_string(*err));
        }
    }

    void execute_payload(const char* filename) const {
        std::ifstream file(filename, std::ios::binary);

        if (!file) {
            throw std::runtime_error((std::string) "failed to open " + filename);
        }

        std::vector<uint8_t> program((std::istreambuf_iterator<char>(file)),
                                     std::istreambuf_iterator<char>());

        throw_for_err(load_and_start_payload(domain, program));
    }

    DomainHandle domain;
};

TEST_F(BmbootFixture, hello_world) {
    // synopsis of test:
    // 1. load payload_hello_world
    // 2. assert that it starts correctly

    execute_payload("payload_hello_world.bin");

    auto state = get_domain_state(domain);
    ASSERT_EQ(state, DomainState::runningPayload);
}

TEST_F(BmbootFixture, access_violation) {
    // synopsis of test:
    // 1. load payload_access_violation
    // 2. assert that it crashes
    // 3. reset the domain back into loader
    // 4. load payload_hello_world
    // 5. assert that it starts correctly

    execute_payload("payload_access_violation.bin");

    std::this_thread::sleep_for(50ms);

    auto state = get_domain_state(domain);
    ASSERT_EQ(state, DomainState::crashedPayload);

    throw_for_err(reset_domain(domain));

    execute_payload("payload_hello_world.bin");

    state = get_domain_state(domain);
    ASSERT_EQ(state, DomainState::runningPayload);
}
