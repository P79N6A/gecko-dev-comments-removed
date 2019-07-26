






























#ifndef _MSC_VER 
#error "Use this header only with Microsoft Visual C++ compilers!"
#endif 

#ifndef _MSC_INTTYPES_H_ 
#define _MSC_INTTYPES_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include <stdint.h>



typedef struct {
   intmax_t quot;
   intmax_t rem;
} imaxdiv_t;



#if !defined(__cplusplus) || defined(__STDC_FORMAT_MACROS) 


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

#define PRId32       "I32d"
#define PRIi32       "I32i"
#define PRIdLEAST32  "I32d"
#define PRIiLEAST32  "I32i"
#define PRIdFAST32   "I32d"
#define PRIiFAST32   "I32i"

#define PRId64       "I64d"
#define PRIi64       "I64i"
#define PRIdLEAST64  "I64d"
#define PRIiLEAST64  "I64i"
#define PRIdFAST64   "I64d"
#define PRIiFAST64   "I64i"

#define PRIdMAX     "I64d"
#define PRIiMAX     "I64i"

#define PRIdPTR     "Id"
#define PRIiPTR     "Ii"


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

#define PRIo32       "I32o"
#define PRIu32       "I32u"
#define PRIx32       "I32x"
#define PRIX32       "I32X"
#define PRIoLEAST32  "I32o"
#define PRIuLEAST32  "I32u"
#define PRIxLEAST32  "I32x"
#define PRIXLEAST32  "I32X"
#define PRIoFAST32   "I32o"
#define PRIuFAST32   "I32u"
#define PRIxFAST32   "I32x"
#define PRIXFAST32   "I32X"

#define PRIo64       "I64o"
#define PRIu64       "I64u"
#define PRIx64       "I64x"
#define PRIX64       "I64X"
#define PRIoLEAST64  "I64o"
#define PRIuLEAST64  "I64u"
#define PRIxLEAST64  "I64x"
#define PRIXLEAST64  "I64X"
#define PRIoFAST64   "I64o"
#define PRIuFAST64   "I64u"
#define PRIxFAST64   "I64x"
#define PRIXFAST64   "I64X"

#define PRIoMAX     "I64o"
#define PRIuMAX     "I64u"
#define PRIxMAX     "I64x"
#define PRIXMAX     "I64X"

#define PRIoPTR     "Io"
#define PRIuPTR     "Iu"
#define PRIxPTR     "Ix"
#define PRIXPTR     "IX"




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
