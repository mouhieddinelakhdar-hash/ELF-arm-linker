#ifndef WRITE_ELF_H
#define WRITE_ELF_H

#include "ELF.h"
#include "section_merge_types.h"
#include "symbol_merge_types.h"

typedef struct {
    Elf32_Ehdr ehdr;
    Elf32_Shdr *sht;              
    size_t sht_count;
    char *shstrtab;               
    size_t shstrtab_size;
    merged_section_t *progbits;   
    size_t progbits_count;
    symbol_merge_result_t *symtab_result; 
} merged_elf_t;

int write_merged_elf(const char *out_filename,
                     const merged_elf_t *merged);

int build_output_sections(const merge_result_t *sec_merge,
                          const symbol_merge_result_t *sym_merge,
                          merged_elf_t *out);

void merged_elf_free(merged_elf_t *m);

#endif