














#ifndef _CDTFRRGSTST
#define _CDTFRRGSTST

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "cintltst.h"




void TestTwoDigitYearDSTParse(void);





void TestPartialParse994(void);




void TestRunTogetherPattern985(void);





void TestCzechMonths459(void);




void TestQuotePattern161(void);


void tryPat994(UDateFormat* format, const char* pat, const char* s, UDate expected);




void TestBooleanAttributes(void);

#endif 

#endif
