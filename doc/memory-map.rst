**********
Memory map
**********

See also: :src_file:`src/bmboot_internal.hpp`

===========  ===========  ======= ==========================================================
Start        End             Size Description
===========  ===========  ======= ==========================================================
0x7800_0000  0x7800_FFFF    64 kB CPU1 Monitor code & private data
0x7801_0000  0x7801_FFFF    64 kB CPU1 Monitor IPC block
0x7802_0000  0x79FF_FFFF   ~32 MB CPU1 Payload code & private data
0x7A00_0000  0x7FFF_FFFF    96 MB reg_loop shared memory region (SHMEM)
===========  ===========  ======= ==========================================================

.. TODO: wtf -- no way to right-align columns in Sphinx?
