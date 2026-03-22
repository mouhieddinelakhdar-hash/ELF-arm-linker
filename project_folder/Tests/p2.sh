#!/bin/bash

set -e

. "$(dirname "$0")/test_common.sh"

TMP_DIR=$(setup_test_environment 2)
navigate_to_project_root
build_examples
check_executable "etape2_elf_display_sec"

echo "Tests Partie 2 (etape2_elf_display_sec)"

ELF32_TEST_FILE=$(find_elf_test_file)

./project_folder/etape2_elf_display_sec "$ELF32_TEST_FILE" > "$TMP_DIR/section_headers.out" 2>&1
if [ $? -eq 0 ] && [ -s "$TMP_DIR/section_headers.out" ] && \
   grep -q "Section Headers:" "$TMP_DIR/section_headers.out" && \
   grep -q "\[Nr\]" "$TMP_DIR/section_headers.out" && \
   grep -q "Name" "$TMP_DIR/section_headers.out" && \
   grep -q "Type" "$TMP_DIR/section_headers.out" && \
   grep -q "^[[:space:]]*\[[[:space:]]*[0-9]\+" "$TMP_DIR/section_headers.out"; then
    print_result 0 "Affichage table des en-têtes de section"
else
    print_result 1 "Affichage table des en-têtes de section"
fi

test_invalid_file "etape2_elf_display_sec" "$TMP_DIR"
test_missing_args "etape2_elf_display_sec" "$TMP_DIR"

if command -v readelf >/dev/null 2>&1 && [ -s "$TMP_DIR/section_headers.out" ]; then
    readelf -S "$ELF32_TEST_FILE" > "$TMP_DIR/readelf_sections.out" 2>&1
    ./project_folder/etape2_elf_display_sec "$ELF32_TEST_FILE" > "$TMP_DIR/our_sections.out" 2>&1
    readelf_count=$(grep -c "^[[:space:]]*\[[[:space:]]*[0-9]\+" "$TMP_DIR/readelf_sections.out" 2>/dev/null || echo "0")
    our_count=$(grep -c "^[[:space:]]*\[[[:space:]]*[0-9]\+" "$TMP_DIR/our_sections.out" 2>/dev/null || echo "0")
    if [ "$readelf_count" -gt 0 ] && [ "$our_count" -gt 0 ] && [ "$readelf_count" -eq "$our_count" ]; then
        print_result 0 "Comparaison readelf (nombre de sections)"
    else
        print_result 1 "Comparaison readelf (nombre de sections)"
    fi
    if grep -q "PROGBITS\|STRTAB\|SYMTAB" "$TMP_DIR/our_sections.out"; then
        print_result 0 "Comparaison readelf (types de sections)"
    else
        print_result 1 "Comparaison readelf (types de sections)"
    fi
fi

exit 0
