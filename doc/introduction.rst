Introduction
============

Motivation
----------

For various reasons, we opted to run bare-metal code on FGC4.
The CPU state can be directly controlled from Linux, but this capability is limited to resetting the core.

_bmboot_ serves the following purposes:
- starting the core in EL3 mode
- loading user payload as requested
- catching crashes in user payload
- stopping/replacing user payload on request

bmboot was developed as a low-complexity alternative to off-the-shelf hypervisors (Xen, Jailhouse...)


Design
------

bmboot executes on EL3, the highest-privilege mode in the CPU.
The payload is executed in EL1. When the payload is running, it has exclusive ownership of CPU time (zero overhead).

Communication occurs primarily via shared memory.

There is also an IPI to intervene (since bmboot is suspended during payload execution, it needs to be interrupted to regain control)


Components
----------

To assemble a working system, several components need to be put together:

Master:
- master library (bmboot_master)
- command-line tools (bmctl) OR a user application embedding bmboot_master

- bmboot_slave_$PLATFORM
- bmboot_payload library
- payload code


Communication
-------------

On the Linux side, the ``/dev/mem`` special device is used. This avoids the need to develop a custom kernel driver.

Cache coherency is not a problem, because the Cortex-A53 CPU includes a Snoop Control Unit (SCU), which synchronizes
L1 caches across the cores.
