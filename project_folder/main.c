#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "printELF.h"
#include "mergeELF.h"

static void usage(const char *name)
{
    fprintf(stderr,
            "Usage:\n"
            "  %s [--help] [--debug file] [--header] [--sections] [--hex name|index]\n"
            "     [--symbols] [--relocs] elf_file\n\n"
            "  %s --merge A.o B.o [-o output.o] [--external] [--quiet]\n\n"
            "Options:\n"
            "  --help              Show this help\n"
            "  --debug <file>      Enable debug() output for the named source file\n"
            "  --header            Print ELF32 header\n"
            "  --sections          Print section header table\n"
            "  --hex <arg>         Hex dump a section by name (e.g. .text) or index (e.g. 5)\n"
            "  --symbols           Print symbol table (.symtab or .dynsym)\n"
            "  --relocs            Print relocation sections\n\n"
            "Merge mode:\n"
            "  --merge             Merge two relocatable objects (expects A.o B.o after it)\n"
            "  -o, --output <file> Output filename (default: merged.o)\n"
            "  --external          Run external validation commands on the output\n"
            "  --quiet             Less verbose output\n\n"
            "Examples:\n"
            "  %s --header prog.o\n"
            "  %s --sections prog.o\n"
            "  %s --hex .text prog.o\n"
            "  %s --symbols --relocs prog.o\n"
            "  %s --merge A.o B.o -o merged.o --external\n",
            name, name, name, name, name, name, name);
}

int main(int argc, char *argv[])
{
    int opt;

    int do_header = 0;
    int do_sections = 0;
    int do_symbols = 0;
    int do_relocs = 0;
    const char *hex_arg = NULL;

    int do_merge = 0;
    const char *out_path = "merged.o";
    int external_validate = 0;
    int verbose = 1;

    struct option longopts[] = {
        {"debug", required_argument, NULL, 'd'},
        {"header", no_argument, NULL, 'h'},
        {"sections", no_argument, NULL, 'S'},
        {"hex", required_argument, NULL, 'x'},
        {"symbols", no_argument, NULL, 's'},
        {"relocs", no_argument, NULL, 'r'},
        {"merge", no_argument, NULL, 'm'},
        {"output", required_argument, NULL, 'o'},
        {"external", no_argument, NULL, 'e'},
        {"quiet", no_argument, NULL, 'q'},
        {"help", no_argument, NULL, '?'},
        {NULL, 0, NULL, 0}};

    while ((opt = getopt_long(argc, argv, "d:hSx:srmo:eq?", longopts, NULL)) != -1)
    {
        switch (opt)
        {
        case 'd':
            add_debug_to(optarg);
            break;

        case 'h':
            do_header = 1;
            break;
        case 'S':
            do_sections = 1;
            break;
        case 'x':
            hex_arg = optarg;
            break;
        case 's':
            do_symbols = 1;
            break;
        case 'r':
            do_relocs = 1;
            break;

        case 'm':
            do_merge = 1;
            break;
        case 'o':
            out_path = optarg;
            break;
        case 'e':
            external_validate = 1;
            break;
        case 'q':
            verbose = 0;
            break;

        case '?':
            usage(argv[0]);
            return 0;

        default:
            fprintf(stderr, "Unrecognized option: -%c\n", opt);
            usage(argv[0]);
            return 1;
        }
    }

    if (do_merge)
    {

        if (optind + 1 >= argc)
        {
            fprintf(stderr, "Error: --merge requires two input files: A.o B.o\n");
            usage(argv[0]);
            return 1;
        }

        const char *A = argv[optind];
        const char *B = argv[optind + 1];

        int rc = elf32_merge_two_objects(A, B, out_path, verbose, external_validate);
        return (rc == 0) ? 0 : 1;
    }

    if (optind >= argc)
    {
        fprintf(stderr, "Error: missing elf_file\n");
        usage(argv[0]);
        return 1;
    }

    const char *path = argv[optind];

    if (!do_header && !do_sections && !do_symbols && !do_relocs && !hex_arg)
    {
        do_header = 1;
        do_sections = 1;
    }

    FILE *f = fopen(path, "rb");
    if (!f)
    {
        perror("fopen");
        return 1;
    }

    int rc = 0;

    if (do_header)
    {
        if (elf32_print_header(f) != 0)
            rc = 1;
        printf("\n");
    }

    if (do_sections)
    {
        if (elf32_print_sections(f) != 0)
            rc = 1;
        printf("\n");
    }

    if (hex_arg)
    {
        if (elf32_dump_section_hex(f, hex_arg) != 0)
            rc = 1;
        printf("\n");
    }

    if (do_symbols)
    {
        if (elf32_print_symbols(f) != 0)
            rc = 1;
        printf("\n");
    }

    if (do_relocs)
    {
        if (elf32_print_relocations(f) != 0)
            rc = 1;
        printf("\n");
    }

    fclose(f);
    return rc;
}
