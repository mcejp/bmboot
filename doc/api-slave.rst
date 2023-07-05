***********
Payload API
***********

Related terminology:

- :term:`monitor`
- :term:`payload`
- :term:`slave domain`

Header: :src_file:`include/bmboot_slave.hpp`

.. TODO: auto-generate, e.g. via Doxygen + Breathe

.. cpp:function:: void bmboot_s::notify_payload_crashed(const char* desc, uintptr_t address)

   Escalate to the monitor after a crash has been detected.
   This would not normally be called by user code.

  :param desc: A textual description of the error (will be trimmed to 32 characters)
  :param address: Code address of the crash


.. cpp:function:: void bmboot_s::notify_payload_started()

   Notify the master that the payload has started successfully.


.. cpp:function:: int bmboot_s::write_stdout(void const* data, size_t size)

   Write to the standard output.

   :param data: Data to write (normally in ASCII encoding)
   :param size: Number of bytes to written
   :returns: Number of bytes actually written, which might be limited by available buffer space
