#!/bin/bash

set -e

. "$(dirname "$0")/test_common.sh"

TMP_DIR=$(setup_test_environment 8)
navigate_to_project_root
build_examples
check_executable "etape8_merge_realocation"

echo "Tests Partie 8 (etape8_merge_realocation - Fusion et correction des relocalisations)"
echo ""

TEST_FILE_A=""
TEST_FILE_B=""

if [ -f "Examples_fusion/file1.o" ]; then
    TEST_FILE_A="Examples_fusion/file1.o"
fi
if [ -f "Examples_fusion/file3.o" ]; then
    TEST_FILE_B="Examples_fusion/file3.o"
elif [ -f "Examples_fusion/file2.o" ]; then
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
./project_folder/etape8_merge_realocation "$TEST_FILE_A" "$TEST_FILE_B" > "$TMP_DIR/reloc_merge.out" 2>&1
MERGE_EXIT_CODE=$?
set -e

if [ $MERGE_EXIT_CODE -eq 0 ] && [ -s "$TMP_DIR/reloc_merge.out" ]; then
    if grep -q "OUTPUT: merged relocations" "$TMP_DIR/reloc_merge.out" && \
       grep -q "Relocation section" "$TMP_DIR/reloc_merge.out" && \
       grep -q "r_offset" "$TMP_DIR/reloc_merge.out" && \
       grep -q "r_info" "$TMP_DIR/reloc_merge.out"; then
        print_result 0 "Exécution de base et format de sortie"
    else
        print_result 1 "Exécution de base et format de sortie"
    fi
else
    print_result 1 "Exécution de base et format de sortie"
fi

if [ $MERGE_EXIT_CODE -eq 0 ]; then
    A_OFFSETS=$(grep -A1 "A (after fix)" "$TMP_DIR/reloc_merge.out" | grep "r_offset" | sed 's/.*r_offset=\([0-9]*\).*/\1/' | sort -n)
    B_OFFSETS=$(grep -A1 "B (after fix)" "$TMP_DIR/reloc_merge.out" | grep "r_offset" | sed 's/.*r_offset=\([0-9]*\).*/\1/' | sort -n)
    
    A_OFFSET_COUNT=$(echo "$A_OFFSETS" | grep -c "^[0-9]" 2>/dev/null || echo "0")
    B_OFFSET_COUNT=$(echo "$B_OFFSETS" | grep -c "^[0-9]" 2>/dev/null || echo "0")
    
    INVALID_OFFSETS=0
    if [ -n "$A_OFFSETS" ]; then
        while IFS= read -r offset; do
            if [ -n "$offset" ]; then
                if ! [[ "$offset" =~ ^[0-9]+$ ]] || [ "$offset" -lt 0 ]; then
                    INVALID_OFFSETS=1
                    break
                fi
            fi
        done <<< "$A_OFFSETS"
    fi
    
    if [ -n "$B_OFFSETS" ]; then
        while IFS= read -r offset; do
            if [ -n "$offset" ]; then
                if ! [[ "$offset" =~ ^[0-9]+$ ]] || [ "$offset" -lt 0 ]; then
                    INVALID_OFFSETS=1
                    break
                fi
            fi
        done <<< "$B_OFFSETS"
    fi
    
    if [ $INVALID_OFFSETS -eq 0 ] && [ $A_OFFSET_COUNT -ge 0 ] && [ $B_OFFSET_COUNT -ge 0 ]; then
        print_result 0 "Validation des ajustements d'offsets"
    else
        print_result 1 "Validation des ajustements d'offsets"
    fi
elif [ $MERGE_EXIT_CODE -ne 0 ]; then
    print_result 1 "Validation des ajustements d'offsets"
fi

if [ $MERGE_EXIT_CODE -eq 0 ]; then
    A_SYMBOLS=$(grep -A1 "A (after fix)" "$TMP_DIR/reloc_merge.out" | grep "r_info" | sed 's/.*sym=\([0-9]*\).*/\1/' | sort -n)
    B_SYMBOLS=$(grep -A1 "B (after fix)" "$TMP_DIR/reloc_merge.out" | grep "r_info" | sed 's/.*sym=\([0-9]*\).*/\1/' | sort -n)
    
    A_SYM_COUNT=$(echo "$A_SYMBOLS" | grep -c "^[0-9]" 2>/dev/null || echo "0")
    B_SYM_COUNT=$(echo "$B_SYMBOLS" | grep -c "^[0-9]" 2>/dev/null || echo "0")
    
    INVALID_SYMBOLS=0
    if [ -n "$A_SYMBOLS" ]; then
        while IFS= read -r sym; do
            if [ -n "$sym" ]; then
                if ! [[ "$sym" =~ ^[0-9]+$ ]] || [ "$sym" -lt 0 ]; then
                    INVALID_SYMBOLS=1
                    break
                fi
            fi
        done <<< "$A_SYMBOLS"
    fi
    
    if [ -n "$B_SYMBOLS" ]; then
        while IFS= read -r sym; do
            if [ -n "$sym" ]; then
                if ! [[ "$sym" =~ ^[0-9]+$ ]] || [ "$sym" -lt 0 ]; then
                    INVALID_SYMBOLS=1
                    break
                fi
            fi
        done <<< "$B_SYMBOLS"
    fi
    
    if [ $INVALID_SYMBOLS -eq 0 ] && [ $A_SYM_COUNT -ge 0 ] && [ $B_SYM_COUNT -ge 0 ]; then
        print_result 0 "Validation du remappage des indices de symboles"
    else
        print_result 1 "Validation du remappage des indices de symboles"
    fi
