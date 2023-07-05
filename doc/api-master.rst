**********
Master API
**********

.. TODO: auto-generate, e.g. via Doxygen + Breathe

.. cpp:enum-class:: bmboot::Domain

   .. cpp:enumerator:: cpu1

      CPU1


.. cpp:function:: DomainHandleOrErrorCode bmboot_m::open_domain(Domain domain)

   Open a domain. This is necessary before any further operations can be done on that domain.

  :param domain: A domain selector
  :param address: Code address of the crash


.. cpp:function:: MaybeError bmboot_m::startup_domain(DomainHandle const& domain)
                  MaybeError bmboot_m::startup_domain(DomainHandle const& domain, \
                                                      std::span<uint8_t const> bmboot_slave_binary)

   Load the monitor code at the appropriate address + take the CPU out of reset.

   This can only be done when the domain state is ``inReset``.


.. cpp:function:: CrashInfo bmboot_m::get_crash_info(DomainHandle const& domain);

.. cpp:function:: DomainState bmboot_m::get_domain_state(DomainHandle const& domain)

.. cpp:function:: MaybeError bmboot_m::reset_domain(DomainHandle const& domain);

.. cpp:function:: MaybeError bmboot_m::load_and_start_payload(DomainHandle const& domain, \
                                                              std::span<uint8_t const> payload_binary)

.. cpp:function:: MaybeError bmboot_m::start_payload_at(DomainHandle const& domain, uintptr_t entry_address)

.. cpp:function:: int bmboot_m::stdout_getchar(DomainHandle const& domain)

.. cpp:function:: MaybeError bmboot_m::dump_core(DomainHandle const& domain, char const* filename)

.. cpp:function:: void bmboot_m::dump_debug_info(Domain domain)
