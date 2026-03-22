
#ifndef ELF32_H
#define ELF32_H

#include <stdint.h>
#include <stdio.h>

#if defined(__cplusplus)
#define ELF_STATIC_ASSERT(cond, msg) static_assert(cond, #msg)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define ELF_STATIC_ASSERT(cond, msg) _Static_assert(cond, #msg)
#else
#define ELF_STATIC_ASSERT(cond, msg) typedef char static_assert_##msg##__LINE__[(cond) ? 1 : -1]
#endif

#define EI_NIDENT 16

#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

typedef enum {
    EI_MAG0 = 0,
    EI_MAG1 = 1,
    EI_MAG2 = 2,
    EI_MAG3 = 3,
    EI_CLASS = 4,
    EI_DATA = 5,
    EI_VERSION = 6,
    EI_PAD = 7
} Elf_Ident_Index;

typedef enum { ELFCLASSNONE = 0, ELFCLASS32 = 1, ELFCLASS64 = 2 } Elf_Class;

typedef enum { ELFDATANONE = 0, ELFDATA2LSB = 1, ELFDATA2MSB = 2 } Elf_DataEncoding;

typedef uint32_t Elf32_Addr;
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef int32_t Elf32_Sword;
typedef uint32_t Elf32_Word;

typedef enum {
    ET_NONE = 0,
    ET_REL = 1,
    ET_EXEC = 2,
    ET_DYN = 3,
    ET_CORE = 4,
    ET_LOPROC = 0xff00,
    ET_HIPROC = 0xffff
} Elf32_Type;

typedef enum {
    EM_NONE = 0,
    EM_M32 = 1,
    EM_SPARC = 2,
    EM_386 = 3,
    EM_68K = 4,
    EM_88K = 5,
    EM_860 = 7,
    EM_MIPS = 8,
    EM_MIPS_RS4_BE = 10,
    EM_RESERVED = 11,
    EM_ARM = 40

} Elf32_Machine;

typedef enum { EV_NONE = 0, EV_CURRENT = 1 } Elf32_Version;

typedef struct {
    unsigned char e_ident[EI_NIDENT];

    Elf32_Half e_type;
    Elf32_Half e_machine;
    Elf32_Word e_version;

    Elf32_Addr e_entry;
    Elf32_Off e_phoff;
    Elf32_Off e_shoff;

    Elf32_Word e_flags;

    Elf32_Half e_ehsize;
    Elf32_Half e_phentsize;
    Elf32_Half e_phnum;
    Elf32_Half e_shentsize;
    Elf32_Half e_shnum;
    Elf32_Half e_shstrndx;
} Elf32_Ehdr;

typedef struct {
    Elf32_Word sh_name;
    Elf32_Word sh_type;
    Elf32_Word sh_flags;
    Elf32_Addr sh_addr;
    Elf32_Off sh_offset;
    Elf32_Word sh_size;
    Elf32_Word sh_link;
    Elf32_Word sh_info;
    Elf32_Word sh_addralign;
    Elf32_Word sh_entsize;
} Elf32_Shdr;

typedef enum {
    SHT_NULL = 0,
    SHT_PROGBITS = 1,
    SHT_SYMTAB = 2,
    SHT_STRTAB = 3,
    SHT_RELA = 4,
    SHT_HASH = 5,
    SHT_DYNAMIC = 6,
    SHT_NOTE = 7,
    SHT_NOBITS = 8,
    SHT_REL = 9,
    SHT_SHLIB = 10,
    SHT_DYNSYM = 11,
    SHT_LOPROC = 0x70000000u,
    SHT_HIPROC = 0x7fffffffu,
    SHT_LOUSER = 0x80000000u,
    SHT_HIUSER = 0xffffffffu
} Elf32_SectionType;

#define SHF_WRITE 0x1u
#define SHF_ALLOC 0x2u
#define SHF_EXECINSTR 0x4u
#define SHF_MASKPROC 0xf0000000u

typedef enum {
    SHN_UNDEF = 0x0000,
    SHN_LORESERVE = 0xff00,
    SHN_LOPROC = 0xff00,
    SHN_HIPROC = 0xff1f,
    SHN_ABS = 0xfff1,
    SHN_COMMON = 0xfff2,
    SHN_HIRESERVE = 0xffff
} Elf32_SpecialSectionIndex;

