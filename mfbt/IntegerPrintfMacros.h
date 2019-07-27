







#ifndef mozilla_IntegerPrintfMacros_h_
#define mozilla_IntegerPrintfMacros_h_






















#if defined(MOZ_CUSTOM_INTTYPES_H)
#  include MOZ_CUSTOM_INTTYPES_H
#elif defined(_MSC_VER)
#  include "mozilla/MSIntTypes.h"
#else
#  include <inttypes.h>
#endif







#if defined(ANDROID) && !defined(__LP64__)
#  undef  PRIdPTR      /* intptr_t  */
#  define PRIdPTR "d"  /* intptr_t  */
#  undef  PRIiPTR      /* intptr_t  */
#  define PRIiPTR "i"  /* intptr_t  */
#  undef  PRIoPTR      /* uintptr_t */
#  define PRIoPTR "o"  /* uintptr_t */
#  undef  PRIuPTR      /* uintptr_t */
#  define PRIuPTR "u"  /* uintptr_t */
#  undef  PRIxPTR      /* uintptr_t */
#  define PRIxPTR "x"  /* uintptr_t */
#  undef  PRIXPTR      /* uintptr_t */
#  define PRIXPTR "X"  /* uintptr_t */
#endif




#define PRIuSIZE PRIuPTR

#endif  
