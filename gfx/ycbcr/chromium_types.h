



































#ifndef GFX_CHROMIUMTYPES_H
#define GFX_CHROMIUMTYPES_H

#include "prtypes.h"

typedef PRUint8 uint8;
typedef PRInt8 int8;
typedef PRInt16 int16;






#if defined(_M_X64) || defined(__x86_64__)
#define ARCH_CPU_X86_FAMILY 1
#define ARCH_CPU_X86_64 1
#define ARCH_CPU_64_BITS 1
#elif defined(_M_IX86) || defined(__i386__) || defined(__i386)
#define ARCH_CPU_X86_FAMILY 1
#define ARCH_CPU_X86 1
#define ARCH_CPU_32_BITS 1
#elif defined(__ARMEL__)
#define ARCH_CPU_ARM_FAMILY 1
#define ARCH_CPU_ARMEL 1
#define ARCH_CPU_32_BITS 1
#elif defined(__ppc__)
#define ARCH_CPU_PPC_FAMILY 1
#define ARCH_CPU_PPC 1
#define ARCH_CPU_32_BITS 1
#elif defined(__sparc)
#define ARCH_CPU_SPARC_FAMILY 1
#define ARCH_CPU_SPARC 1
#define ARCH_CPU_32_BITS 1
#elif defined(__sparcv9)
#define ARCH_CPU_SPARC_FAMILY 1
#define ARCH_CPU_SPARC 1
#define ARCH_CPU_64_BITS 1
#else
#warning Please add support for your architecture in chromium_types.h
#endif

#endif 
