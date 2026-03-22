#include "elf_object.h"
#include "ELFcntx.h"
#include "readELF.h"
#include <stdlib.h>
#include <string.h>

static void elf_object_zero(elf_object_t *obj) {
    if (!obj)
        return;
    memset(obj, 0, sizeof(*obj));
}

int elf_object_load(FILE *f, elf_object_t *obj, int load_all_sections) {
    if (!f || !obj)
        return -1;
    elf_object_zero(obj);

    obj->f = f;

    ELFcntx ctx;
    memset(&ctx, 0, sizeof(ctx));

    if (elf32_open(f, &ctx) != 0) {
        return -1;
    }
    obj->ehdr = ctx.ehdr;

    obj->sht = elf32_read_sht(f, &obj->ehdr);
    if (!obj->sht) {
        elf_object_free(obj);
        return -1;
    }

    obj->shstrtab = elf32_read_shstrtab(f, &obj->ehdr, obj->sht);
    if (!obj->shstrtab) {
        elf_object_free(obj);
        return -1;
    }

    obj->sec_data = (void **)calloc(obj->ehdr.e_shnum, sizeof(void *));
    if (!obj->sec_data) {
        elf_object_free(obj);
        return -1;
    }

    if (load_all_sections) {
        for (Elf32_Half i = 0; i < obj->ehdr.e_shnum; i++) {
            const Elf32_Shdr *sh = &obj->sht[i];

            // Don't load NOBITS into memory
            if (sh->sh_type == SHT_NOBITS || sh->sh_size == 0) {
                obj->sec_data[i] = NULL;
                continue;
            }

            obj->sec_data[i] = elf32_read_section(f, sh);
        }
    }

    return 0;
}

void elf_object_free(elf_object_t *obj) {
    if (!obj)
        return;

    if (obj->sec_data) {
        for (Elf32_Half i = 0; i < obj->ehdr.e_shnum; i++) {
            if (obj->sec_data[i]) {
                elf32_free(obj->sec_data[i]);
                obj->sec_data[i] = NULL;
            }
        }
        free(obj->sec_data);
        obj->sec_data = NULL;
    }

    if (obj->shstrtab) {
        elf32_free(obj->shstrtab);
        obj->shstrtab = NULL;
    }

    if (obj->sht) {
        elf32_free(obj->sht);
        obj->sht = NULL;
    }

    memset(&obj->ehdr, 0, sizeof(obj->ehdr));
    obj->f = NULL;
}
