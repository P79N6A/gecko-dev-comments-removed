






#ifndef _UCURR_IMP_H_
#define _UCURR_IMP_H_

#include "unicode/utypes.h"
#include "unicode/unistr.h"
#include "unicode/parsepos.h"











U_CFUNC void
uprv_getStaticCurrencyName(const UChar* iso, const char* loc,
                           icu::UnicodeString& result, UErrorCode& ec);





















U_CFUNC void
uprv_parseCurrency(const char* locale,
                   const icu::UnicodeString& text,
                   icu::ParsePosition& pos,
                   int8_t type,
                   UChar* result,
                   UErrorCode& ec);

#endif 


