How to write a payload
======================

Example: :src_file:`src/payloads/hello_world.cpp`

- compile as aarch64 bare metal binary
- entry point in EL1 @ 0x7802'0000 (for CPU1)
- assume nothing is set up for you (stack, page tables etc.)
- link against ``bmboot_payload_runtime`` library

    - call :cpp:func:`bmboot::notifyPayloadStarted` to notify the manager that the payload initialized successfully
    - if you catch a CPU exception, call :cpp:func:`bmboot::notifyPayloadCrashed`
