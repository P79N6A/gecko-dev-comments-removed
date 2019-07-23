












































#ifndef __VMPI_h__
#define __VMPI_h__

#if defined(HAVE_CONFIG_H) && defined(NANOJIT_CENTRAL)
#include "config.h"
#endif

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>

#if defined(AVMPLUS_UNIX) || defined(AVMPLUS_OS2)
#include <unistd.h>
#include <sys/mman.h>
#endif

#ifdef AVMPLUS_WIN32
#if ! defined(_STDINT_H)
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed __int64 int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned __int64 uint64_t;
#endif
#else
#include <stdint.h>
#include <inttypes.h>
#endif

#define VMPI_strlen strlen
#define VMPI_strcat strcat
#define VMPI_strcmp strcmp
#define VMPI_strncat strncat
#define VMPI_strcpy strcpy
#define VMPI_sprintf sprintf
#define VMPI_vfprintf vfprintf
#define VMPI_memset memset
#define VMPI_isdigit isdigit
#define VMPI_getDate()

extern void VMPI_setPageProtection(void *address,
                                   size_t size,
                                   bool executableFlag,
                                   bool writeableFlag);

#endif
