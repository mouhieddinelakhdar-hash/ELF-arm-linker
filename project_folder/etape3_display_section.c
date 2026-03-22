#include "printELF.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Utilisation: %s <elf_file> <section_name_or_index>\n", argv[0]);
        fprintf(stderr, "Exemple: %s file.o .text\n", argv[0]);
        fprintf(stderr, "Exemple: %s file.o 1\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("Erreur : Échec de l'ouverture du fichier");
        return 1;
    }

    int rc = elf32_dump_section_hex(f, argv[2]);

    fclose(f);
    return (rc == 0) ? 0 : 1;
}
