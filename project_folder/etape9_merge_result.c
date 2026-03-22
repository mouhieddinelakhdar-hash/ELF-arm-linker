#include <stdio.h>
#include <stdlib.h>

#include "mergeELF.h"

static void usage(const char *argv0)
{
    fprintf(stderr,
            "Usage: %s <A.o> <B.o> [output.o]\n"
            "\n"
            "Step 9: Merge two relocatable ELF32 objects into one relocatable.\n"
            "\n"
            "Args:\n"
            "  A.o        First input relocatable object\n"
            "  B.o        Second input relocatable object\n"
            "  output.o   Output filename (default: merged.o)\n",
            argv0);
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        usage(argv[0]);
        return 1;
    }

    const char *pathA = argv[1];
    const char *pathB = argv[2];
    const char *out = (argc > 3) ? argv[3] : "merged.o";

    int verbose = 1;
    int external_validate = 1;

    int rc = elf32_merge_two_objects(pathA, pathB, out, verbose, external_validate);
    return (rc == 0) ? 0 : 1;
}
