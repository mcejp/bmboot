//! @file
//! @brief  Executor internal functions
//! @author Martin Cejp

#pragma once

#include "bmboot_internal.hpp"

namespace bmboot::internal
{

inline uint32_t read32(size_t offset) {
    return *(uint32_t volatile*)offset;
}

inline void write32(size_t offset, uint32_t value) {
    *(uint32_t volatile*)offset = value;
}

int getCpuIndex();
IpcBlock& getIpcBlock();

}
