



































#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "unicode/uloc.h"
#include "cintltst.h"
#include "cg7coll.h"
#include "ccolltst.h"
#include "callcoll.h"
#include "unicode/ustring.h"

const char* locales[8] = {
        "en_US",
        "en_GB",
        "en_CA",
        "fr_FR",
        "fr_CA",
        "de_DE",
        "it_IT",
        "ja_JP"
};



const static UChar testCases[][MAX_TOKEN_LEN] = {
    {  0x0062 , 0x006c , 0x0061 , 0x0062 , 0x006b , 
        0x0062 , 0x0069 , 0x0072 , 0x0064 , 0x0073 , 0x0000},                    
    { 0x0050 , 0x0061 , 0x0074, 0x0000},                                                    
    { 0x0070 , 0x00E9, 0x0063 , 0x0068 , 0x00E9, 0x0000},                                    
    { 0x0070 , 0x00EA, 0x0063 , 0x0068 , 0x0065 , 0x0000},                           
    { 0x0070 , 0x00E9, 0x0063 , 0x0068 , 0x0065 , 0x0072 , 0x0000},            
    { 0x0070 , 0x00EA, 0x0063 , 0x0068 , 0x0065 , 0x0072 , 0x0000},            
    { 0x0054 , 0x006f , 0x0064 , 0x0000},                                                    
    { 0x0054 , 0x00F6, 0x006e , 0x0065 , 0x0000},                                            
    { 0x0054 , 0x006f , 0x0066 , 0x0075 , 0x0000},                                   
    { 0x0062 , 0x006c , 0x0061 , 0x0062 , 0x006b , 
      0x0062  , 0x0069 , 0x0072 , 0x0064 , 0x0000},                                    
    { 0x0054 , 0x006f , 0x006e , 0x0000},                                                    
    { 0x0050  , 0x0041 , 0x0054 , 0x0000},                                                    
    { 0x0062 , 0x006c , 0x0061 , 0x0062 , 0x006b , 
        0x002d ,  0x0062 , 0x0069 , 0x0072 , 0x0064 , 0x0000},                
    { 0x0062 , 0x006c , 0x0061 , 0x0062 , 0x006b , 
        0x002d ,  0x0062 , 0x0069 , 0x0072 , 0x0064 , 0x0073, 0x0000},  
    {0x0070 , 0x0061 , 0x0074 , 0x0000},                                                    
    
    { 0x0063 , 0x007a , 0x0061 , 0x0072 , 0x0000 },                                 
    { 0x0063 , 0x0068 , 0x0075 , 0x0072 , 0x006f , 0x0000 },                  
    { 0x0063 , 0x0061 , 0x0074 , 0x000 },                                                     
    { 0x0064 , 0x0061 , 0x0072 , 0x006e , 0x0000 },                                 
    { 0x003f , 0x0000 },                                                                                
    { 0x0071 , 0x0075 , 0x0069 , 0x0063 , 0x006b , 0x0000 },                  
    { 0x0023 , 0x0000 },                                                                                
    { 0x0026 , 0x0000 },                                                                                
    {  0x0061 , 0x002d , 0x0072 , 0x0064 , 0x0076 , 0x0061 , 
                0x0072, 0x006b, 0x0000},                                                        
    { 0x0061 , 0x0061 , 0x0072 , 0x0064 , 0x0076 , 0x0061 , 
                0x0072, 0x006b, 0x0000},                                                        
    { 0x0061 , 0x0062 , 0x0062 , 0x006f , 0x0074 , 0x0000},                   
    { 0x0063 , 0x006f , 0x002d , 0x0070 , 0x0000},                                 
    { 0x0063 , 0x006f  , 0x0070 , 0x0000},                                                
    { 0x0063 , 0x006f , 0x006f , 0x0070 , 0x0000},                                 
    { 0x007a , 0x0065  , 0x0062 , 0x0072 , 0x0061 , 0x0000}                    
};

