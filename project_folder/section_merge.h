#ifndef SECTION_MERGE_H
#define SECTION_MERGE_H

#include "elf_object.h"
#include "section_merge_types.h"

int merge_progbits_sections(const elf_object_t *A, const elf_object_t *B, merge_result_t *out);

Elf32_Word align_up_u32(Elf32_Word x, Elf32_Word align);

#endif
