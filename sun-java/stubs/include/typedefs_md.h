



































#ifndef sun_java_typedefs_md_h___
#define sun_java_typedefs_md_h___

#include "prtypes.h"
#include "nspr_md.h"


#ifdef HAVE_SYS_BITYPES_H
#include <sys/bitypes.h>
#endif

PR_BEGIN_EXTERN_C

#ifndef HAVE_INT16_T
typedef int16 int16_t;
#endif

#ifndef HAVE_INT32_T
typedef int32 int32_t;
#endif

#ifndef HAVE_UINT16_T
typedef uint16 uint16_t;
#ifndef	_UINT32_T
#define	_UINT32_T
typedef uint32 uint32_t;
#endif
#endif

typedef prword_t uintVP_t; 

#if defined(HAVE_INT64) && !defined(HAVE_INT64_T)
typedef int64 int64_t;
#endif

#if !defined(HAVE_INT64) && defined(HAVE_INT64_T)




#define int64_t int64
#endif

#ifndef HAVE_UINT_T
#ifndef XP_MAC
typedef unsigned int uint_t;
#else



#pragma warning_errors off
#ifndef __OPENTRANSPORT__
typedef unsigned long uint_t;	


#endif 
#pragma warning_errors reset
#endif
#endif

#if defined(XP_WIN)
typedef long int32_t;
#endif

PR_END_EXTERN_C

#endif 
