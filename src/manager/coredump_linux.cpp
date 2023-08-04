/* Copyright (c) 2005-2007, Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ---
 * Author: Markus Gutschke
 * References:
 *   https://github.com/rogercollins/bare_core/blob/master/elfcore.c
 *   https://github.com/anatol/google-coredumper/blob/master/src/elfcore.c
 *
 * Converted to C++ and modified for Aarch64 by Martin Cejp
 */

#include "coredump_linux.hpp"

#include <elf.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

using namespace bmboot;
using namespace bmboot::internal;

using std::as_bytes;
using std::byte;
using std::span;

// ************************************************************

template<size_t alignment>
static auto make_padding_span(size_t length)
{
    static const byte zeros[alignment - 1] {};
    auto padding_needed = (length % alignment) == 0 ? 0 : (alignment - length % alignment);

    return span{zeros, padding_needed};
}

static bool write_note(FILE* f, const char* name, Elf64_Word type, span<byte const> desc)
{
    auto terminated_name_len = strlen(name) + 1;
    auto nhdr = Elf64_Nhdr { .n_namesz = (Elf64_Word) terminated_name_len,
                             .n_descsz = (Elf64_Word) desc.size(),
                             .n_type = type };

    auto name_padding = make_padding_span<4>(terminated_name_len);

    if (fwrite(&nhdr, 1, sizeof(nhdr), f) != sizeof(nhdr)
            || fwrite(name, 1, terminated_name_len, f) != terminated_name_len
            || fwrite(name_padding.data(), 1, name_padding.size(), f) != name_padding.size()
            || fwrite(desc.data(), 1, desc.size(), f) != desc.size())
    {
        return false;
    }

    return true;
}

// ************************************************************

void internal::writeCoreDump(char const *fn,
                             span<MemorySegment const> segments,
                             Aarch64_Regs const& the_regs,
                             Aarch64_FpRegs const& fpregs)
{
    // Note: This code is derived from an older implementation by Google; it is quite fragile in how the offsets are
    //       computed and separate concerns are mixed together.
    //       A cleaner solution would be to prepare a high-level representation of the ELF structure in memory and then
    //       have separate code that serializes it to disk.

    int const NUM_THREADS = 1;
    size_t const PAGESIZE = 4096;       // TODO: can we make this assumption and does it matter?

    FILE* f = fopen(fn, "wb");

    // Write out the ELF header
    Elf64_Ehdr ehdr;
    memset(&ehdr, 0, sizeof(ehdr));
    ehdr.e_ident[0] = ELFMAG0;
    ehdr.e_ident[1] = ELFMAG1;
    ehdr.e_ident[2] = ELFMAG2;
    ehdr.e_ident[3] = ELFMAG3;
    ehdr.e_ident[4] = ELFCLASS64;
    ehdr.e_ident[5] = ELFDATA2LSB;
    ehdr.e_ident[6] = EV_CURRENT;
    ehdr.e_type     = ET_CORE;
    ehdr.e_machine  = EM_AARCH64;
    ehdr.e_version  = EV_CURRENT;
    ehdr.e_phoff    = sizeof(ehdr);
    ehdr.e_ehsize   = sizeof(ehdr);
    ehdr.e_phentsize= sizeof(Elf64_Phdr);
    ehdr.e_phnum    = segments.size() + 1;
    ehdr.e_shentsize= sizeof(Elf64_Shdr);

    if (fwrite(&ehdr, 1, sizeof(ehdr), f) != sizeof(ehdr))
    {
        return;
    }

    // Write program headers, starting with the PT_NOTE entry
    Elf64_Phdr phdr;
    size_t offset   = sizeof(Elf64_Ehdr) + ehdr.e_phnum * sizeof(phdr);
    size_t filesz   = sizeof(Elf64_Nhdr) + 8 + sizeof(elf_prpsinfo) + NUM_THREADS * (
                        sizeof(Elf64_Nhdr) + 8 + sizeof(elf_prstatus) +
                        sizeof(Elf64_Nhdr) + 8 + sizeof(fpregs));

    memset(&phdr, 0, sizeof(phdr));
    phdr.p_type     = PT_NOTE;
    phdr.p_offset   = offset;
    phdr.p_filesz   = filesz;

    if (fwrite(&phdr, 1, sizeof(phdr), f) != sizeof(phdr))
    {
        return;
    }

    // Now follow with program headers for each of the memory segments
    phdr.p_type     = PT_LOAD;
    phdr.p_align    = PAGESIZE;
    phdr.p_paddr    = 0;
    auto note_align = phdr.p_align - ((offset + filesz) % phdr.p_align);

    if (note_align == phdr.p_align)
    {
        note_align = 0;
    }

    offset         += note_align;
    for (const auto& seg : segments)
    {
        offset       += filesz;
        filesz        = seg.size;
        phdr.p_offset = offset;
        phdr.p_vaddr  = seg.start_address;
        phdr.p_memsz  = filesz;

        /* Do not write contents for memory segments that are read-only  */
//            if ((mappings[i].flags & PF_W) == 0)
//                filesz      = 0;
        phdr.p_filesz = filesz;
        phdr.p_flags  = PF_R | PF_X | PF_W; //mappings[i].flags;

        if (fwrite(&phdr, 1, sizeof(phdr), f) != sizeof(phdr))
        {
            return;
        }
    }

    // Write note section
    elf_prpsinfo prpsinfo = {};
    // TODO: could include payload name & hash (if we had those)
    strncpy(prpsinfo.pr_psargs, "(bmboot payload)", sizeof(prpsinfo.pr_psargs));

    if (!write_note(f, "CORE", NT_PRPSINFO, as_bytes(span{&prpsinfo, 1})))
    {
        return;
    }

    // Process status and integer registers
    elf_prstatus prstatus {};
    prstatus.pr_pid = 1;
    static_assert(sizeof(prstatus.pr_reg) == sizeof(the_regs));
    memcpy(&prstatus.pr_reg, &the_regs, sizeof(the_regs));

    if (!write_note(f, "CORE", NT_PRSTATUS, as_bytes(span{&prstatus, 1})))
    {
        return;
    }

    // FPU registers
    if (!write_note(f, "CORE", NT_FPREGSET, as_bytes(span{&fpregs, 1})))
    {
        return;
    }

    // Align all following segments to multiples of page size
    if (note_align)
    {
        char scratch[note_align];

        memset(scratch, 0, sizeof(scratch));
        if (fwrite(scratch, 1, sizeof(scratch), f) != sizeof(scratch))
        {
            return;
        }
    }

    // Write all memory segments
    for (const auto& seg : segments)
    {
        if (/*(seg.flags & PF_W) &&*/ fwrite(seg.ptr, 1, seg.size, f) != seg.size)
        {
            return;
        }
    }

    fclose(f);
}
