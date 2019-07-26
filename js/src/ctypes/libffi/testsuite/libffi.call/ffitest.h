#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <ffi.h>
#include "fficonfig.h"

#if defined HAVE_STDINT_H
#include <stdint.h>
#endif

#if defined HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#define MAX_ARGS 256

#define CHECK(x) (void)(!(x) ? (abort(), 1) : 0)


#undef __UNUSED__
#if defined(__GNUC__)
#define __UNUSED__ __attribute__((__unused__))
#define __STDCALL__ __attribute__((stdcall))
#define __THISCALL__ __attribute__((thiscall))
#define __FASTCALL__ __attribute__((fastcall))
#else
#define __UNUSED__
#define __STDCALL__ __stdcall
#define __THISCALL__ __thiscall
#define __FASTCALL__ __fastcall
#endif

#ifndef ABI_NUM
#define ABI_NUM FFI_DEFAULT_ABI
#define ABI_ATTR
#endif



#ifdef HAVE_MMAP_ANON
# undef HAVE_MMAP_DEV_ZERO

# include <sys/mman.h>
# ifndef MAP_FAILED
#  define MAP_FAILED -1
# endif
# if !defined (MAP_ANONYMOUS) && defined (MAP_ANON)
#  define MAP_ANONYMOUS MAP_ANON
# endif
# define USING_MMAP

#endif

#ifdef HAVE_MMAP_DEV_ZERO

# include <sys/mman.h>
# ifndef MAP_FAILED
#  define MAP_FAILED -1
# endif
# define USING_MMAP

#endif


#ifdef _WIN64
#define PRIdLL "I64d"
#define PRIuLL "I64u"
#else
#define PRIdLL "lld"
#define PRIuLL "llu"
#endif


#if defined(__alpha__) && defined(__osf__)

#undef PRIdLL
#define PRIdLL "ld"
#undef PRIuLL
#define PRIuLL "lu"
#define PRId8 "hd"
#define PRIu8 "hu"
#define PRId64 "ld"
#define PRIu64 "lu"
#define PRIuPTR "lu"
#endif


#if defined(__hppa__) && defined(__hpux__) && !defined(PRIuPTR)
#define PRIuPTR "lu"
#endif


#if defined(__sgi)


#define PRId8 "hhd"
#define PRIu8 "hhu"
#if (_MIPS_SZLONG == 32)
#define PRId64 "lld"
#define PRIu64 "llu"
#endif



#if (_MIPS_SZLONG == 64)
#define PRId64 "ld"
#define PRIu64 "lu"
#endif


#define PRIuPTR "lu"
#endif


#if defined(__sun__) && defined(__svr4__) && !defined(PRIuPTR)
#if defined(__arch64__) || defined (__x86_64__)
#define PRIuPTR "lu"
#else
#define PRIuPTR "u"
#endif
#endif


#if defined _MSC_VER
#define PRIuPTR "lu"
#define PRIu8 "u"
#define PRId8 "d"
#define PRIu64 "I64u"
#define PRId64 "I64d"
#endif

#ifndef PRIuPTR
#define PRIuPTR "u"
#endif
