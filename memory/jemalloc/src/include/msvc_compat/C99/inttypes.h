






























#ifndef _MSC_VER 
#error "Use this header only with Microsoft Visual C++ compilers!"
#endif 

#ifndef _MSC_INTTYPES_H_ 
#define _MSC_INTTYPES_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "stdint.h"



typedef struct {
   intmax_t quot;
   intmax_t rem;
} imaxdiv_t;



#if !defined(__cplusplus) || defined(__STDC_FORMAT_MACROS) 

#ifdef _WIN64
#  define __PRI64_PREFIX        "l"
#  define __PRIPTR_PREFIX       "l"
#else
#  define __PRI64_PREFIX        "ll"
#  define __PRIPTR_PREFIX
#endif


#define PRId8       "d"
#define PRIi8       "i"
#define PRIdLEAST8  "d"
#define PRIiLEAST8  "i"
#define PRIdFAST8   "d"
#define PRIiFAST8   "i"

#define PRId16       "hd"
#define PRIi16       "hi"
#define PRIdLEAST16  "hd"
#define PRIiLEAST16  "hi"
#define PRIdFAST16   "hd"
#define PRIiFAST16   "hi"

#define PRId32       "d"
#define PRIi32       "i"
#define PRIdLEAST32  "d"
#define PRIiLEAST32  "i"
#define PRIdFAST32   "d"
#define PRIiFAST32   "i"

#define PRId64       __PRI64_PREFIX "d"
#define PRIi64       __PRI64_PREFIX "i"
#define PRIdLEAST64  __PRI64_PREFIX "d"
#define PRIiLEAST64  __PRI64_PREFIX "i"
#define PRIdFAST64   __PRI64_PREFIX "d"
#define PRIiFAST64   __PRI64_PREFIX "i"

#define PRIdMAX     __PRI64_PREFIX "d"
#define PRIiMAX     __PRI64_PREFIX "i"

#define PRIdPTR     __PRIPTR_PREFIX "d"
#define PRIiPTR     __PRIPTR_PREFIX "i"


#define PRIo8       "o"
#define PRIu8       "u"
#define PRIx8       "x"
#define PRIX8       "X"
#define PRIoLEAST8  "o"
#define PRIuLEAST8  "u"
#define PRIxLEAST8  "x"
#define PRIXLEAST8  "X"
#define PRIoFAST8   "o"
#define PRIuFAST8   "u"
#define PRIxFAST8   "x"
#define PRIXFAST8   "X"

#define PRIo16       "ho"
#define PRIu16       "hu"
#define PRIx16       "hx"
#define PRIX16       "hX"
#define PRIoLEAST16  "ho"
#define PRIuLEAST16  "hu"
#define PRIxLEAST16  "hx"
#define PRIXLEAST16  "hX"
#define PRIoFAST16   "ho"
#define PRIuFAST16   "hu"
#define PRIxFAST16   "hx"
#define PRIXFAST16   "hX"

#define PRIo32       "o"
#define PRIu32       "u"
#define PRIx32       "x"
#define PRIX32       "X"
#define PRIoLEAST32  "o"
#define PRIuLEAST32  "u"
#define PRIxLEAST32  "x"
#define PRIXLEAST32  "X"
#define PRIoFAST32   "o"
#define PRIuFAST32   "u"
#define PRIxFAST32   "x"
#define PRIXFAST32   "X"

#define PRIo64       __PRI64_PREFIX "o"
#define PRIu64       __PRI64_PREFIX "u"
#define PRIx64       __PRI64_PREFIX "x"
#define PRIX64       __PRI64_PREFIX "X"
#define PRIoLEAST64  __PRI64_PREFIX "o"
#define PRIuLEAST64  __PRI64_PREFIX "u"
#define PRIxLEAST64  __PRI64_PREFIX "x"
#define PRIXLEAST64  __PRI64_PREFIX "X"
#define PRIoFAST64   __PRI64_PREFIX "o"
#define PRIuFAST64   __PRI64_PREFIX "u"
#define PRIxFAST64   __PRI64_PREFIX "x"
#define PRIXFAST64   __PRI64_PREFIX "X"

#define PRIoMAX     __PRI64_PREFIX "o"
#define PRIuMAX     __PRI64_PREFIX "u"
#define PRIxMAX     __PRI64_PREFIX "x"
#define PRIXMAX     __PRI64_PREFIX "X"

