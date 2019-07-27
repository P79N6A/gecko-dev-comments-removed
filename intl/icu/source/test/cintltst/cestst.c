



















#include <stdlib.h>

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "unicode/uloc.h"
#include "cintltst.h"
#include "cestst.h"
#include "ccolltst.h"
#include "callcoll.h"
#include "unicode/ustring.h"
#include "string.h"

static UCollator *myCollation;
const static UChar testSourceCases[][MAX_TOKEN_LEN] = {
    {0x0062, 0x006c, 0x0069, 0x0061, 0x0073, 0x0000},
    {0x0045, 0x006c, 0x006c, 0x0069, 0x006f, 0x0074, 0x0000},
    {0x0048, 0x0065, 0x006c, 0x006c, 0x006f, 0x0000},
    {0x0061, 0x0063, 0x0048, 0x0063, 0x0000},
    {0x0061, 0x0063, 0x0063, 0x0000},
    {0x0061, 0x006c, 0x0069, 0x0061, 0x0073, 0x0000},
    {0x0061, 0x0063, 0x0048, 0x0063, 0x0000},
    {0x0061, 0x0063, 0x0063, 0x0000},
    {0x0048, 0x0065, 0x006c, 0x006c, 0x006f, 0x0000},
};

const static UChar testTargetCases[][MAX_TOKEN_LEN] = {
    {0x0062, 0x006c, 0x006c, 0x0069, 0x0061, 0x0073, 0x0000},
    {0x0045, 0x006d, 0x0069, 0x006f, 0x0074, 0x0000},
    {0x0068, 0x0065, 0x006c, 0x006c, 0x006f, 0x0000},
    {0x0061, 0x0043, 0x0048, 0x0063, 0x0000},
    {0x0061, 0x0043, 0x0048, 0x0063, 0x0000},
    {0x0062, 0x006c, 0x006c, 0x0069, 0x0061, 0x0073, 0x0000},
    {0x0061, 0x0043, 0x0048, 0x0063, 0x0000},
    {0x0061, 0x0043, 0x0048, 0x0063, 0x0000},
    {0x0068, 0x0065, 0x006c, 0x006c, 0x006f, 0x0000},
};

const static UCollationResult results[] = {
    UCOL_LESS,
    UCOL_LESS,
    UCOL_GREATER,
    UCOL_LESS,
    UCOL_LESS,
    
    UCOL_LESS,
    UCOL_EQUAL,
    UCOL_LESS,
    UCOL_EQUAL
};


void addSpanishCollTest(TestNode** root)
{
    
    
    addTest(root, &TestPrimary, "tscoll/cestst/TestPrimary");
    addTest(root, &TestTertiary, "tscoll/cestst/TestTertiary");

}


static void TestTertiary( )
{
    
    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("es_ES", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: %s: in creation of rule based collator: %s\n", __FILE__, myErrorName(status));
        return;
    }
    log_verbose("Testing Spanish Collation with Tertiary strength\n");
    ucol_setStrength(myCollation, UCOL_TERTIARY);
    for (i = 0; i < 5 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    ucol_close(myCollation);
}

static void TestPrimary()
{
    
    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("es_ES", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: %s: in creation of rule based collator: %s\n", __FILE__, myErrorName(status));
        return;
    }
    log_verbose("Testing Spanish Collation with Primary strength\n");
    ucol_setStrength(myCollation, UCOL_PRIMARY);
    for (i = 5; i < 9; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    ucol_close(myCollation);
}

#endif 
