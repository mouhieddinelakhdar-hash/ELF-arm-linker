#include "printELF.h"
#include "ELF.h"
#include "ELFcntx.h"
#include "readELF.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// KHAWTI
//  logic ta3 lprint ta3kom lahna mch fl etapeX.c
//  etapeX.c just ysta3ml l functions bah n9drou ndirou tests wahadhom
//  rabi yahfdkom

static const char *class_str(unsigned char c)
{
    return (c == ELFCLASS32) ? "ELF32" : (c == ELFCLASS64) ? "ELF64"
                                                           : "Invalid";
}

static const char *data_str(unsigned char d)
{
    return (d == ELFDATA2MSB) ? "Big endian" : (d == ELFDATA2LSB) ? "Little endian"
                                                                  : "Invalid";
}

static const char *type_str(Elf32_Half t)
{
    switch (t)
    {
    case ET_REL:
        return "ET_REL (Relocatable)";
    case ET_EXEC:
        return "ET_EXEC (Executable)";
    case ET_DYN:
        return "ET_DYN (Shared object)";
    case ET_CORE:
        return "ET_CORE (core file)";
    default:
        return "Other (processor specific)";
    }
}

static const char *machine_str(Elf32_Half m) { return (m == EM_ARM) ? "ARM" : "Other than ARM"; }

int elf32_print_header(FILE *f)
{
    if (!f)
        return -1;

    rewind(f);
    ELFcntx ctx;
    if (elf32_open(f, &ctx) != 0)
    {
        fprintf(stderr, "Error: not a valid ELF32 file\n");
        return -1;
    }

    const Elf32_Ehdr *ehdr = elf32_ehdr(&ctx);
    if (!ehdr)
    {
        fprintf(stderr, "Error: invalid ELF context\n");
        return -1;
    }

    printf("ELF Header:\n");
    printf("  Class:       %s\n", class_str(ehdr->e_ident[EI_CLASS]));
    printf("  Data:        %s\n", data_str(ehdr->e_ident[EI_DATA]));
    printf("  Version:     %u\n", (unsigned)ehdr->e_ident[EI_VERSION]);
    printf("  Type:        %s\n", type_str(ehdr->e_type));
    printf("  Machine:     %s (%u)\n", machine_str(ehdr->e_machine), (unsigned)ehdr->e_machine);
    printf("  Flags:       0x%08x\n", (unsigned)ehdr->e_flags);
    printf("  EH size:     %u\n", (unsigned)ehdr->e_ehsize);
    printf("  SH off:      %u\n", (unsigned)ehdr->e_shoff);
    printf("  SH entsize:  %u\n", (unsigned)ehdr->e_shentsize);
    printf("  SH num:      %u\n", (unsigned)ehdr->e_shnum);
    printf("  SHSTR index: %u\n", (unsigned)ehdr->e_shstrndx);

    return 0;
}
const char *shtype_str(Elf32_Word t)
{
    switch (t)
    {
    case SHT_NULL:
        return "NULL";
    case SHT_PROGBITS:
        return "PROGBITS";
    case SHT_SYMTAB:
        return "SYMTAB";
    case SHT_STRTAB:
        return "STRTAB";
    case SHT_RELA:
        return "RELA";
    case SHT_HASH:
        return "HASH";
    case SHT_DYNAMIC:
        return "DYNAMIC";
    case SHT_NOTE:
        return "NOTE";
    case SHT_NOBITS:
        return "NOBITS";
    case SHT_REL:
        return "REL";
    case SHT_DYNSYM:
        return "DYNSYM";
    default:
        return "OTHER";
    }
}

void shflags_to_str(Elf32_Word flags, char out[8])
{
    int k = 0;
    if (flags & SHF_WRITE)
    {
        out[k] = 'W';
        k++;
    }
    if (flags & SHF_ALLOC)
    {
        out[k] = 'A';
        k++;
    }
    if (flags & SHF_EXECINSTR)
    {
        out[k] = 'X';
        k++;
    }
    if (k == 0)
    {
        out[k] = '-';
        k++;
    }
    out[k] = '\0';
}

