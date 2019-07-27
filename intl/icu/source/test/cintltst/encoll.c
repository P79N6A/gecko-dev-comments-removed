



















#include <stdlib.h>
#include <string.h>

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "unicode/uloc.h"
#include "cintltst.h"
#include "encoll.h"
#include "ccolltst.h"
#include "callcoll.h"
#include "unicode/ustring.h"

static UCollator *myCollation = NULL;
const static UChar testSourceCases[][MAX_TOKEN_LEN] = {
        {(UChar)0x0061 , (UChar)0x0062 , 0},
        {(UChar)0x0062 , (UChar)0x006C , (UChar)0x0061 , (UChar)0x0063 , (UChar)0x006B , (UChar)0x002D , (UChar)0x0062 , (UChar)0x0069 , (UChar)0x0072 , (UChar)0x0064 , 0},
        {(UChar)0x0062 , (UChar)0x006C , (UChar)0x0061 , (UChar)0x0063 , (UChar)0x006B , (UChar)0x0020 , (UChar)0x0062 , (UChar)0x0069 , (UChar)0x0072 , (UChar)0x0064 , 0},
        {(UChar)0x0062 , (UChar)0x006C , (UChar)0x0061 , (UChar)0x0063 , (UChar)0x006B , (UChar)0x002D , (UChar)0x0062 , (UChar)0x0069 , (UChar)0x0072 , (UChar)0x0064 , 0},
        {(UChar)0x0048 , (UChar)0x0065 , (UChar)0x006C , (UChar)0x006C , (UChar)0x006F , 0},
        {(UChar)0x0041 , (UChar)0x0042 , (UChar)0x0043 , 0}, 
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0062 , (UChar)0x006C , (UChar)0x0061 , (UChar)0x0063 , (UChar)0x006B , (UChar)0x0062 , (UChar)0x0069 , (UChar)0x0072 , (UChar)0x0064 , 0},
        {(UChar)0x0062 , (UChar)0x006C , (UChar)0x0061 , (UChar)0x0063 , (UChar)0x006B , (UChar)0x002D , (UChar)0x0062 , (UChar)0x0069 , (UChar)0x0072 , (UChar)0x0064 , 0},
        {(UChar)0x0062 , (UChar)0x006C , (UChar)0x0061 , (UChar)0x0063 , (UChar)0x006B , (UChar)0x002D , (UChar)0x0062 , (UChar)0x0069 , (UChar)0x0072 , (UChar)0x0064 , 0},
        {(UChar)0x0070 , 0x00EA, (UChar)0x0063 , (UChar)0x0068 , (UChar)0x0065 , 0},                                            
        {(UChar)0x0070 , 0x00E9, (UChar)0x0063 , (UChar)0x0068 , 0x00E9, 0},
        {0x00C4, (UChar)0x0042 , 0x0308, (UChar)0x0043 , 0x0308, 0},
        {(UChar)0x0061 , 0x0308, (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0070 , 0x00E9, (UChar)0x0063 , (UChar)0x0068 , (UChar)0x0065 , (UChar)0x0072 , 0},
        {(UChar)0x0072 , (UChar)0x006F , (UChar)0x006C , (UChar)0x0065 , (UChar)0x0073 , 0},
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0041 , 0},
        {(UChar)0x0041 , 0},
        {(UChar)0x0061 , (UChar)0x0062 , 0},                                                                
        {(UChar)0x0074 , (UChar)0x0063 , (UChar)0x006F , (UChar)0x006D , (UChar)0x0070 , (UChar)0x0061 , (UChar)0x0072 , (UChar)0x0065 , (UChar)0x0070 , (UChar)0x006C , (UChar)0x0061 , (UChar)0x0069 , (UChar)0x006E , 0},
        {(UChar)0x0061 , (UChar)0x0062 , 0}, 
        {(UChar)0x0061 , (UChar)0x0023 , (UChar)0x0062 , 0},
        {(UChar)0x0061 , (UChar)0x0023 , (UChar)0x0062 , 0},
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0041 , (UChar)0x0062 , (UChar)0x0063 , (UChar)0x0064 , (UChar)0x0061 , 0},
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , (UChar)0x0064 , (UChar)0x0061 , 0},
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , (UChar)0x0064 , (UChar)0x0061 , 0},
        {0x00E6, (UChar)0x0062 , (UChar)0x0063 , (UChar)0x0064 , (UChar)0x0061 , 0},
        {0x00E4, (UChar)0x0062 , (UChar)0x0063 , (UChar)0x0064 , (UChar)0x0061 , 0},                                            
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0061 , (UChar)0x0063 , (UChar)0x0048 , (UChar)0x0063 , 0},
        {(UChar)0x0061 , 0x0308, (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0074 , (UChar)0x0068 , (UChar)0x0069 , 0x0302, (UChar)0x0073 , 0},
        {(UChar)0x0070 , 0x00EA, (UChar)0x0063 , (UChar)0x0068 , (UChar)0x0065 },
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , 0},                                                         
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0061 , 0x00E6, (UChar)0x0063 , 0},
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0061 , 0x00E6, (UChar)0x0063 , 0},
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , 0},               
        {(UChar)0x0070 , 0x00E9, (UChar)0x0063 , (UChar)0x0068 , 0x00E9, 0}                                            
};

