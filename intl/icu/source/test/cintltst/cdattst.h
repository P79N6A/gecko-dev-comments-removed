














#ifndef _CDATFRMTST
#define _CDATFRMTST

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "cintltst.h"

    


    static void TestDateFormat(void);
    static void TestRelativeDateFormat(void);

    


    static void TestSymbols(void);

    


    static void TestDateFormatCalendar(void);

    


    static void VerifygetSymbols(UDateFormat*, UDateFormatSymbolType, int32_t, const char*);
    static void VerifysetSymbols(UDateFormat*, UDateFormatSymbolType, int32_t, const char*);
    static void VerifygetsetSymbols(UDateFormat*, UDateFormat*, UDateFormatSymbolType, int32_t);
    
    


    static UChar* myNumformat(const UNumberFormat* numfor, double d);
    static int getCurrentYear(void);

    


     static void TestOverrideNumberFormat(void);


#endif 

#endif
