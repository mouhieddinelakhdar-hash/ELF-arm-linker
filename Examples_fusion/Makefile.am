ACLOCAL_AMFLAGS = -I ../m4

# no code transformations to work with thumb
AM_CFLAGS= @VD_CROSS_NOPIE_CFLAGS@ -mbig-endian -mno-thumb-interwork
AM_CCASFLAGS=-mbig-endian -mno-thumb-interwork

AM_LDFLAGS=

if HAVE_NO_STDLIB
# sans newlib, on a besoin de "puts" pour générer le programme, ainsi que d'un point d'entrée
fusion_result_SOURCES=mini-lib.c
AM_LDFLAGS += -nostdlib --entry main
endif

# Big endian
AM_LDFLAGS+=-Wl,-EB

if HAVE_ARM_COMPILER

noinst_PROGRAMS=fusion_result exemple_fused.o

# il faudra linker avec fusion_result.o
fusion_result_LDADD=fusion_result.o

# mais il faut aussi construire explicitement fusion_result.o
# (cf la règle suivante)
nodist_EXTRA_fusion_result_SOURCES=fusion_result.o

file3.o: file3.c
	$(AM_V_CC)arm-none-eabi-gcc -c $(AM_CFLAGS) $(srcdir)/file3.c -o file3.o

BUILT_SOURCES = file1.o file3.o

# Appel du linker avec -nostdlib et -r car on ne fait qu'un link partiel
fusion_result.o: file1.o file2.o
	$(AM_V_CCLD)$(LINK) -nostdlib -Wl,-r $^

# Second exemple qui ne fait que la fusion partielle
exemple_fused_o_SOURCES=exemple1.s exemple2.s
exemple_fused_o_LINK=$(AM_V_CCLD)$(LINK) -nostdlib -Wl,-r

else # HAVE_ARM_COMPILER
# Compilation avec gcc (utilise as en arrière-plan) en mode 32 bits pour les tests
file1.o: file1.c
	$(AM_V_CC)gcc -m32 -c $(srcdir)/file1.c -o file1.o

file2.o: file2.c
	-$(AM_V_CC)gcc -m32 -c $(srcdir)/file2.c -o file2.o 2>/dev/null || true

file3.o: file3.c
	-$(AM_V_CC)gcc -m32 -c $(srcdir)/file3.c -o file3.o 2>/dev/null || true

BUILT_SOURCES = file1.o
endif # HAVE_ARM_COMPILER

clean-local:
	rm -f .deps/*.Po
	rm -f file1.o file2.o file3.o

EXTRA_DIST=file1.c file2.c file3.c
