































#ifndef _COMMON_DWARF_TYPES_H__
#define _COMMON_DWARF_TYPES_H__

#include <stdint.h>

typedef signed char         int8;
typedef short               int16;
typedef int                 int32;
typedef long long           int64;

typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long long uint64;

#ifdef __PTRDIFF_TYPE__
typedef          __PTRDIFF_TYPE__ intptr;
typedef unsigned __PTRDIFF_TYPE__ uintptr;
#else
#error "Can't find pointer-sized integral types."
#endif

#endif 