#define PRIoPTR     __PRIPTR_PREFIX "o"
#define PRIuPTR     __PRIPTR_PREFIX "u"
#define PRIxPTR     __PRIPTR_PREFIX "x"
#define PRIXPTR     __PRIPTR_PREFIX "X"


#define SCNd8       "d"
#define SCNi8       "i"
#define SCNdLEAST8  "d"
#define SCNiLEAST8  "i"
#define SCNdFAST8   "d"
#define SCNiFAST8   "i"

#define SCNd16       "hd"
#define SCNi16       "hi"
#define SCNdLEAST16  "hd"
#define SCNiLEAST16  "hi"
#define SCNdFAST16   "hd"
#define SCNiFAST16   "hi"

#define SCNd32       "ld"
#define SCNi32       "li"
#define SCNdLEAST32  "ld"
#define SCNiLEAST32  "li"
#define SCNdFAST32   "ld"
#define SCNiFAST32   "li"

#define SCNd64       "I64d"
#define SCNi64       "I64i"
#define SCNdLEAST64  "I64d"
#define SCNiLEAST64  "I64i"
#define SCNdFAST64   "I64d"
#define SCNiFAST64   "I64i"

#define SCNdMAX     "I64d"
#define SCNiMAX     "I64i"

#ifdef _WIN64 
#  define SCNdPTR     "I64d"
#  define SCNiPTR     "I64i"
#else  
#  define SCNdPTR     "ld"
#  define SCNiPTR     "li"
#endif  


#define SCNo8       "o"
#define SCNu8       "u"
#define SCNx8       "x"
#define SCNX8       "X"
#define SCNoLEAST8  "o"
#define SCNuLEAST8  "u"
#define SCNxLEAST8  "x"
#define SCNXLEAST8  "X"
#define SCNoFAST8   "o"
#define SCNuFAST8   "u"
#define SCNxFAST8   "x"
#define SCNXFAST8   "X"

#define SCNo16       "ho"
#define SCNu16       "hu"
#define SCNx16       "hx"
#define SCNX16       "hX"
#define SCNoLEAST16  "ho"
#define SCNuLEAST16  "hu"
#define SCNxLEAST16  "hx"
#define SCNXLEAST16  "hX"
#define SCNoFAST16   "ho"
#define SCNuFAST16   "hu"
#define SCNxFAST16   "hx"
#define SCNXFAST16   "hX"

#define SCNo32       "lo"
#define SCNu32       "lu"
#define SCNx32       "lx"
#define SCNX32       "lX"
#define SCNoLEAST32  "lo"
#define SCNuLEAST32  "lu"
#define SCNxLEAST32  "lx"
#define SCNXLEAST32  "lX"
#define SCNoFAST32   "lo"
#define SCNuFAST32   "lu"
#define SCNxFAST32   "lx"
#define SCNXFAST32   "lX"

#define SCNo64       "I64o"
#define SCNu64       "I64u"
#define SCNx64       "I64x"
#define SCNX64       "I64X"
#define SCNoLEAST64  "I64o"
#define SCNuLEAST64  "I64u"
#define SCNxLEAST64  "I64x"
#define SCNXLEAST64  "I64X"
#define SCNoFAST64   "I64o"
#define SCNuFAST64   "I64u"
#define SCNxFAST64   "I64x"
#define SCNXFAST64   "I64X"

#define SCNoMAX     "I64o"
#define SCNuMAX     "I64u"
#define SCNxMAX     "I64x"
#define SCNXMAX     "I64X"

#ifdef _WIN64 
#  define SCNoPTR     "I64o"
#  define SCNuPTR     "I64u"
#  define SCNxPTR     "I64x"
#  define SCNXPTR     "I64X"
#else  
#  define SCNoPTR     "lo"
#  define SCNuPTR     "lu"
#  define SCNxPTR     "lx"
#  define SCNXPTR     "lX"
#endif  

#endif 




#define imaxabs _abs64





#ifdef STATIC_IMAXDIV 
static
#else 
_inline
#endif 
imaxdiv_t __cdecl imaxdiv(intmax_t numer, intmax_t denom)
{
   imaxdiv_t result;

   result.quot = numer / denom;
   result.rem = numer % denom;

   if (numer < 0 && result.rem > 0) {
      
      ++result.quot;
      result.rem -= denom;
   }

   return result;
}


#define strtoimax _strtoi64
#define strtoumax _strtoui64


#define wcstoimax _wcstoi64
#define wcstoumax _wcstoui64


#endif 
