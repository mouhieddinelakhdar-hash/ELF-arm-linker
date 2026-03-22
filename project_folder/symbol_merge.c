#include "symbol_merge.h"
#include "readELF.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Etape 7 : Fusion des symboles
// hadchi bach nfuso les symboles dyal A w B w n3tiwhom des indices jdid
// hhhhh had lpartie kharbo9a bzzaf mais bismillah

void symbol_merge_result_free(symbol_merge_result_t *r) {
    if (!r)
        return;

    if (r->out_symtab) {
        free(r->out_symtab);
        r->out_symtab = NULL;
    }

    if (r->out_strtab) {
        free(r->out_strtab);
        r->out_strtab = NULL;
    }

    if (r->remap.sym_map_a_to_out) {
        free(r->remap.sym_map_a_to_out);
        r->remap.sym_map_a_to_out = NULL;
    }

    if (r->remap.sym_map_b_to_out) {
        free(r->remap.sym_map_b_to_out);
        r->remap.sym_map_b_to_out = NULL;
    }

    memset(r, 0, sizeof(*r));
}

// kaydir mapping dyal section index men fichier original l merged
// 7it les sections 7awlou 3la indices jdid
static Elf32_Half map_section_index(Elf32_Half sec_idx, const elf_object_t *obj,
                                    const merge_result_t *merge_result, int is_file_b) {
    if (!ELF32_IS_REAL_SHNDX(sec_idx))
        return sec_idx;

    if (!is_file_b) {
        // File A : n9drou ndiro b ism d section
        const char *sec_name = elf_obj_sec_name(obj, sec_idx);
        if (!sec_name)
            return SHN_UNDEF;

        for (size_t i = 0; i < merge_result->out_count; i++) {
            if (merge_result->out_secs[i].name &&
                strcmp(merge_result->out_secs[i].name, sec_name) == 0) {
                return (Elf32_Half)(i + 1);
            }
        }
        return SHN_UNDEF;
    } else {
        // File B : 3andna mapping direct
        if (sec_idx >= (Elf32_Half)merge_result->remap.b_shnum)
            return SHN_UNDEF;

        return merge_result->remap.sec_map_b_to_out[sec_idx];
    }
}

static Elf32_Word add_string_to_strtab(char **strtab, size_t *strtab_size, const char *str) {
    if (!str)
        return 0;

    size_t len = strlen(str) + 1;
    size_t old_size = *strtab_size;

    char *new_strtab = realloc(*strtab, old_size + len);
    if (!new_strtab)
        return 0;

    *strtab = new_strtab;

    memcpy(*strtab + old_size, str, len);

    Elf32_Word offset = (Elf32_Word)old_size;
    *strtab_size = old_size + len;

    return offset;
}

static int add_symbol_to_output(Elf32_Sym **out_symtab, size_t *out_count, char **out_strtab,
                                size_t *out_strtab_size, const Elf32_Sym *sym, const char *strtab,
                                Elf32_Word new_st_name_offset, Elf32_Half new_st_shndx,
                                Elf32_Word st_value_adjust) {
    size_t new_count = *out_count + 1;
    Elf32_Sym *tmp = realloc(*out_symtab, new_count * sizeof(Elf32_Sym));
    if (!tmp)
        return -1;

    *out_symtab = tmp;

    Elf32_Sym *out_sym = &(*out_symtab)[*out_count];
    *out_sym = *sym;
    out_sym->st_name = new_st_name_offset;
    out_sym->st_shndx = new_st_shndx;
    out_sym->st_value += st_value_adjust;

    *out_count = new_count;
    return 0;
}

static int find_global_symbol_by_name(const Elf32_Sym *symtab, int nsyms, const char *strtab,
                                      const char *name) {
    for (int i = 0; i < nsyms; i++) {
        unsigned char bind = ELF32_ST_BIND(symtab[i].st_info);
        if (bind == STB_GLOBAL || bind == STB_WEAK) {
            const char *sym_name = elf32_symbol_name(&symtab[i], strtab);
            if (sym_name && strcmp(sym_name, name) == 0)
                return i;
        }
    }
    return -1;
}