const static int32_t results[TESTLOCALES][TOTALTESTSET] = {
    { 12, 13, 9, 0, 14, 1, 11, 2, 3, 4, 5, 6, 8, 10, 7, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 }, 
    { 12, 13, 9, 0, 14, 1, 11, 2, 3, 4, 5, 6, 8, 10, 7, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 }, 
    { 12, 13, 9, 0, 14, 1, 11, 2, 3, 4, 5, 6, 8, 10, 7, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 }, 
    { 12, 13, 9, 0, 14, 1, 11, 2, 3, 4, 5, 6, 8, 10, 7, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 }, 
    { 12, 13, 9, 0, 14, 1, 11, 3, 2, 4, 5, 6, 8, 10, 7, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 }, 
    { 12, 13, 9, 0, 14, 1, 11, 2, 3, 4, 5, 6, 8, 10, 7, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 }, 
    { 12, 13, 9, 0, 14, 1, 11, 2, 3, 4, 5, 6, 8, 10, 7, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 }, 
    { 12, 13, 9, 0, 14, 1, 11, 2, 3, 4, 5, 6, 8, 10, 7, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 }, 
    
    { 12, 13, 9, 0, 6, 8, 10, 7, 14, 1, 11, 2, 3, 4, 5, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31, 31 }, 
    
    { 19, 22, 21, 23, 24, 25, 12, 13, 9, 0, 17, 26, 28, 27, 15, 16, 18, 14, 1, 11, 2, 3, 4, 5, 20, 6, 8, 10, 7, 29 },
    
    { 23, 24, 25, 22, 12, 13, 9, 0, 17, 16, 26, 28, 27, 15, 18, 21, 14, 1, 11, 2, 3, 4, 5, 19, 20, 6, 8, 10, 7, 29 },
      
    { 19, 22, 21, 24, 23, 25, 12, 13, 9, 0, 17, 16, 28, 26, 27, 15, 18, 14, 1, 11, 2, 3, 4, 5, 20, 6, 8, 10, 7, 29 }
};

void addRuleBasedCollTest(TestNode** root)
{
    addTest(root, &TestG7Locales, "tscoll/cg7coll/TestG7Locales");
    addTest(root, &TestDemo1, "tscoll/cg7coll/TestDemo1");
    addTest(root, &TestDemo2, "tscoll/cg7coll/TestDemo2");
    addTest(root, &TestDemo3, "tscoll/cg7coll/TestDemo3");
    addTest(root, &TestDemo4, "tscoll/cg7coll/TestDemo4");

    
}

static void TestG7Locales()
{
    UCollator *myCollation;
    UErrorCode status = U_ZERO_ERROR;
    const UChar *defRules;
    int32_t i, rlen, j, n;
    log_verbose("Testing  ucol_openRules for all the locales\n");
    for (i = 0; i < UPRV_LENGTHOF(locales); i++)
    {
        const char *locale = locales[i];
        status = U_ZERO_ERROR;
        myCollation = ucol_open(locale, &status);
        ucol_setAttribute(myCollation, UCOL_STRENGTH, UCOL_QUATERNARY, &status);
        ucol_setAttribute(myCollation, UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, &status);

        if (U_FAILURE(status))
        {
            log_err_status(status, "Error in creating collator in %s:  %s\n", locale, myErrorName(status));
            ucol_close(myCollation);
            continue;
        }

        defRules = ucol_getRules(myCollation, &rlen);
        if (rlen == 0 && (strcmp(locale, "fr_CA") == 0 || strcmp(locale, "ja_JP") == 0)) {
            log_data_err("%s UCollator missing rule string\n", locale);
            if (log_knownIssue("10671", "TestG7Locales does not test ignore-punctuation")) {
                ucol_close(myCollation);
                continue;
            }
        } else {
            UCollator *tblColl1;
            status = U_ZERO_ERROR;
            tblColl1 = ucol_openRules(defRules, rlen, UCOL_OFF,
                    UCOL_DEFAULT_STRENGTH,NULL, &status);
            ucol_close(myCollation);
            if (U_FAILURE(status))
            {
                log_err_status(status, "Error in creating collator in %s:  %s\n", locale, myErrorName(status));
                continue;
            }
            myCollation = tblColl1;
        }

        log_verbose("Locale  %s\n", locales[i]);
        log_verbose("  tests start...\n");

        j = 0;
        n = 0;
        for (j = 0; j < FIXEDTESTSET; j++)
        {
            for (n = j+1; n < FIXEDTESTSET; n++)
            {
                doTest(myCollation, testCases[results[i][j]], testCases[results[i][n]], UCOL_LESS);
            }
        }

        ucol_close(myCollation);
    }
}