int elf32_print_sections(FILE *f)
{
    if (!f)
        return -1;

    rewind(f);
    ELFcntx ctx;
    if (elf32_open(f, &ctx) != 0)
    {
        fprintf(stderr, "Error: not a valid ELF32 file\n");
        return -1;
    }

    const Elf32_Ehdr *ehdr = elf32_ehdr(&ctx);
    if (!ehdr)
    {
        fprintf(stderr, "Error: invalid ELF context\n");
        return -1;
    }

    Elf32_Shdr *sht = elf32_read_sht(f, ehdr);
    if (!sht)
    {
        fprintf(stderr, "Error: cannot read section header table\n");
        return -1;
    }

    char *shstrtab = elf32_read_shstrtab(f, ehdr, sht);
    if (!shstrtab)
    {
        fprintf(stderr, "Error: cannot read .shstrtab\n");
        elf32_free(sht);
        return -1;
    }

    printf("Section Headers:\n");
    printf("  [Nr] Name              Type       Flg  Off      Size     Addr     Align\n");

    for (int i = 0; i < (int)ehdr->e_shnum; i++)
    {
        const char *name = elf32_section_name(&sht[i], shstrtab);
        char flg[8];
        shflags_to_str(sht[i].sh_flags, flg);

        printf("  [%2d] %-17s %-10s %-3s  0x%06x 0x%06x 0x%08x %u\n", i, name,
               shtype_str(sht[i].sh_type), flg, (unsigned)sht[i].sh_offset,
               (unsigned)sht[i].sh_size, (unsigned)sht[i].sh_addr, (unsigned)sht[i].sh_addralign);
    }

    elf32_free(shstrtab);
    elf32_free(sht);
    return 0;
}
static void print_hex_dump(const unsigned char *data, size_t size)
{
    for (size_t i = 0; i < size; i += 16)
    {
        printf("  0x%08zx", i);

        size_t bytes_in_line = (size - i < 16) ? (size - i) : 16;
        size_t chars_printed = 13;

        for (size_t j = 0; j < bytes_in_line; j += 4)
        {
            printf(" ");
            chars_printed++;

            size_t bytes_in_group = (bytes_in_line - j < 4) ? (bytes_in_line - j) : 4;
            for (size_t k = 0; k < bytes_in_group; k++)
                printf("%02x", data[i + j + k]);

            chars_printed += bytes_in_group * 2;
        }

        size_t padding_needed = (chars_printed < 50) ? (50 - chars_printed) : 1;
        for (size_t p = 0; p < padding_needed; p++)
            printf(" ");

        for (size_t j = 0; j < bytes_in_line; j++)
            printf("%c", isprint(data[i + j]) ? data[i + j] : '.');

        printf("\n");
    }
}

static int find_section_by_name_or_index(ELFcntx *ctx, const char *arg, int *section_index,
                                         Elf32_Shdr **out_sht, char **out_shstrtab)
{
    const Elf32_Ehdr *ehdr = elf32_ehdr(ctx);
    if (!ehdr)
        return -1;

    FILE *f = ctx->rd.f;

    Elf32_Shdr *sht = elf32_read_sht(f, ehdr);
    if (!sht)
    {
        fprintf(stderr, "Erreur : Échec de la lecture du tableau d'en-têtes de section\n");
        return -1;
    }

    char *shstrtab = elf32_read_shstrtab(f, ehdr, sht);
    if (!shstrtab)
    {
        fprintf(stderr,
                "Erreur : Échec de la lecture de la table de chaînes de noms de sections\n");
        elf32_free(sht);
        return -1;
    }

    /* numeric index */
    char *endptr;
    long num = strtol(arg, &endptr, 10);
    if (*endptr == '\0' && num >= 0 && num < (long)ehdr->e_shnum)
    {
        *section_index = (int)num;
        *out_sht = sht;
        *out_shstrtab = shstrtab;
        return 0;
    }

    /* name lookup */
    int idx = elf32_find_section(ehdr, sht, shstrtab, arg);
    if (idx >= 0)
    {
        *section_index = idx;
        *out_sht = sht;
        *out_shstrtab = shstrtab;
        return 0;
    }

    fprintf(stderr, "Erreur : La section '%s' n'a pas été trouvée\n", arg);
    elf32_free(shstrtab);
    elf32_free(sht);
    return -1;
}

