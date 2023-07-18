//! @file
//! @brief  crc32 function
//! @author Martin Cejp

#pragma once

#include <cstdint>
#include <cstdlib>

namespace bmboot
{

extern "C" uint32_t crc32(uint32_t crc, const void *buf, size_t size);

}
