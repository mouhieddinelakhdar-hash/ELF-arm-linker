#include "writeELF.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Etape 9 : Write ELF
// hadchi akhir 7aja, bach nktbo l fichier final
// inshallah khyar

// align l offset bach yb9a multiple dyal align
// hadchi mzyan 7it ELF kay7taj alignment
static Elf32_Word align_offset(Elf32_Word offset, Elf32_Word align) {
    if (align == 0 || align == 1)
        return offset;
    return (offset + align - 1) & ~(align - 1);
}

// bach nbuildiw section header table dyal l output
// hadchi kolchi bach nktbo f ELF final
int build_output_sections(const merge_result_t *sec_merge, const symbol_merge_result_t *sym_merge,
                          merged_elf_t *out) {
    if (!sec_merge || !sym_merge || !out)
        return -1;

    // NULL section + merged sections + .symtab + .strtab + .shstrtab
    size_t total_sections = 1 + sec_merge->out_count + 3;
    out->sht = calloc(total_sections, sizeof(Elf32_Shdr));
    if (!out->sht)
        return -1;
    out->sht_count = total_sections;

    // bach nbdaw b .shstrtab (string table dyal sections)
    // 7tajna kolchi bach n9dro n3rfo smiya d sections
    out->shstrtab_size = 1;
    out->shstrtab = malloc(out->shstrtab_size);
    if (!out->shstrtab)
        return -1;
    out->shstrtab[0] = '\0'; // NULL string l awl

    size_t symtab_name_off, strtab_name_off, shstrtab_name_off;

    symtab_name_off = out->shstrtab_size;
    out->shstrtab_size += strlen(".symtab") + 1;
    out->shstrtab = realloc(out->shstrtab, out->shstrtab_size);
    if (!out->shstrtab)
        return -1;
    strcpy(out->shstrtab + symtab_name_off, ".symtab");

    strtab_name_off = out->shstrtab_size;
    out->shstrtab_size += strlen(".strtab") + 1;
    out->shstrtab = realloc(out->shstrtab, out->shstrtab_size);
    if (!out->shstrtab)
        return -1;
    strcpy(out->shstrtab + strtab_name_off, ".strtab");

    shstrtab_name_off = out->shstrtab_size;
    out->shstrtab_size += strlen(".shstrtab") + 1;
    out->shstrtab = realloc(out->shstrtab, out->shstrtab_size);
    if (!out->shstrtab)
        return -1;
    strcpy(out->shstrtab + shstrtab_name_off, ".shstrtab");

    for (size_t i = 0; i < sec_merge->out_count; i++) {
        merged_section_t *sec = &sec_merge->out_secs[i];
        sec->shdr.sh_name = (Elf32_Word)out->shstrtab_size;
        out->shstrtab_size += strlen(sec->name) + 1;
        out->shstrtab = realloc(out->shstrtab, out->shstrtab_size);
        if (!out->shstrtab)
            return -1;
        strcpy(out->shstrtab + sec->shdr.sh_name, sec->name);
    }

    memset(&out->sht[0], 0, sizeof(Elf32_Shdr));
    out->sht[0].sh_name = 0;
    out->sht[0].sh_type = SHT_NULL;

    for (size_t i = 0; i < sec_merge->out_count; i++) {
        out->sht[1 + i] = sec_merge->out_secs[i].shdr;
        out->sht[1 + i].sh_offset = 0;
    }

    size_t symtab_idx = 1 + sec_merge->out_count;
    out->sht[symtab_idx].sh_name = (Elf32_Word)symtab_name_off;
    out->sht[symtab_idx].sh_type = SHT_SYMTAB;
    out->sht[symtab_idx].sh_flags = 0;
    out->sht[symtab_idx].sh_addr = 0;
    out->sht[symtab_idx].sh_offset = 0;
    out->sht[symtab_idx].sh_size = sym_merge->out_sym_count * sizeof(Elf32_Sym);
    out->sht[symtab_idx].sh_link = (Elf32_Word)(symtab_idx + 1);
    out->sht[symtab_idx].sh_info = (Elf32_Word)(sym_merge->local_symbols_count + 1);
    out->sht[symtab_idx].sh_addralign = 4;
    out->sht[symtab_idx].sh_entsize = sizeof(Elf32_Sym);

    size_t strtab_idx = symtab_idx + 1;
    out->sht[strtab_idx].sh_name = (Elf32_Word)strtab_name_off;
    out->sht[strtab_idx].sh_type = SHT_STRTAB;
    out->sht[strtab_idx].sh_flags = 0;
    out->sht[strtab_idx].sh_addr = 0;
    out->sht[strtab_idx].sh_offset = 0;
    out->sht[strtab_idx].sh_size = sym_merge->out_strtab_size;
    out->sht[strtab_idx].sh_link = 0;
    out->sht[strtab_idx].sh_info = 0;
    out->sht[strtab_idx].sh_addralign = 1;
    out->sht[strtab_idx].sh_entsize = 0;

    size_t shstrtab_idx = strtab_idx + 1;
    out->sht[shstrtab_idx].sh_name = (Elf32_Word)shstrtab_name_off;
    out->sht[shstrtab_idx].sh_type = SHT_STRTAB;
    out->sht[shstrtab_idx].sh_flags = 0;
    out->sht[shstrtab_idx].sh_addr = 0;
    out->sht[shstrtab_idx].sh_offset = 0;
    out->sht[shstrtab_idx].sh_size = out->shstrtab_size;
    out->sht[shstrtab_idx].sh_link = 0;
    out->sht[shstrtab_idx].sh_info = 0;
    out->sht[shstrtab_idx].sh_addralign = 1;
    out->sht[shstrtab_idx].sh_entsize = 0;

    // hadchi ELF header dyal l output file
    // khasna n3tioh kolchi bach yb9a valid ELF32
    memset(&out->ehdr, 0, sizeof(Elf32_Ehdr));

    // ELF magic bytes (0x7F 'E' 'L' 'F')
    out->ehdr.e_ident[EI_MAG0] = ELFMAG0;
    out->ehdr.e_ident[EI_MAG1] = ELFMAG1;
    out->ehdr.e_ident[EI_MAG2] = ELFMAG2;
    out->ehdr.e_ident[EI_MAG3] = ELFMAG3;
    out->ehdr.e_ident[EI_CLASS] = ELFCLASS32; // 32-bit
    out->ehdr.e_ident[EI_DATA] = ELFDATA2LSB; // Little-endian
    out->ehdr.e_ident[EI_VERSION] = EV_CURRENT;

    out->ehdr.e_type = ET_REL; // Relocatable object file
    out->ehdr.e_machine = EM_ARM;
    out->ehdr.e_version = EV_CURRENT;

    out->ehdr.e_entry = 0;
    out->ehdr.e_phoff = 0;
    out->ehdr.e_shoff = 0;

    out->ehdr.e_flags = 0;
    out->ehdr.e_ehsize = sizeof(Elf32_Ehdr);
    out->ehdr.e_phentsize = 0;
    out->ehdr.e_phnum = 0;
    out->ehdr.e_shentsize = sizeof(Elf32_Shdr);
    out->ehdr.e_shnum = (Elf32_Half)total_sections;
    out->ehdr.e_shstrndx = (Elf32_Half)shstrtab_idx;

    out->progbits = sec_merge->out_secs;
    out->progbits_count = sec_merge->out_count;
    out->symtab_result = (symbol_merge_result_t *)sym_merge;

    return 0;
}

