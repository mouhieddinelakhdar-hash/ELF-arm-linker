#include "symbol_merge_types.h"
#include <stdlib.h>
#include <string.h>

void symbol_merge_result_free(symbol_merge_result_t *r) {
    if (!r) return;
    free(r->out_symtab);
    free(r->out_strtab);
    free(r->remap.sym_map_a_to_out);
    free(r->remap.sym_map_b_to_out);
    memset(r, 0, sizeof(*r));
}
