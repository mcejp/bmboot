#include "bmboot/domain.hpp"
#include "../utility/crc32.hpp"

#include <gtest/gtest.h>

#include <fstream>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace bmboot;
using namespace std::chrono_literals;

struct BmbootFixture : public ::testing::Test
{
    void SetUp() override
    {
        auto which_domain = bmboot::DomainIndex::cpu1;

        auto maybe_domain = IDomain::open(which_domain);

        if (!std::holds_alternative<std::unique_ptr<IDomain>>(maybe_domain))
        {
            throw std::runtime_error("IDomain::open: error: " + toString(std::get<bmboot::ErrorCode>(maybe_domain)));
        }

        this->domain = std::move(std::get<std::unique_ptr<IDomain>>(maybe_domain));

        auto state = domain->getState();

        if (state == DomainState::in_reset)
        {
            auto err = domain->startup();

            if (err.has_value())
            {
                throw std::runtime_error("IDomain::startup: error: " + toString(*err));
            }

            state = domain->getState();
        }
        else if (state != DomainState::monitor_ready)
        {
            // attempt to reset it

            auto err = domain->terminatePayload();

            if (err.has_value())
            {
                throw std::runtime_error("IDomain::terminatePayload: error: " + toString(*err));
            }

            state = domain->getState();
        }

        if (state != DomainState::monitor_ready)
        {
            throw std::runtime_error("ensure_monitor_ready: bad state " + toString(state));
        }
    }

    void TearDown() override
    {
    }

    static void throw_for_err(MaybeError err)
    {
        if (err.has_value())
        {
            throw std::runtime_error(toString(*err));
        }
    }

    std::string execute_command_and_capture_output(std::string const& command)
    {
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe)
        {
            throw std::runtime_error("Failed to execute command: " + std::string(strerror(errno)));
        }

        std::stringstream output_stream;
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            output_stream << buffer;
        }

        int exit_code = pclose(pipe);
        if (exit_code != 0)
        {
            throw std::runtime_error("Command exited with an error (exit code: " + std::to_string(exit_code) + ")");
        }

        return output_stream.str();
    }

    void execute_payload(const char* filename) const
    {
        std::ifstream file(filename, std::ios::binary);

        if (!file)
        {
            throw std::runtime_error((std::string) "failed to open " + filename);
        }

        std::vector<uint8_t> program((std::istreambuf_iterator<char>(file)),
                                     std::istreambuf_iterator<char>());

        auto crc = crc32(0, program.data(), program.size());
        throw_for_err(domain->loadAndStartPayload(program, crc, 0));
    }

    std::unique_ptr<IDomain> domain;
};

static bool ContainsSubstring(std::string const& output, std::string const& substring)
{
    return output.find(substring) != std::string::npos;
}

TEST_F(BmbootFixture, hello_world)
{
    // synopsis of test:
    // 1. load payload_hello_world
    // 2. assert that it starts correctly

    execute_payload("payload_hello_world_cpu1.bin");

    auto state = domain->getState();
    ASSERT_EQ(state, DomainState::running_payload);
}

TEST_F(BmbootFixture, access_violation)
{
    // synopsis of test:
    // 1. load payload_access_violation
    // 2. assert that it crashes
    // 3. extract a core dump
    // 4. parse the core dump and check for an expected known symbol
    // 5. reset the domain back into loader
    // 6. load payload_hello_world
    // 7. assert that it starts correctly

    execute_payload("payload_access_violation_cpu1.bin");

    std::this_thread::sleep_for(50ms);

    auto state = domain->getState();;
    ASSERT_EQ(state, DomainState::crashed_payload);

    // Save core dump
    auto err = domain->dumpCore("core");
    ASSERT_FALSE(err.has_value());

    // Ensure the core dump was created
    struct stat st {};
    ASSERT_EQ(stat("core", &st), 0);
    ASSERT_GT(st.st_size, 4096);

    // Invoke gdb in batch mode to extract the stack trace
    auto output = execute_command_and_capture_output("gdb --batch -n -ex bt payload_access_violation_cpu1.elf core");
    // Watch out; the format changes depending on whether the payload was built with debug information:
    // Release:        in access_invalid_memory()
    // RelWithDebInfo: in access_invalid_memory () at ...
    EXPECT_PRED2(ContainsSubstring, output, "in access_invalid_memory");

    // Reset domain
    throw_for_err(domain->terminatePayload());

    execute_payload("payload_hello_world_cpu1.bin");

    state = domain->getState();
    ASSERT_EQ(state, DomainState::running_payload);
}
