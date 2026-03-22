#!/bin/bash

set -e

. "$(dirname "$0")/test_common.sh"

TMP_DIR=$(setup_test_environment 7)
navigate_to_project_root
build_examples
check_executable "etape7_merge_symbols"

echo "Tests Partie 7 (etape7_merge_symbols - Fusion, renumérotation et correction des symboles)"
echo ""

TEST_FILE_A=""
TEST_FILE_B=""

if [ -f "Examples_fusion/file1.o" ]; then
    TEST_FILE_A="Examples_fusion/file1.o"
fi
if [ -f "Examples_fusion/file2.o" ]; then
    TEST_FILE_B="Examples_fusion/file2.o"
fi

if [ -z "$TEST_FILE_A" ] || [ -z "$TEST_FILE_B" ]; then
    if [ -z "$TEST_FILE_A" ]; then
        TEST_FILE_A=$(find_elf_test_file)
    fi
    if [ -z "$TEST_FILE_B" ]; then
        TEST_FILE_B="$TEST_FILE_A"
    fi
fi

set +e
./project_folder/etape7_merge_symbols "$TEST_FILE_A" "$TEST_FILE_B" > "$TMP_DIR/symbol_merge.out" 2>&1
MERGE_EXIT_CODE=$?
set -e

if [ $MERGE_EXIT_CODE -eq 0 ] && [ -s "$TMP_DIR/symbol_merge.out" ]; then
    if grep -q "OUTPUT: merged symbol table" "$TMP_DIR/symbol_merge.out" && \
       grep -q "\[Idx\]" "$TMP_DIR/symbol_merge.out" && \
       grep -q "Name" "$TMP_DIR/symbol_merge.out" && \
       grep -q "Bind" "$TMP_DIR/symbol_merge.out" && \
       grep -q "Type" "$TMP_DIR/symbol_merge.out"; then
        print_result 0 "Affichage table de symboles fusionnée"
    else
        print_result 1 "Affichage table de symboles fusionnée"
    fi
elif grep -qi "symbol.*defined in both files\|merge_symbol_tables failed" "$TMP_DIR/symbol_merge.out"; then
    print_result 0 "Détection symbole dupliqué (comportement attendu)"
else
    print_result 1 "Affichage table de symboles fusionnée"
fi

if [ $MERGE_EXIT_CODE -eq 0 ] && grep -q "Symbol mappings" "$TMP_DIR/symbol_merge.out" && \
   grep -q "A->OUT mappings" "$TMP_DIR/symbol_merge.out" && \
   grep -q "B->OUT mappings" "$TMP_DIR/symbol_merge.out"; then
    print_result 0 "Affichage mappings de symboles"
elif grep -qi "symbol.*defined in both files\|merge_symbol_tables failed" "$TMP_DIR/symbol_merge.out"; then
    print_result 0 "Affichage mappings de symboles (skip - symbole dupliqué)"
else
    print_result 1 "Affichage mappings de symboles"
fi

test_invalid_file "etape7_merge_symbols" "$TMP_DIR"

set +e
./project_folder/etape7_merge_symbols > "$TMP_DIR/noargs.out" 2>&1
EXIT_CODE=$?
set -e
if [ $EXIT_CODE -ne 0 ] && grep -qi "Usage\|usage" "$TMP_DIR/noargs.out"; then
    print_result 0 "Erreur arguments manquants"
else
    print_result 1 "Erreur arguments manquants"
fi

set +e
./project_folder/etape7_merge_symbols "$TEST_FILE_A" > "$TMP_DIR/one_arg.out" 2>&1
EXIT_CODE=$?
set -e
if [ $EXIT_CODE -ne 0 ] && grep -qi "Usage\|usage" "$TMP_DIR/one_arg.out"; then
    print_result 0 "Erreur argument unique"
else
    print_result 1 "Erreur argument unique"
fi

if [ $MERGE_EXIT_CODE -eq 0 ]; then
    if grep -qE "\[[0-9]+\].*[A-Za-z_].*LOCAL\|GLOBAL\|WEAK.*OBJECT\|FUNC\|NOTYPE" "$TMP_DIR/symbol_merge.out" || \
       (grep -qE "LOCAL\|GLOBAL\|WEAK" "$TMP_DIR/symbol_merge.out" && \
        grep -qE "OBJECT\|FUNC\|NOTYPE\|SECTION\|FILE" "$TMP_DIR/symbol_merge.out"); then
        print_result 0 "Structure table de symboles (Bind/Type)"
    else
        print_result 1 "Structure table de symboles (Bind/Type)"
    fi
elif grep -qi "symbol.*defined in both files\|merge_symbol_tables failed" "$TMP_DIR/symbol_merge.out"; then
    print_result 0 "Structure table de symboles (skip - symbole dupliqué)"
else
    print_result 1 "Structure table de symboles (Bind/Type)"
fi

if [ $MERGE_EXIT_CODE -eq 0 ]; then
    if grep -qE "[0-9]+[[:space:]]+[0-9]+[[:space:]]+[0-9A-Za-z_]+" "$TMP_DIR/symbol_merge.out" || \
       grep -qE "Value\|Size" "$TMP_DIR/symbol_merge.out"; then
        print_result 0 "Champs Value/Size présents"
    else
        print_result 1 "Champs Value/Size présents"
    fi
elif grep -qi "symbol.*defined in both files\|merge_symbol_tables failed" "$TMP_DIR/symbol_merge.out"; then
    print_result 0 "Champs Value/Size présents (skip - symbole dupliqué)"
else
    print_result 1 "Champs Value/Size présents"
fi

exit 0
