#include "ELFcntx.h"
#include <string.h>


int elf32_open(FILE *f, ELFcntx *ctx) {
    if (!f || !ctx)
        return -1;
    memset(ctx, 0, sizeof(*ctx));

    if (fseek(f, 0, SEEK_SET) != 0)
        return -1;
    if (fread(ctx->ehdr.e_ident, 1, EI_NIDENT, f) != EI_NIDENT)
        return -1;

    if (!(ctx->ehdr.e_ident[EI_MAG0] == ELFMAG0 && ctx->ehdr.e_ident[EI_MAG1] == ELFMAG1 &&
          ctx->ehdr.e_ident[EI_MAG2] == ELFMAG2 && ctx->ehdr.e_ident[EI_MAG3] == ELFMAG3)) {
        return -1;
    }

    if (ctx->ehdr.e_ident[EI_CLASS] != ELFCLASS32)
        return -1;

    unsigned char data = ctx->ehdr.e_ident[EI_DATA];
    if (data != ELFDATA2LSB && data != ELFDATA2MSB)
        return -1;

    ctx->rd.f = f;
    ctx->rd.data = (Elf_DataEncoding)data;

    ctx->ehdr.e_type = (Elf32_Half)r_u16(&ctx->rd);
    ctx->ehdr.e_machine = (Elf32_Half)r_u16(&ctx->rd);
    ctx->ehdr.e_version = (Elf32_Word)r_u32(&ctx->rd);

    ctx->ehdr.e_entry = (Elf32_Addr)r_u32(&ctx->rd);
    ctx->ehdr.e_phoff = (Elf32_Off)r_u32(&ctx->rd);
    ctx->ehdr.e_shoff = (Elf32_Off)r_u32(&ctx->rd);

    ctx->ehdr.e_flags = (Elf32_Word)r_u32(&ctx->rd);

    ctx->ehdr.e_ehsize = (Elf32_Half)r_u16(&ctx->rd);
    ctx->ehdr.e_phentsize = (Elf32_Half)r_u16(&ctx->rd);
    ctx->ehdr.e_phnum = (Elf32_Half)r_u16(&ctx->rd);
    ctx->ehdr.e_shentsize = (Elf32_Half)r_u16(&ctx->rd);
    ctx->ehdr.e_shnum = (Elf32_Half)r_u16(&ctx->rd);
    ctx->ehdr.e_shstrndx = (Elf32_Half)r_u16(&ctx->rd);

    if (!ELF32_IS_ELF(ctx->ehdr))
        return -1;

    ctx->ok = 1;
    return 0;
}
