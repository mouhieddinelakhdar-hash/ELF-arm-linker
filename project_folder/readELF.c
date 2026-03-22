#include "readELF.h"
#include "util.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


// KHAWTI
//  logic ta3 read ta3kom lahna mch fl etapeX.c
//  etapeX.c just ysta3ml l functions bah n9drou ndirou tests wahadhom
//  rabi yahfdkom

Elf32_Shdr *elf32_read_sht(FILE *f, const Elf32_Ehdr *ehdr) 
{
    if (!f || !ehdr || ehdr->e_shnum == 0)
        return NULL;

    Elf32_Shdr *sht = malloc(ehdr->e_shnum * sizeof(Elf32_Shdr));
    if (!sht)
        return NULL;

    if (fseek(f, ehdr->e_shoff, SEEK_SET) != 0) {
        free(sht);
        return NULL;
    }

    elf_reader_t rd; 
    rd.f = f;
    rd.data = (Elf_DataEncoding)ehdr->e_ident[EI_DATA];

    for (int i = 0; i < ehdr->e_shnum; i++) {
        sht[i].sh_name = r_u32(&rd);
        sht[i].sh_type = r_u32(&rd);
        sht[i].sh_flags = r_u32(&rd);
        sht[i].sh_addr = r_u32(&rd);
        sht[i].sh_offset = r_u32(&rd);
        sht[i].sh_size = r_u32(&rd);
        sht[i].sh_link = r_u32(&rd);
        sht[i].sh_info = r_u32(&rd);
        sht[i].sh_addralign = r_u32(&rd);
        sht[i].sh_entsize = r_u32(&rd);
    }

    return sht;
}

char *elf32_read_shstrtab(FILE *f, const Elf32_Ehdr *ehdr, const Elf32_Shdr *sht) {
    if (!f || !ehdr || !sht || ehdr->e_shstrndx >= ehdr->e_shnum)
        return NULL;

    const Elf32_Shdr *shstrtab_shdr = &sht[ehdr->e_shstrndx];
    if (shstrtab_shdr->sh_type != SHT_STRTAB)
        return NULL;

    char *strtab = malloc(shstrtab_shdr->sh_size);
    if (!strtab)
        return NULL;

    if (fseek(f, shstrtab_shdr->sh_offset, SEEK_SET) != 0) {
        free(strtab);
        return NULL;
    }

    if (fread(strtab, 1, shstrtab_shdr->sh_size, f) != shstrtab_shdr->sh_size) {
        free(strtab);
        return NULL;
    }

    return strtab;
}

const char *elf32_section_name(const Elf32_Shdr *shdr, const char *shstrtab) {
    if (!shdr || !shstrtab)
        return NULL;
    return shstrtab + shdr->sh_name;
}

int elf32_find_section(const Elf32_Ehdr *ehdr, const Elf32_Shdr *sht, const char *shstrtab,
                       const char *name) {
    if (!ehdr || !sht || !shstrtab || !name)
        return -1;

    for (int i = 0; i < ehdr->e_shnum; i++) {
        const char *sname = elf32_section_name(&sht[i], shstrtab);
        if (sname && strcmp(sname, name) == 0)
            return i;
    }
    return -1;
}

void *elf32_read_section(FILE *f, const Elf32_Shdr *shdr) {
    if (!f || !shdr)
        return NULL;

    if (shdr->sh_type == SHT_NOBITS || shdr->sh_size == 0)
        return NULL;

    void *data = malloc(shdr->sh_size);
    if (!data)
        return NULL;

    if (fseek(f, shdr->sh_offset, SEEK_SET) != 0) {
        free(data);
        return NULL;
    }

    if (fread(data, 1, shdr->sh_size, f) != shdr->sh_size) {
        free(data);
        return NULL;
    }

    return data;
}


char *elf32_read_strtab(FILE *f, const Elf32_Shdr *shdr) {
    if (!f || !shdr || shdr->sh_type != SHT_STRTAB)
        return NULL;

    return (char *)elf32_read_section(f, shdr);
}


