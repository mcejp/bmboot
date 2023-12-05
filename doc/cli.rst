**********************
Command-Line Reference
**********************

.. _bmctl1:

bmctl(1)
********

Synopsis
========

.. code::

 Start Bmboot on a given CPU
  bmctl boot <cpu>

 Check Bmboot status
  bmctl status <domain>

 Launch a payload
  bmctl start <cpu> <filename>

 Terminate a running payload
  bmctl terminate <cpu>

 Run a payload and display its output until terminated
  bmctl run <cpu> <filename>

 Generate core dump of a crashed payload
  bmctl core <domain>

Description
===========

The :program:`bmctl` executable is the command-line interface of Bmboot.
The above `Synopsis`_ lists various actions the tool can perform.
