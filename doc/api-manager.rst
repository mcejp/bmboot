**************
API -- Manager
**************

Header: :src_file:`include/bmboot/domain.hpp`

Domain initialization & state
=============================

.. doxygenfunction:: bmboot::IDomain::open

.. doxygenclass:: bmboot::IDomain

.. doxygenfunction:: bmboot::IDomain::getState

.. doxygenfunction:: bmboot::IDomain::startup


Payload execution
=================

.. doxygenfunction:: bmboot::IDomain::ensureReadyToLoadPayload

.. doxygenfunction:: bmboot::IDomain::loadAndStartPayload

.. doxygenfunction:: bmboot::IDomain::getchar


Crash handling and recovery
===========================

.. doxygenfunction:: bmboot::IDomain::dumpCore

.. doxygenfunction:: bmboot::IDomain::terminatePayload


Debugging/special functions
===========================

.. doxygenfunction:: bmboot::IDomain::dumpDebugInfo

.. doxygenfunction:: bmboot::IDomain::getCrashInfo

.. doxygenfunction:: bmboot::IDomain::startDummyPayload


Utility types
=============

.. doxygentypedef:: bmboot::DomainInstanceOrErrorCode

.. doxygentypedef:: bmboot::MaybeError
