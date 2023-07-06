#pragma once

#include "../cpu_state.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace bmboot {

struct MemorySegment {
    size_t start_address, size;
    void const* ptr;
};

void write_core_dump(char const *fn,
                     std::span<MemorySegment const> segments,
                     Aarch64_Regs const& the_regs,
                     Aarch64_FpRegs const& fpregs);

}
