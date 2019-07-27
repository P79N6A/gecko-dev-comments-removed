
























#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING
#include "unicode/udat.h"
#include "unicode/udatpg.h"
#include "unicode/ustring.h"
#include "cintltst.h"

void addDateTimePatternGeneratorTest(TestNode** root);

#define TESTCASE(x) addTest(root, &x, "tsformat/udatpg_test/" #x)

static void TestOpenClose(void);
static void TestUsage(void);
static void TestBuilder(void);
static void TestOptions(void);

void addDateTimePatternGeneratorTest(TestNode** root) {
    TESTCASE(TestOpenClose);
    TESTCASE(TestUsage);
    TESTCASE(TestBuilder);
    TESTCASE(TestOptions);
}





static const UChar pipeString[]={ 0x7c, 0x0a };

static const UChar testSkeleton1[]={ 0x48, 0x48, 0x6d, 0x6d, 0 }; 
static const UChar expectingBestPattern[]={ 0x48, 0x2e, 0x6d, 0x6d, 0 }; 
static const UChar testPattern[]={ 0x48, 0x48, 0x3a, 0x6d, 0x6d, 0 }; 
static const UChar expectingSkeleton[]= { 0x48, 0x48, 0x6d, 0x6d, 0 }; 
static const UChar expectingBaseSkeleton[]= { 0x48, 0x6d, 0 }; 
static const UChar redundantPattern[]={ 0x79, 0x79, 0x4d, 0x4d, 0x4d, 0 }; 
static const UChar testFormat[]= {0x7B, 0x31, 0x7D, 0x20, 0x7B, 0x30, 0x7D, 0};  
static const UChar appendItemName[]= {0x68, 0x72, 0};  
static const UChar testPattern2[]={ 0x48, 0x48, 0x3a, 0x6d, 0x6d, 0x20, 0x76, 0 }; 
static const UChar replacedStr[]={ 0x76, 0x76, 0x76, 0x76, 0 }; 

static const UChar resultBaseSkeletons[2][10] = {{0x48,0x6d, 0x76, 0}, {0x79, 0x4d, 0x4d, 0x4d, 0 } };
static const UChar sampleFormatted[] = {0x31, 0x30, 0x20, 0x6A, 0x75, 0x69, 0x6C, 0x2E, 0}; 
static const UChar skeleton[]= {0x4d, 0x4d, 0x4d, 0x64, 0};  
static const UChar timeZoneGMT[] = { 0x0047, 0x004d, 0x0054, 0x0000 };  

static void TestOpenClose() {
    UErrorCode errorCode=U_ZERO_ERROR;
    UDateTimePatternGenerator *dtpg, *dtpg2;
    const UChar *s;
    int32_t length;

    
    dtpg=udatpg_open(NULL, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err_status(errorCode, "udatpg_open(NULL) failed - %s\n", u_errorName(errorCode));
        return;
    }
    udatpg_close(dtpg);

    
    dtpg=udatpg_open("de", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("udatpg_open(de) failed - %s\n", u_errorName(errorCode));
        return;
    }

    
    udatpg_setDecimal(dtpg, pipeString, 1);

    
    dtpg2=udatpg_clone(dtpg, &errorCode);
    if(U_FAILURE(errorCode) || dtpg2==NULL) {
        log_err("udatpg_clone() failed - %s\n", u_errorName(errorCode));
        return;
    }

    
    s=udatpg_getDecimal(dtpg2, &length);
    if(s==pipeString || length!=1 || 0!=u_memcmp(s, pipeString, length) || s[length]!=0) { 
        log_err("udatpg_getDecimal(cloned object) did not return the expected string\n");
        return;
    }

    udatpg_close(dtpg);
    udatpg_close(dtpg2);
}

typedef struct {
    UDateTimePatternField field;
    UChar name[12];
} AppendItemNameData;

