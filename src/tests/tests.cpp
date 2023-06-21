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

    std::string execute_command_and_capture_output(std::string const& command) {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            throw std::runtime_error("Failed to execute command: " + std::string(strerror(errno)));
        }

        std::stringstream output_stream;
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output_stream << buffer;
        }

        int exit_code = pclose(pipe);
        if (exit_code != 0) {
            throw std::runtime_error("Command exited with an error (exit code: " + std::to_string(exit_code) + ")");
        }

        return output_stream.str();
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

static bool ContainsSubstring(std::string const& output, std::string const& substring) {
    return output.find(substring) != std::string::npos;
}

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
    // 3. extract a core dump
    // 4. parse the core dump and check for an expected known symbol
    // 5. reset the domain back into loader
    // 6. load payload_hello_world
    // 7. assert that it starts correctly

    execute_payload("payload_access_violation.bin");

    std::this_thread::sleep_for(50ms);

    auto state = get_domain_state(domain);
    ASSERT_EQ(state, DomainState::crashedPayload);

    // Save core dump
    auto err = dump_core(domain, "core");
    ASSERT_FALSE(err.has_value());

    // Ensure the core dump was created
    struct stat st {};
    ASSERT_EQ(stat("core", &st), 0);
    ASSERT_GT(st.st_size, 4096);

    // Invoke gdb in batch mode to extract the stack trace
    auto output = execute_command_and_capture_output("gdb --batch -n -ex bt payload_access_violation.elf core");
    EXPECT_PRED2(ContainsSubstring, output, "in access_invalid_memory()");

    // Reset domain
    throw_for_err(reset_domain(domain));

    execute_payload("payload_hello_world.bin");

    state = get_domain_state(domain);
    ASSERT_EQ(state, DomainState::runningPayload);
}
