














#include <stdlib.h>

#include "unicode/utypes.h"

#if (!UCONFIG_NO_COLLATION)  

#include "unicode/ucol.h"
#include "unicode/uloc.h"
#include "cintltst.h"
#include "ccurrtst.h"
#include "ccolltst.h"
#include "unicode/ustring.h"
#include "cmemory.h"

#define ARRAY_LENGTH(array) (sizeof array / sizeof array[0]) 

void addCurrencyCollTest(TestNode** root)
{
    
    addTest(root, &currTest, "tscoll/ccurrtst/currTest");
    
}


void currTest()
{
    
    static const UChar currency[][2] =
    {
      { 0x00A4, 0x0000}, 
      { 0x00A2, 0x0000}, 
      { 0xFFE0, 0x0000}, 
      { 0x0024, 0x0000}, 
      { 0xFF04, 0x0000}, 
      { 0xFE69, 0x0000}, 
      { 0x00A3, 0x0000}, 
      { 0xFFE1, 0x0000}, 
      { 0x00A5, 0x0000}, 
      { 0xFFE5, 0x0000}, 
      { 0x09F2, 0x0000}, 
      { 0x09F3, 0x0000}, 
      { 0x0E3F, 0x0000}, 
      { 0x17DB, 0x0000}, 
      { 0x20A0, 0x0000}, 
      { 0x20A1, 0x0000}, 
      { 0x20A2, 0x0000}, 
      { 0x20A3, 0x0000}, 
      { 0x20A4, 0x0000}, 
      { 0x20A5, 0x0000}, 
      { 0x20A6, 0x0000}, 
      { 0x20A7, 0x0000}, 
      { 0x20A9, 0x0000}, 
      { 0xFFE6, 0x0000}, 
      { 0x20AA, 0x0000}, 
      { 0x20AB, 0x0000}, 
      { 0x20AC, 0x0000}, 
      { 0x20AD, 0x0000}, 
      { 0x20AE, 0x0000}, 
      { 0x20AF, 0x0000}, 
    };

#if 0
    
    static const UChar currency[][2] =
    {
        { 0x00a4, 0x0000}, 
        { 0x0e3f, 0x0000}, 
        { 0x00a2, 0x0000}, 
        { 0x20a1, 0x0000}, 
        { 0x20a2, 0x0000}, 
        { 0x0024, 0x0000}, 
        { 0x20ab, 0x0000}, 
        { 0x20ac, 0x0000}, 
        { 0x20a3, 0x0000}, 
        { 0x20a4, 0x0000}, 
        { 0x20a5, 0x0000}, 
        { 0x20a6, 0x0000}, 
        { 0x20a7, 0x0000}, 
        { 0x00a3, 0x0000}, 
        { 0x20a8, 0x0000}, 
        { 0x20aa, 0x0000}, 
        { 0x20a9, 0x0000}, 
        { 0x00a5, 0x0000}  
    };
#endif

    UChar source[2], target[2];
    int32_t i, j, sortklen;
    int res;
    UCollator *c;
    uint8_t *sortKey1, *sortKey2;
    UErrorCode status = U_ZERO_ERROR;
    UCollationResult compareResult, keyResult;
    UCollationResult expectedResult = UCOL_EQUAL;
    log_verbose("Testing currency of all locales\n");
    c = ucol_open("en_US", &status);
    if (U_FAILURE(status))
    {
        log_err_status(status, "collator open failed! :%s\n", myErrorName(status));
        return;
    }

    

    for (i = 0; i < ARRAY_LENGTH(currency); i += 1)
    {
        for (j = 0; j < ARRAY_LENGTH(currency); j += 1)
        {
             u_strcpy(source, currency[i]);
             u_strcpy(target, currency[j]);
            
            if (i < j)
            {
                expectedResult = UCOL_LESS;
            }
            else if ( i == j)
            {
                expectedResult = UCOL_EQUAL;
            }
            else
            {
                expectedResult = UCOL_GREATER;
            }

            compareResult = ucol_strcoll(c, source, u_strlen(source), target, u_strlen(target));
            
            status = U_ZERO_ERROR;

            sortklen=ucol_getSortKey(c, source, u_strlen(source),  NULL, 0);
            sortKey1=(uint8_t*)malloc(sizeof(uint8_t) * (sortklen+1));
            ucol_getSortKey(c, source, u_strlen(source), sortKey1, sortklen+1);

            sortklen=ucol_getSortKey(c, target, u_strlen(target),  NULL, 0);
            sortKey2=(uint8_t*)malloc(sizeof(uint8_t) * (sortklen+1));
            ucol_getSortKey(c, target, u_strlen(target), sortKey2, sortklen+1);

            res = uprv_memcmp(sortKey1, sortKey2, sortklen);
            if (res < 0) keyResult = (UCollationResult)-1;
            else if (res > 0) keyResult = (UCollationResult)1;
            else keyResult = (UCollationResult)0;
            
            reportCResult( source, target, sortKey1, sortKey2, compareResult, keyResult, compareResult, expectedResult );

            free(sortKey1);
            free(sortKey2);

        }
    }
    ucol_close(c);
}

#endif 