static const AppendItemNameData appendItemNameData[] = { 
    { UDATPG_YEAR_FIELD,    {0x0076,0x0075,0x006F,0x0073,0x0069,0} }, 
    { UDATPG_MONTH_FIELD,   {0x006B,0x0075,0x0075,0x006B,0x0061,0x0075,0x0073,0x0069,0} }, 
    { UDATPG_WEEKDAY_FIELD, {0x0076,0x0069,0x0069,0x006B,0x006F,0x006E,0x0070,0x00E4,0x0069,0x0076,0x00E4,0} },
    { UDATPG_DAY_FIELD,     {0x0070,0x00E4,0x0069,0x0076,0x00E4,0} },
    { UDATPG_HOUR_FIELD,    {0x0074,0x0075,0x006E,0x0074,0x0069,0} }, 
    { UDATPG_FIELD_COUNT,   {0}        }  
};

static void TestUsage() {
    UErrorCode errorCode=U_ZERO_ERROR;
    UDateTimePatternGenerator *dtpg;
    const AppendItemNameData * appItemNameDataPtr;
    UChar bestPattern[20];
    UChar result[20];
    int32_t length;    
    UChar *s;
    const UChar *r;
    
    dtpg=udatpg_open("fi", &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err_status(errorCode, "udatpg_open(fi) failed - %s\n", u_errorName(errorCode));
        return;
    }
    length = udatpg_getBestPattern(dtpg, testSkeleton1, 4,
                                   bestPattern, 20, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("udatpg_getBestPattern failed - %s\n", u_errorName(errorCode));
        return;
    }
    if((u_memcmp(bestPattern, expectingBestPattern, length)!=0) || bestPattern[length]!=0) { 
        log_err("udatpg_getBestPattern did not return the expected string\n");
        return;
    }
    
    
    
    s=NULL;
    length = udatpg_getBestPattern(dtpg, s, 0, bestPattern, 20, &errorCode);
    if(!U_FAILURE(errorCode)&&(length!=0) ) {
        log_err("udatpg_getBestPattern failed in illegal argument - skeleton is NULL.\n");
        return;
    }
    
    
    length = udatpg_getSkeleton(dtpg, testPattern, 5, result, 20,  &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("udatpg_getSkeleton failed - %s\n", u_errorName(errorCode));
        return;
    }
    if((u_memcmp(result, expectingSkeleton, length)!=0) || result[length]!=0) { 
        log_err("udatpg_getSkeleton did not return the expected string\n");
        return;
    }
    
    
    s=NULL;
    length = udatpg_getSkeleton(dtpg, s, 0, result, 20, &errorCode);
    if(!U_FAILURE(errorCode)&&(length!=0) ) {
        log_err("udatpg_getSkeleton failed in illegal argument - pattern is NULL.\n");
        return;
    }    
    
    
    length = udatpg_getBaseSkeleton(dtpg, testPattern, 5, result, 20,  &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("udatpg_getBaseSkeleton failed - %s\n", u_errorName(errorCode));
        return;
    }
    if((u_memcmp(result, expectingBaseSkeleton, length)!=0) || result[length]!=0) { 
        log_err("udatpg_getBaseSkeleton did not return the expected string\n");
        return;
    }
    
    
    s=NULL;
    length = udatpg_getBaseSkeleton(dtpg, s, 0, result, 20, &errorCode);
    if(!U_FAILURE(errorCode)&&(length!=0) ) {
        log_err("udatpg_getBaseSkeleton failed in illegal argument - pattern is NULL.\n");
        return;
    }
    
    
    udatpg_setAppendItemFormat( dtpg, UDATPG_MONTH_FIELD, testFormat, 7 );
    r = udatpg_getAppendItemFormat(dtpg, UDATPG_MONTH_FIELD, &length);
    
    
    if(length!=7 || 0!=u_memcmp(r, testFormat, length) || r[length]!=0) { 
        log_err("udatpg_setAppendItemFormat did not return the expected string\n");
        return;
    }
    
    for (appItemNameDataPtr = appendItemNameData; appItemNameDataPtr->field <  UDATPG_FIELD_COUNT; appItemNameDataPtr++) {
        int32_t nameLength;
        const UChar * namePtr = udatpg_getAppendItemName(dtpg, appItemNameDataPtr->field, &nameLength);
        if ( namePtr == NULL || u_strncmp(appItemNameDataPtr->name, namePtr, nameLength) != 0 ) {
            log_err("udatpg_getAppendItemName returns invalid name for field %d\n", (int)appItemNameDataPtr->field);
        }
    }
    
    
    udatpg_setAppendItemName( dtpg, UDATPG_HOUR_FIELD, appendItemName, 7 );
    r = udatpg_getAppendItemName(dtpg, UDATPG_HOUR_FIELD, &length);
    
    if(length!=7 || 0!=u_memcmp(r, appendItemName, length) || r[length]!=0) { 
        log_err("udatpg_setAppendItemName did not return the expected string\n");
        return;
    }
    
    
    udatpg_setDateTimeFormat( dtpg, testFormat, 7 );
    r = udatpg_getDateTimeFormat(dtpg, &length);
    
    if(length!=7 || 0!=u_memcmp(r, testFormat, length) || r[length]!=0) { 
        log_err("udatpg_setDateTimeFormat did not return the expected string\n");
        return;
    }
    udatpg_close(dtpg);
}

