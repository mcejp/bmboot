****************
Testing strategy
****************

It is quite difficult to create unit tests for bmboot, since it operates so close to hardware.

Currently, there are a few system-level tests. These are based on the `GoogleTest`_ framework. The tests are compiled
as a separate executable (*bmtest*) that must be copied to and executed on the target device.

Implementation: :src_file:`src/tests/tests.cpp`

.. _GoogleTest: https://github.com/google/googletest
