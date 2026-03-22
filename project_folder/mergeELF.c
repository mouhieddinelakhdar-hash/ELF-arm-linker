#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ELF.h"
#include "ELFcntx.h"
#include "readELF.h"

#include "elf_object.h"
#include "section_merge.h"
#include "section_merge_types.h"
#include "symbol_merge.h"
#include "symbol_merge_types.h"
#include "realocation_merge.h"
#include "writeELF.h"

#include "mergeELF.h"

int elf32_validate_relocatable_file(const char *filename, int verbose)
{
    FILE *f = fopen(filename, "rb");
    if (!f)
    {
        perror("fopen");
        return -1;
    }

    ELFcntx ctx;
    if (elf32_open(f, &ctx) != 0)
    {
        if (verbose)
            fprintf(stderr, "Error: %s is not a valid ELF32 file\n", filename);
        fclose(f);
        return -1;
    }

    const Elf32_Ehdr *ehdr = elf32_ehdr(&ctx);
    if (!ehdr)
    {
        if (verbose)
            fprintf(stderr, "Error: invalid ELF context\n");
        fclose(f);
        return -1;
    }

    if (ehdr->e_type != ET_REL)
    {
        if (verbose)
            fprintf(stderr, "Error: %s is not relocatable (type: 0x%x)\n",
                    filename, ehdr->e_type);
        fclose(f);
        return -1;
    }

    if (ehdr->e_machine != EM_ARM && ehdr->e_machine != EM_386)
    {
        if (verbose)
            fprintf(stderr, "Error: %s unsupported machine: 0x%x\n",
                    filename, ehdr->e_machine);
        fclose(f);
        return -1;
    }

    if (ehdr->e_shoff == 0 || ehdr->e_shnum == 0)
    {
        if (verbose)
            fprintf(stderr, "Error: %s has no section header table\n", filename);
        fclose(f);
        return -1;
    }

    if (ehdr->e_shstrndx >= ehdr->e_shnum)
    {
        if (verbose)
            fprintf(stderr, "Error: invalid .shstrtab index in %s\n", filename);
        fclose(f);
        return -1;
    }

    if (verbose)
        printf("✓ %s is a valid ELF32 relocatable file\n", filename);

    Elf32_Shdr *sht = elf32_read_sht(f, ehdr);
    if (!sht)
    {
        if (verbose)
            fprintf(stderr, "Error: cannot read section header table\n");
        fclose(f);
        return -1;
    }

    char *shstrtab = elf32_read_shstrtab(f, ehdr, sht);
    if (!shstrtab)
    {
        if (verbose)
            fprintf(stderr, "Error: cannot read .shstrtab\n");
        free(sht);
        fclose(f);
        return -1;
    }

    int has_symtab = 0, has_strtab = 0, has_shstrtab = 0, has_progbits = 0;

    for (unsigned int i = 0; i < ehdr->e_shnum; i++)
    {
        const char *name = elf32_section_name(&sht[i], shstrtab);
        if (!name)
            name = "(unnamed)";

        if (sht[i].sh_type == SHT_SYMTAB || sht[i].sh_type == SHT_DYNSYM)
            has_symtab = 1;
        else if (sht[i].sh_type == SHT_STRTAB)
        {
            if (strcmp(name, ".strtab") == 0)
                has_strtab = 1;
            else if (strcmp(name, ".shstrtab") == 0)
                has_shstrtab = 1;
        }
        else if (sht[i].sh_type == SHT_PROGBITS || sht[i].sh_type == SHT_NOBITS)
            has_progbits = 1;
    }

    if (verbose)
    {
        printf("  Sections: %u\n", (unsigned)ehdr->e_shnum);
        printf("  Section header offset: 0x%x\n", (unsigned)ehdr->e_shoff);
        printf("  Contains: ");
        if (has_progbits)
            printf(".progbits/.nobits ");
        if (has_symtab)
            printf(".symtab ");
        if (has_strtab)
            printf(".strtab ");
        if (has_shstrtab)
            printf(".shstrtab ");
        printf("\n");
    }

    free(shstrtab);
    free(sht);
    fclose(f);
    return 0;
}

void elf32_run_external_validation(const char *filename)
{
    printf("\n=== External validation ===\n");

    char cmd[256];

    printf("$ file %s\n", filename);
    snprintf(cmd, sizeof(cmd), "file %s", filename);
    system(cmd);

    printf("\n$ readelf -h %s\n", filename);
    snprintf(cmd, sizeof(cmd), "readelf -h %s 2>/dev/null || echo 'readelf not found'", filename);
    system(cmd);

    printf("\n$ objdump -h %s\n", filename);
    snprintf(cmd, sizeof(cmd), "objdump -h %s 2>/dev/null || echo 'objdump not found'", filename);
    system(cmd);

    printf("\n=== ARM-specific validation ===\n");
    snprintf(cmd, sizeof(cmd), "arm-none-eabi-objdump -h %s 2>/dev/null", filename);
    if (system(cmd) != 0)
        printf("arm-none-eabi-objdump not available or failed\n");

    printf("\n=== Link test (dry run) ===\n");
    snprintf(cmd, sizeof(cmd), "arm-none-eabi-ld -r %s -o /tmp/test_linked.o 2>&1 | head -20",
             filename);
    system(cmd);

    printf("\n");
}

