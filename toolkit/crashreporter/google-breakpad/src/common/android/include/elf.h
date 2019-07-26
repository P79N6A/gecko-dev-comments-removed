




























#ifndef GOOGLE_BREAKPAD_COMMON_ANDROID_INCLUDE_ELF_H
#define GOOGLE_BREAKPAD_COMMON_ANDROID_INCLUDE_ELF_H

#include <stdint.h>
#include <libgen.h>

#ifdef __cplusplus
extern "C" {
#endif









#define Elf32_Nhdr   __bsd_Elf32_Nhdr
#define Elf64_Nhdr   __bsd_Elf64_Nhdr


#define Elf32_auxv_t  __bionic_Elf32_auxv_t
#define Elf64_auxv_t  __bionic_Elf64_auxv_t

#define Elf32_Dyn     __bionic_Elf32_Dyn
#define Elf64_Dyn     __bionic_Elf64_Dyn

#include_next <elf.h>

#undef Elf32_Nhdr
#undef Elf64_Nhdr

typedef struct {
  Elf32_Word n_namesz;
  Elf32_Word n_descsz;
  Elf32_Word n_type;
} Elf32_Nhdr;

typedef struct {
  Elf64_Word n_namesz;
  Elf64_Word n_descsz;
  Elf64_Word n_type;
} Elf64_Nhdr;

#undef Elf32_auxv_t
#undef Elf64_auxv_t

typedef struct {
    uint32_t a_type;
    union {
      uint32_t a_val;
    } a_un;
} Elf32_auxv_t;

typedef struct {
    uint64_t a_type;
    union {
      uint64_t a_val;
    } a_un;
} Elf64_auxv_t;

#undef Elf32_Dyn
#undef Elf64_Dyn

typedef struct {
  Elf32_Sword   d_tag;
  union {
    Elf32_Word  d_val;
    Elf32_Addr  d_ptr;
  } d_un;
} Elf32_Dyn;

typedef struct {
  Elf64_Sxword   d_tag;
  union {
    Elf64_Xword  d_val;
    Elf64_Addr   d_ptr;
  } d_un;
} Elf64_Dyn;




#ifndef __WORDSIZE
#define __WORDSIZE 32
#endif


#ifndef EM_X86_64
#define EM_X86_64  62
#endif

#ifndef EM_PPC64
#define EM_PPC64   21
#endif

#ifndef EM_S390
#define EM_S390    22
#endif

#if !defined(AT_SYSINFO_EHDR)
#define AT_SYSINFO_EHDR 33
#endif

#if !defined(NT_PRSTATUS)
#define NT_PRSTATUS 1
#endif

#if !defined(NT_PRPSINFO)
#define NT_PRPSINFO 3
#endif

#if !defined(NT_AUXV)
#define NT_AUXV   6
#endif

#if !defined(NT_PRXFPREG)
#define NT_PRXFPREG 0x46e62b7f
#endif

#if !defined(NT_FPREGSET)
#define NT_FPREGSET 2
#endif

#ifdef __cplusplus
}  
#endif  

#endif
