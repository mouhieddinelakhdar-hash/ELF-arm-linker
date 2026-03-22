#include "section_merge_types.h"
#include <stdlib.h>
#include <string.h>

void merge_result_free(merge_result_t *r) {
    if (!r)
        return;

    if (r->out_secs) {
        for (size_t i = 0; i < r->out_count; i++) {
            free(r->out_secs[i].name);
            free(r->out_secs[i].bytes);
            r->out_secs[i].name = NULL;
            r->out_secs[i].bytes = NULL;
            r->out_secs[i].size = 0;
            memset(&r->out_secs[i].shdr, 0, sizeof(r->out_secs[i].shdr));
        }
        free(r->out_secs);
        r->out_secs = NULL;
        r->out_count = 0;
    }

    free(r->remap.sec_map_b_to_out);
    free(r->remap.concat_off_b);
    r->remap.sec_map_b_to_out = NULL;
    r->remap.concat_off_b = NULL;
    r->remap.b_shnum = 0;
}
