#ifndef SYMBOL_MERGE_TYPES_H
#define SYMBOL_MERGE_TYPES_H

#include <stddef.h>
#include "ELF.h"

typedef struct {
    Elf32_Sym *out_symtab;
    size_t out_sym_count;
    size_t local_symbols_count;
    char *out_strtab;
    size_t out_strtab_size;
    struct {
        size_t a_sym_count;
        size_t b_sym_count;
        Elf32_Word *sym_map_a_to_out;
        Elf32_Word *sym_map_b_to_out;
    } remap;
} symbol_merge_result_t;

#endif