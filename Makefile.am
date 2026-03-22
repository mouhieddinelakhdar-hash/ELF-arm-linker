ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = subdir-objects

AM_CFLAGS=-Wall -Werror -g -I$(srcdir)/project_folder
AM_CFLAGS+=-D DEBUG
AM_CFLAGS+=-D WARNING

SUBDIRS=.

bin_PROGRAMS=
bin_PROGRAMS+=Options_and_debug_example
bin_PROGRAMS+=etape3_display_section
bin_PROGRAMS+=etape4_display_symtab
bin_PROGRAMS+=etape5_display_reloc
bin_PROGRAMS+=etape1_ehdr
bin_PROGRAMS+=etape2_elf_display_sec
bin_PROGRAMS+=etape6_usage_example
bin_PROGRAMS+=etape7_merge_symbols
bin_PROGRAMS+=etape8_merge_realocation
bin_PROGRAMS+=etape9_merge_relocations
bin_PROGRAMS+=main

COMMON=project_folder/debug.h project_folder/debug.c project_folder/util.h project_folder/util.c
ELF_OPS=project_folder/readELF.c project_folder/readELF.h project_folder/printELF.c project_folder/printELF.h project_folder/ELFcntx.c project_folder/ELFcntx.h project_folder/ELF.h

MERGE_OPS=project_folder/mergeELF.c project_folder/mergeELF.h

ETAPE6_OPS=project_folder/elf_object.c project_folder/elf_object.h \
           project_folder/section_merge.c project_folder/section_merge.h \
           project_folder/section_merge_types.c project_folder/section_merge_types.h \
           project_folder/section_index.c project_folder/section_index.h

ETAPE7_OPS=$(ETAPE6_OPS) \
           project_folder/symbol_merge.c project_folder/symbol_merge.h \
           project_folder/symbol_merge_types.h

ETAPE8_OPS=$(ETAPE7_OPS) \
           project_folder/realocation_merge.c project_folder/realocation_merge.h

ETAPE9_OPS=$(ETAPE8_OPS) \
           project_folder/writeELF.c project_folder/writeELF.h

EXTRA_DIST=filtre.pl .gitignore test_p3.sh test_p4.sh
EXTRA_DIST+=project_folder/ELF.h
EXTRA_DIST+=Examples_fusion

EXTRA_DIST+=project_folder/elf_object.c project_folder/elf_object.h
EXTRA_DIST+=project_folder/section_merge.c project_folder/section_merge.h
EXTRA_DIST+=project_folder/section_merge_types.c project_folder/section_merge_types.h
EXTRA_DIST+=project_folder/section_index.c project_folder/section_index.h
EXTRA_DIST+=project_folder/etape6_usage_example.c
EXTRA_DIST+=project_folder/symbol_merge.c project_folder/symbol_merge.h
EXTRA_DIST+=project_folder/symbol_merge_types.c project_folder/symbol_merge_types.h
EXTRA_DIST+=project_folder/etape7_merge_symbols.c
EXTRA_DIST+=project_folder/realocation_merge.c project_folder/realocation_merge.h
EXTRA_DIST+=project_folder/etape8_merge_realocation.c
EXTRA_DIST+=project_folder/writeELF.c project_folder/writeELF.h
EXTRA_DIST+=project_folder/etape9_merge_result.c
EXTRA_DIST+=project_folder/mergeELF.c project_folder/mergeELF.h
EXTRA_DIST+=project_folder/main.c

Options_and_debug_example_SOURCES=$(COMMON) project_folder/Options_and_debug_example.c

etape3_display_section_SOURCES=$(COMMON) $(ELF_OPS) project_folder/etape3_display_section.c
etape4_display_symtab_SOURCES=$(COMMON) $(ELF_OPS) project_folder/etape4_display_symtab.c
etape5_display_reloc_SOURCES=$(COMMON) $(ELF_OPS) project_folder/etape5_display_reloc.c
etape1_ehdr_SOURCES=$(COMMON) $(ELF_OPS) project_folder/etape1.c
etape2_elf_display_sec_SOURCES=$(COMMON) $(ELF_OPS) project_folder/etape2_elf_display_sec.c

etape6_usage_example_SOURCES=$(COMMON) $(ELF_OPS) $(ETAPE6_OPS) project_folder/etape6_usage_example.c

etape7_merge_symbols_SOURCES=$(COMMON) $(ELF_OPS) $(ETAPE7_OPS) project_folder/etape7_merge_symbols.c

etape8_merge_realocation_SOURCES=$(COMMON) $(ELF_OPS) $(ETAPE8_OPS) project_folder/etape8_merge_realocation.c

etape9_merge_relocations_SOURCES=$(COMMON) $(ELF_OPS) $(MERGE_OPS) $(ETAPE9_OPS) project_folder/etape9_merge_result.c

main_SOURCES=$(COMMON) $(ELF_OPS) $(MERGE_OPS) $(ETAPE9_OPS) project_folder/main.c

all-local:
	@mkdir -p project_folder
	@for prog in Options_and_debug_example etape3_display_section etape4_display_symtab etape5_display_reloc etape1_ehdr etape2_elf_display_sec etape6_usage_example etape7_merge_symbols etape8_merge_realocation etape9_merge_relocations main; do \
		if [ -f "$$prog" ]; then \
			cp -f "$$prog" "project_folder/$$prog" || true; \
			rm -f "$$prog" || true; \
		fi; \
	done

clean-local:
	@rm -f Options_and_debug_example etape3_display_section etape4_display_symtab etape5_display_reloc etape1_ehdr etape2_elf_display_sec etape6_usage_example etape7_merge_symbols etape8_merge_realocation etape9_merge_relocations main
	@rm -f project_folder/Options_and_debug_example project_folder/etape3_display_section project_folder/etape4_display_symtab project_folder/etape5_display_reloc project_folder/etape1_ehdr project_folder/etape2_elf_display_sec project_folder/etape6_usage_example project_folder/etape7_merge_symbols project_folder/etape8_merge_realocation project_folder/etape9_merge_relocations project_folder/main
