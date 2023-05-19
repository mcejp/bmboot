How to write a payload
======================

(see src/example_payload)

- compile as aarch64 bare metal binary
- entry point in EL1 @ 0x7802'0000 (for CPU1)
- assume nothing is set up for you (stack, page tables etc.)
- link in libbmboot_slave
    - call ``bmboot_s::notify_payload_started`` to notify master that you're alive
    - if you catch an unhandled exception, try to call ``bmboot_s::notify_payload_crashed``
