#ifndef SYMBOL_MERGE_H
#define SYMBOL_MERGE_H

#include "elf_object.h"
#include "section_merge_types.h"
#include "symbol_merge_types.h"

int merge_symbol_tables(const elf_object_t *A, const elf_object_t *B,
                        const merge_result_t *merge_result, symbol_merge_result_t *out);
void symbol_merge_result_free(symbol_merge_result_t *r);

#endif