int merge_symbol_tables(const elf_object_t *A, const elf_object_t *B,
                        const merge_result_t *merge_result, symbol_merge_result_t *out) {
    if (!A || !B || !merge_result || !out)
        return -1;

    memset(out, 0, sizeof(*out));
    out->local_symbols_count = 0;

    int symtab_idx_a = elf32_find_section(&A->ehdr, A->sht, A->shstrtab, ".symtab");
    int symtab_idx_b = elf32_find_section(&B->ehdr, B->sht, B->shstrtab, ".symtab");
    int strtab_idx_a = elf32_find_section(&A->ehdr, A->sht, A->shstrtab, ".strtab");
    int strtab_idx_b = elf32_find_section(&B->ehdr, B->sht, B->shstrtab, ".strtab");

    if (symtab_idx_a < 0 || symtab_idx_b < 0)
        return -1;
    if (strtab_idx_a < 0 || strtab_idx_b < 0)
        return -1;

    int nsyms_a, nsyms_b;
    Elf32_Sym *symtab_a = elf32_read_symtab(A->f, &A->sht[symtab_idx_a], &nsyms_a);
    Elf32_Sym *symtab_b = elf32_read_symtab(B->f, &B->sht[symtab_idx_b], &nsyms_b);
    if (!symtab_a || !symtab_b) {
        if (symtab_a)
            free(symtab_a);
        if (symtab_b)
            free(symtab_b);
        return -1;
    }

    char *strtab_a = elf32_read_strtab(A->f, &A->sht[strtab_idx_a]);
    char *strtab_b = elf32_read_strtab(B->f, &B->sht[strtab_idx_b]);
    if (!strtab_a || !strtab_b) {
        free(symtab_a);
        free(symtab_b);
        if (strtab_a)
            free(strtab_a);
        if (strtab_b)
            free(strtab_b);
        return -1;
    }

    out->remap.a_sym_count = (size_t)nsyms_a;
    out->remap.b_sym_count = (size_t)nsyms_b;
    out->remap.sym_map_a_to_out = (Elf32_Word *)calloc(nsyms_a, sizeof(Elf32_Word));
    out->remap.sym_map_b_to_out = (Elf32_Word *)calloc(nsyms_b, sizeof(Elf32_Word));
    if (!out->remap.sym_map_a_to_out || !out->remap.sym_map_b_to_out) {
        free(symtab_a);
        free(symtab_b);
        free(strtab_a);
        free(strtab_b);
        symbol_merge_result_free(out);
        return -1;
    }

    out->out_strtab = (char *)malloc(1);
    if (!out->out_strtab) {
        free(symtab_a);
        free(symtab_b);
        free(strtab_a);
        free(strtab_b);
        symbol_merge_result_free(out);
        return -1;
    }
    out->out_strtab[0] = '\0';
    out->out_strtab_size = 1;

    Elf32_Word *temp_map_a = calloc(nsyms_a, sizeof(Elf32_Word));
    Elf32_Word *temp_map_b = calloc(nsyms_b, sizeof(Elf32_Word));
    if (!temp_map_a || !temp_map_b) {
        free(temp_map_a);
        free(temp_map_b);
        free(symtab_a);
        free(symtab_b);
        free(strtab_a);
        free(strtab_b);
        symbol_merge_result_free(out);
        return -1;
    }

    // Step 1: Local symbols dyal A (kolchi local yji blawal)
    for (int i = 0; i < nsyms_a; i++) {
        if (ELF32_ST_BIND(symtab_a[i].st_info) == STB_LOCAL) {
            const char *sym_name = elf32_symbol_name(&symtab_a[i], strtab_a);
            Elf32_Word name_offset =
                add_string_to_strtab(&out->out_strtab, &out->out_strtab_size, sym_name);

            Elf32_Half new_shndx = map_section_index(symtab_a[i].st_shndx, A, merge_result, 0);

            if (add_symbol_to_output(&out->out_symtab, &out->out_sym_count, &out->out_strtab,
                                     &out->out_strtab_size, &symtab_a[i], strtab_a, name_offset,
                                     new_shndx, 0) != 0) {
                free(temp_map_a);
                free(temp_map_b);
                free(symtab_a);
                free(symtab_b);
                free(strtab_a);
                free(strtab_b);
                symbol_merge_result_free(out);
                return -1;
            }

            temp_map_a[i] = (Elf32_Word)(out->out_sym_count - 1);
            out->local_symbols_count++;
        }
    }

    // Step 2: Local symbols dyal B (nfs chi 7aja, mais liya B)
    for (int i = 0; i < nsyms_b; i++) {
        const Elf32_Sym *sym = &symtab_b[i];
        unsigned char bind = ELF32_ST_BIND(sym->st_info);

        // hadi dyal symbol NULL, ma 7tajnach
        if (i == 0 && sym->st_name == 0 && sym->st_shndx == SHN_UNDEF) {
            temp_map_b[i] = 0;
            continue;
        }

        if (bind == STB_LOCAL) {
            const char *sym_name = elf32_symbol_name(sym, strtab_b);
            if (!sym_name)
                sym_name = "";

            Elf32_Word name_offset =
                add_string_to_strtab(&out->out_strtab, &out->out_strtab_size, sym_name);

            Elf32_Half new_shndx = map_section_index(sym->st_shndx, B, merge_result, 1);
            Elf32_Word value_adjust = 0;

            if (ELF32_IS_REAL_SHNDX(sym->st_shndx) &&
                sym->st_shndx < (Elf32_Half)merge_result->remap.b_shnum) {
                value_adjust = merge_result->remap.concat_off_b[sym->st_shndx];
            }

            if (add_symbol_to_output(&out->out_symtab, &out->out_sym_count, &out->out_strtab,
                                     &out->out_strtab_size, sym, strtab_b, name_offset, new_shndx,
                                     value_adjust) != 0) {
                free(temp_map_a);
                free(temp_map_b);
                free(symtab_a);
                free(symtab_b);
                free(strtab_a);
                free(strtab_b);
                symbol_merge_result_free(out);
                return -1;
            }

            temp_map_b[i] = (Elf32_Word)(out->out_sym_count - 1);
            out->local_symbols_count++;
        }
    }

    // Step 3: Global symbols dyal A (hna bda li kaykon m3r9)
    for (int i = 0; i < nsyms_a; i++) {
        if (ELF32_ST_BIND(symtab_a[i].st_info) != STB_LOCAL) {
            const char *sym_name = elf32_symbol_name(&symtab_a[i], strtab_a);
            Elf32_Word name_offset =
                add_string_to_strtab(&out->out_strtab, &out->out_strtab_size, sym_name);

            Elf32_Half new_shndx = map_section_index(symtab_a[i].st_shndx, A, merge_result, 0);

            if (add_symbol_to_output(&out->out_symtab, &out->out_sym_count, &out->out_strtab,
                                     &out->out_strtab_size, &symtab_a[i], strtab_a, name_offset,
                                     new_shndx, 0) != 0) {
                free(temp_map_a);
                free(temp_map_b);
                free(symtab_a);
                free(symtab_b);
                free(strtab_a);
                free(strtab_b);
                symbol_merge_result_free(out);
                return -1;
            }

            temp_map_a[i] = (Elf32_Word)(out->out_sym_count - 1);
        }
    }

    for (int i = 0; i < nsyms_b; i++) {
        const Elf32_Sym *sym = &symtab_b[i];
        unsigned char bind = ELF32_ST_BIND(sym->st_info);

        if (i == 0 && sym->st_name == 0 && sym->st_shndx == SHN_UNDEF)
            continue;

        if (bind != STB_LOCAL) {
            const char *sym_name = elf32_symbol_name(sym, strtab_b);
            if (!sym_name)
                sym_name = "";

            // shof wach 3andna symbol b nfs smiya
            int existing_idx = find_global_symbol_by_name(out->out_symtab, (int)out->out_sym_count,
                                                          out->out_strtab, sym_name);

            int is_defined = (sym->st_shndx != SHN_UNDEF) &&
                             (ELF32_IS_REAL_SHNDX(sym->st_shndx) || sym->st_shndx == SHN_ABS ||
                              sym->st_shndx == SHN_COMMON);

            if (existing_idx >= 0) {
                const Elf32_Sym *existing = &out->out_symtab[existing_idx];
                int existing_is_defined =
                    (existing->st_shndx != SHN_UNDEF) &&
                    (ELF32_IS_REAL_SHNDX(existing->st_shndx) || existing->st_shndx == SHN_ABS ||
                     existing->st_shndx == SHN_COMMON);

                // HNA HADCHI MA YKHOUNCH : ida symbol defined fi A w B, error!
                if (is_defined && existing_is_defined) {
                    fprintf(stderr, "Error: symbol '%s' defined in both files\n", sym_name);
                    free(temp_map_a);
                    free(temp_map_b);
                    free(symtab_a);
                    free(symtab_b);
                    free(strtab_a);
                    free(strtab_b);
                    symbol_merge_result_free(out);
                    return -1;
                } else if (is_defined && !existing_is_defined) {
                    Elf32_Word name_offset =
                        add_string_to_strtab(&out->out_strtab, &out->out_strtab_size, sym_name);
                    Elf32_Half new_shndx = map_section_index(sym->st_shndx, B, merge_result, 1);
                    Elf32_Word value_adjust = 0;

                    if (ELF32_IS_REAL_SHNDX(sym->st_shndx) &&
                        sym->st_shndx < (Elf32_Half)merge_result->remap.b_shnum) {
                        value_adjust = merge_result->remap.concat_off_b[sym->st_shndx];
                    }

                    Elf32_Sym *existing_sym = &out->out_symtab[existing_idx];
                    *existing_sym = *sym;
                    existing_sym->st_name = name_offset;
                    existing_sym->st_shndx = new_shndx;
                    existing_sym->st_value += value_adjust;

                    temp_map_b[i] = (Elf32_Word)existing_idx;
                } else {
                    temp_map_b[i] = (Elf32_Word)existing_idx;
                }
            } else {
                Elf32_Word name_offset =
                    add_string_to_strtab(&out->out_strtab, &out->out_strtab_size, sym_name);

                Elf32_Half new_shndx = map_section_index(sym->st_shndx, B, merge_result, 1);
                Elf32_Word value_adjust = 0;

                if (ELF32_IS_REAL_SHNDX(sym->st_shndx) &&
                    sym->st_shndx < (Elf32_Half)merge_result->remap.b_shnum) {
                    value_adjust = merge_result->remap.concat_off_b[sym->st_shndx];
                }

                if (add_symbol_to_output(&out->out_symtab, &out->out_sym_count, &out->out_strtab,
                                         &out->out_strtab_size, sym, strtab_b, name_offset,
                                         new_shndx, value_adjust) != 0) {
                    free(temp_map_a);
                    free(temp_map_b);
                    free(symtab_a);
                    free(symtab_b);
                    free(strtab_a);
                    free(strtab_b);
                    symbol_merge_result_free(out);
                    return -1;
                }

                temp_map_b[i] = (Elf32_Word)(out->out_sym_count - 1);
            }
        }
    }

    memcpy(out->remap.sym_map_a_to_out, temp_map_a, nsyms_a * sizeof(Elf32_Word));
    memcpy(out->remap.sym_map_b_to_out, temp_map_b, nsyms_b * sizeof(Elf32_Word));

    free(temp_map_a);
    free(temp_map_b);
    free(symtab_a);
    free(symtab_b);
    free(strtab_a);
    free(strtab_b);

    return 0;
}