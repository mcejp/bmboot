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

   Allow loading of user-provided payloads


.. req:: Slave domain status API
   :id: FUNC_02
   :status: implemented
   :severity: mandatory

   Provide an API for querying the status of the slave domain


.. req:: Slave domain status CLI
   :id: FUNC_03
   :status: implemented
   :severity: mandatory

   Provide CLI for querying the status of the slave domain


.. req:: Resetting the slave domain
   :id: FUNC_04
   :status: implemented
   :severity: mandatory

   Provide a facility for resetting the slave domain, allowing new payload to run fresh


.. req:: Stdout forwarding API (slave)
   :id: FUNC_05
   :status: implemented
   :severity: mandatory

   Provide forwarding API for the slave's standard output (``printf`` etc.)


.. req:: Stdout forwarding API (master)
   :id: FUNC_06
   :status: implemented
   :severity: mandatory

   Provide API for following the slave stdout


.. req:: Stdout forwarding CLI (master)
   :id: FUNC_07
   :status: implemented
   :severity: mandatory

   Provide CLI for following the slave stdout


.. req:: Core dump
   :id: FUNC_08
   :status: implemented
   :severity: mandatory

   Support retrieving a core dump from a crashed slave.

   For implementation details see :doc:`core-dump`.


.. req:: Stack trace
   :id: FUNC_09
   :status: implemented
   :severity: mandatory

   Support retrieving a stack trace from a crashed slave


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


Non-functional
--------------

.. req:: Comprehensive documentation
   :id: NONFUNC_01
   :status: in progress
   :severity: mandatory


.. req:: Support multiple slave domains
   :id: NONFUNC_02
   :status: open
   :severity: mandatory

   It shall support multiple slave domains


.. req:: Support reg_loop multi-tenancy
   :id: NONFUNC_03
   :status: implemented
   :severity: mandatory
   :links: NONFUNC_02

   Support reg_loop multi-tenancy, including tenants scattered across multiple bare-metal cores.

   (This does not translate to any specific features.)


.. req:: Ensure payload integrity
   :id: NONFUNC_04
   :status: open
   :severity: mandatory

   (validate CRC before execution)
