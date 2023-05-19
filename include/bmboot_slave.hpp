#pragma once

#include "bmboot.hpp"

namespace bmboot_s {

using namespace bmboot;

void notify_payload_crashed(const char* desc, uintptr_t address);
void notify_payload_started();
bool payload_requested_to_stop();

int write_stdout(void const* data, size_t size);

// TODO: can have some IPC here too

}