static void TestDemo1()
{
    UCollator *myCollation;
    int32_t j, n;
    static const char rules[] = "& Z < p, P";
    int32_t len=(int32_t)strlen(rules);
    UChar temp[sizeof(rules)];
    UErrorCode status = U_ZERO_ERROR;
    u_uastrcpy(temp, rules);

    log_verbose("Demo Test 1 : Create a new table collation with rules \" & Z < p, P \" \n");

    myCollation = ucol_openRules(temp, len, UCOL_OFF, UCOL_DEFAULT_STRENGTH,NULL, &status);

    if (U_FAILURE(status))
    {
        log_err_status(status, "Demo Test 1 Rule collation object creation failed. : %s\n", myErrorName(status));
        return;
    }

    for (j = 0; j < FIXEDTESTSET; j++)
    {
        for (n = j+1; n < FIXEDTESTSET; n++)
        {
            doTest(myCollation, testCases[results[8][j]], testCases[results[8][n]], UCOL_LESS);
        }
    }

    ucol_close(myCollation); 
}

static void TestDemo2()
{
    UCollator *myCollation;
    int32_t j, n;
    static const char rules[] = "& C < ch , cH, Ch, CH";
    int32_t len=(int32_t)strlen(rules);
    UChar temp[sizeof(rules)];
    UErrorCode status = U_ZERO_ERROR;
    u_uastrcpy(temp, rules);

    log_verbose("Demo Test 2 : Create a new table collation with rules \"& C < ch , cH, Ch, CH\"");

    myCollation = ucol_openRules(temp, len, UCOL_OFF, UCOL_DEFAULT_STRENGTH, NULL, &status);

    if (U_FAILURE(status))
    {
        log_err_status(status, "Demo Test 2 Rule collation object creation failed.: %s\n", myErrorName(status));
        return;
    }
    for (j = 0; j < TOTALTESTSET; j++)
    {
        for (n = j+1; n < TOTALTESTSET; n++)
        {
            doTest(myCollation, testCases[results[9][j]], testCases[results[9][n]], UCOL_LESS);
        }
    }
    ucol_close(myCollation); 
}

static void TestDemo3()
{
    UCollator *myCollation;
    int32_t j, n;
    static const char rules[] = "& Question'-'mark ; '?' & Hash'-'mark ; '#' & Ampersand ; '&'";
    int32_t len=(int32_t)strlen(rules);
    UChar temp[sizeof(rules)];
    UErrorCode status = U_ZERO_ERROR;
    u_uastrcpy(temp, rules);

    log_verbose("Demo Test 3 : Create a new table collation with rules \"& Question'-'mark ; '?' & Hash'-'mark ; '#' & Ampersand ; '&'\" \n");

    myCollation = ucol_openRules(temp, len, UCOL_OFF, UCOL_DEFAULT_STRENGTH, NULL, &status);
    
    if (U_FAILURE(status))
    {
        log_err_status(status, "Demo Test 3 Rule collation object creation failed.: %s\n", myErrorName(status));
        return;
    }

    for (j = 0; j < TOTALTESTSET; j++)
    {
        for (n = j+1; n < TOTALTESTSET; n++)
        {
            doTest(myCollation, testCases[results[10][j]], testCases[results[10][n]], UCOL_LESS);
        }
    }
    ucol_close(myCollation); 
}

static void TestDemo4()
{
    UCollator *myCollation;
    int32_t j, n;
    static const char rules[] = " & aa ; a'-' & ee ; e'-' & ii ; i'-' & oo ; o'-' & uu ; u'-' ";
    int32_t len=(int32_t)strlen(rules);
    UChar temp[sizeof(rules)];
    UErrorCode status = U_ZERO_ERROR;
    u_uastrcpy(temp, rules);

    log_verbose("Demo Test 4 : Create a new table collation with rules \" & aa ; a'-' & ee ; e'-' & ii ; i'-' & oo ; o'-' & uu ; u'-' \"\n");

    myCollation = ucol_openRules(temp, len, UCOL_OFF, UCOL_DEFAULT_STRENGTH, NULL, &status);
    
    if (U_FAILURE(status))
    {
        log_err_status(status, "Demo Test 4 Rule collation object creation failed.: %s\n", myErrorName(status));
        return;
    }
    for (j = 0; j < TOTALTESTSET; j++)
    {
        for (n = j+1; n < TOTALTESTSET; n++)
        {
            doTest(myCollation, testCases[results[11][j]], testCases[results[11][n]], UCOL_LESS);
        }
    }
    ucol_close(myCollation); 
}

#endif 
