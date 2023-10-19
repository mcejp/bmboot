//! @file
//! @brief  Platform-specific definitions
//! @author Martin Cejp

#pragma once

#include "zynqmp.hpp"

namespace bmboot::mach {

constexpr inline auto CNTPNS_IRQ_CHANNEL = 30;

// implemented in interrupt_controller.cpp
int getInterruptIdForIpi(IpiChannel ipi_channel);

}
