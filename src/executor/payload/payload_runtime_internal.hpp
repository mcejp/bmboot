//! @file
//! @brief  Mechanisms internal to payload runtime
//! @author Martin Cejp

#pragma once

#include "bmboot_internal.hpp"

namespace bmboot::internal
{

extern InterruptHandler user_interrupt_handlers[(GIC_MAX_USER_INTERRUPT_ID + 1) - GIC_MIN_USER_INTERRUPT_ID];

void handleTimerIrq();

}
