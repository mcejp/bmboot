#include <bmboot/domain_helpers.hpp>

#include "../utility/crc32.hpp"

#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

using namespace bmboot;
using namespace std::chrono_literals;
using std::chrono::milliseconds;

void bmboot::displayOutputContinuously(IDomain& domain)
{
    std::stringstream stdout_accum;

    auto start = std::chrono::system_clock::now();

    for (;;)
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
