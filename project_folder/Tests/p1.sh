#!/bin/bash

set -e

. "$(dirname "$0")/test_common.sh"

TMP_DIR=$(setup_test_environment 1)
navigate_to_project_root
build_examples
check_executable "etape1_ehdr"

echo "Tests Partie 1 (etape1_ehdr)"

ELF32_TEST_FILE=$(find_elf_test_file)

./project_folder/etape1_ehdr "$ELF32_TEST_FILE" > "$TMP_DIR/ehdr.out" 2>&1
if [ $? -eq 0 ] && [ -s "$TMP_DIR/ehdr.out" ] && \
   grep -q "ELF Header:" "$TMP_DIR/ehdr.out" && \
   grep -q "Class:" "$TMP_DIR/ehdr.out" && \
   grep -q "Data:" "$TMP_DIR/ehdr.out" && \
   grep -q "Type:" "$TMP_DIR/ehdr.out" && \
   grep -q "Machine:" "$TMP_DIR/ehdr.out"; then
    print_result 0 "Affichage en-tête ELF"
else
    print_result 1 "Affichage en-tête ELF"
fi

test_invalid_file "etape1_ehdr" "$TMP_DIR"
test_missing_args "etape1_ehdr" "$TMP_DIR"

if command -v readelf >/dev/null 2>&1 && [ -s "$TMP_DIR/ehdr.out" ]; then
    readelf -h "$ELF32_TEST_FILE" > "$TMP_DIR/readelf_ehdr.out" 2>&1
    ./etape1_ehdr "$ELF32_TEST_FILE" > "$TMP_DIR/our_ehdr.out" 2>&1
    if grep -qi "ELF32" "$TMP_DIR/our_ehdr.out" && grep -qi "ELF32" "$TMP_DIR/readelf_ehdr.out"; then
        print_result 0 "Comparaison readelf (classe ELF32)"
    else
        print_result 1 "Comparaison readelf (classe ELF32)"
    fi
    if grep -qi "Little endian\|Big endian" "$TMP_DIR/our_ehdr.out" && grep -qi "little endian\|big endian" "$TMP_DIR/readelf_ehdr.out"; then
        print_result 0 "Comparaison readelf (endianness)"
    else
        print_result 1 "Comparaison readelf (endianness)"
    fi
fi

exit 0
