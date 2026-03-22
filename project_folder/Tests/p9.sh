#!/bin/bash

set -e

. "$(dirname "$0")/test_common.sh"

TMP_DIR=$(setup_test_environment 9)
navigate_to_project_root
build_examples
check_executable "etape9_merge_relocations"

echo "Tests Partie 9 (etape9_merge_relocations - Génération du fichier objet ELF)"
echo ""

if ! command -v arm-none-eabi-gcc >/dev/null 2>&1; then
    echo -e "${RED}Erreur : arm-none-eabi-gcc non installé${NC}"
    exit 1
fi

if ! command -v arm-none-eabi-ld >/dev/null 2>&1; then
    echo -e "${RED}Erreur : arm-none-eabi-ld non installé${NC}"
    exit 1
fi

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
    echo -e "${RED}Erreur : Fichiers de test manquants${NC}"
    exit 1
fi

ARM_DIR="$TMP_DIR/arm_build"
GCC_OUTPUT="$TMP_DIR/gcc_arm_merged.o"
OUR_OUTPUT="$TMP_DIR/our_merged_result.o"

mkdir -p "$ARM_DIR"

if [ -f "Examples_fusion/file1.c" ]; then
    arm-none-eabi-gcc -c "Examples_fusion/file1.c" -o "$ARM_DIR/file1_arm.o" 2>/dev/null || true
    if [ -f "$ARM_DIR/file1_arm.o" ]; then
        TEST_FILE_A="$ARM_DIR/file1_arm.o"
    fi
fi

if [ -f "Examples_fusion/file2.c" ]; then
    arm-none-eabi-gcc -c "Examples_fusion/file2.c" -o "$ARM_DIR/file2_arm.o" 2>/dev/null || true
    if [ -f "$ARM_DIR/file2_arm.o" ]; then
        TEST_FILE_B="$ARM_DIR/file2_arm.o"
    fi
fi

if [ ! -f "$TEST_FILE_A" ] || [ ! -f "$TEST_FILE_B" ]; then
    echo -e "${YELLOW}⚠ Fichiers ARM non disponibles, utilisation des fichiers existants${NC}"
fi

arm-none-eabi-ld -r "$TEST_FILE_A" "$TEST_FILE_B" -o "$GCC_OUTPUT" 2>/dev/null || {
    echo -e "${YELLOW}⚠ Échec de la fusion ARM de référence, test de base uniquement${NC}"
    GCC_OUTPUT=""
}

set +e
./project_folder/etape9_merge_relocations "$TEST_FILE_A" "$TEST_FILE_B" "$OUR_OUTPUT" > "$TMP_DIR/merge.out" 2>&1
EXIT_CODE=$?
set -e

if [ $EXIT_CODE -eq 0 ] && [ -f "$OUR_OUTPUT" ]; then
    print_result 0 "Génération du fichier objet ELF"
else
    print_result 1 "Génération du fichier objet ELF"
fi

