#!/bin/bash

set -e

. "$(dirname "$0")/test_common.sh"

TMP_DIR=$(setup_test_environment 5)
navigate_to_project_root
build_examples
check_executable "etape5_display_reloc"

echo "Tests Partie 5 (etape5_display_reloc)"

ELF32_TEST_FILE=$(find_elf_test_file)

./project_folder/etape5_display_reloc "$ELF32_TEST_FILE" > "$TMP_DIR/reloc.out" 2>&1
if [ $? -eq 0 ] && [ -s "$TMP_DIR/reloc.out" ] && \
   grep -q "Relocation section" "$TMP_DIR/reloc.out" && \
   grep -q "Offset" "$TMP_DIR/reloc.out" && \
   grep -q "Type" "$TMP_DIR/reloc.out" && \
   grep -q "Sym. Name" "$TMP_DIR/reloc.out" && \
   grep -q "0x[0-9a-f]" "$TMP_DIR/reloc.out"; then
    print_result 0 "Affichage tables de réimplantation"
else
    print_result 1 "Affichage tables de réimplantation"
fi

test_invalid_file "etape5_display_reloc" "$TMP_DIR"
test_missing_args "etape5_display_reloc" "$TMP_DIR"

if command -v readelf >/dev/null 2>&1 && [ -s "$TMP_DIR/reloc.out" ]; then
    readelf -r "$ELF32_TEST_FILE" > "$TMP_DIR/readelf_reloc.out" 2>&1
    ./project_folder/etape5_display_reloc "$ELF32_TEST_FILE" > "$TMP_DIR/our_reloc.out" 2>&1
    if grep -q "Relocation section" "$TMP_DIR/our_reloc.out" && \
       grep -q "Offset" "$TMP_DIR/our_reloc.out" && \
       grep -q "Type" "$TMP_DIR/our_reloc.out"; then
        print_result 0 "Comparaison readelf (structure)"
    else
        print_result 1 "Comparaison readelf (structure)"
    fi
fi

exit 0

