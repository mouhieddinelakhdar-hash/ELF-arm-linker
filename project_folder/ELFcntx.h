#ifndef ELFCNTX_H
#define ELFCNTX_H

#include "ELF.h"
#include "readELF.h"

typedef struct {
    elf_reader_t rd;
    Elf32_Ehdr ehdr; // cached header (WAJD CB)
    int ok;          
} ELFcntx;

int elf32_open(FILE *f, ELFcntx *ctx);

static inline const Elf32_Ehdr *elf32_ehdr(const ELFcntx *ctx) {
    return (ctx && ctx->ok) ? &ctx->ehdr : NULL;
}

#endif
