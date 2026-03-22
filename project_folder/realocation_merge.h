#ifndef RELOCATION_MERGE_H
#define RELOCATION_MERGE_H

#include "ELF.h"
#include "elf_object.h"
#include "section_merge_types.h"
#include "symbol_merge_types.h"

int merge_and_fix_relocations(
    const elf_object_t *A,
    const elf_object_t *B,
    const merge_result_t *sec_merge,
    const symbol_merge_result_t *sym_merge,
    merged_section_t *out_secs,
    size_t out_sec_count
);

#endif
