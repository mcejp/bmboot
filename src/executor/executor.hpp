//! @file
//! @brief  Executor internal functions
//! @author Martin Cejp

#pragma once

#include "bmboot_internal.hpp"

namespace bmboot::internal
{

int getCpuIndex();
IpcBlock& getIpcBlock();

}