int elf32_dump_section_hex(FILE *f, const char *section_arg)
{
    if (!f || !section_arg)
        return -1;

    rewind(f);

    ELFcntx ctx;
    if (elf32_open(f, &ctx) != 0)
    {
        fprintf(stderr, "Erreur : Le fichier n'est pas un fichier ELF32 valide\n");
        return -1;
    }

    const Elf32_Ehdr *ehdr = elf32_ehdr(&ctx);
    if (!ehdr)
    {
        fprintf(stderr, "Erreur : Contexte ELF invalide\n");
        return -1;
    }

    int section_index = -1;
    Elf32_Shdr *sht = NULL;
    char *shstrtab = NULL;

    rewind(f);
    if (find_section_by_name_or_index(&ctx, section_arg, &section_index, &sht, &shstrtab) != 0)
        return -1;

    const Elf32_Shdr *shdr = &sht[section_index];
    const char *section_name = shstrtab ? elf32_section_name(shdr, shstrtab) : "inconnu";

    printf("Section: %s (index %d)\n", section_name, section_index);
    printf("Offset: 0x%08x, Size: %u bytes\n", (unsigned)shdr->sh_offset, (unsigned)shdr->sh_size);

    if (shdr->sh_type == SHT_NOBITS)
    {
        printf("Section type: NOBITS (no data in file)\n");
        elf32_free(shstrtab);
        elf32_free(sht);
        return 0;
    }

    rewind(f);
    void *section_data = elf32_read_section(f, shdr);
    if (!section_data)
    {
        fprintf(stderr, "Erreur : Échec de la lecture du contenu de la section\n");
        elf32_free(shstrtab);
        elf32_free(sht);
        return -1;
    }

    printf("\nHex dump of section '%s':\n", section_name);
    print_hex_dump((unsigned char *)section_data, shdr->sh_size);

    elf32_free(section_data);
    elf32_free(shstrtab);
    elf32_free(sht);
    return 0;
}
const char *elf32_get_symbol_type_name(unsigned char st_type)
{
    switch (st_type)
    {
    case STT_NOTYPE:
        return "NOTYPE";
    case STT_OBJECT:
        return "OBJECT";
    case STT_FUNC:
        return "FUNC";
    case STT_SECTION:
        return "SECTION";
    case STT_FILE:
        return "FILE";
    default:
        return "UNKNOWN";
    }
}

const char *elf32_get_symbol_bind_name(unsigned char st_bind)
{
    switch (st_bind)
    {
    case STB_LOCAL:
        return "LOCAL";
    case STB_GLOBAL:
        return "GLOBAL";
    case STB_WEAK:
        return "WEAK";
    default:
        return "UNKNOWN";
    }
}

void elf32_format_section_index_name(char *buf, size_t bufsize, Elf32_Half st_shndx,
                                     const Elf32_Shdr *sht, const char *shstrtab)
{
    if (st_shndx == SHN_UNDEF)
    {
        snprintf(buf, bufsize, "UNDEF");
        return;
    }
    if (st_shndx == SHN_ABS)
    {
        snprintf(buf, bufsize, "ABS");
        return;
    }
    if (st_shndx == SHN_COMMON)
    {
        snprintf(buf, bufsize, "COMMON");
        return;
    }

    if (st_shndx >= (Elf32_Half)SHN_LORESERVE && st_shndx <= (Elf32_Half)SHN_HIRESERVE)
    {
        snprintf(buf, bufsize, "RESERVED(%d)", st_shndx);
        return;
    }

    if (ELF32_IS_REAL_SHNDX(st_shndx) && sht && shstrtab)
    {
        const char *name = elf32_section_name(&sht[st_shndx], shstrtab);
        snprintf(buf, bufsize, "%d (%s)", st_shndx, name ? name : "?");
        return;
    }

    snprintf(buf, bufsize, "%d", st_shndx);
}

