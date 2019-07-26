



#ifndef BASE_FORMAT_MACROS_H_
#define BASE_FORMAT_MACROS_H_

















#include "build/build_config.h"

#if defined(OS_POSIX)

#if (defined(_INTTYPES_H) || defined(_INTTYPES_H_)) && !defined(PRId64)
#error "inttypes.h has already been included before this header file, but "
#error "without __STDC_FORMAT_MACROS defined."
#endif

#if !defined(__STDC_FORMAT_MACROS)
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>



#define WidePRId64 PRId64
#define WidePRIu64 PRIu64
#define WidePRIx64 PRIx64

#if !defined(PRIuS)
#define PRIuS "zu"
#endif

#else  

#if !defined(PRId64)
#define PRId64 "I64d"
#endif

#if !defined(PRIu64)
#define PRIu64 "I64u"
#endif

#if !defined(PRIx64)
#define PRIx64 "I64x"
#endif

#define WidePRId64 L"I64d"
#define WidePRIu64 L"I64u"
#define WidePRIx64 L"I64x"

#if !defined(PRIuS)
#define PRIuS "Iu"
#endif

#endif

#endif  
