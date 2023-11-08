**************
API -- Payload
**************

Related terminology:

- :term:`monitor`
- :term:`payload`
- :term:`executor domain`

Header: :src_file:`include/bmboot/payload_runtime.hpp`

Periodic interrupt (private timer)
==================================

.. doxygenfunction:: bmboot::setupPeriodicInterrupt

.. doxygenfunction:: bmboot::startPeriodicInterrupt

.. doxygenfunction:: bmboot::stopPeriodicInterrupt


Other interrupts
================

.. doxygenfunction:: bmboot::disableInterruptHandling

.. doxygenfunction:: bmboot::enableInterruptHandling

.. doxygenfunction:: bmboot::setupInterruptHandling

.. doxygentypedef:: bmboot::InterruptHandler

.. doxygenenum:: bmboot::PayloadInterruptPriority


Miscellaneous
=============

.. doxygenfunction:: bmboot::getCpuIndex

.. doxygenfunction:: bmboot::getPayloadArgument

.. doxygenfunction:: bmboot::notifyPayloadCrashed(const char* desc, uintptr_t address)

.. doxygenfunction:: bmboot::notifyPayloadStarted()

.. doxygenfunction:: bmboot::writeToStdout(void const* data, size_t size)
