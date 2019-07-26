























#ifndef _PTYPES_H
#define _PTYPES_H









#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif


#include <stddef.h>








#include "unicode/platform.h"








#if U_HAVE_STDINT_H






#include <stdint.h>

#if U_PLATFORM == U_PF_OS390

#include <features.h>

#if !defined(__uint8_t)
#define __uint8_t 1
typedef unsigned char uint8_t;
#endif
#endif 

#elif U_HAVE_INTTYPES_H

#   include <inttypes.h>

#else 

#if ! U_HAVE_INT8_T
typedef signed char int8_t;
#endif

#if ! U_HAVE_UINT8_T
typedef unsigned char uint8_t;
#endif

#if ! U_HAVE_INT16_T
typedef signed short int16_t;
#endif

#if ! U_HAVE_UINT16_T
typedef unsigned short uint16_t;
#endif

#if ! U_HAVE_INT32_T
typedef signed int int32_t;
#endif

#if ! U_HAVE_UINT32_T
typedef unsigned int uint32_t;
#endif

#if ! U_HAVE_INT64_T
#ifdef _MSC_VER
    typedef signed __int64 int64_t;
#else
    typedef signed long long int64_t;
#endif
#endif

#if ! U_HAVE_UINT64_T
#ifdef _MSC_VER
    typedef unsigned __int64 uint64_t;
#else
    typedef unsigned long long uint64_t;
#endif
#endif

#endif 

#endif 