static void TestBuilder() {
    UErrorCode errorCode=U_ZERO_ERROR;
    UDateTimePatternGenerator *dtpg;
    UDateTimePatternConflict conflict;
    UEnumeration *en;
    UChar result[20];
    int32_t length, pLength;  
    const UChar *s, *p;
    const UChar* ptrResult[2]; 
    int32_t count=0;
    UDateTimePatternGenerator *generator;
    int32_t formattedCapacity, resultLen,patternCapacity ;
    UChar   pattern[40], formatted[40];
    UDateFormat *formatter;
    UDate sampleDate = 837039928046.0;
    static const char locale[]= "fr";
    UErrorCode status=U_ZERO_ERROR;
    
    
    dtpg=udatpg_openEmpty(&errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("udatpg_openEmpty() failed - %s\n", u_errorName(errorCode));
        return;
    }
    
    
    conflict = udatpg_addPattern(dtpg, redundantPattern, 5, FALSE, result, 20, 
                                 &length, &errorCode);
    if(U_FAILURE(errorCode)) {
        log_err("udatpg_addPattern() failed - %s\n", u_errorName(errorCode));
        return;
    }
    
    conflict = udatpg_addPattern(dtpg, redundantPattern, 5, FALSE, result, 20,
                                 &length, &errorCode);
    if(conflict == UDATPG_NO_CONFLICT) {
        log_err("udatpg_addPattern() failed to find the duplicate pattern.\n");
        return;
    }
    
    s=NULL;
    length = udatpg_addPattern(dtpg, s, 0, FALSE, result, 20,
                               &length, &errorCode);
    if(!U_FAILURE(errorCode)&&(length!=0) ) {
        log_err("udatpg_addPattern failed in illegal argument - pattern is NULL.\n");
        return;
    }

    
    errorCode=U_ZERO_ERROR;
    conflict = udatpg_addPattern(dtpg, testPattern2, 7, FALSE, result, 20,
                                 &length, &errorCode);
    if((conflict != UDATPG_NO_CONFLICT)||U_FAILURE(errorCode)) {
        log_err("udatpg_addPattern() failed to add HH:mm v. - %s\n", u_errorName(errorCode));
        return;
    }
    length = udatpg_replaceFieldTypes(dtpg, testPattern2, 7, replacedStr, 4,
                                      result, 20, &errorCode);
    if (U_FAILURE(errorCode) || (length==0) ) {
        log_err("udatpg_replaceFieldTypes failed!\n");
        return;
    }
    
    
    ptrResult[0] = testPattern2;
    ptrResult[1] = redundantPattern; 
    count=0;
    en = udatpg_openSkeletons(dtpg, &errorCode);  
    if (U_FAILURE(errorCode) || (length==0) ) {
        log_err("udatpg_openSkeletons failed!\n");
        return;
    }
    while ( (s=uenum_unext(en, &length, &errorCode))!= NULL) {
        p = udatpg_getPatternForSkeleton(dtpg, s, length, &pLength);
        if (U_FAILURE(errorCode) || p==NULL || u_memcmp(p, ptrResult[count], pLength)!=0 ) {
            log_err("udatpg_getPatternForSkeleton failed!\n");
            return;
        }
        count++;
    }
    uenum_close(en);
    
    
    en = udatpg_openBaseSkeletons(dtpg, &errorCode);
    count=0;
    while ( (s=uenum_unext(en, &length, &errorCode))!= NULL) {
        p = udatpg_getPatternForSkeleton(dtpg, s, length, &pLength);
        if (U_FAILURE(errorCode) || p==NULL || u_memcmp(p, resultBaseSkeletons[count], pLength)!=0 ) {
            log_err("udatpg_getPatternForSkeleton failed!\n");
            return;
        }
        count++;
    }
    if (U_FAILURE(errorCode) || (length==0) ) {
        log_err("udatpg_openSkeletons failed!\n");
        return;
    }
    uenum_close(en);
    
    udatpg_close(dtpg);
    
    
    patternCapacity = (int32_t)(sizeof(pattern)/sizeof((pattern)[0]));
    status=U_ZERO_ERROR;
    generator=udatpg_open(locale, &status);
    if(U_FAILURE(status)) {
        return;
    }

    
    length = udatpg_getBestPattern(generator, skeleton, 4,
                                   pattern, patternCapacity, &status);
    formatter = udat_open(UDAT_PATTERN, UDAT_PATTERN, locale, timeZoneGMT, -1,
                          pattern, length, &status);
    if (formatter==NULL) {
        log_err("Failed to initialize the UDateFormat of the sample code in Userguide.\n");
        udatpg_close(generator);
        return;
    }

    
    formattedCapacity = (int32_t)(sizeof(formatted)/sizeof((formatted)[0]));
    resultLen=udat_format(formatter, ucal_getNow(), formatted, formattedCapacity,
                          NULL, &status);
    

    
    resultLen=udat_format(formatter, sampleDate, formatted, formattedCapacity,
                          NULL, &status);
    if ( u_memcmp(sampleFormatted, formatted, resultLen) != 0 ) {
        log_err("Failed udat_format() of sample code in Userguide.\n");
    }
    udatpg_close(generator);
    udat_close(formatter);
}

