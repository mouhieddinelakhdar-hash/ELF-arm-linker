#ifndef SECTION_INDEX_H
#define SECTION_INDEX_H

#include "ELF.h"

// just helpers !

static inline int is_real_shndx(Elf32_Half shndx, Elf32_Half shnum) {
    return (shndx < shnum) && ELF32_IS_REAL_SHNDX(shndx);
}

static inline int is_progbits(const Elf32_Shdr *sh) { return sh && sh->sh_type == SHT_PROGBITS; }

#endif
