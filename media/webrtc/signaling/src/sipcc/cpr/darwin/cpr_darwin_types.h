



#ifndef _CPR_DARWIN_TYPES_H_
#define _CPR_DARWIN_TYPES_H_

#include <sys/types.h>
#include <sys/param.h>
#include <stddef.h>
#include "inttypes.h"












typedef uint8_t boolean;















#ifndef MIN
#ifdef __GNUC__
#define MIN(a,b)  ({ typeof(a) _a = (a); typeof(b) _b = (b); _a < _b ? _a : _b; })
#else
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif
#endif

#ifndef MAX
#ifdef __GNUC__
#define MAX(a,b)  ({ typeof(a) _a = (a); typeof(b) _b = (b); _a > _b ? _a : _b; })
#else
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif
#endif






#ifndef NUL
#define NUL '\0'
#endif







#if defined(_POSIX_C_SOURCE) && defined(__GNUC__)
#define RESTRICT __restrict
#else
#define RESTRICT
#endif






#define CONST const






#ifdef __STRICT_ANSI__
#define INLINE
#else
#define INLINE __inline__
#endif






#ifndef __BEGIN_DECLS
#ifdef __cplusplus
#define __BEGIN_DECLS extern "C" {
#else
#define __BEGIN_DECLS
#endif
#endif

#ifndef __END_DECLS
#ifdef __cplusplus
#define __END_DECLS   }
#else
#define __END_DECLS
#endif
#endif





#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif






#define FIELDOFFSET(struct_name, field_name) (long)(&(((struct_name *)0)->field_name))


#endif
