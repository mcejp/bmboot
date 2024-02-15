#include <chrono>
#include <functional>

#include <bmboot/payload_runtime.hpp>

using BenchmarkFunc = std::function<void(int iterations)>;
using std::chrono::duration;

static duration<double, std::nano> doTest(BenchmarkFunc callback, char const* test_name, int iterations);

// defined in fpga_latency.s
extern "C" void ld32_fixed_address(uint32_t volatile* addr, size_t iterations);
extern "C" void ld32_dsb_fixed_address(uint32_t volatile* addr, size_t iterations);
extern "C" void st32_fixed_address(uint32_t volatile* addr, size_t iterations);
extern "C" void st32_dsb_fixed_address(uint32_t volatile* addr, size_t iterations);

inline uint32_t* volatile FPGA_REGISTER = (uint32_t* volatile) 0xb000'0000;

int main()
{
    bmboot::notifyPayloadStarted();

    auto ld32 = doTest([](int iterations) {
        ld32_fixed_address(FPGA_REGISTER, iterations);
    }, "Read", 5'000'000);

    auto ld32_dsb = doTest([](int iterations) {
        ld32_dsb_fixed_address(FPGA_REGISTER, iterations);
    }, "Read w/ barrier", 5'000'000);

    auto st32 = doTest([](int iterations) {
        st32_fixed_address(FPGA_REGISTER, iterations);
    }, "Write (pipelined)", 10'000'000);

    auto st32_dsb = doTest([](int iterations) {
        st32_dsb_fixed_address(FPGA_REGISTER, iterations);
    }, "Write (w/ barrier)", 5'000'000);
}

static duration<double, std::nano> doTest(BenchmarkFunc callback, char const* test_name, int iterations)
{
    auto startCnt = bmboot::getBuiltinTimerValue();

    callback(iterations);

    auto endCnt = bmboot::getBuiltinTimerValue();

    auto ns = duration<double, std::nano>((double)(endCnt - startCnt) / iterations / (bmboot::getBuiltinTimerFrequency() / 1.0e9));

    printf("%-20s %5.1f ns\n", test_name, ns.count());

    return ns;
}
