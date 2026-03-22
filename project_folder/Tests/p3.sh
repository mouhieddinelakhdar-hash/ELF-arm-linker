#!/bin/bash

set -e

. "$(dirname "$0")/test_common.sh"

TMP_DIR=$(setup_test_environment 3)
navigate_to_project_root
build_examples
check_executable "etape3_display_section"

echo "Tests Partie 3 (etape3_display_section)"

ELF32_TEST_FILE=$(find_elf_test_file)

SECTION_NAME=$(readelf -S "$ELF32_TEST_FILE" 2>/dev/null | grep -E "^[[:space:]]*\[.*\][[:space:]]+\.[a-z]" | head -1 | awk '{print $2}' | tr -d ']')

if [ -n "$SECTION_NAME" ]; then
    ./project_folder/etape3_display_section "$ELF32_TEST_FILE" "$SECTION_NAME" > "$TMP_DIR/section_text.out" 2>&1
    if [ $? -eq 0 ] && grep -q "Hex dump:" "$TMP_DIR/section_text.out" && grep -q "Section:" "$TMP_DIR/section_text.out"; then
        print_result 0 "Section par nom"
    else
        print_result 1 "Section par nom"
    fi
fi

./project_folder/etape3_display_section "$ELF32_TEST_FILE" 1 > "$TMP_DIR/section_index.out" 2>&1
if [ $? -eq 0 ] && grep -q "Section:" "$TMP_DIR/section_index.out"; then
    print_result 0 "Section par index"
else
    print_result 1 "Section par index"
fi

set +e
./project_folder/etape3_display_section "$ELF32_TEST_FILE" .data > "$TMP_DIR/section_data.out" 2>&1
EXIT_CODE=$?
set -e
if [ $EXIT_CODE -eq 0 ] || grep -qi "not found\|trouvée" "$TMP_DIR/section_data.out"; then
    print_result 0 "Section .data"
else
    print_result 1 "Section .data"
fi

set +e
./project_folder/etape3_display_section "$ELF32_TEST_FILE" .nonexistent > "$TMP_DIR/section_nonexistent.out" 2>&1
EXIT_CODE=$?
set -e
if [ $EXIT_CODE -ne 0 ] && grep -qi "not found\|Error\|Erreur\|trouvée" "$TMP_DIR/section_nonexistent.out"; then
    print_result 0 "Erreur section inexistante"
else
    print_result 1 "Erreur section inexistante"
fi

set +e
./project_folder/etape3_display_section /dev/null .text > "$TMP_DIR/section_invalid.out" 2>&1
EXIT_CODE=$?
set -e
if [ $EXIT_CODE -ne 0 ]; then
    print_result 0 "Erreur fichier invalide"
else
    print_result 1 "Erreur fichier invalide"
fi
test_missing_args "etape3_display_section" "$TMP_DIR"

if command -v readelf >/dev/null 2>&1 && [ -n "$SECTION_NAME" ]; then
    readelf -x "$SECTION_NAME" "$ELF32_TEST_FILE" > "$TMP_DIR/readelf_section.out" 2>&1
    ./project_folder/display_section "$ELF32_TEST_FILE" "$SECTION_NAME" > "$TMP_DIR/our_section.out" 2>&1
    readelf_lines=$(grep "^[[:space:]]*0x" "$TMP_DIR/readelf_section.out")
    our_lines=$(grep "^[[:space:]]*0x" "$TMP_DIR/our_section.out")
    if [ "$readelf_lines" = "$our_lines" ]; then
        print_result 0 "Comparaison readelf"
    else
        print_result 1 "Comparaison readelf"
    fi
fi

exit 0