int elf32_print_symbols(FILE *f)
{
    if (!f)
        return -1;

    rewind(f);

    ELFcntx ctx;
    if (elf32_open(f, &ctx) != 0)
    {
        fprintf(stderr, "Erreur : Le fichier n'est pas un fichier ELF32 valide\n");
        return -1;
    }

    const Elf32_Ehdr *ehdr = elf32_ehdr(&ctx);
    if (!ehdr)
    {
        fprintf(stderr, "Erreur : Contexte ELF invalide\n");
        return -1;
    }

    rewind(f);
    Elf32_Shdr *sht = elf32_read_sht(f, ehdr);
    if (!sht)
    {
        fprintf(stderr, "Erreur : Échec de la lecture du tableau d'en-têtes de section\n");
        return -1;
    }

    rewind(f);
    char *shstrtab = elf32_read_shstrtab(f, ehdr, sht);
    if (!shstrtab)
    {
        fprintf(stderr,
                "Erreur : Échec de la lecture de la table de chaînes de noms de sections\n");
        elf32_free(sht);
        return -1;
    }

    int symtab_idx = elf32_find_section(ehdr, sht, shstrtab, ".symtab");
    if (symtab_idx < 0)
    {
        fprintf(stderr, "Attention : La section .symtab n'a pas été trouvée, essayant .dynsym\n");
        symtab_idx = elf32_find_section(ehdr, sht, shstrtab, ".dynsym");
    }

    if (symtab_idx < 0)
    {
        fprintf(stderr, "Erreur : Aucune table de symboles trouvée\n");
        elf32_free(shstrtab);
        elf32_free(sht);
        return -1;
    }

    const Elf32_Shdr *symtab_shdr = &sht[symtab_idx];

    int strtab_idx = symtab_shdr->sh_link;
    if (strtab_idx < 0 || strtab_idx >= (int)ehdr->e_shnum)
    {
        fprintf(stderr, "Erreur : Index de table de chaînes de caractères invalide\n");
        elf32_free(shstrtab);
        elf32_free(sht);
        return -1;
    }

    const Elf32_Shdr *strtab_shdr = &sht[strtab_idx];

    rewind(f);
    char *strtab = elf32_read_strtab(f, strtab_shdr);
    if (!strtab)
    {
        fprintf(stderr, "Erreur : Échec de la lecture de la table de chaînes de caractères\n");
        elf32_free(shstrtab);
        elf32_free(sht);
        return -1;
    }

    int nsyms = 0;
    elf_reader_t rd = ctx.rd; // copy of the reader for the bug

    if (fseek(f, symtab_shdr->sh_offset, SEEK_SET) != 0)
    {
        fprintf(stderr, "Erreur : Échec de la recherche de la table de symboles\n");
        elf32_free(strtab);
        elf32_free(shstrtab);
        elf32_free(sht);
        return -1;
    }

    if (symtab_shdr->sh_entsize == 0)
    {
        fprintf(stderr, "Erreur : sh_entsize == 0 dans la table de symboles\n");
        elf32_free(strtab);
        elf32_free(shstrtab);
        elf32_free(sht);
        return -1;
    }

    nsyms = (int)(symtab_shdr->sh_size / symtab_shdr->sh_entsize);

    Elf32_Sym *symtab = (Elf32_Sym *)malloc((size_t)nsyms * sizeof(Elf32_Sym));
    if (!symtab)
    {
        fprintf(stderr, "Erreur : Échec de l'allocation de mémoire pour la table de symboles\n");
        elf32_free(strtab);
        elf32_free(shstrtab);
        elf32_free(sht);
        return -1;
    }

    for (int i = 0; i < nsyms; i++)
    {
        symtab[i].st_name = r_u32(&rd);
        symtab[i].st_value = r_u32(&rd);
        symtab[i].st_size = r_u32(&rd);
        symtab[i].st_info = r_u8(&rd);
        symtab[i].st_other = r_u8(&rd);
        symtab[i].st_shndx = r_u16(&rd);
    }

    printf("Symbol table '%s' contains %d entries:\n", elf32_section_name(symtab_shdr, shstrtab),
           nsyms);
    printf("   Num:    Value  Size Type    Bind   Vis      Ndx Name\n");

    for (int i = 0; i < nsyms; i++)
    {
        const Elf32_Sym *sym = &symtab[i];
        const char *name = elf32_symbol_name(sym, strtab);

        unsigned char st_bind = ELF32_ST_BIND(sym->st_info);
        unsigned char st_type = ELF32_ST_TYPE(sym->st_info);

        char section_name_buf[64];
        elf32_format_section_index_name(section_name_buf, sizeof(section_name_buf), sym->st_shndx,
                                        sht, shstrtab);

        printf("%6d: %08x %5u %-7s %-6s %-7s %6s %s\n", i, (unsigned)sym->st_value,
               (unsigned)sym->st_size, elf32_get_symbol_type_name(st_type),
               elf32_get_symbol_bind_name(st_bind), "DEFAULT", section_name_buf, name ? name : "");
    }

    free(symtab);
    elf32_free(strtab);
    elf32_free(shstrtab);
    elf32_free(sht);
    return 0;
}

