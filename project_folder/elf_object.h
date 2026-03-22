#ifndef ELF_OBJECT_H
#define ELF_OBJECT_H

#include "ELF.h"
#include "readELF.h"
#include <stdint.h>
#include <stdio.h>

typedef struct {
    Elf32_Ehdr ehdr; 
    Elf32_Shdr *sht; 
    char *shstrtab;  
    void **sec_data; 
    FILE *f;
} elf_object_t;

int elf_object_load(FILE *f, elf_object_t *obj, int load_all_sections);

void elf_object_free(elf_object_t *obj);

static inline const char *elf_obj_sec_name(const elf_object_t *obj, int shndx) {
    if (!obj || !obj->sht || !obj->shstrtab)
        return NULL;
    return elf32_section_name(&obj->sht[shndx], obj->shstrtab);
}

static inline const uint8_t *elf_obj_sec_bytes(const elf_object_t *obj, int shndx) {
    if (!obj || !obj->sec_data)
        return NULL;
    return (const uint8_t *)obj->sec_data[shndx];
}

#endif
