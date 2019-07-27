



 
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "miscdtfm.h"

#include "unicode/format.h"
#include "unicode/decimfmt.h"
#include "unicode/datefmt.h"
#include "unicode/smpdtfmt.h"
#include "unicode/dtfmtsym.h"
#include "unicode/locid.h"
#include "unicode/msgfmt.h"
#include "unicode/numfmt.h"
#include "unicode/choicfmt.h"
#include "unicode/gregocal.h"





#define CASE(id,test) case id: name = #test; if (exec) { logln(#test "---"); logln((UnicodeString)""); test(); } break;

void 
DateFormatMiscTests::runIndexedTest( int32_t index, UBool exec, const char* &name, char*  )
{
    
    switch (index) {
        CASE(0, test4097450)
        CASE(1, test4099975)
        CASE(2, test4117335)

        default: name = ""; break;
    }
}

UBool 
DateFormatMiscTests::failure(UErrorCode status, const char* msg)
{
    if(U_FAILURE(status)) {
        errcheckln(status, UnicodeString("FAIL: ") + msg + " failed, error " + u_errorName(status));
        return TRUE;
    }

    return FALSE;
}




void
DateFormatMiscTests::test4097450()
{
    
    
    
    UnicodeString  dstring [] = {
        UnicodeString("97"),
        UnicodeString("1997"),  
        UnicodeString("97"),
        UnicodeString("1997"),
        UnicodeString("01"),
        UnicodeString("2001"),  
        UnicodeString("01"),
        UnicodeString("2001"),  
        UnicodeString("1"),
        UnicodeString("1"),
        UnicodeString("11"),  
        UnicodeString("11"),
        UnicodeString("111"), 
        UnicodeString("111")
    };
    
    UnicodeString dformat [] = {
        UnicodeString("yy"),  
        UnicodeString("yy"),
        UnicodeString("yyyy"),
        UnicodeString("yyyy"),
        UnicodeString("yy"),  
        UnicodeString("yy"),
        UnicodeString("yyyy"),
        UnicodeString("yyyy"),
        UnicodeString("yy"),
        UnicodeString("yyyy"),
        UnicodeString("yy"),
        UnicodeString("yyyy"), 
        UnicodeString("yy"),
        UnicodeString("yyyy")
    };
    

















    UErrorCode status = U_ZERO_ERROR;
    SimpleDateFormat *formatter;
    SimpleDateFormat *resultFormatter = new SimpleDateFormat((UnicodeString)"yyyy", status);
    if (U_FAILURE(status)) {
        dataerrln("Fail new SimpleDateFormat: %s", u_errorName(status));
        return;
    }

    logln("Format\tSource\tResult");
    logln("-------\t-------\t-------");
    for (int i = 0; i < 14; i++)
    {
        log(dformat[i] + "\t" + dstring[i] + "\t");
        formatter = new SimpleDateFormat(dformat[i], status);
        if(failure(status, "new SimpleDateFormat")) return;
        
        UnicodeString str;
        FieldPosition pos(FieldPosition::DONT_CARE);
        logln(resultFormatter->format(formatter->parse(dstring[i], status), str, pos));
        failure(status, "resultFormatter->format");
            
        




        delete formatter;
        logln();
    }

    delete resultFormatter;
}




