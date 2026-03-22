#include "printELF.h"
#include <stdio.h>

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file.o>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    if (elf32_print_sections(f) != 0) {
        fclose(f);
        return 1;
    }

    fclose(f);
    return 0;
}
