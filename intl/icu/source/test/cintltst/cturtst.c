



















#include <stdlib.h>

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "unicode/uloc.h"
#include "cintltst.h"
#include "ccolltst.h"
#include "callcoll.h"
#include "cturtst.h"
#include "unicode/ustring.h"
#include "string.h"

static UCollator *myCollation;
const static UChar testSourceCases[][MAX_TOKEN_LEN] = {
    {0x0073, 0x0327, 0x0000},
    {0x0076, 0x00E4, 0x0074, 0x0000},
    {0x006f, 0x006c, 0x0064, 0x0000},
    {0x00FC, 0x006f, 0x0069, 0x0064, 0x0000},
    {0x0068, 0x011E, 0x0061, 0x006c, 0x0074, 0x0000},
    {0x0073, 0x0074, 0x0072, 0x0065, 0x0073, 0x015E, 0x0000},
    {0x0076, 0x006f, 0x0131, 0x0064, 0x0000},
    {0x0069, 0x0064, 0x0065, 0x0061, 0x0000},
    {0x00FC, 0x006f, 0x0069, 0x0064 , 0x0000},
    {0x0076, 0x006f, 0x0131, 0x0064 , 0x0000},
    {0x0069, 0x0064, 0x0065, 0x0061, 0x0000},
};

const static UChar testTargetCases[][MAX_TOKEN_LEN] = {
    {0x0075, 0x0308, 0x0000},
    {0x0076, 0x0062, 0x0074, 0x0000},
    {0x00D6, 0x0061, 0x0079, 0x0000},
    {0x0076, 0x006f, 0x0069, 0x0064 , 0x0000},
    {0x0068, 0x0061,  0x006c, 0x0074, 0x0000},
    {0x015E, 0x0074, 0x0072, 0x0065, 0x015E, 0x0073, 0x0000},
    {0x0076, 0x006f, 0x0069, 0x0064 , 0x0000},
    {0x0049, 0x0064, 0x0065, 0x0061, 0x0000},
    {0x0076, 0x006f, 0x0069, 0x0064 , 0x0000},
    {0x0076, 0x006f, 0x0069, 0x0064 , 0x0000},
    {0x0049, 0x0064, 0x0065, 0x0061, 0x0000},
};

const static UCollationResult results[] = {
    UCOL_LESS,
    UCOL_LESS,
    UCOL_LESS,
    UCOL_LESS,
    UCOL_GREATER,
    UCOL_LESS,
    UCOL_LESS,
    UCOL_GREATER,
    
    UCOL_LESS,
    UCOL_LESS, 
    UCOL_GREATER
};



void addTurkishCollTest(TestNode** root)
{
    
    addTest(root, &TestPrimary, "tscoll/cturtst/TestPrimary");
    addTest(root, &TestTertiary, "tscoll/cturtst/TestTertiary");


}

static void TestTertiary( )
{
    
    int32_t i;

    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("tr", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n", myErrorName(status));
        return;
    }
    log_verbose("Testing Turkish Collation with Tertiary strength\n");
    ucol_setStrength(myCollation, UCOL_TERTIARY);
    for (i = 0; i < 8 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    ucol_close(myCollation);
}

static void TestPrimary()
{
    
    int32_t i;

    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("tr", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n", myErrorName(status));
        return;
    }
    log_verbose("Testing Turkish Collation with Primary strength\n");
    ucol_setStrength(myCollation, UCOL_PRIMARY);
    for (i = 8; i < 11; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    ucol_close(myCollation);
}

#endif 