int elf32_print_symbol_table(const Elf32_Sym *symtab, size_t nsyms, const char *strtab,
                             const Elf32_Shdr *sht, const char *shstrtab)
{
    if (!symtab || !strtab)
        return -1;

    printf("Symbol table contains %zu entries:\n", nsyms);
    printf("   Num:    Value  Size Type    Bind   Vis      Ndx Name\n");

    for (size_t i = 0; i < nsyms; i++)
    {
        const Elf32_Sym *sym = &symtab[i];
        const char *name = elf32_symbol_name(sym, strtab);

        unsigned char st_bind = ELF32_ST_BIND(sym->st_info);
        unsigned char st_type = ELF32_ST_TYPE(sym->st_info);

        char section_name_buf[64];
        elf32_format_section_index_name(section_name_buf, sizeof(section_name_buf), sym->st_shndx,
                                        sht, shstrtab);

        printf("%6zu: %08x %5u %-7s %-6s %-7s %6s %s\n", i, (unsigned)sym->st_value,
               (unsigned)sym->st_size, elf32_get_symbol_type_name(st_type),
               elf32_get_symbol_bind_name(st_bind), "DEFAULT", section_name_buf, name ? name : "");
    }

    return 0;
}

static const char *arm_reloc_type_str(unsigned char type)
{
    switch (type)
    {
    case R_ARM_NONE:
        return "R_ARM_NONE";
    case R_ARM_PC24:
        return "R_ARM_PC24";
    case R_ARM_ABS32:
        return "R_ARM_ABS32";
    case R_ARM_REL32:
        return "R_ARM_REL32";
    case R_ARM_PC13:
        return "R_ARM_PC13";
    case R_ARM_ABS16:
        return "R_ARM_ABS16";
    case R_ARM_ABS12:
        return "R_ARM_ABS12";
    case R_ARM_THM_ABS5:
        return "R_ARM_THM_ABS5";
    case R_ARM_ABS8:
        return "R_ARM_ABS8";
    case R_ARM_SBREL32:
        return "R_ARM_SBREL32";
    case R_ARM_THM_PC22:
        return "R_ARM_THM_PC22";
    case R_ARM_THM_PC8:
        return "R_ARM_THM_PC8";
    case R_ARM_AMP_VCALL9:
        return "R_ARM_AMP_VCALL9";
    case R_ARM_SWI24:
        return "R_ARM_SWI24";
    case R_ARM_THM_SWI8:
        return "R_ARM_THM_SWI8";
    case R_ARM_XPC25:
        return "R_ARM_XPC25";
    case R_ARM_THM_XPC22:
        return "R_ARM_THM_XPC22";
    case R_ARM_PLT32:
        return "R_ARM_PLT32";
    case R_ARM_CALL:
        return "R_ARM_CALL";
    case R_ARM_JUMP24:
        return "R_ARM_JUMP24";
    case R_ARM_THM_JUMP24:
        return "R_ARM_THM_JUMP24";
    case R_ARM_GOT_BREL:
        return "R_ARM_GOT_BREL";
    case R_ARM_GOT_PREL:
        return "R_ARM_GOT_PREL";
    default:
        return "UNKNOWN";
    }
}

