Payload core dump for post-mortem debugging
===========================================

Related requirements: :need:`FUNC_08`, :need:`FUNC_09`

Implementation: :src_file:`src/coredump_linux.cpp`

Core dump format
----------------

Unfortunately, there is no standard for core dumps on bare metal. Similarly to executable files, each operating system
will have its own format that it considers "native". How, then, can we produce core dumps that can be inspected using
a widely-available tool like GDB?

The answer is to imitate the format used by Linux. This is, in fact, an ELF file containing general information about
the process, a snapshot of its (writable) memory, as well as CPU state for each thread.

A lot of this information does not apply to bmboot payloads -- arguably even the concept of a process does not exist.
These have to be filled with made-up values (mostly zeroes). Despite this impedance mismatch, GDB provides a rather
ergonomic view of the crashed program state.


How it works
------------

When the payload attempts an invalid memory access, a *synchronous interrupt* is triggered.
(see ``SynchronousInterruptHandler`` in asm_vectors.S and ``SynchronousInterrupt`` in vectors.cpp)
The handler starts by saving general-purpose registers on the stack.
The C++ part of the code proceeds by saving also floating-point registers and other CPU state that is not always saved.
Everything goes into a pair of structures within ``IpcBlock``.
Finally, the domain state is set to ``crashed_payload`` and the core spins until reset to monitor.

Ideally this code should reside in bmboot (at EL3), but until the exception handlers are unified, the payload carries
its own copy of the exception vectors. One of the reasons to promote this code is that EL3 could then save the register
state event if EL1 were to corrupt/overrun its stack pointer.

On Linux side, the core dump ELF file is assembled using the saved registers and the payload's memory space,
which is read out directly via ``/dev/mem``. Ideally, only writable ranges should be dumped, but at the moment bmboot
does not have the information necessary to make this distinction. (see :need:`[[id]] <Q_PLD_FMT>`)

The ELF writing part of the code is derived from the Google `Coredumper <https://github.com/anatol/google-coredumper>`_
library, but all portability is thrown out, so the code is Aarch64-specific.


How to extract & process the dump
---------------------------------

After a payload has crashed, the core dump must be extracted manually using the command ``bmctl core``.
This will produce a file called ``core`` which can be evacuated using scp.
It is also necessary to have the original ELF file of the payload. The core dump currently does not contain any version
information or hash of the executable. (see :need:`[[id]] <Q_CORE_ID>`)

Note that it is necessary to use GDB from the Linux-Aarch64 toolchain; for the reasons explained above, the bare-metal
Aarch64 build of GDB does not support core dumps.

However, the core dump can also be processed on-device (assuming that the OS ships with GDB); this is taken advantage of
to implement bmboot tests.
For example, the stack trace can be extracted without any user interaction like this::

    gdb --batch -n -ex bt my_payload.elf core