typedef struct DTPtnGenOptionsData {
    const char *                    locale;
    const UChar *                   skel;
    UDateTimePatternMatchOptions    options;
    const UChar *                   expectedPattern;
} DTPtnGenOptionsData;
enum { kTestOptionsPatLenMax = 32 };

static const UChar skel_Hmm[]     = { 0x0048, 0x006D, 0x006D, 0 };
static const UChar skel_HHmm[]    = { 0x0048, 0x0048, 0x006D, 0x006D, 0 };
static const UChar skel_hhmm[]    = { 0x0068, 0x0068, 0x006D, 0x006D, 0 };
static const UChar patn_hcmm_a[]  = { 0x0068, 0x003A, 0x006D, 0x006D, 0x0020, 0x0061, 0 }; 
static const UChar patn_HHcmm[]   = { 0x0048, 0x0048, 0x003A, 0x006D, 0x006D, 0 }; 
static const UChar patn_hhcmm_a[] = { 0x0068, 0x0068, 0x003A, 0x006D, 0x006D, 0x0020, 0x0061, 0 }; 
static const UChar patn_HHpmm[]   = { 0x0048, 0x0048, 0x002E, 0x006D, 0x006D, 0 }; 
static const UChar patn_hpmm_a[]  = { 0x0068, 0x002E, 0x006D, 0x006D, 0x0020, 0x0061, 0 }; 
static const UChar patn_Hpmm[]    = { 0x0048, 0x002E, 0x006D, 0x006D, 0 }; 
static const UChar patn_hhpmm_a[] = { 0x0068, 0x0068, 0x002E, 0x006D, 0x006D, 0x0020, 0x0061, 0 }; 

