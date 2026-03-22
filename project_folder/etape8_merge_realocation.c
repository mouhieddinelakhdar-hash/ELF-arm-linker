#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ELF.h"
#include "elf_object.h"
#include "section_merge.h"
#include "section_merge_types.h"
#include "symbol_merge.h"
#include "symbol_merge_types.h"
#include "realocation_merge.h"

typedef struct {
    uint32_t offset;
    uint32_t sym_idx;
    uint32_t type;
    int is_file_b;
} relocation_info_t;

static relocation_info_t *orig_relocs_a = NULL;
static relocation_info_t *orig_relocs_b = NULL;
static size_t orig_count_a = 0;
static size_t orig_count_b = 0;

static relocation_info_t *mod_relocs_a = NULL;
static relocation_info_t *mod_relocs_b = NULL;
static size_t mod_count_a = 0;
static size_t mod_count_b = 0;

static void collect_original_relocations(const elf_object_t *obj, 
                                        int is_file_b,
                                        relocation_info_t **relocs,
                                        size_t *count) {
    size_t capacity = 10;
    *relocs = malloc(capacity * sizeof(relocation_info_t));
    *count = 0;
    
    for (Elf32_Half i = 0; i < obj->ehdr.e_shnum; i++) {
        const Elf32_Shdr *sh = &obj->sht[i];
        if (sh->sh_type != SHT_REL)
            continue;
        
        int nrel = sh->sh_size / sizeof(Elf32_Rel);
        Elf32_Rel *rels = elf32_read_rel(obj->f, sh);
        if (!rels)
            continue;
        
        for (int r = 0; r < nrel; r++) {
            Elf32_Rel *rel = &rels[r];
            
            if (*count >= capacity) {
                capacity *= 2;
                *relocs = realloc(*relocs, capacity * sizeof(relocation_info_t));
            }
            
            (*relocs)[*count].offset = rel->r_offset;
            (*relocs)[*count].sym_idx = ELF32_R_SYM(rel->r_info);
            (*relocs)[*count].type = ELF32_R_TYPE(rel->r_info);
            (*relocs)[*count].is_file_b = is_file_b;
            (*count)++;
        }
        
        free(rels);
    }
}

static void print_relocation_comparison(const char *tag,
                                       relocation_info_t *orig,
                                       relocation_info_t *mod,
                                       size_t count,
                                       const merge_result_t *sec_merge,
                                       const symbol_merge_result_t *sym_merge) {
    printf("=== %s: Relocation Transformation Details ===\n", tag);
    printf("Idx | Original offset | Modified offset | Delta | Original sym | Modified sym | Type\n");
    printf("----|-----------------|-----------------|-------|--------------|--------------|-----\n");
    
    for (size_t i = 0; i < count; i++) {
        int32_t delta = (int32_t)mod[i].offset - (int32_t)orig[i].offset;
        
        int32_t expected_delta = 0;
        if (orig[i].is_file_b) {
            expected_delta = 0;
        }
        
        printf("%3zu | %15u | %15u | %+5d | %12u | %12u | %u",
               i, orig[i].offset, mod[i].offset, delta,
               orig[i].sym_idx, mod[i].sym_idx, orig[i].type);
        
        if (delta == expected_delta) {
            printf(" ✓\n");
        } else {
            printf(" ✗ (expected delta: %d)\n", expected_delta);
        }
    }
    printf("\n");
}

static void print_relocations(const char *tag,
                              const elf_object_t *obj,
                              const merged_section_t *out_secs) {
    printf("=== %s: Relocation sections ===\n", tag);

    int found_reloc = 0;
    
    for (Elf32_Half i = 0; i < obj->ehdr.e_shnum; i++) {
        const Elf32_Shdr *sh = &obj->sht[i];
        if (sh->sh_type != SHT_REL)
            continue;
        
        found_reloc = 1;
        printf("Relocation section [%u], targets section %u\n",
               i, sh->sh_info);
        
        int nrel = sh->sh_size / sizeof(Elf32_Rel);
        Elf32_Rel *rels = elf32_read_rel(obj->f, sh);
        if (!rels)
            continue;
        
        for (int r = 0; r < nrel; r++) {
            Elf32_Rel *rel = &rels[r];
            printf("  r_offset=%u  r_info(sym=%u,type=%u)\n",
                   rel->r_offset,
                   ELF32_R_SYM(rel->r_info),
                   ELF32_R_TYPE(rel->r_info));
        }
        
        free(rels);
    }
    
    if (!found_reloc) {
        printf("No relocation sections found\n");
    }
    
    printf("\n");
}