if [ -f "$OUR_OUTPUT" ]; then
    if command -v readelf >/dev/null 2>&1; then
        OUR_MACHINE=$(readelf -h "$OUR_OUTPUT" 2>/dev/null | grep "Machine:" | awk '{print $2}')
        OUR_TYPE=$(readelf -h "$OUR_OUTPUT" 2>/dev/null | grep "Type:" | awk '{print $2}')
        OUR_CLASS=$(readelf -h "$OUR_OUTPUT" 2>/dev/null | grep "Class:" | awk '{print $2}')
        
        if [ -n "$OUR_MACHINE" ] && [ -n "$OUR_TYPE" ] && [ -n "$OUR_CLASS" ]; then
            if [ "$OUR_TYPE" = "REL" ] && [ "$OUR_CLASS" = "ELF32" ]; then
                print_result 0 "Validité du header ELF (Type/Class)"
            else
                print_result 1 "Validité du header ELF (Type/Class)"
            fi
        else
            print_result 1 "Validité du header ELF (Type/Class)"
        fi
        
        if [ -n "$GCC_OUTPUT" ] && [ -f "$GCC_OUTPUT" ]; then
            echo ""
            echo "=== Comparaison avec GCC ARM ==="
            echo ""
            
            echo "Tailles:"
            GCC_SIZE=$(stat -c "%s" "$GCC_OUTPUT" 2>/dev/null || echo "0")
            OUR_SIZE=$(stat -c "%s" "$OUR_OUTPUT" 2>/dev/null || echo "0")
            echo "  GCC ARM : $GCC_SIZE octets"
            echo "  P9    : $OUR_SIZE octets"
            echo ""
            
            echo "ELF Header (Machine / Type):"
            echo "GCC ARM:"
            readelf -h "$GCC_OUTPUT" 2>/dev/null | grep -E "(Class|Data|Type|Machine)" || true
            echo ""
            echo "P9:"
            readelf -h "$OUR_OUTPUT" 2>/dev/null | grep -E "(Class|Data|Type|Machine)" || true
            echo ""
            
            echo "Sections:"
            echo "GCC ARM:"
            readelf -S "$GCC_OUTPUT" 2>/dev/null | awk '/^\s*\[/ && !/\[Nr\]/ {for(i=2; i<=NF; i++) {if($i ~ /^\./) {print $i; break}}}' || true
            echo ""
            echo "P9:"
            readelf -S "$OUR_OUTPUT" 2>/dev/null | awk '/^\s*\[/ && !/\[Nr\]/ {for(i=2; i<=NF; i++) {if($i ~ /^\./) {print $i; break}}}' || true
            echo ""
            
            echo "Symboles globaux:"
            echo "GCC ARM:"
            readelf -s "$GCC_OUTPUT" 2>/dev/null | grep GLOBAL || true
            echo ""
            echo "P9:"
            readelf -s "$OUR_OUTPUT" 2>/dev/null | grep GLOBAL || true
            echo ""
            
            GCC_SECTIONS=$(readelf -S "$GCC_OUTPUT" 2>/dev/null | grep -c "^\s*\[" || echo "0")
            OUR_SECTIONS=$(readelf -S "$OUR_OUTPUT" 2>/dev/null | grep -c "^\s*\[" || echo "0")
            
            if [ "$OUR_SECTIONS" -gt 0 ]; then
                print_result 0 "Présence de sections"
            else
                print_result 1 "Présence de sections"
            fi
        else
            if readelf -S "$OUR_OUTPUT" 2>/dev/null | grep -q "^\s*\["; then
                print_result 0 "Présence de sections"
            else
                print_result 1 "Présence de sections"
            fi
        fi
        
        OUR_SYMBOLS=$(readelf -s "$OUR_OUTPUT" 2>/dev/null | grep -c "GLOBAL\|LOCAL" || echo "0")
        if [ "$OUR_SYMBOLS" -gt 0 ]; then
            print_result 0 "Présence de symboles"
        else
            print_result 1 "Présence de symboles"
        fi
    else
        if [ -f "$OUR_OUTPUT" ]; then
            OUR_SIZE=$(stat -c "%s" "$OUR_OUTPUT" 2>/dev/null || echo "0")
            if [ "$OUR_SIZE" -gt 0 ]; then
                print_result 0 "Fichier généré non vide"
            else
                print_result 1 "Fichier généré non vide"
            fi
        fi
    fi
fi

test_invalid_file "etape9_merge_relocations" "$TMP_DIR"

set +e
./project_folder/etape9_merge_relocations > "$TMP_DIR/noargs.out" 2>&1
EXIT_CODE=$?
set -e
if [ $EXIT_CODE -ne 0 ] && grep -qi "Usage\|usage" "$TMP_DIR/noargs.out"; then
    print_result 0 "Erreur arguments manquants"
else
    print_result 1 "Erreur arguments manquants"
fi

set +e
./project_folder/etape9_merge_relocations "$TEST_FILE_A" > "$TMP_DIR/one_arg.out" 2>&1
EXIT_CODE=$?
set -e
if [ $EXIT_CODE -ne 0 ] && grep -qi "Usage\|usage" "$TMP_DIR/one_arg.out"; then
    print_result 0 "Erreur argument unique"
else
    print_result 1 "Erreur argument unique"
fi

exit 0
