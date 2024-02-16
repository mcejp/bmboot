#include <chrono>
#include <functional>

#include <bmboot/payload_runtime.hpp>

using BenchmarkFunc = std::function<void(int iterations)>;
using std::chrono::duration;

static void doTest(BenchmarkFunc callback, char const* test_name, int iterations);

// defined in fpga_latency.s
extern "C" void ld32_fixed_address(uint32_t volatile* addr, size_t iterations);
extern "C" void ld32_dsb_fixed_address(uint32_t volatile* addr, size_t iterations);
extern "C" void st32_fixed_address(uint32_t volatile* addr, size_t iterations);
extern "C" void st32_dsb_fixed_address(uint32_t volatile* addr, size_t iterations);

inline uint32_t* volatile FPGA_REGISTER = (uint32_t* volatile) 0xb000'0000;

int main()
{
    bmboot::notifyPayloadStarted();

    doTest([](int iterations) {
        ld32_fixed_address(FPGA_REGISTER, iterations);
    }, "Read", 5'000'000);

    // Barrier shouldn't make a difference, since this is already Device-type read
    doTest([](int iterations) {
        ld32_dsb_fixed_address(FPGA_REGISTER, iterations);
    }, "Read w/ barrier", 5'000'000);

    // The average access time should be less than the full bus round-trip, due to store buffering,
    doTest([](int iterations) {
        st32_fixed_address(FPGA_REGISTER, iterations);
    }, "Write (pipelined)", 10'000'000);

    doTest([](int iterations) {
        st32_dsb_fixed_address(FPGA_REGISTER, iterations);
    }, "Write (w/ barrier)", 5'000'000);
}

static void doTest(BenchmarkFunc callback, char const* test_name, int iterations)
{
    auto start_cnt = bmboot::getBuiltinTimerValue();
    callback(iterations);
    auto end_cnt = bmboot::getBuiltinTimerValue();

    auto time_per_iter = duration<double, std::nano>((double)(end_cnt - start_cnt)
                                                     / iterations
                                                     / (bmboot::getBuiltinTimerFrequency() / 1.0e9));
    printf("%-20s %5.1f ns\n", test_name, time_per_iter.count());
}