Elf32_Sym *elf32_read_symtab(FILE *f, const Elf32_Shdr *shdr, int *nsyms) {
    if (!f || !shdr || !nsyms)
        return NULL;

    if (shdr->sh_type != SHT_SYMTAB && shdr->sh_type != SHT_DYNSYM)
        return NULL;

    *nsyms = shdr->sh_size / shdr->sh_entsize;
    if (*nsyms == 0)
        return NULL;

    Elf32_Sym *symtab = malloc(*nsyms * sizeof(Elf32_Sym));
    if (!symtab)
        return NULL;

    if (fseek(f, shdr->sh_offset, SEEK_SET) != 0) {
        free(symtab);
        return NULL;
    }

    elf_reader_t rd;
    rd.f = f;
    unsigned char e_ident[EI_NIDENT];
    long saved_pos = ftell(f);
    if (fseek(f, 0, SEEK_SET) == 0 && fread(e_ident, 1, EI_NIDENT, f) == EI_NIDENT) {
        rd.data = (Elf_DataEncoding)e_ident[EI_DATA];
    } else {
        rd.data = ELFDATA2LSB; /* default */
    }
    fseek(f, saved_pos, SEEK_SET);

    for (int i = 0; i < *nsyms; i++) {
        symtab[i].st_name = r_u32(&rd);
        symtab[i].st_value = r_u32(&rd);
        symtab[i].st_size = r_u32(&rd);
        symtab[i].st_info = r_u8(&rd);
        symtab[i].st_other = r_u8(&rd);
        symtab[i].st_shndx = r_u16(&rd);
    }

    return symtab;
}

Elf32_Sym *elf32_read_symtab_with_ctx(FILE *f, const Elf32_Shdr *shdr, const struct ELFcntx *ctx,
                                      int *nsyms) {
    if (!f || !shdr || !ctx || !nsyms)
        return NULL;

    if (shdr->sh_type != SHT_SYMTAB && shdr->sh_type != SHT_DYNSYM)
        return NULL;

    *nsyms = shdr->sh_size / shdr->sh_entsize;
    if (*nsyms == 0)
        return NULL;

    Elf32_Sym *symtab = malloc(*nsyms * sizeof(Elf32_Sym));
    if (!symtab)
        return NULL;

    if (fseek(f, shdr->sh_offset, SEEK_SET) != 0) {
        free(symtab);
        return NULL;
    }

    elf_reader_t rd;
    rd.f = f;
    unsigned char e_ident[EI_NIDENT];
    long saved_pos = ftell(f);
    if (fseek(f, 0, SEEK_SET) == 0 && fread(e_ident, 1, EI_NIDENT, f) == EI_NIDENT) {
        rd.data = (Elf_DataEncoding)e_ident[EI_DATA];
    } else {
        rd.data = ELFDATA2LSB; 
    }
    fseek(f, saved_pos, SEEK_SET);

    for (int i = 0; i < *nsyms; i++) {
        symtab[i].st_name = r_u32(&rd);
        symtab[i].st_value = r_u32(&rd);
        symtab[i].st_size = r_u32(&rd);
        symtab[i].st_info = r_u8(&rd);
        symtab[i].st_other = r_u8(&rd);
        symtab[i].st_shndx = r_u16(&rd);
    }

    return symtab;
}

const char *elf32_symbol_name(const Elf32_Sym *sym, const char *strtab) {
    if (!sym || !strtab)
        return NULL;
    if (sym->st_name == 0)
        return "";
    return strtab + sym->st_name;
}

int elf32_symbol_has_section(const Elf32_Sym *sym) {
    if (!sym)
        return 0;
    return ELF32_IS_REAL_SHNDX(sym->st_shndx);
}

const Elf32_Shdr *elf32_symbol_section(const Elf32_Sym *sym, const Elf32_Shdr *sht) {
    if (!sym || !sht || !elf32_symbol_has_section(sym))
        return NULL;
    return &sht[sym->st_shndx];
}