int elf32_merge_two_objects(const char *pathA, const char *pathB,
                            const char *output_path,
                            int verbose,
                            int external_validate)
{
    if (!pathA || !pathB || !output_path)
        return -1;

    if (verbose)
    {
        printf("=== Merge ELF relocatables ===\n");
        printf("A: %s\n", pathA);
        printf("B: %s\n", pathB);
        printf("OUT: %s\n\n", output_path);
    }

    if (verbose)
        printf("=== Validating input files ===\n");
    if (elf32_validate_relocatable_file(pathA, verbose) != 0 ||
        elf32_validate_relocatable_file(pathB, verbose) != 0)
    {
        fprintf(stderr, "Error: invalid input files\n");
        return -1;
    }

    FILE *fA = fopen(pathA, "rb");
    FILE *fB = fopen(pathB, "rb");
    if (!fA || !fB)
    {
        perror("fopen");
        if (fA)
            fclose(fA);
        if (fB)
            fclose(fB);
        return -1;
    }

    elf_object_t A, B;
    memset(&A, 0, sizeof(A));
    memset(&B, 0, sizeof(B));

    if (verbose)
        printf("\n=== Loading ELF objects ===\n");
    if (elf_object_load(fA, &A, 1) != 0 || elf_object_load(fB, &B, 1) != 0)
    {
        fprintf(stderr, "Error: failed to load objects\n");
        fclose(fA);
        fclose(fB);
        return -1;
    }

    if (verbose)
        printf("✓ Loaded\n");

    if (verbose)
        printf("\n=== Step 6: Merging PROGBITS sections ===\n");
    merge_result_t sec_merge;
    if (merge_progbits_sections(&A, &B, &sec_merge) != 0)
    {
        fprintf(stderr, "✗ merge_progbits_sections failed\n");
        elf_object_free(&A);
        elf_object_free(&B);
        fclose(fA);
        fclose(fB);
        return -1;
    }
    if (verbose)
        printf("✓ Merged %zu PROGBITS sections\n", sec_merge.out_count);

    if (verbose)
        printf("\n=== Step 7: Merging symbol tables ===\n");
    symbol_merge_result_t sym_merge;
    if (merge_symbol_tables(&A, &B, &sec_merge, &sym_merge) != 0)
    {
        fprintf(stderr, "✗ merge_symbol_tables failed\n");
        merge_result_free(&sec_merge);
        elf_object_free(&A);
        elf_object_free(&B);
        fclose(fA);
        fclose(fB);
        return -1;
    }
    if (verbose)
        printf("✓ Merged symbols: %zu\n", sym_merge.out_sym_count);

    if (verbose)
        printf("\n=== Step 8: Fixing relocations ===\n");
    if (merge_and_fix_relocations(&A, &B, &sec_merge, &sym_merge,
                                  sec_merge.out_secs, sec_merge.out_count) != 0)
    {
        fprintf(stderr, "✗ merge_and_fix_relocations failed\n");
        symbol_merge_result_free(&sym_merge);
        merge_result_free(&sec_merge);
        elf_object_free(&A);
        elf_object_free(&B);
        fclose(fA);
        fclose(fB);
        return -1;
    }
    if (verbose)
        printf("✓ Relocations fixed\n");

    if (verbose)
        printf("\n=== Step 9: Building + writing output ELF ===\n");
    merged_elf_t merged_elf;
    if (build_output_sections(&sec_merge, &sym_merge, &merged_elf) != 0)
    {
        fprintf(stderr, "✗ build_output_sections failed\n");
        symbol_merge_result_free(&sym_merge);
        merge_result_free(&sec_merge);
        elf_object_free(&A);
        elf_object_free(&B);
        fclose(fA);
        fclose(fB);
        return -1;
    }

    if (write_merged_elf(output_path, &merged_elf) != 0)
    {
        fprintf(stderr, "✗ write_merged_elf failed\n");
        merged_elf_free(&merged_elf);
        symbol_merge_result_free(&sym_merge);
        merge_result_free(&sec_merge);
        elf_object_free(&A);
        elf_object_free(&B);
        fclose(fA);
        fclose(fB);
        return -1;
    }

    if (verbose)
        printf("✓ Wrote %s\n", output_path);

    if (verbose)
        printf("\n=== Validating output file ===\n");
    if (elf32_validate_relocatable_file(output_path, verbose) != 0)
    {
        fprintf(stderr, "✗ output validation failed\n");
        merged_elf_free(&merged_elf);
        symbol_merge_result_free(&sym_merge);
        merge_result_free(&sec_merge);
        elf_object_free(&A);
        elf_object_free(&B);
        fclose(fA);
        fclose(fB);
        return -1;
    }

    if (external_validate)
        elf32_run_external_validation(output_path);

    merged_elf_free(&merged_elf);
    symbol_merge_result_free(&sym_merge);
    merge_result_free(&sec_merge);
    elf_object_free(&A);
    elf_object_free(&B);
    fclose(fA);
    fclose(fB);

    if (verbose)
        printf("\n=== Merge OK ===\n");
    return 0;
}
