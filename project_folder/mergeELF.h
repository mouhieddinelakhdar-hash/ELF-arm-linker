#ifndef MERGEELF_H
#define MERGEELF_H


int elf32_validate_relocatable_file(const char *filename, int verbose);


void elf32_run_external_validation(const char *filename);


int elf32_merge_two_objects(const char *pathA, const char *pathB,
                            const char *output_path,
                            int verbose,
                            int external_validate);

#endif
