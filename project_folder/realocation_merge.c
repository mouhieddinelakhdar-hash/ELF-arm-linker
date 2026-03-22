#include "realocation_merge.h"
#include <stdlib.h>
#include <string.h>

// Etape 8 : Fix relocations
// hadchi bach n7awlo l relocations bach yt9ado 3la sections merged
// hhhhh hadchi m3r9 bzzaf

// hadchi bach n7awlo addend dyal relocation b7sb type d relocation
// 7it ARM 3ando types khrjin (R_ARM_CALL, R_ARM_JUMP24, etc.)
static void patch_section_addend(merged_section_t *out_sec, Elf32_Word r_offset,
                                 Elf32_Word sec_concat_off, uint32_t r_type) {
    uint32_t *where = (uint32_t *)(out_sec->bytes + r_offset);

    switch (r_type) {

    case R_ARM_ABS32:
        *where += sec_concat_off;
        break;

    case R_ARM_CALL:
    case R_ARM_JUMP24:
        *where += (sec_concat_off / 4);
        break;

    default:
        break;
    }
}

// hadchi kay7awlo kol relocation entry
// kay7taj: n7awlo r_offset, n7awlo r_info (symbol index), w n7awlo addend
static int fix_relocation_entries(const elf_object_t *obj, int is_file_b,
                                  const merge_result_t *sec_merge,
                                  const symbol_merge_result_t *sym_merge,
                                  merged_section_t *out_secs, size_t out_sec_count) {
    (void)out_sec_count;

    // nparcouiw kol sections w nshofo win kaynin relocations
    for (Elf32_Half i = 0; i < obj->ehdr.e_shnum; i++) {
        const Elf32_Shdr *sh = &obj->sht[i];

        if (sh->sh_type != SHT_REL) // hadchi relocation section
            continue;

        int target_sec_idx =
            is_file_b ? sec_merge->remap.sec_map_b_to_out[sh->sh_info] : sh->sh_info;

        if (target_sec_idx == SHN_UNDEF || target_sec_idx == 0)
            continue;

        merged_section_t *out_sec = &out_secs[target_sec_idx - 1];

        int nrel = sh->sh_size / sizeof(Elf32_Rel);

        Elf32_Rel *rels = elf32_read_rel(obj->f, sh);
        if (!rels)
            return -1;

        // nparcouiw kol relocation entry f had section
        for (int r = 0; r < nrel; r++) {
            Elf32_Rel *rel = &rels[r];

            // Fix r_offset: ida file B, 7tajna nzido concat offset
            if (is_file_b) {
                Elf32_Half orig_sec = sh->sh_info;
                rel->r_offset += sec_merge->remap.concat_off_b[orig_sec];
            }

            // Fix r_info: n7awlo symbol index
            uint32_t old_sym = ELF32_R_SYM(rel->r_info);
            uint32_t r_type = ELF32_R_TYPE(rel->r_info);

            // shof win rah symbol dyalna f merged table
            uint32_t new_sym = is_file_b ? sym_merge->remap.sym_map_b_to_out[old_sym]
                                         : sym_merge->remap.sym_map_a_to_out[old_sym];

            rel->r_info = ELF32_R_INFO(new_sym, r_type); // update

            Elf32_Sym *sym = &sym_merge->out_symtab[new_sym];

            if (ELF32_ST_TYPE(sym->st_info) == STT_SECTION && is_file_b &&
                ELF32_IS_REAL_SHNDX(sym->st_shndx)) {

                Elf32_Word sec_off = sec_merge->remap.concat_off_b[sym->st_shndx];

                patch_section_addend(out_sec, rel->r_offset, sec_off, r_type);
            }
        }

        free(rels);
    }

    return 0;
}

// hadchi main function dyal étape 8
// kaydir fix dyal relocations dyal A w B
int merge_and_fix_relocations(const elf_object_t *A, const elf_object_t *B,
                              const merge_result_t *sec_merge,
                              const symbol_merge_result_t *sym_merge, merged_section_t *out_secs,
                              size_t out_sec_count) {
    // fix relocations dyal A
    if (fix_relocation_entries(A, 0, sec_merge, sym_merge, out_secs, out_sec_count) != 0)
        return -1;

    // fix relocations dyal B
    if (fix_relocation_entries(B, 1, sec_merge, sym_merge, out_secs, out_sec_count) != 0)
        return -1;

    return 0; // mzyan!
}

int write_relocation_sections(const elf_object_t *A, const elf_object_t *B,
                              const merge_result_t *sec_merge,
                              const symbol_merge_result_t *sym_merge, merged_section_t *out_secs,
                              size_t out_sec_count, Elf32_Shdr *out_sht, size_t *sht_index) {

    return 0;
}