const static UChar testTargetCases[][MAX_TOKEN_LEN] = {
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0062 , (UChar)0x006C , (UChar)0x0061 , (UChar)0x0063 , (UChar)0x006B , (UChar)0x0062 , (UChar)0x0069 , (UChar)0x0072 , (UChar)0x0064 , 0},
        {(UChar)0x0062 , (UChar)0x006C , (UChar)0x0061 , (UChar)0x0063 , (UChar)0x006B , (UChar)0x002D , (UChar)0x0062 , (UChar)0x0069 , (UChar)0x0072 , (UChar)0x0064 , 0},
        {(UChar)0x0062 , (UChar)0x006C , (UChar)0x0061 , (UChar)0x0063 , (UChar)0x006B , 0},
        {(UChar)0x0068 , (UChar)0x0065 , (UChar)0x006C , (UChar)0x006C , (UChar)0x006F , 0},
        {(UChar)0x0041 , (UChar)0x0042 , (UChar)0x0043 , 0},
        {(UChar)0x0041 , (UChar)0x0042 , (UChar)0x0043 , 0},
        {(UChar)0x0062 , (UChar)0x006C , (UChar)0x0061 , (UChar)0x0063 , (UChar)0x006B , (UChar)0x0062 , (UChar)0x0069 , (UChar)0x0072 , (UChar)0x0064 , (UChar)0x0073 , 0},
        {(UChar)0x0062 , (UChar)0x006C , (UChar)0x0061 , (UChar)0x0063 , (UChar)0x006B , (UChar)0x0062 , (UChar)0x0069 , (UChar)0x0072 , (UChar)0x0064 , (UChar)0x0073 , 0},
        {(UChar)0x0062 , (UChar)0x006C , (UChar)0x0061 , (UChar)0x0063 , (UChar)0x006B , (UChar)0x0062 , (UChar)0x0069 , (UChar)0x0072 , (UChar)0x0064 , 0},                             
        {(UChar)0x0070 , 0x00E9, (UChar)0x0063 , (UChar)0x0068 , 0x00E9, 0},
        {(UChar)0x0070 , 0x00E9, (UChar)0x0063 , (UChar)0x0068 , (UChar)0x0065 , (UChar)0x0072 , 0},
        {0x00C4, (UChar)0x0042 , 0x0308, (UChar)0x0043 , 0x0308, 0},
        {(UChar)0x0041 , 0x0308, (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0070 , 0x00E9, (UChar)0x0063 , (UChar)0x0068 , (UChar)0x0065 , 0},
        {(UChar)0x0072 , (UChar)0x006F , 0x0302, (UChar)0x006C , (UChar)0x0065 , 0},
        {(UChar)0x0041 , 0x00E1, (UChar)0x0063 , (UChar)0x0064 , 0},
        {(UChar)0x0041 , 0x00E1, (UChar)0x0063 , (UChar)0x0064 , 0},
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , 0},                                                             
        {(UChar)0x0054 , (UChar)0x0043 , (UChar)0x006F , (UChar)0x006D , (UChar)0x0070 , (UChar)0x0061 , (UChar)0x0072 , (UChar)0x0065 , (UChar)0x0050 , (UChar)0x006C , (UChar)0x0061 , (UChar)0x0069 , (UChar)0x006E , 0},
        {(UChar)0x0061 , (UChar)0x0042 , (UChar)0x0063 , 0},
        {(UChar)0x0061 , (UChar)0x0023 , (UChar)0x0042 , 0},
        {(UChar)0x0061 , (UChar)0x0026 , (UChar)0x0062 , 0},
        {(UChar)0x0061 , (UChar)0x0023 , (UChar)0x0063 , 0},
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , (UChar)0x0064 , (UChar)0x0061 , 0},
        {0x00C4, (UChar)0x0062 , (UChar)0x0063 , (UChar)0x0064 , (UChar)0x0061 , 0},
        {0x00E4, (UChar)0x0062 , (UChar)0x0063 , (UChar)0x0064 , (UChar)0x0061 , 0},
        {0x00C4, (UChar)0x0062 , (UChar)0x0063 , (UChar)0x0064 , (UChar)0x0061 , 0},
        {0x00C4, (UChar)0x0062 , (UChar)0x0063 , (UChar)0x0064 , (UChar)0x0061 , 0},                                             
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0023 , (UChar)0x0063 , 0},
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x003D , (UChar)0x0063 , 0},
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0064 , 0},
        {0x00E4, (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0061 , (UChar)0x0043 , (UChar)0x0048 , (UChar)0x0063 , 0},
        {0x00E4, (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0074 , (UChar)0x0068 , 0x00EE, (UChar)0x0073 , 0},
        {(UChar)0x0070 , 0x00E9, (UChar)0x0063 , (UChar)0x0068 , 0x00E9, 0},
        {(UChar)0x0061 , (UChar)0x0042 , (UChar)0x0043 , 0},                                                          
        {(UChar)0x0061 , (UChar)0x0062 , (UChar)0x0064 , 0},
        {0x00E4, (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0061 , 0x00C6, (UChar)0x0063 , 0},
        {(UChar)0x0061 , (UChar)0x0042 , (UChar)0x0064 , 0},
        {0x00E4, (UChar)0x0062 , (UChar)0x0063 , 0},
        {(UChar)0x0061 , 0x00C6, (UChar)0x0063 , 0},
        {(UChar)0x0061 , (UChar)0x0042 , (UChar)0x0064 , 0},
        {0x00E4, (UChar)0x0062 , (UChar)0x0063 , 0},          
        {(UChar)0x0070 , 0x00EA, (UChar)0x0063 , (UChar)0x0068 , (UChar)0x0065 , 0}                                                 
};

const static UCollationResult results[] = {
        UCOL_LESS, 
        UCOL_LESS, 
        UCOL_LESS,
        UCOL_GREATER,
        UCOL_GREATER,
        UCOL_EQUAL,
        UCOL_LESS,
        UCOL_LESS,
        UCOL_LESS,
        UCOL_LESS,                                                           
        UCOL_GREATER,
        UCOL_LESS,
        UCOL_EQUAL,
        UCOL_LESS,
        UCOL_GREATER,
        UCOL_GREATER,
        UCOL_GREATER,
        UCOL_LESS,
        UCOL_LESS,
        UCOL_LESS,                                                             
        UCOL_LESS,
        UCOL_LESS,
        UCOL_LESS,
        UCOL_GREATER,
        UCOL_GREATER,
        UCOL_GREATER,
        
        UCOL_LESS,
        UCOL_LESS,
        UCOL_GREATER,
        UCOL_LESS,                                                             
        UCOL_GREATER,
        UCOL_EQUAL,
        UCOL_GREATER,
        UCOL_LESS,
        UCOL_LESS,
        UCOL_LESS,
        
        UCOL_EQUAL,
        UCOL_EQUAL,
        
        UCOL_EQUAL,
        UCOL_EQUAL,                                                            
        UCOL_LESS,
        UCOL_EQUAL,
        UCOL_EQUAL,
        
        UCOL_LESS,
        UCOL_LESS,
        UCOL_EQUAL,
        UCOL_LESS,
        UCOL_LESS, 
        UCOL_LESS                                                                         
};

const static UChar testBugs[][MAX_TOKEN_LEN] = {
    {(UChar)0x0061 , 0},
    {(UChar)0x0041 , 0},
    {(UChar)0x0065 , 0},
    {(UChar)0x0045 , 0},
    {0x00e9, 0},
    {0x00e8, 0},
    {0x00ea, 0},
    {0x00eb, 0},
    {(UChar)0x0065 , (UChar)0x0061 , 0},
    {(UChar)0x0078 , 0}
        
};



const static UChar testAcute[][MAX_TOKEN_LEN] = {
    {(UChar)0x0065 , (UChar)0x0065 , 0},
    {(UChar)0x0065 , (UChar)0x0065 , 0x0301, 0},
    {(UChar)0x0065 , (UChar)0x0065 , 0x0301, 0x0300, 0},
    {(UChar)0x0065 , (UChar)0x0065 , 0x0300, 0},
    {(UChar)0x0065 , (UChar)0x0065 , 0x0300, 0x0301, 0},
    {(UChar)0x0065 , 0x0301, (UChar)0x0065 , 0},
    {(UChar)0x0065 , 0x0301, (UChar)0x0065 , 0x0301, 0},
    {(UChar)0x0065 , 0x0301, (UChar)0x0065 , 0x0301, 0x0300, 0},
    {(UChar)0x0065 , 0x0301, (UChar)0x0065 , 0x0300, 0},
    {(UChar)0x0065 , 0x0301, (UChar)0x0065 , 0x0300, 0x0301, 0},
    {(UChar)0x0065 , 0x0301, 0x0300, (UChar)0x0065 , 0},
    {(UChar)0x0065 , 0x0301, 0x0300, (UChar)0x0065 , 0x0301, 0},
    {(UChar)0x0065 , 0x0301, 0x0300, (UChar)0x0065 , 0x0301, 0x0300, 0},
    {(UChar)0x0065 , 0x0301, 0x0300, (UChar)0x0065 , 0x0300, 0},
    {(UChar)0x0065 , 0x0301, 0x0300, (UChar)0x0065 , 0x0300, 0x0301, 0},
    {(UChar)0x0065 , 0x0300, (UChar)0x0065 , 0},
    {(UChar)0x0065 , 0x0300, (UChar)0x0065 , 0x0301, 0},
    {(UChar)0x0065 , 0x0300, (UChar)0x0065 , 0x0301, 0x0300, 0},
    {(UChar)0x0065 , 0x0300, (UChar)0x0065 , 0x0300, 0},
    {(UChar)0x0065 , 0x0300, (UChar)0x0065 , 0x0300, 0x0301, 0},
    {(UChar)0x0065 , 0x0300, 0x0301, (UChar)0x0065 , 0},
    {(UChar)0x0065 , 0x0300, 0x0301, (UChar)0x0065 , 0x0301, 0},
    {(UChar)0x0065 , 0x0300, 0x0301, (UChar)0x0065 , 0x0301, 0x0300, 0},
    {(UChar)0x0065 , 0x0300, 0x0301, (UChar)0x0065 , 0x0300, 0},
    {(UChar)0x0065 , 0x0300, 0x0301, (UChar)0x0065 , 0x0300, 0x0301, 0}
};

static const UChar testMore[][MAX_TOKEN_LEN] = {
    {(UChar)0x0061 , (UChar)0x0065 , 0},
    { 0x00E6, 0},
    { 0x00C6, 0},
    {(UChar)0x0061 , (UChar)0x0066 , 0},
    {(UChar)0x006F , (UChar)0x0065 , 0},
    { 0x0153, 0},
    { 0x0152, 0},
    {(UChar)0x006F , (UChar)0x0066 , 0},
};


void addEnglishCollTest(TestNode** root)
{
    
    addTest(root, &TestPrimary, "tscoll/encoll/TestPrimary");
    addTest(root, &TestSecondary, "tscoll/encoll/TestSecondary");
    addTest(root, &TestTertiary, "tscoll/encoll/TestTertiary");

}

static void TestTertiary( )
{
    int32_t testMoreSize;
    UCollationResult expected=UCOL_EQUAL;
    int32_t i,j;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("en_US", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n", myErrorName(status));
        return;
    }
    log_verbose("Testing English Collation with Tertiary strength\n");

    ucol_setStrength(myCollation, UCOL_TERTIARY);
    for (i = 0; i < 38 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
  

    j = 0;
   for (i = 0; i < 10; i++)
    {
        for (j = i+1; j < 10; j++)
        {
            doTest(myCollation, testBugs[i], testBugs[j], UCOL_LESS);
        }
   }
    
    testMoreSize = sizeof(testMore) / sizeof(testMore[0]);
    for (i = 0; i < testMoreSize; i++)
    {
        for (j = 0; j < testMoreSize; j++)
        {
            if (i <  j) expected = UCOL_LESS;
            if (i == j) expected = UCOL_EQUAL;
            if (i >  j) expected = UCOL_GREATER;
            doTest(myCollation, testMore[i], testMore[j], expected );
        }
    }
    ucol_close(myCollation);
}

static void TestPrimary()
{
    
    int32_t i;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("en_US", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n", myErrorName(status));
        return;
    }
    ucol_setStrength(myCollation, UCOL_PRIMARY);
    log_verbose("Testing English Collation with Primary strength\n");
    for (i = 38; i < 43 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    ucol_close(myCollation);
}

static void TestSecondary()
{
    UCollationResult expected=UCOL_EQUAL;
    int32_t i,j, testAcuteSize;
    UErrorCode status = U_ZERO_ERROR;
    myCollation = ucol_open("en_US", &status);
    if(U_FAILURE(status)){
        log_err_status(status, "ERROR: in creation of rule based collator: %s\n", myErrorName(status));
        return;
    }
    ucol_setStrength(myCollation, UCOL_SECONDARY);
    log_verbose("Testing English Collation with Secondary strength\n");
    for (i = 43; i < 49 ; i++)
    {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
    

    
    testAcuteSize = sizeof(testAcute) / sizeof(testAcute[0]);
    for (i = 0; i < testAcuteSize; i++)
    {
        for (j = 0; j < testAcuteSize; j++)
        {
            if (i <  j) expected = UCOL_LESS;
            if (i == j) expected = UCOL_EQUAL;
            if (i >  j) expected = UCOL_GREATER;
            doTest(myCollation, testAcute[i], testAcute[j], expected );
        }
    }
ucol_close(myCollation);
}

#endif 
