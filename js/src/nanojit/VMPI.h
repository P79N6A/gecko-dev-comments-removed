












































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



#ifdef _MSC_VER
	#pragma warning(disable:4201) // nonstandard extension used : nameless struct/union
	#pragma warning(disable:4512) // assignment operator could not be generated
	#pragma warning(disable:4511) // can't generate copy ctor
	#pragma warning(disable:4127) // conditional expression is constant - appears to be compiler noise primarily
    #pragma warning(disable:4611) // interaction between _setjmp and destruct
    #pragma warning(disable:4725) // instruction may be inaccurate on some Pentiums
    #pragma warning(disable:4611) // interaction between '_setjmp' and C++ object destruction is non-portable
    #pragma warning(disable:4251) // X needs to have dll-interface to be used by clients of class Y

    
    #pragma warning(default:4265)   // 'class' : class has virtual functions, but destructor is not virtual
    #pragma warning(default:4905)   // wide string literal cast to 'LPSTR'
    #pragma warning(default:4906)   // string literal cast to 'LPWSTR'
    #pragma warning(default:4263)   // 'function' : member function does not override any base class virtual member function
    #pragma warning(default:4264)   // 'virtual_function' : no override available for virtual member function from base 'class'; function is hidden
    #pragma warning(default:4266)   // 'function' : no override available for virtual member function from base 'type'; function is hidden
    #pragma warning(default:4242)   // 'identifier' : conversion from 'type1' to 'type2', possible loss of data
    #pragma warning(default:4263)   // member function does not override any base class virtual member function
    #pragma warning(default:4296)   // expression is always true (false) (Generally, an unsigned variable was used in a comparison operation with zero.)
#endif


#ifdef _MSC_VER
#pragma warning(disable:4996)       // 'scanf' was declared deprecated
#endif


#ifdef _MSC_VER
#pragma warning(disable:4291)       // presence of a 'new' operator in nanojit/Allocator.h without matching 'delete'
#endif

#endif