// hadchi kayktb l fichier ELF final
// kay7taj order mzyan : ELF header, sections, symbol table, string tables, section header table
int write_merged_elf(const char *out_filename, const merged_elf_t *merged) {
    FILE *f = fopen(out_filename, "wb");
    if (!f)
        return -1;

    // nktbo ELF header blawal (b ghayr shoff hadchi bach nzido b3d)
    Elf32_Ehdr ehdr = merged->ehdr;
    ehdr.e_shoff = 0; // b3d nzido offset dyal section header table
    fwrite(&ehdr, sizeof(ehdr), 1, f);

    Elf32_Word curr_offset = sizeof(Elf32_Ehdr); // b3d ELF header
    for (size_t i = 0; i < merged->progbits_count; i++) {
        merged_section_t *sec = &merged->progbits[i];

        if (sec->shdr.sh_type == SHT_NOBITS) {
            sec->shdr.sh_offset = curr_offset;
            continue;
        }

        curr_offset = align_offset(curr_offset, sec->shdr.sh_addralign);
        sec->shdr.sh_offset = curr_offset;
        fseek(f, curr_offset, SEEK_SET);
        if (sec->size > 0 && sec->bytes) {
            fwrite(sec->bytes, 1, sec->size, f);
        }
        curr_offset += sec->size;
    }

    // Symbol table (.symtab)
    curr_offset = align_offset(curr_offset, 4); // alignment 4 bytes
    merged->sht[1 + merged->progbits_count].sh_offset = curr_offset;
    fseek(f, curr_offset, SEEK_SET);
    fwrite(merged->symtab_result->out_symtab, sizeof(Elf32_Sym),
           merged->symtab_result->out_sym_count, f);
    curr_offset += merged->symtab_result->out_sym_count * sizeof(Elf32_Sym);

    curr_offset = align_offset(curr_offset, 1);
    merged->sht[2 + merged->progbits_count].sh_offset = curr_offset;
    fseek(f, curr_offset, SEEK_SET);
    fwrite(merged->symtab_result->out_strtab, 1, merged->symtab_result->out_strtab_size, f);
    curr_offset += merged->symtab_result->out_strtab_size;

    curr_offset = align_offset(curr_offset, 1);
    merged->sht[3 + merged->progbits_count].sh_offset = curr_offset;
    fseek(f, curr_offset, SEEK_SET);
    fwrite(merged->shstrtab, 1, merged->shstrtab_size, f);
    curr_offset += merged->shstrtab_size;

    // Akhir 7aja : Section Header Table
    ehdr.e_shoff = align_offset(curr_offset, 4);
    fseek(f, ehdr.e_shoff, SEEK_SET);
    fwrite(merged->sht, sizeof(Elf32_Shdr), merged->sht_count, f);

    // bach nzido offset dyal section header table f ELF header
    fseek(f, 0, SEEK_SET);
    fwrite(&ehdr, sizeof(ehdr), 1, f);

    fclose(f);
    return 0; // Alhamdulillah, khlasna!
}

void merged_elf_free(merged_elf_t *m) {
    if (!m)
        return;
    free(m->sht);
    free(m->shstrtab);
    memset(m, 0, sizeof(*m));
}