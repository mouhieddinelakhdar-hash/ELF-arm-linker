#ifndef SECTION_MERGE_TYPES_H
#define SECTION_MERGE_TYPES_H

#include "ELF.h"
#include <stddef.h>
#include <stdint.h>

typedef struct {
    Elf32_Half *sec_map_b_to_out;
    Elf32_Word *concat_off_b;
    size_t b_shnum;
} section_remap_t;

typedef struct {
    char *name;
    Elf32_Shdr shdr;
    uint8_t *bytes;
    size_t size;
} merged_section_t;

typedef struct {
    merged_section_t *out_secs;
    size_t out_count;

    section_remap_t remap;
} merge_result_t;

void merge_result_free(merge_result_t *r);

#endif