static void TestOptions() {
    const DTPtnGenOptionsData testData[] = {
        
        { "en", skel_Hmm,  UDATPG_MATCH_NO_OPTIONS,        patn_HHcmm   },
        { "en", skel_HHmm, UDATPG_MATCH_NO_OPTIONS,        patn_HHcmm   },
        { "en", skel_hhmm, UDATPG_MATCH_NO_OPTIONS,        patn_hcmm_a  },
        { "en", skel_Hmm,  UDATPG_MATCH_HOUR_FIELD_LENGTH, patn_HHcmm   },
        { "en", skel_HHmm, UDATPG_MATCH_HOUR_FIELD_LENGTH, patn_HHcmm   },
        { "en", skel_hhmm, UDATPG_MATCH_HOUR_FIELD_LENGTH, patn_hhcmm_a },
        { "be", skel_Hmm,  UDATPG_MATCH_NO_OPTIONS,        patn_HHpmm   },
        { "be", skel_HHmm, UDATPG_MATCH_NO_OPTIONS,        patn_HHpmm   },
        { "be", skel_hhmm, UDATPG_MATCH_NO_OPTIONS,        patn_hpmm_a  },
        { "be", skel_Hmm,  UDATPG_MATCH_HOUR_FIELD_LENGTH, patn_Hpmm    },
        { "be", skel_HHmm, UDATPG_MATCH_HOUR_FIELD_LENGTH, patn_HHpmm   },
        { "be", skel_hhmm, UDATPG_MATCH_HOUR_FIELD_LENGTH, patn_hhpmm_a },
    };

    int count = sizeof(testData) / sizeof(testData[0]);
    const DTPtnGenOptionsData * testDataPtr = testData;

    for (; count-- > 0; ++testDataPtr) {
        UErrorCode status = U_ZERO_ERROR;
        UDateTimePatternGenerator * dtpgen = udatpg_open(testDataPtr->locale, &status);
        if ( U_SUCCESS(status) ) {
            UChar pattern[kTestOptionsPatLenMax];
            int32_t patLen = udatpg_getBestPatternWithOptions(dtpgen, testDataPtr->skel, -1,
                                                              testDataPtr->options, pattern,
                                                              kTestOptionsPatLenMax, &status);
            if ( U_FAILURE(status) || u_strncmp(pattern, testDataPtr->expectedPattern, patLen+1) != 0 ) {
                char skelBytes[kTestOptionsPatLenMax];
                char expectedPatternBytes[kTestOptionsPatLenMax];
                char patternBytes[kTestOptionsPatLenMax];
                log_err("ERROR udatpg_getBestPatternWithOptions, locale %s, skeleton %s, options 0x%04X, expected pattern %s, got %s, status %d\n",
                        testDataPtr->locale, u_austrncpy(skelBytes,testDataPtr->skel,kTestOptionsPatLenMax), testDataPtr->options,
                        u_austrncpy(expectedPatternBytes,testDataPtr->expectedPattern,kTestOptionsPatLenMax),
                        u_austrncpy(patternBytes,pattern,kTestOptionsPatLenMax), status );
            }
            udatpg_close(dtpgen);
        } else {
            log_data_err("ERROR udatpg_open failed for locale %s : %s - (Are you missing data?)\n", testDataPtr->locale, myErrorName(status));
        }
    }
}

#endif
