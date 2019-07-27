
















#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "cintltst.h"
#include "cformtst.h"

void addCalTest(TestNode**);
void addDateForTest(TestNode**);
void addDateTimePatternGeneratorTest(TestNode**);
void addDateIntervalFormatTest(TestNode**);
void addNumForTest(TestNode**);
void addMsgForTest(TestNode**);
void addDateForRgrTest(TestNode**);
void addNumFrDepTest(TestNode**);
void addDtFrDepTest(TestNode**);
void addUtmsTest(TestNode**);
void addCurrencyTest(TestNode**);
void addPluralRulesTest(TestNode**);
void addURegionTest(TestNode** root);
void addUListFmtTest(TestNode** root);

void addFormatTest(TestNode** root);

void addFormatTest(TestNode** root)
{
    addCalTest(root);
    addDateForTest(root);
    addDateTimePatternGeneratorTest(root);
    addDateIntervalFormatTest(root);
    addNumForTest(root);
    addNumFrDepTest(root);
    addMsgForTest(root);
    addDateForRgrTest(root);
    addDtFrDepTest(root);
    addUtmsTest(root);
    addCurrencyTest(root);
    addPluralRulesTest(root);
    addURegionTest(root);
    addUListFmtTest(root);
}


UChar* myDateFormat(UDateFormat* dat, UDate d1)
{
    UChar *result1=NULL;
    int32_t resultlength, resultlengthneeded;
    UErrorCode status = U_ZERO_ERROR;


    resultlength=0;
    resultlengthneeded=udat_format(dat, d1, NULL, resultlength, NULL, &status);
    if(status==U_BUFFER_OVERFLOW_ERROR)
    {
        status=U_ZERO_ERROR;
        resultlength=resultlengthneeded+1;
        result1=(UChar*)ctst_malloc(sizeof(UChar) * resultlength);
        udat_format(dat, d1, result1, resultlength, NULL, &status);
    }
    if(U_FAILURE(status))
    {
        log_err("Error in formatting using udat_format(.....): %s\n", myErrorName(status));
        return 0;
    }
    return result1;

}

#endif 
