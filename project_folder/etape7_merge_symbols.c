#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ELF.h"
#include "elf_object.h"
#include "printELF.h"
#include "readELF.h"
#include "section_merge.h"
#include "section_merge_types.h"
#include "symbol_merge.h"
#include "symbol_merge_types.h"

// Etape 7 : Merge symbols
// hadchi main program dyal étape 7
// kaydir merge dyal symbol tables dyal A w B

static void print_symbol_table(const symbol_merge_result_t *result)
{
    printf("=== OUTPUT: merged symbol table ===\n");

    if (elf32_print_symbol_table(result->out_symtab, result->out_sym_count, result->out_strtab,
                                 NULL, NULL) != 0)
    {
        fprintf(stderr, "Error: failed to print symbol table\n");
    }
    printf("\n");
}

static void print_symbol_maps(const symbol_merge_result_t *result, const elf_object_t *A,
                              const elf_object_t *B)
{
    printf("=== Symbol mappings ===\n");
    printf("A->OUT mappings (first %zu symbols):\n", result->remap.a_sym_count);
    for (size_t i = 0; i < result->remap.a_sym_count && i < 20; i++)
    {
        printf("  A[%zu] -> OUT[%u]\n", i, (unsigned)result->remap.sym_map_a_to_out[i]);
    }
    if (result->remap.a_sym_count > 20)
        printf("  ... (%zu more)\n", result->remap.a_sym_count - 20);

    printf("\nB->OUT mappings (first %zu symbols):\n", result->remap.b_sym_count);
    for (size_t i = 0; i < result->remap.b_sym_count && i < 20; i++)
    {
        printf("  B[%zu] -> OUT[%u]\n", i, (unsigned)result->remap.sym_map_b_to_out[i]);
    }
    if (result->remap.b_sym_count > 20)
        printf("  ... (%zu more)\n", result->remap.b_sym_count - 20);
    printf("\n");
}

static void usage(const char *argv0)
{
    fprintf(stderr,
            "Usage: %s <A.o> <B.o>\n"
            "\n"
            "Reads two ELF32 relocatable objects and performs Step 7:\n"
            "  - merge symbol tables from A and B\n"
            "  - renumber symbols in output table\n"
            "  - update symbol section references to output sections\n"
            "  - correct symbol values for merged sections\n",
            argv0);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        usage(argv[0]);
        return 1;
    }

    const char *pathA = argv[1];
    const char *pathB = argv[2];

    FILE *fA = fopen(pathA, "rb");
    if (!fA)
    {
        perror("fopen A");
        return 1;
    }

    FILE *fB = fopen(pathB, "rb");
    if (!fB)
    {
        perror("fopen B");
        fclose(fA);
        return 1;
    }

    elf_object_t A, B;
    memset(&A, 0, sizeof(A));
    memset(&B, 0, sizeof(B));

    if (elf_object_load(fA, &A, 1) != 0)
    {
        fprintf(stderr, "Error: failed to load %s as ELF32 relocatable\n", pathA);
        fclose(fA);
        fclose(fB);
        return 1;
    }
    if (elf_object_load(fB, &B, 1) != 0)
    {
        fprintf(stderr, "Error: failed to load %s as ELF32 relocatable\n", pathB);
        elf_object_free(&A);
        fclose(fA);
        fclose(fB);
        return 1;
    }

    if (A.ehdr.e_type != ET_REL || B.ehdr.e_type != ET_REL)
    {
        fprintf(stderr, "Error: both inputs must be ET_REL (.o) objects\n");
        elf_object_free(&A);
        elf_object_free(&B);
        fclose(fA);
        fclose(fB);
        return 1;
    }
    // validation: kolchi khass ARM (7it project kyna ARM)
    if (A.ehdr.e_machine != EM_ARM || B.ehdr.e_machine != EM_ARM)
    {
        fprintf(stderr, "Error: both inputs must be ARM (e_machine = EM_ARM)\n");
        elf_object_free(&A);
        elf_object_free(&B);
        fclose(fA);
        fclose(fB);
        return 1;
    }

    // Step 1: merge sections (étape 6)
    merge_result_t merge_result;
    memset(&merge_result, 0, sizeof(merge_result));

    if (merge_progbits_sections(&A, &B, &merge_result) != 0)
    {
        fprintf(stderr, "Error: merge_progbits_sections failed\n");
        elf_object_free(&A);
        elf_object_free(&B);
        fclose(fA);
        fclose(fB);
        return 1;
    }

    // Step 2: merge symbols (étape 7 - hadchi li kaykon f hadchi file)
    symbol_merge_result_t symbol_result;
    memset(&symbol_result, 0, sizeof(symbol_result));

    if (merge_symbol_tables(&A, &B, &merge_result, &symbol_result) != 0)
    {
        fprintf(stderr, "Error: merge_symbol_tables failed\n");
        merge_result_free(&merge_result);
        elf_object_free(&A);
        elf_object_free(&B);
        fclose(fA);
        fclose(fB);
        return 1;
    }

    print_symbol_table(&symbol_result);
    print_symbol_maps(&symbol_result, &A, &B);

    symbol_merge_result_free(&symbol_result);
    merge_result_free(&merge_result);
    elf_object_free(&A);
    elf_object_free(&B);
    fclose(fA);
    fclose(fB);

    return 0;
}