typedef struct {
    Elf32_Word st_name;     
    Elf32_Addr st_value;    
    Elf32_Word st_size;     
    unsigned char st_info;  
    unsigned char st_other; 
    Elf32_Half st_shndx;    
} Elf32_Sym;

#define ELF32_ST_BIND(i) ((unsigned char)((i) >> 4))
#define ELF32_ST_TYPE(i) ((unsigned char)((i) & 0x0f))
#define ELF32_ST_INFO(b, t) ((unsigned char)(((b) << 4) + ((t) & 0x0f)))

typedef enum {
    STB_LOCAL = 0,
    STB_GLOBAL = 1,
    STB_WEAK = 2,
    STB_LOPROC = 13,
    STB_HIPROC = 15
} Elf32_SymBind;

typedef enum {
    STT_NOTYPE = 0,
    STT_OBJECT = 1,
    STT_FUNC = 2,
    STT_SECTION = 3,
    STT_FILE = 4,
    STT_LOPROC = 13,
    STT_HIPROC = 15
} Elf32_SymType;

#define STN_UNDEF 0

typedef struct {
    Elf32_Addr r_offset; 
    Elf32_Word r_info;   
} Elf32_Rel;

typedef struct {
    Elf32_Addr r_offset;
    Elf32_Word r_info;
    Elf32_Sword r_addend; 
} Elf32_Rela;

#define ELF32_R_SYM(i) ((Elf32_Word)((i) >> 8))
#define ELF32_R_TYPE(i) ((unsigned char)((i) & 0xff))
#define ELF32_R_INFO(s, t) ((Elf32_Word)(((s) << 8) + (unsigned char)(t)))

#define SHT_ARM_ATTRIBUTES 0x70000003  
#define R_ARM_NONE 0
#define R_ARM_PC24 1
#define R_ARM_ABS32 2
#define R_ARM_REL32 3
#define R_ARM_PC13 4
#define R_ARM_ABS16 5
#define R_ARM_ABS12 6
#define R_ARM_THM_ABS5 7
#define R_ARM_ABS8 8
#define R_ARM_SBREL32 9
#define R_ARM_THM_PC22 10
#define R_ARM_THM_PC8 11
#define R_ARM_AMP_VCALL9 12
#define R_ARM_SWI24 13
#define R_ARM_THM_SWI8 14
#define R_ARM_XPC25 15
#define R_ARM_THM_XPC22 16
#define R_ARM_PLT32 27
#define R_ARM_CALL 28
#define R_ARM_JUMP24 29
#define R_ARM_THM_JUMP24 30
#define R_ARM_GOT_BREL 96
#define R_ARM_GOT_PREL 97

#define ELF32_IS_ELF_PTR(ehdr_ptr)                                                                 \
    ((ehdr_ptr) && (ehdr_ptr)->e_ident[EI_MAG0] == ELFMAG0 &&                                      \
     (ehdr_ptr)->e_ident[EI_MAG1] == ELFMAG1 && (ehdr_ptr)->e_ident[EI_MAG2] == ELFMAG2 &&         \
     (ehdr_ptr)->e_ident[EI_MAG3] == ELFMAG3)

#define ELF32_IS_ELF(ehdr)                                                                         \
    ((ehdr).e_ident[EI_MAG0] == ELFMAG0 && (ehdr).e_ident[EI_MAG1] == ELFMAG1 &&                   \
     (ehdr).e_ident[EI_MAG2] == ELFMAG2 && (ehdr).e_ident[EI_MAG3] == ELFMAG3)

#define ELF32_IS_REAL_SHNDX(shndx) ((Elf32_Half)(shndx) < (Elf32_Half)SHN_LORESERVE)

ELF_STATIC_ASSERT(sizeof(Elf32_Ehdr) == 52, elf32_ehdr_size_must_be_52_bytes);
ELF_STATIC_ASSERT(sizeof(Elf32_Shdr) == 40, elf32_shdr_size_must_be_40_bytes);
ELF_STATIC_ASSERT(sizeof(Elf32_Sym) == 16, elf32_sym_size_must_be_16_bytes);
ELF_STATIC_ASSERT(sizeof(Elf32_Rel) == 8, elf32_rel_size_must_be_8_bytes);
ELF_STATIC_ASSERT(sizeof(Elf32_Rela) == 12, elf32_rela_size_must_be_12_bytes);

// WAHD MA YZID YDICLARI FONCTION LAHNAA !

#endif
