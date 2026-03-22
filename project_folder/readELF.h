#ifndef READELF_API_H
#define READELF_API_H

#include "ELF.h"
#include "util.h"
#include <stdint.h>
#include <stdio.h>


// sara7a sara7a nrml reader ma ykounch lahna loulad , bsh l code yfsd ki njarb n5rjou 5atr rah lhal
// hhhhhhh 5aliwh w brk

struct ELFcntx;

typedef struct {
    FILE *f;
    Elf_DataEncoding data;
} elf_reader_t;

static inline int elf_reader_need_swap(const elf_reader_t *r) {
    int host_big = is_big_endian();
    int file_big = (r->data == ELFDATA2MSB);
    return host_big != file_big;
}

uint8_t r_u8(elf_reader_t *r);
uint16_t r_u16(elf_reader_t *r);
uint32_t r_u32(elf_reader_t *r);
int32_t r_s32(elf_reader_t *r);

Elf32_Shdr *elf32_read_sht(FILE *f, const Elf32_Ehdr *ehdr);
void *elf32_read_section(FILE *f, const Elf32_Shdr *shdr);
char *elf32_read_shstrtab(FILE *f, const Elf32_Ehdr *ehdr, const Elf32_Shdr *sht);
const char *elf32_section_name(const Elf32_Shdr *shdr, const char *shstrtab);
int elf32_find_section(const Elf32_Ehdr *ehdr, const Elf32_Shdr *sht, const char *shstrtab,
                       const char *name);

char *elf32_read_strtab(FILE *f, const Elf32_Shdr *shdr);

Elf32_Sym *elf32_read_symtab(FILE *f, const Elf32_Shdr *shdr, int *nsyms);
Elf32_Sym *elf32_read_symtab_with_ctx(FILE *f, const Elf32_Shdr *shdr, const struct ELFcntx *ctx,
                                      int *nsyms);
const char *elf32_symbol_name(const Elf32_Sym *sym, const char *strtab);

int elf32_symbol_has_section(const Elf32_Sym *sym);
const Elf32_Shdr *elf32_symbol_section(const Elf32_Sym *sym, const Elf32_Shdr *sht);

Elf32_Rel *elf32_read_reltab(FILE *f, const Elf32_Shdr *shdr, int *nrels);
Elf32_Rela *elf32_read_relatab(FILE *f, const Elf32_Shdr *shdr, int *nrelas);
Elf32_Rel *elf32_read_rel(FILE *f, const Elf32_Shdr *shdr);
void elf32_free(void *ptr);

#endif
