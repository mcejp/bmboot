**************
API -- Payload
**************

Related terminology:

- :term:`monitor`
- :term:`payload`
- :term:`executor domain`

Header: :src_file:`include/bmboot/payload_runtime.hpp`

.. doxygenfunction:: bmboot::notifyPayloadCrashed(const char* desc, uintptr_t address)

.. doxygenfunction:: bmboot::notifyPayloadStarted()

.. doxygenfunction:: bmboot::writeToStdout(void const* data, size_t size)