static void usage(const char *argv0) {
    fprintf(stderr,
            "Usage: %s <A.o> <B.o>\n"
            "\n"
            "Step 8:\n"
            "  - merge PROGBITS sections (step 6)\n"
            "  - merge and renumber symbols (step 7)\n"
            "  - fix relocation entries (step 8)\n",
            argv0);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }

    const char *pathA = argv[1];
    const char *pathB = argv[2];

    FILE *fA = fopen(pathA, "rb");
    FILE *fB = fopen(pathB, "rb");
    if (!fA || !fB) {
        perror("fopen");
        return 1;
    }

    elf_object_t A, B;
    memset(&A, 0, sizeof(A));
    memset(&B, 0, sizeof(B));

    if (elf_object_load(fA, &A, 1) != 0 ||
        elf_object_load(fB, &B, 1) != 0) {
        fprintf(stderr, "Error: failed to load ELF objects\n");
        fclose(fA);
        fclose(fB);
        return 1;
    }

    if (A.ehdr.e_type != ET_REL || B.ehdr.e_type != ET_REL) {
        fprintf(stderr, "Error: inputs must be relocatable objects\n");
        elf_object_free(&A);
        elf_object_free(&B);
        fclose(fA);
        fclose(fB);
        return 1;
    }

    collect_original_relocations(&A, 0, &orig_relocs_a, &orig_count_a);
    collect_original_relocations(&B, 1, &orig_relocs_b, &orig_count_b);

    merge_result_t sec_merge;
    if (merge_progbits_sections(&A, &B, &sec_merge) != 0) {
        fprintf(stderr, "merge_progbits_sections failed\n");
        free(orig_relocs_a);
        free(orig_relocs_b);
        elf_object_free(&A);
        elf_object_free(&B);
        fclose(fA);
        fclose(fB);
        return 1;
    }

    symbol_merge_result_t sym_merge;
    if (merge_symbol_tables(&A, &B, &sec_merge, &sym_merge) != 0) {
        fprintf(stderr, "merge_symbol_tables failed\n");
        free(orig_relocs_a);
        free(orig_relocs_b);
        merge_result_free(&sec_merge);
        elf_object_free(&A);
        elf_object_free(&B);
        fclose(fA);
        fclose(fB);
        return 1;
    }

    printf("=== OUTPUT: merged relocations ===\n\n");
    
    printf("Original relocation counts:\n");
    printf("  File A: %zu relocations\n", orig_count_a);
    printf("  File B: %zu relocations\n", orig_count_b);
    printf("  Total:  %zu relocations\n\n", orig_count_a + orig_count_b);
    
    print_relocations("A (original)", &A, sec_merge.out_secs);
    print_relocations("B (original)", &B, sec_merge.out_secs);
    
    if (merge_and_fix_relocations(&A, &B,
                                  &sec_merge,
                                  &sym_merge,
                                  sec_merge.out_secs,
                                  sec_merge.out_count) != 0) {
        fprintf(stderr, "merge_and_fix_relocations failed\n");
        free(orig_relocs_a);
        free(orig_relocs_b);
        symbol_merge_result_free(&sym_merge);
        merge_result_free(&sec_merge);
        elf_object_free(&A);
        elf_object_free(&B);
        fclose(fA);
        fclose(fB);
        return 1;
    }

    collect_original_relocations(&A, 0, &mod_relocs_a, &mod_count_a);
    collect_original_relocations(&B, 1, &mod_relocs_b, &mod_count_b);
    
    printf("=== Relocations after merging and fixing ===\n\n");
    
    printf("Modified relocation counts:\n");
    printf("  File A: %zu relocations\n", mod_count_a);
    printf("  File B: %zu relocations\n", mod_count_b);
    printf("  Total:  %zu relocations\n\n", mod_count_a + mod_count_b);
    
    if (orig_count_a != mod_count_a) {
        printf("WARNING: File A relocation count changed! (%zu -> %zu)\n", 
               orig_count_a, mod_count_a);
    }
    if (orig_count_b != mod_count_b) {
        printf("WARNING: File B relocation count changed! (%zu -> %zu)\n", 
               orig_count_b, mod_count_b);
    }
    
    print_relocations("A (after fix)", &A, sec_merge.out_secs);
    print_relocations("B (after fix)", &B, sec_merge.out_secs);
    
    if (orig_count_a > 0 && mod_count_a > 0) {
        print_relocation_comparison("File A transformation",
                                   orig_relocs_a, mod_relocs_a,
                                   orig_count_a, &sec_merge, &sym_merge);
    }
    
    if (orig_count_b > 0 && mod_count_b > 0) {
        print_relocation_comparison("File B transformation",
                                   orig_relocs_b, mod_relocs_b,
                                   orig_count_b, &sec_merge, &sym_merge);
    }

    free(orig_relocs_a);
    free(orig_relocs_b);
    free(mod_relocs_a);
    free(mod_relocs_b);
    
    symbol_merge_result_free(&sym_merge);
    merge_result_free(&sec_merge);
    elf_object_free(&A);
    elf_object_free(&B);
    fclose(fA);
    fclose(fB);

    return 0;
}