void
DateFormatMiscTests::test4099975()
{
    





    UDate d = Calendar::getNow();
    {
        UErrorCode status = U_ZERO_ERROR;
        DateFormatSymbols* symbols = new DateFormatSymbols(Locale::getUS(), status);
        if (U_FAILURE(status)) {
            dataerrln("Unable to create DateFormatSymbols - %s", u_errorName(status));
            return;	
        }
        SimpleDateFormat *df = new SimpleDateFormat(UnicodeString("E hh:mm"), *symbols, status);
        if(failure(status, "new SimpleDateFormat")) return;
        UnicodeString format0;
        format0 = df->format(d, format0);
        UnicodeString localizedPattern0;
        localizedPattern0 = df->toLocalizedPattern(localizedPattern0, status);
        failure(status, "df->toLocalizedPattern");
        symbols->setLocalPatternChars(UnicodeString("abcdefghijklmonpqr")); 
        UnicodeString format1;
        format1 = df->format(d, format1);
        if (format0 != format1) {
            errln(UnicodeString("Formats are different. format0: ") + format0 
                + UnicodeString("; format1: ") + format1);
        }
        UnicodeString localizedPattern1;
        localizedPattern1 = df->toLocalizedPattern(localizedPattern1, status);
        failure(status, "df->toLocalizedPattern");
        if (localizedPattern0 != localizedPattern1) {
            errln(UnicodeString("Pattern has been changed. localizedPattern0: ") + localizedPattern0 
                + UnicodeString("; localizedPattern1: ") + localizedPattern1);
        }
        delete symbols;
        delete df;
    }
    



    {
        UErrorCode status = U_ZERO_ERROR;
        DateFormatSymbols* symbols = new DateFormatSymbols(Locale::getUS(), status);
        if(failure(status, "new DateFormatSymbols")) return;
        SimpleDateFormat *df = new SimpleDateFormat(UnicodeString("E hh:mm"), status);
        if(failure(status, "new SimpleDateFormat")) return;
        df->setDateFormatSymbols(*symbols);
        UnicodeString format0;
        format0 = df->format(d, format0);
        UnicodeString localizedPattern0;
        localizedPattern0 = df->toLocalizedPattern(localizedPattern0, status);
        failure(status, "df->toLocalizedPattern");
        symbols->setLocalPatternChars(UnicodeString("abcdefghijklmonpqr")); 
        UnicodeString format1;
        format1 = df->format(d, format1);
        if (format0 != format1) {
            errln(UnicodeString("Formats are different. format0: ") + format0 
                + UnicodeString("; format1: ") + format1);
        }
        UnicodeString localizedPattern1;
        localizedPattern1 = df->toLocalizedPattern(localizedPattern1, status);
        failure(status, "df->toLocalizedPattern");
        if (localizedPattern0 != localizedPattern1) {
            errln(UnicodeString("Pattern has been changed. localizedPattern0: ") + localizedPattern0 
                + UnicodeString("; localizedPattern1: ") + localizedPattern1);
        }
        delete symbols;
        delete df;

    }
    
    {
        UErrorCode status = U_ZERO_ERROR;
        DateFormatSymbols* symbols = new DateFormatSymbols(Locale::getUS(), status);
        if(failure(status, "new DateFormatSymbols")) return;
        SimpleDateFormat *df = new SimpleDateFormat(UnicodeString("E hh:mm"), symbols, status);
        if(failure(status, "new SimpleDateFormat")) return;
        UnicodeString format0;
        format0 = df->format(d, format0);
        UnicodeString localizedPattern0;
        localizedPattern0 = df->toLocalizedPattern(localizedPattern0, status);
        failure(status, "df->toLocalizedPattern");
        symbols->setLocalPatternChars(UnicodeString("abcdefghijklmonpqr")); 
        UnicodeString format1;
        format1 = df->format(d, format1);
        if (format0 != format1) {
            errln(UnicodeString("Formats are different. format0: ") + format0 
                + UnicodeString("; format1: ") + format1);
        }
        UnicodeString localizedPattern1;
        localizedPattern1 = df->toLocalizedPattern(localizedPattern1, status);
        failure(status, "df->toLocalizedPattern");
        if (localizedPattern0 == localizedPattern1) {
            errln(UnicodeString("Pattern should have been changed. localizedPattern0: ") + localizedPattern0 
                + UnicodeString("; localizedPattern1: ") + localizedPattern1);
        }
        
        delete df;
    }
    
    {
        UErrorCode status = U_ZERO_ERROR;
        DateFormatSymbols* symbols = new DateFormatSymbols(Locale::getUS(), status);
        failure(status, "new DateFormatSymbols");
        SimpleDateFormat *df = new SimpleDateFormat(UnicodeString("E hh:mm"), status);
        if(failure(status, "new SimpleDateFormat")) return;
        df-> adoptDateFormatSymbols(symbols);
        UnicodeString format0;
        format0 = df->format(d, format0);
        UnicodeString localizedPattern0;
        localizedPattern0 = df->toLocalizedPattern(localizedPattern0, status);
        failure(status, "df->toLocalizedPattern");
        symbols->setLocalPatternChars(UnicodeString("abcdefghijklmonpqr")); 
        UnicodeString format1;
        format1 = df->format(d, format1);
        if (format0 != format1) {
            errln(UnicodeString("Formats are different. format0: ") + format0 
                + UnicodeString("; format1: ") + format1);
        }
        UnicodeString localizedPattern1;
        localizedPattern1 = df->toLocalizedPattern(localizedPattern1, status);
        failure(status, "df->toLocalizedPattern");
        if (localizedPattern0 == localizedPattern1) {
            errln(UnicodeString("Pattern should have been changed. localizedPattern0: ") + localizedPattern0 
                + UnicodeString("; localizedPattern1: ") + localizedPattern1);
        }
        
        delete df;
    }
}






void
DateFormatMiscTests::test4117335()
{
    
    UChar bcC [] = {
        0x7D00,
        0x5143,
        0x524D
    };
    UnicodeString bc(bcC, 3, 3);

    
    UChar adC [] = {
        0x897F,
        0x66A6
    };
    UnicodeString ad(adC, 2, 2);
    
    
    UChar jstLongC [] = {
        0x65e5,
        0x672c,
        0x6a19,
        0x6e96,
        0x6642
    };
    UChar jdtLongC [] = {0x65E5, 0x672C, 0x590F, 0x6642, 0x9593};

    UnicodeString jstLong(jstLongC, 5, 5);


    
    UnicodeString tzID = "Asia/Tokyo";

    UnicodeString jdtLong(jdtLongC, 5, 5);
 

    UErrorCode status = U_ZERO_ERROR;
    DateFormatSymbols *symbols = new DateFormatSymbols(Locale::getJapan(), status);
    if(U_FAILURE(status)) {
      dataerrln("Failure creating DateFormatSymbols, %s", u_errorName(status));
      delete symbols;
      return;
    }
    failure(status, "new DateFormatSymbols");
    int32_t eraCount = 0;
    const UnicodeString *eras = symbols->getEraNames(eraCount);
    
    logln(UnicodeString("BC = ") + eras[0]);
    if (eras[0] != bc) {
        errln("*** Should have been " + bc);
        
    }

    logln(UnicodeString("AD = ") + eras[1]);
    if (eras[1] != ad) {
        errln("*** Should have been " + ad);
        
    }

    int32_t rowCount, colCount;
    const UnicodeString **zones = symbols->getZoneStrings(rowCount, colCount);
    
    int32_t index = -1;
    for (int32_t i = 0; i < rowCount; ++i) {
        if (tzID == (zones[i][0])) {
            index = i;
            break;
        }
    }
    logln(UnicodeString("Long zone name = ") + zones[index][1]);
    if (zones[index][1] != jstLong) {
        errln("*** Should have been " + prettify(jstLong)+ " but it is: " + prettify(zones[index][1]));
        
    }





    logln(UnicodeString("Long zone name = ") + zones[index][3]);
    if (zones[index][3] != jdtLong) {
        errln("*** Should have been " + prettify(jstLong) + " but it is: " + prettify(zones[index][3]));
        
    }





    delete symbols;

}

#endif 
