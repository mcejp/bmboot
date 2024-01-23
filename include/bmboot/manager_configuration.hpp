//! @file
//! @brief  Manager configuration
//! @author Martin Cejp

#pragma once

//#include <filesystem>
#include <stdint.h>

namespace bmboot
{

struct ManagerConfiguration
{
    uint32_t cntfrq;            // frequency of the Generic Timer
};

bool loadConfigurationFromDefaultFile(ManagerConfiguration& config_out);

}
