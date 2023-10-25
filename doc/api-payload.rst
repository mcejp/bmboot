**************
API -- Payload
**************

Related terminology:

- :term:`monitor`
- :term:`payload`
- :term:`executor domain`

Header: :src_file:`include/bmboot/payload_runtime.hpp`

.. doxygenfunction:: bmboot::configureAndEnableInterrupt

.. doxygenfunction:: bmboot::getCpuIndex

.. doxygenfunction:: bmboot::notifyPayloadCrashed(const char* desc, uintptr_t address)

.. doxygenfunction:: bmboot::notifyPayloadStarted()

.. doxygenfunction:: bmboot::startPeriodicInterrupt

.. doxygenfunction:: bmboot::stopPeriodicInterrupt

.. doxygenfunction:: bmboot::writeToStdout(void const* data, size_t size)

.. doxygentypedef:: bmboot::InterruptHandler

.. doxygenenum:: bmboot::PayloadInterruptPriority
