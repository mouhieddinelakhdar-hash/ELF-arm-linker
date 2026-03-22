#!/bin/bash

set -e

. "$(dirname "$0")/test_common.sh"

TMP_DIR=$(setup_test_environment 4)
navigate_to_project_root
build_examples
check_executable "etape4_display_symtab"

echo "Tests Partie 4 (etape4_display_symtab)"

ELF32_TEST_FILE=$(find_elf_test_file)

./project_folder/etape4_display_symtab "$ELF32_TEST_FILE" > "$TMP_DIR/symtab.out" 2>&1
if [ $? -eq 0 ] && [ -s "$TMP_DIR/symtab.out" ] && \
   grep -q "Num:" "$TMP_DIR/symtab.out" && grep -q "Value" "$TMP_DIR/symtab.out" && \
   grep -q "^[[:space:]]*[0-9]\+:" "$TMP_DIR/symtab.out"; then
    print_result 0 "Affichage table symboles"
else
    print_result 1 "Affichage table symboles"
fi

test_invalid_file "etape4_display_symtab" "$TMP_DIR"
test_missing_args "etape4_display_symtab" "$TMP_DIR"

if command -v readelf >/dev/null 2>&1 && [ -s "$TMP_DIR/symtab.out" ]; then
    readelf -s "$ELF32_TEST_FILE" > "$TMP_DIR/readelf_symtab.out" 2>&1
    ./project_folder/etape4_display_symtab "$ELF32_TEST_FILE" > "$TMP_DIR/our_symtab.out" 2>&1
    readelf_count=$(grep -c "^[[:space:]]*[0-9]\+:" "$TMP_DIR/readelf_symtab.out" 2>/dev/null || echo "0")
    our_count=$(grep -c "^[[:space:]]*[0-9]\+:" "$TMP_DIR/our_symtab.out" 2>/dev/null || echo "0")
    if [ "$readelf_count" -gt 0 ] && [ "$our_count" -gt 0 ] && grep -q "Value\|Type\|Bind" "$TMP_DIR/our_symtab.out"; then
        print_result 0 "Comparaison readelf"
    else
        print_result 1 "Comparaison readelf"
    fi
fi

exit 0