Elf32_Rel *elf32_read_reltab(FILE *f, const Elf32_Shdr *shdr, int *nrels) {
    if (!f || !shdr || !nrels)
        return NULL;

    if (shdr->sh_type != SHT_REL)
        return NULL;

    *nrels = shdr->sh_size / shdr->sh_entsize;
    if (*nrels == 0)
        return NULL;

    Elf32_Rel *reltab = malloc(*nrels * sizeof(Elf32_Rel));
    if (!reltab)
        return NULL;

    if (fseek(f, shdr->sh_offset, SEEK_SET) != 0) {
        free(reltab);
        return NULL;
    }

    elf_reader_t rd;
    rd.f = f;
    unsigned char e_ident[EI_NIDENT];
    long saved_pos = ftell(f);
    if (fseek(f, 0, SEEK_SET) == 0 && fread(e_ident, 1, EI_NIDENT, f) == EI_NIDENT) {
        rd.data = (Elf_DataEncoding)e_ident[EI_DATA];
    } else {
        rd.data = ELFDATA2LSB;
    }
    fseek(f, saved_pos, SEEK_SET);

    for (int i = 0; i < *nrels; i++) {
        reltab[i].r_offset = r_u32(&rd);
        reltab[i].r_info = r_u32(&rd);
    }

    return reltab;
}

Elf32_Rela *elf32_read_relatab(FILE *f, const Elf32_Shdr *shdr, int *nrelas) {
    if (!f || !shdr || !nrelas)
        return NULL;

    if (shdr->sh_type != SHT_RELA)
        return NULL;

    *nrelas = shdr->sh_size / shdr->sh_entsize;
    if (*nrelas == 0)
        return NULL;

    Elf32_Rela *relatab = malloc(*nrelas * sizeof(Elf32_Rela));
    if (!relatab)
        return NULL;

    if (fseek(f, shdr->sh_offset, SEEK_SET) != 0) {
        free(relatab);
        return NULL;
    }

    elf_reader_t rd;
    rd.f = f;
    unsigned char e_ident[EI_NIDENT];
    long saved_pos = ftell(f);
    if (fseek(f, 0, SEEK_SET) == 0 && fread(e_ident, 1, EI_NIDENT, f) == EI_NIDENT) {
        rd.data = (Elf_DataEncoding)e_ident[EI_DATA];
    } else {
        rd.data = ELFDATA2LSB;
    }
    fseek(f, saved_pos, SEEK_SET);

    for (int i = 0; i < *nrelas; i++) {
        relatab[i].r_offset = r_u32(&rd);
        relatab[i].r_info = r_u32(&rd);
        relatab[i].r_addend = r_s32(&rd);
    }

    return relatab;
}

Elf32_Rel *elf32_read_rel(FILE *f, const Elf32_Shdr *shdr) {
    int nrels;
    return elf32_read_reltab(f, shdr, &nrels);
}


uint8_t r_u8(elf_reader_t *r) {
    uint8_t x = 0;
    if (fread(&x, 1, 1, r->f) != 1)
        return 0;
    return x;
}

uint16_t r_u16(elf_reader_t *r) {
    uint16_t x = 0;
    if (fread(&x, sizeof(x), 1, r->f) != 1)
        return 0;
    if (elf_reader_need_swap(r))
        x = (uint16_t)reverse_2(x);
    return x;
}

uint32_t r_u32(elf_reader_t *r) {
    uint32_t x = 0;
    if (fread(&x, sizeof(x), 1, r->f) != 1)
        return 0;
    if (elf_reader_need_swap(r))
        x = (uint32_t)reverse_4(x);
    return x;
}

int32_t r_s32(elf_reader_t *r) {
    int32_t x = 0;
    if (fread(&x, sizeof(x), 1, r->f) != 1)
        return 0;
    if (elf_reader_need_swap(r))
        x = (int32_t)reverse_4((uint32_t)x);
    return x;
}


void elf32_free(void *ptr) { free(ptr); }
