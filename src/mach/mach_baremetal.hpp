#pragma once

#include "bmboot.hpp"
#include "mach_baremetal_defs.hpp"

#include <span>

namespace bmboot::mach {

using namespace bmboot;

void flush_icache();

void send_ipi_message(std::span<const std::byte> message);

void enable_CPU_interrupts();
void enable_IPI_reception(int src_channel);
void setup_interrupt(int ch, int target_cpu);

}
