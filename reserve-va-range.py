"""
This script adds a new, zero-initialized segment to an ELF file.
It might be necessary to use on the FGCD executable if there are collisions between dynamically loaded libraries and
the required shared memory mapping. (see Question Q_SHMEM_VA)
"""

import sys

import lief

INPUT, OUTPUT = sys.argv[1:]

binary = lief.parse(INPUT)

segment = lief.ELF.Segment()

segment           = lief.ELF.Segment()
segment.type      = lief.ELF.SEGMENT_TYPES.LOAD
segment.flags     = lief.ELF.SEGMENT_FLAGS(lief.ELF.SEGMENT_FLAGS.R.value | lief.ELF.SEGMENT_FLAGS.W.value)
segment.alignment = 0x1000
segment.virtual_address = 0x7A000000
segment = binary.add(segment)

# watch out: https://github.com/lief-project/LIEF/issues/625
segment.virtual_size = 96*1024*1024

binary.write(OUTPUT)
