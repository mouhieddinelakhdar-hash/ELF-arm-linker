#!/bin/bash

# Common test utilities for ELF linker tests

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_result() {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}✔${NC} $2"
    else
        echo -e "${RED}✘${NC} $2"
    fi
}

setup_test_environment() {
    local test_num=$1
    local TMP_DIR="/tmp/elf_test_p${test_num}_$$"
    mkdir -p "$TMP_DIR"
    echo "$TMP_DIR"
}

navigate_to_project_root() {
    # Navigate to project root (two levels up from project_folder/Tests/)
    local script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    cd "$script_dir/../.." || exit 1
}

build_examples() {
    cd Examples_fusion 2>/dev/null || {
        echo -e "${RED}Erreur : Examples_fusion introuvable${NC}"
        exit 1
    }
    
    make clean >/dev/null 2>&1 || true
    make >/dev/null 2>&1 || true
    cd ..
}

check_executable() {
    local exec_name=$1
    if [ ! -f "./project_folder/$exec_name" ]; then
        echo -e "${RED}Erreur : $exec_name non trouvé${NC}"
        exit 1
    fi
}

find_elf_test_file() {
    if [ -f "Examples_fusion/file1.o" ]; then
        echo "Examples_fusion/file1.o"
    elif [ -f "Examples_fusion/file2.o" ]; then
        echo "Examples_fusion/file2.o"
    else
        echo -e "${RED}Erreur : Aucun fichier ELF32 trouvé${NC}"
        exit 1
    fi
}

test_invalid_file() {
    local executable=$1
    local tmp_dir=$2
    local test_name="${executable}_invalid"
    
    set +e
    ./project_folder/"$executable" /dev/null > "$tmp_dir/${test_name}.out" 2>&1
    local EXIT_CODE=$?
    set -e
    
    if [ $EXIT_CODE -ne 0 ] && grep -qi "Error\|error\|invalid\|ELF\|Erreur" "$tmp_dir/${test_name}.out"; then
        print_result 0 "Erreur fichier invalide"
    else
        print_result 1 "Erreur fichier invalide"
    fi
}

test_missing_args() {
    local executable=$1
    local tmp_dir=$2
    local test_name="${executable}_noargs"
    
    set +e
    ./project_folder/"$executable" > "$tmp_dir/${test_name}.out" 2>&1
    local EXIT_CODE=$?
    set -e
    
    if [ $EXIT_CODE -ne 0 ] && grep -qi "Usage\|usage" "$tmp_dir/${test_name}.out"; then
        print_result 0 "Erreur arguments manquants"
    else
        print_result 1 "Erreur arguments manquants"
    fi
}

