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

static std::atomic_bool console_interrupted[DomainIndex::max_domain];

static std::thread console_threads[DomainIndex::max_domain];

/*
 * Console works like this:
 *
 * - it always runs on a thread
 * - in stand-alone mode we just instantiate it for 1 domain (so it's the user's responisbility that it has been initialized etc.)
 * - when embedded as library, you spawn a thread per each domain spun up, and then it's up to you to tear 'em down
 */

static void runConsole(IDomain& domain)
{
    static const int MAX_LINE_LENGTH = 160;

    auto domain_name = toString(domain.getIndex());

    auto start = std::chrono::system_clock::now();

    std::stringstream stdout_accum;

    auto flush = [&]()
    {
        auto now = std::chrono::system_clock::now();
        printf("[%s %7.3f] %s\n",
               domain_name.c_str(),
               duration_cast<std::chrono::duration<float>>((now - start)).count(),
               stdout_accum.str().c_str());
        std::stringstream().swap(stdout_accum);         // https://stackoverflow.com/a/23266418
    };

    while (!console_interrupted[domain.getIndex()])
    {
        int c = domain.getchar();

        if (c >= 0)
        {
            if (c == '\n')
            {
                flush();
            }
            else
            {
                stdout_accum << (char)c;

                if (stdout_accum.tellp() >= MAX_LINE_LENGTH)
                {
                    flush();
                }
            }
        }
        else
        {
            std::this_thread::sleep_for(1ms);
        }
    }

    if (stdout_accum.tellp() > 0)
    {
        flush();
    }
}

void bmboot::runConsoleUntilInterrupted(IDomain& domain)
{
    console_interrupted[domain.getIndex()] = false;

    struct sigaction sa;
    sa.sa_handler = [](int signal)
    {
        for (auto& stop : console_interrupted)
        {
            stop = true;
        }
    };
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);

    startConsoleThread(domain);
    console_threads[domain.getIndex()].join();
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

    throwOnError(domain.loadAndStartPayload(program, crc, 123), "loadAndStartPayload");
}

void bmboot::startConsoleThread(IDomain& domain)
{
    auto& thread = console_threads[domain.getIndex()];

    if (!thread.joinable())
    {
        thread = std::thread(runConsole, std::ref(domain));
    }
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
