#ifndef PRINTELF_H
#define PRINTELF_H

#include "ELF.h"
#include <stddef.h>
#include <stdio.h>

int elf32_print_header(FILE *f);

int elf32_print_sections(FILE *f);

int elf32_dump_section_hex(FILE *f, const char *section_arg);

const char *elf32_get_symbol_type_name(unsigned char st_type);
const char *elf32_get_symbol_bind_name(unsigned char st_bind);
void elf32_format_section_index_name(char *buf, size_t bufsize, Elf32_Half st_shndx,
                                     const Elf32_Shdr *sht, const char *shstrtab);

int elf32_print_symbols(FILE *f);
int elf32_print_symbol_table(const Elf32_Sym *symtab, size_t nsyms, const char *strtab,
                             const Elf32_Shdr *sht, const char *shstrtab);

int elf32_print_relocations(FILE *f);

#endif
