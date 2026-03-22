#include "section_merge.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

Elf32_Word align_up_u32(Elf32_Word x, Elf32_Word align) {
    if (align == 0 || align == 1)
        return x;
    Elf32_Word rem = x % align;
    return rem ? (x + (align - rem)) : x;
}

static char *xstrdup(const char *s) {
    if (!s)
        return NULL;
    size_t n = strlen(s);
    char *p = (char *)malloc(n + 1);
    if (!p)
        return NULL;
    memcpy(p, s, n + 1);
    return p;
}

static int out_find_by_name(const merge_result_t *out, const char *name) {
    if (!out || !name)
        return -1;
    for (size_t i = 0; i < out->out_count; i++) {
        if (out->out_secs[i].name && strcmp(out->out_secs[i].name, name) == 0)
            return (int)i;
    }
    return -1;
}

static int out_append_section(merge_result_t *out, const char *name, const Elf32_Shdr *src_shdr,
                              const uint8_t *bytes, size_t size) {
    size_t new_count = out->out_count + 1;
    merged_section_t *tmp =
        (merged_section_t *)realloc(out->out_secs, new_count * sizeof(merged_section_t));
    if (!tmp)
        return -1;
    out->out_secs = tmp;

    merged_section_t *ms = &out->out_secs[out->out_count];
    memset(ms, 0, sizeof(*ms));

    ms->name = xstrdup(name);
    if (!ms->name)
        return -1;

    if (src_shdr)
        ms->shdr = *src_shdr;

    ms->bytes = NULL;
    ms->size = 0;

    if (src_shdr->sh_type != SHT_NOBITS && src_shdr->sh_type != SHT_NULL && size > 0) {
        ms->bytes = (uint8_t *)malloc(size);
        if (!ms->bytes)
            return -1;
        if (bytes)
            memcpy(ms->bytes, bytes, size);
        else
            memset(ms->bytes, 0, size);
        ms->size = size;
    }

    ms->shdr.sh_size = (Elf32_Word)ms->size;
    ms->shdr.sh_offset = 0;
    ms->shdr.sh_addr = 0;

    out->out_count = new_count;
    return (int)(new_count - 1);
}

static int out_concat_into(merged_section_t *dst, const Elf32_Shdr *bsh, const uint8_t *bbytes,
                           size_t bsize, Elf32_Word *out_concat_off) {
    if (!dst || !bsh)
        return -1;

    if (bsh->sh_type == SHT_NOBITS) {
        if (out_concat_off)
            *out_concat_off = (Elf32_Word)dst->size;

        dst->size += bsize;
        dst->shdr.sh_size = (Elf32_Word)dst->size;

        if (bsh->sh_addralign > dst->shdr.sh_addralign)
            dst->shdr.sh_addralign = bsh->sh_addralign;

        dst->shdr.sh_flags |= bsh->sh_flags;
        return 0;
    }

    Elf32_Word a_size = (Elf32_Word)dst->size;
    Elf32_Word merged_align = dst->shdr.sh_addralign;
    if (bsh->sh_addralign > merged_align)
        merged_align = bsh->sh_addralign;
    if (merged_align == 0)
        merged_align = 1;
    dst->shdr.sh_addralign = merged_align;

    Elf32_Word startB = align_up_u32(a_size, merged_align);
    Elf32_Word padding = startB - a_size;

    if (out_concat_off)
        *out_concat_off = startB;

    size_t new_size = (size_t)startB + bsize;
    uint8_t *tmp = (uint8_t *)realloc(dst->bytes, new_size);
    if (!tmp && new_size > 0)
        return -1;
    dst->bytes = tmp;

    if (padding > 0) {
        memset(dst->bytes + a_size, 0, padding);
    }

    if (bsize > 0 && bbytes) {
        memcpy(dst->bytes + startB, bbytes, bsize);
    } else if (bsize > 0) {
        memset(dst->bytes + startB, 0, bsize);
    }

    dst->size = new_size;
    dst->shdr.sh_size = (Elf32_Word)dst->size;
    dst->shdr.sh_flags |= bsh->sh_flags;

    return 0;
}

static int should_merge_section(Elf32_Word sh_type) {
    return (sh_type == SHT_PROGBITS || sh_type == SHT_NOBITS ||
            sh_type == SHT_ARM_ATTRIBUTES ||
            sh_type == SHT_REL);
}

/* static int should_copy_section(Elf32_Word sh_type) {
    return (sh_type == SHT_STRTAB ||
            sh_type == SHT_SYMTAB ||
            sh_type == SHT_NULL);
} */

int merge_progbits_sections(const elf_object_t *A, const elf_object_t *B, merge_result_t *out) {
    if (!A || !B || !out)
        return -1;
    memset(out, 0, sizeof(*out));

    out->remap.b_shnum = (size_t)B->ehdr.e_shnum;
    out->remap.sec_map_b_to_out = (Elf32_Half *)malloc(sizeof(Elf32_Half) * out->remap.b_shnum);
    out->remap.concat_off_b = (Elf32_Word *)malloc(sizeof(Elf32_Word) * out->remap.b_shnum);
    if (!out->remap.sec_map_b_to_out || !out->remap.concat_off_b) {
        merge_result_free(out);
        return -1;
    }

    for (size_t i = 0; i < out->remap.b_shnum; i++) {
        out->remap.sec_map_b_to_out[i] = SHN_UNDEF;
        out->remap.concat_off_b[i] = 0;
    }

    for (Elf32_Half i = 0; i < A->ehdr.e_shnum; i++) {
        const Elf32_Shdr *ash = &A->sht[i];

        if (!should_merge_section(ash->sh_type))
            continue;

        const char *name = elf_obj_sec_name(A, i);
        if (!name)
            continue;

        const uint8_t *abytes = elf_obj_sec_bytes(A, i);
        size_t asize = (size_t)ash->sh_size;

        if (ash->sh_type == SHT_REL) {
            continue;
        }

        if (out_append_section(out, name, ash, abytes, asize) < 0) {
            merge_result_free(out);
            return -1;
        }
    }

    for (Elf32_Half j = 0; j < B->ehdr.e_shnum; j++) {
        const Elf32_Shdr *bsh = &B->sht[j];

        if (!should_merge_section(bsh->sh_type))
            continue;

        const char *name = elf_obj_sec_name(B, j);
        if (!name)
            continue;

        const uint8_t *bbytes = elf_obj_sec_bytes(B, j);
        size_t bsize = (size_t)bsh->sh_size;

        if (bsh->sh_type == SHT_REL) {
            continue;
        }

        int idx = out_find_by_name(out, name);

        if (idx >= 0) {
            Elf32_Word concat_off = 0;
            if (out_concat_into(&out->out_secs[idx], bsh, bbytes, bsize, &concat_off) != 0) {
                merge_result_free(out);
                return -1;
            }

            out->remap.sec_map_b_to_out[j] = (Elf32_Half)(idx + 1);
            out->remap.concat_off_b[j] = concat_off;
        } else {
            int new_idx = out_append_section(out, name, bsh, bbytes, bsize);
            if (new_idx < 0) {
                merge_result_free(out);
                return -1;
            }

            out->remap.sec_map_b_to_out[j] = (Elf32_Half)(new_idx + 1);
            out->remap.concat_off_b[j] = 0;
        }
    }

    return 0;
}