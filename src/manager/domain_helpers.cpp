#include <bmboot/domain_helpers.hpp>

#include "../utility/crc32.hpp"

#include <csignal>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

using namespace bmboot;
using namespace std::chrono_literals;
using std::chrono::milliseconds;

static std::atomic<bool> console_interrupted;

void bmboot::displayOutputContinuously(IDomain& domain)
{
    console_interrupted = false;

    struct sigaction sa;
    sa.sa_handler = [](int signal) { console_interrupted = true; };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);

    auto start = std::chrono::system_clock::now();

    std::stringstream stdout_accum;

    while (!console_interrupted)
    {
        int c = domain.getchar();

        if (c >= 0)
        {
            if (c == '\n')
            {
                auto now = std::chrono::system_clock::now();
                printf("[%6ld] %s\n", duration_cast<milliseconds>((now - start)).count(), stdout_accum.str().c_str());
                std::stringstream().swap(stdout_accum);         // https://stackoverflow.com/a/23266418
            }
            else
            {
                stdout_accum << (char)c;
            }
        }
        else
        {
            std::this_thread::sleep_for(1ms);
        }
    }
}

void bmboot::loadPayloadFromFileOrThrow(IDomain& domain, std::filesystem::path const& path)
{
    std::ifstream file(path, std::ios::binary);

    if (!file)
    {
        throw std::runtime_error("failed to open " + path.string());
    }

    std::vector<uint8_t> program((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());

    auto crc = crc32(0, program.data(), program.size());

    throwOnError(domain.loadAndStartPayload(program, crc), "loadAndStartPayload");
}

std::unique_ptr<IDomain> bmboot::throwOnError(DomainInstanceOrErrorCode maybe_domain, const char* function_name)
{
    if (!std::holds_alternative<std::unique_ptr<IDomain>>(maybe_domain))
    {
        throw std::runtime_error((std::string) function_name + ": error: " + toString(std::get<ErrorCode>(maybe_domain)));
    }

    return std::move(std::get<std::unique_ptr<IDomain>>(maybe_domain));
}

void bmboot::throwOnError(MaybeError err, const char* function_name)
{
    if (err.has_value())
    {
        throw std::runtime_error((std::string) function_name + ": error: " + toString(*err));
    }
}