int elf32_print_relocations(FILE *f)
{
    if (!f)
        return -1;

    rewind(f);

    ELFcntx ctx;
    if (elf32_open(f, &ctx) != 0)
    {
        fprintf(stderr, "Erreur : Le fichier n'est pas un fichier ELF32 valide\n");
        return -1;
    }

    const Elf32_Ehdr *ehdr = elf32_ehdr(&ctx);
    if (!ehdr)
    {
        fprintf(stderr, "Erreur : Contexte ELF invalide\n");
        return -1;
    }

    rewind(f);
    Elf32_Shdr *sht = elf32_read_sht(f, ehdr);
    if (!sht)
    {
        fprintf(stderr, "Erreur : Échec de la lecture du tableau d'en-têtes de section\n");
        return -1;
    }

    rewind(f);
    char *shstrtab = elf32_read_shstrtab(f, ehdr, sht);
    if (!shstrtab)
    {
        fprintf(stderr,
                "Erreur : Échec de la lecture de la table de chaînes de noms de sections\n");
        elf32_free(sht);
        return -1;
    }

    int symtab_idx = elf32_find_section(ehdr, sht, shstrtab, ".symtab");
    if (symtab_idx < 0)
        symtab_idx = elf32_find_section(ehdr, sht, shstrtab, ".dynsym");

    Elf32_Sym *symtab = NULL;
    char *strtab = NULL;
    int nsyms = 0;

    if (symtab_idx >= 0)
    {
        const Elf32_Shdr *symtab_shdr = &sht[symtab_idx];
        int strtab_idx = (int)symtab_shdr->sh_link;

        if (strtab_idx >= 0 && strtab_idx < (int)ehdr->e_shnum)
        {
            const Elf32_Shdr *strtab_shdr = &sht[strtab_idx];
            rewind(f);
            strtab = elf32_read_strtab(f, strtab_shdr);
        }

        rewind(f);
        symtab = elf32_read_symtab_with_ctx(f, symtab_shdr, (const struct ELFcntx *)&ctx, &nsyms);
    }

    int found_reloc = 0;

    for (int i = 0; i < (int)ehdr->e_shnum; i++)
    {
        const Elf32_Shdr *shdr = &sht[i];
        if (shdr->sh_type != SHT_REL && shdr->sh_type != SHT_RELA)
            continue;

        found_reloc = 1;
        const char *section_name = elf32_section_name(shdr, shstrtab);

        if (shdr->sh_type == SHT_REL)
        {
            int nrels = 0;
            rewind(f);
            Elf32_Rel *reltab = elf32_read_reltab(f, shdr, &nrels);
            if (!reltab)
            {
                fprintf(stderr, "Erreur : Échec de la lecture de la table de réimplantation %s\n",
                        section_name ? section_name : "?");
                continue;
            }

            printf("Relocation section '%s' at offset 0x%06x contains %d entries:\n",
                   section_name ? section_name : "?", (unsigned)shdr->sh_offset, nrels);
            printf(" Offset     Info    Type            Sym.Value  Sym. Name\n");

            for (int j = 0; j < nrels; j++)
            {
                Elf32_Word sym_idx = ELF32_R_SYM(reltab[j].r_info);
                unsigned char type = ELF32_R_TYPE(reltab[j].r_info);

                const char *sym_name = "?";
                Elf32_Addr sym_value = 0;

                if (symtab && sym_idx < (Elf32_Word)nsyms)
                {
                    const Elf32_Sym *sym = &symtab[sym_idx];
                    sym_value = sym->st_value;

                    if (ELF32_ST_TYPE(sym->st_info) == STT_SECTION)
                    {
                        if (ELF32_IS_REAL_SHNDX(sym->st_shndx) && shstrtab)
                        {
                            sym_name = elf32_section_name(&sht[sym->st_shndx], shstrtab);
                            if (!sym_name)
                                sym_name = "?";
                        }
                    }
                    else if (strtab)
                    {
                        sym_name = elf32_symbol_name(sym, strtab);
                        if (!sym_name || !sym_name[0])
                            sym_name = "?";
                    }
                }

                printf("0x%08x  %08x %-15s %08x  %s\n", (unsigned)reltab[j].r_offset,
                       (unsigned)reltab[j].r_info, arm_reloc_type_str(type), (unsigned)sym_value,
                       sym_name && sym_name[0] ? sym_name : "?");
            }

            elf32_free(reltab);
        }
        else
        {
            int nrelas = 0;
            rewind(f);
            Elf32_Rela *relatab = elf32_read_relatab(f, shdr, &nrelas);
            if (!relatab)
            {
                fprintf(stderr, "Erreur : Échec de la lecture de la table de réimplantation %s\n",
                        section_name ? section_name : "?");
                continue;
            }

            printf("Relocation section '%s' at offset 0x%06x contains %d entries:\n",
                   section_name ? section_name : "?", (unsigned)shdr->sh_offset, nrelas);
            printf(" Offset     Info    Type            Sym.Value  Sym. Name  Addend\n");

            for (int j = 0; j < nrelas; j++)
            {
                Elf32_Word sym_idx = ELF32_R_SYM(relatab[j].r_info);
                unsigned char type = ELF32_R_TYPE(relatab[j].r_info);

                const char *sym_name = "?";
                Elf32_Addr sym_value = 0;

                if (symtab && sym_idx < (Elf32_Word)nsyms)
                {
                    const Elf32_Sym *sym = &symtab[sym_idx];
                    sym_value = sym->st_value;

                    if (ELF32_ST_TYPE(sym->st_info) == STT_SECTION)
                    {
                        if (ELF32_IS_REAL_SHNDX(sym->st_shndx) && shstrtab)
                        {
                            sym_name = elf32_section_name(&sht[sym->st_shndx], shstrtab);
                            if (!sym_name)
                                sym_name = "?";
                        }
                    }
                    else if (strtab)
                    {
                        sym_name = elf32_symbol_name(sym, strtab);
                        if (!sym_name || !sym_name[0])
                            sym_name = "?";
                    }
                }

                printf("0x%08x  %08x %-15s %08x  %-15s %08x\n", (unsigned)relatab[j].r_offset,
                       (unsigned)relatab[j].r_info, arm_reloc_type_str(type), (unsigned)sym_value,
                       sym_name && sym_name[0] ? sym_name : "?", (unsigned)relatab[j].r_addend);
            }

            elf32_free(relatab);
        }

        printf("\n");
    }

    if (!found_reloc)
        printf("No relocation sections found in this file.\n");

    if (symtab)
        elf32_free(symtab);
    if (strtab)
        elf32_free(strtab);
    elf32_free(shstrtab);
    elf32_free(sht);
    return 0;
}