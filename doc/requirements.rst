Design requirements
===================

.. needtable:: Summary of functional requirements
   :filter: id.startswith("FUNC_")
   :columns: id;title;status;severity
   :style: table


.. needtable:: Summary of non-functional requirements
   :filter: id.startswith("NONFUNC_")
   :columns: id;title;status;severity
   :style: table


Functional
----------

.. req:: Loading of user-provided payloads
   :id: FUNC_01
   :status: implemented
   :severity: mandatory

   Enable loading of user-provided payloads conforming to the specified format and API.


.. req:: Executor domain status API
   :id: FUNC_02
   :status: implemented
   :severity: mandatory

   Provide an API for querying the status of the executor domain


.. req:: Executor domain status CLI
   :id: FUNC_03
   :status: implemented
   :severity: mandatory

   Provide CLI for querying the status of the executor domain


.. req:: Resetting the executor domain
   :id: FUNC_04
   :status: implemented
   :severity: mandatory

   Provide a facility for resetting the executor domain, allowing new payload to run fresh


.. req:: Stdout forwarding API (executor)
   :id: FUNC_05
   :status: implemented
   :severity: mandatory

   Provide forwarding API for the payload's standard output (``printf`` etc.)


.. req:: Stdout forwarding API (manager)
   :id: FUNC_06
   :status: implemented
   :severity: mandatory

   Provide API for following the executor stdout


.. req:: Stdout forwarding CLI (manager)
   :id: FUNC_07
   :status: implemented
   :severity: mandatory

   Provide CLI for following the executor stdout


.. req:: Core dump
   :id: FUNC_08
   :status: implemented
   :severity: mandatory

   Support retrieving a core dump from a crashed executor.

   For implementation details see :doc:`core-dump`.


.. req:: Stack trace
   :id: FUNC_09
   :status: implemented
   :severity: mandatory

   Support retrieving a stack trace from a crashed executor


.. req:: Interrupt API
   :id: FUNC_10
   :status: open
   :severity: mandatory

   Provide API to configure timers and interrupts


.. req:: Memory protection and management
   :id: FUNC_11
   :status: open
   :severity: optional

   Provide some memory management functions


.. req:: Shared memory for communication
   :id: FUNC_12
   :status: resolved
   :severity: mandatory

   A block of shared memory shall be provided for communication.


.. req:: Consistent addressing in shared memory
   :id: FUNC_13
   :status: resolved
   :severity: mandatory
   :links: FUNC_12

   The shared memory must appear at the same physical address in the executor and virtual address in the manager.


Non-functional
--------------

.. req:: Comprehensive documentation
   :id: NONFUNC_01
   :status: in progress
   :severity: mandatory


.. req:: Support multiple executor domains
   :id: NONFUNC_02
   :status: open
   :severity: mandatory

   It shall support multiple executor domains, corresponding to different cores of the CPU
   (minus the cores running Linux).

   Rationale: required for reg_loop+i_loop, as well as scattered reg_loop multi-tenancy.


.. req:: Ensure payload integrity
   :id: NONFUNC_04
   :status: open
   :severity: mandatory

   (validate CRC before execution)


.. req:: Vitis compatibility
   :id: NONFUNC_05
   :status: in progress
   :severity: mandatory

   Vitis is the IDE of choice for v_loop development.
   For user convenience, it shall be possible to use the built-in debugging flow which is unaware of bmboot. Moreover,
   Vitis insists on being in control of the :term:`BSP` and only provides libraries for :term:`EL1` and :term:`EL3`.


.. req:: Real-time behavior
   :id: NONFUNC_06
   :status: resolved
   :severity: mandatory

   Assuming a user payload executing real-time control at a rate of 100 kHz, the worst-case iteration overhead caused
   by bmboot shall be negligible.
