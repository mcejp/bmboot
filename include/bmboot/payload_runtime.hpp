#pragma once

#include "bmboot.hpp"

namespace bmboot {

/*!
 * Escalate to the monitor after a crash has been detected.
 * This would not normally be called by user code.
 *
 * @param desc A textual description of the error (will be trimmed to 32 characters)
 * @param address Code address of the crash
 */
void notifyPayloadCrashed(const char* desc, uintptr_t address);

/*!
 * Notify the manager that the payload has started successfully.
 */
void notifyPayloadStarted();

/*!
 * Write to the standard output.
 *
 * @param data Data to write (normally in ASCII encoding)
 * @param size Number of bytes to written
 * @return Number of bytes actually written, which might be limited by available buffer space
 */
int writeToStdout(void const* data, size_t size);

// TODO: can have some IPC here too

}
