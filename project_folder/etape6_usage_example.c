// this is only an example of the usage of the functions created in etape 6
// the goal is to just giving an idea how u can use them

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ELF.h"
#include "elf_object.h"
#include "section_merge.h"
#include "section_merge_types.h"

static void print_input_progbits(const char *tag, const elf_object_t *obj) {
    printf("=== %s: PROGBITS sections ===\n", tag);
    for (Elf32_Half i = 0; i < obj->ehdr.e_shnum; i++) {
        const Elf32_Shdr *sh = &obj->sht[i];
        if (sh->sh_type != SHT_PROGBITS)
            continue;

        const char *name = elf_obj_sec_name(obj, i);
        if (!name)
            name = "<noname>";

        printf("  [%u] %-16s size=%u flags=0x%X align=%u\n", (unsigned)i, name,
               (unsigned)sh->sh_size, (unsigned)sh->sh_flags, (unsigned)sh->sh_addralign);
    }
    printf("\n");
}

static void print_merge_result(const merge_result_t *R) {
    printf("=== OUTPUT: merged PROGBITS sections (Step 6 result) ===\n");
    for (size_t i = 0; i < R->out_count; i++) {
        const merged_section_t *s = &R->out_secs[i];
        printf("  OUT[%zu] (shndx-like=%zu) %-16s size=%u flags=0x%X align=%u\n", i, i + 1,
               s->name ? s->name : "<noname>", (unsigned)s->shdr.sh_size,
               (unsigned)s->shdr.sh_flags, (unsigned)s->shdr.sh_addralign);
    }
    printf("\n");
}

static void print_b_maps(const elf_object_t *B, const merge_result_t *R) {
    printf("=== MAPS for B (used by Step 7) ===\n");
    printf("Format: B_shndx  B_name   -> OUT_shndx_like   concat_off\n");

    for (Elf32_Half b = 0; b < B->ehdr.e_shnum; b++) {
        const Elf32_Shdr *bsh = &B->sht[b];
        if (bsh->sh_type != SHT_PROGBITS)
            continue;

        const char *bname = elf_obj_sec_name(B, b);
        if (!bname)
            bname = "<noname>";

        Elf32_Half out_shndx_like = R->remap.sec_map_b_to_out[b];
        Elf32_Word off = R->remap.concat_off_b[b];

        printf("  [%u] %-16s -> %-14u  %u\n", (unsigned)b, bname, (unsigned)out_shndx_like,
               (unsigned)off);
    }

    printf("\n");
}

static void usage(const char *argv0) {
    fprintf(stderr,
            "Usage: %s <A.o> <B.o>\n"
            "\n"
            "Reads two ELF32 relocatable objects and performs Step 6:\n"
            "  - merge PROGBITS sections by name (A then B)\n"
            "  - build B->OUT section index map\n"
            "  - build concat offsets for B\n",
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
    if (!fA) {
        perror("fopen A");
        return 1;
    }

    FILE *fB = fopen(pathB, "rb");
    if (!fB) {
        perror("fopen B");
        fclose(fA);
        return 1;
    }

    elf_object_t A, B;
    memset(&A, 0, sizeof(A));
    memset(&B, 0, sizeof(B));

    if (elf_object_load(fA, &A, 1) != 0) {
        fprintf(stderr, "Error: failed to load %s as ELF32 relocatable\n", pathA);
        fclose(fA);
        fclose(fB);
        return 1;
    }
    if (elf_object_load(fB, &B, 1) != 0) {
        fprintf(stderr, "Error: failed to load %s as ELF32 relocatable\n", pathB);
        elf_object_free(&A);
        fclose(fA);
        fclose(fB);
        return 1;
    }

    if (A.ehdr.e_type != ET_REL || B.ehdr.e_type != ET_REL) {
        fprintf(stderr, "Error: both inputs must be ET_REL (.o) objects\n");
        elf_object_free(&A);
        elf_object_free(&B);
        fclose(fA);
        fclose(fB);
        return 1;
    }
    if (A.ehdr.e_machine != EM_ARM || B.ehdr.e_machine != EM_ARM) {
        fprintf(stderr, "Error: both inputs must be ARM (e_machine = EM_ARM)\n");
        elf_object_free(&A);
        elf_object_free(&B);
        fclose(fA);
        fclose(fB);
        return 1;
    }

    print_input_progbits("A", &A);
    print_input_progbits("B", &B);

    merge_result_t R;
    if (merge_progbits_sections(&A, &B, &R) != 0) {
        fprintf(stderr, "Error: merge_progbits_sections failed\n");
        elf_object_free(&A);
        elf_object_free(&B);
        fclose(fA);
        fclose(fB);
        return 1;
    }

    print_merge_result(&R);
    print_b_maps(&B, &R);

    merge_result_free(&R);
    elf_object_free(&A);
    elf_object_free(&B);
    fclose(fA);
    fclose(fB);

    return 0;
}