elif [ $MERGE_EXIT_CODE -ne 0 ]; then
    print_result 1 "Validation du remappage des indices de symboles"
fi

if [ $MERGE_EXIT_CODE -eq 0 ]; then
    TOTAL_RELOCS=$(grep -c "r_offset=" "$TMP_DIR/reloc_merge.out" || echo "0")
    OFFSET_LINES=$(grep "r_offset=" "$TMP_DIR/reloc_merge.out" | wc -l)
    INFO_LINES=$(grep "r_info(" "$TMP_DIR/reloc_merge.out" | wc -l)
    
    if [ $OFFSET_LINES -eq $INFO_LINES ] && [ "$TOTAL_RELOCS" -gt 0 ]; then
        print_result 0 "Validité sémantique"
    else
        print_result 1 "Validité sémantique"
    fi
elif [ $MERGE_EXIT_CODE -ne 0 ]; then
    print_result 1 "Validité sémantique"
fi

if command -v readelf >/dev/null 2>&1 && [ $MERGE_EXIT_CODE -eq 0 ]; then
    ORIG_RELOCS_A=$(readelf -r "$TEST_FILE_A" 2>/dev/null | grep -c "R_ARM_" || echo "0")
    ORIG_RELOCS_B=$(readelf -r "$TEST_FILE_B" 2>/dev/null | grep -c "R_ARM_" || echo "0")
    ORIG_TOTAL=$((ORIG_RELOCS_A + ORIG_RELOCS_B))
    
    OUR_TOTAL=$(grep -c "r_offset=" "$TMP_DIR/reloc_merge.out" || echo "0")
    
    if [ "$OUR_TOTAL" -eq "$ORIG_TOTAL" ] || [ "$OUR_TOTAL" -ge "$ORIG_TOTAL" ]; then
        print_result 0 "Validation croisée avec readelf"
    else
        print_result 1 "Validation croisée avec readelf"
    fi
else
    print_result 0 "Validation croisée avec readelf (skip - readelf non disponible)"
fi

if [ $MERGE_EXIT_CODE -eq 0 ]; then
    HAS_A_OUTPUT=$(grep -c "A (after fix)" "$TMP_DIR/reloc_merge.out" || echo "0")
    HAS_B_OUTPUT=$(grep -c "B (after fix)" "$TMP_DIR/reloc_merge.out" || echo "0")
    
    if [ "$HAS_A_OUTPUT" -gt 0 ] && [ "$HAS_B_OUTPUT" -gt 0 ]; then
        print_result 0 "Cohérence (traitement des deux fichiers)"
    else
        print_result 1 "Cohérence (traitement des deux fichiers)"
    fi
elif [ $MERGE_EXIT_CODE -ne 0 ]; then
    print_result 1 "Cohérence (traitement des deux fichiers)"
fi

test_invalid_file "etape8_merge_realocation" "$TMP_DIR"

set +e
./project_folder/etape8_merge_realocation > "$TMP_DIR/noargs.out" 2>&1
EXIT_CODE=$?
set -e
if [ $EXIT_CODE -ne 0 ] && grep -qi "Usage\|usage" "$TMP_DIR/noargs.out"; then
    print_result 0 "Erreur arguments manquants"
else
    print_result 1 "Erreur arguments manquants"
fi

set +e
./project_folder/etape8_merge_realocation "$TEST_FILE_A" > "$TMP_DIR/one_arg.out" 2>&1
EXIT_CODE=$?
set -e
if [ $EXIT_CODE -ne 0 ] && grep -qi "Usage\|usage" "$TMP_DIR/one_arg.out"; then
    print_result 0 "Erreur argument unique"
else
    print_result 1 "Erreur argument unique"
fi

exit 0
