



















#include <stdlib.h>

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "unicode/uloc.h"
#include "cintltst.h"
#include "ccolltst.h"
#include "callcoll.h"
#include "cfintst.h"
#include "unicode/ustring.h"
#include "string.h"

static UCollator *myCollation;
const static UChar testSourceCases[][MAX_TOKEN_LEN] = {
    {0x0077, 0x0061, 0x0074, 0x0000},
    {0x0076, 0x0061, 0x0074, 0x0000},
    {0x0061, 0x00FC, 0x0062, 0x0065, 0x0063, 0x006b, 0x0000},
    {0x004c, 0x00E5, 0x0076, 0x0069, 0x0000},
    {0x0077, 0x0061, 0x0074, 0x0000}
};

const static UChar testTargetCases[][MAX_TOKEN_LEN] = {
    {0x0076, 0x0061, 0x0074, 0x0000},
    {0x0077, 0x0061, 0x0079, 0x0000},
    {0x0061, 0x0078, 0x0062, 0x0065, 0x0063, 0x006b, 0x0000},
    {0x004c, 0x00E4, 0x0077, 0x0065, 0x0000},
    {0x0076, 0x0061, 0x0074, 0x0000}
};

const static UCollationResult results[] = {
    UCOL_GREATER,
    UCOL_LESS,
    UCOL_GREATER,
    UCOL_LESS,
    
    UCOL_GREATER    
};



void addFinnishCollTest(TestNode** root)
{
    
    
    addTest(root, &TestPrimary, "tscoll/cfintst/TestPrimary");
    addTest(root, &TestTertiary, "tscoll/cfintst/TestTertiary");
    


}


static void TestTertiary( )
{
    
    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("fi_FI@collation=standard", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n", myErrorName(status));
    }
    log_verbose("Testing Finnish Collation with Tertiary strength\n");
    ucol_setStrength(myCollation, UCOL_TERTIARY);
    for (i = 0; i < 4 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    ucol_close(myCollation);
}

static void TestPrimary()
{
    
    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("fi_FI@collation=standard", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n", myErrorName(status));
    }
    log_verbose("Testing Finnish Collation with Primary strength\n");
    ucol_setStrength(myCollation, UCOL_PRIMARY);
    for (i = 4; i < 5; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    ucol_close(myCollation);
}

#endif 
