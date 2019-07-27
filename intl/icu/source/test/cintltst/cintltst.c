
















#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "unicode/utypes.h"
#include "unicode/putil.h"
#include "cstring.h"
#include "cintltst.h"
#include "uassert.h"
#include "cmemory.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/ucnv.h"
#include "unicode/ures.h"
#include "unicode/uclean.h"
#include "unicode/ucal.h"
#include "uoptions.h"
#include "putilimp.h" 
#ifdef URES_DEBUG
#include "uresimp.h" 
#endif

#ifdef XP_MAC_CONSOLE
#   include <console.h>
#endif

#define CTST_MAX_ALLOC 8192

static void * ctst_allocated_stuff[CTST_MAX_ALLOC] = {0};
static int ctst_allocated = 0;
static UBool ctst_free = FALSE;
static int ctst_allocated_total = 0;

#define CTST_LEAK_CHECK 1

#ifdef CTST_LEAK_CHECK
static void ctst_freeAll(void);
#endif

static char* _testDataPath=NULL;




void ctest_setICU_DATA(void);



#if UCONFIG_NO_LEGACY_CONVERSION
#   define TRY_CNV_1 "iso-8859-1"
#   define TRY_CNV_2 "ibm-1208"
#else
#   define TRY_CNV_1 "iso-8859-7"
#   define TRY_CNV_2 "sjis"
#endif

static int gOrigArgc;
static const char* const * gOrigArgv;

int main(int argc, const char* const argv[])
{
    int nerrors = 0;
    UBool   defaultDataFound;
    TestNode *root;
    const char *warnOrErr = "Failure"; 
    UDate startTime, endTime;
    int32_t diffTime;

    
    UErrorCode errorCode = U_ZERO_ERROR;
    UResourceBundle *rb;
    UConverter *cnv;

    U_MAIN_INIT_ARGS(argc, argv);

    startTime = uprv_getRawUTCtime();

    gOrigArgc = argc;
    gOrigArgv = argv;
    if (!initArgs(argc, argv, NULL, NULL)) {
        
        return -1;
    }
    
    







    defaultDataFound = TRUE;
    u_init(&errorCode);
    if (U_FAILURE(errorCode)) {
        fprintf(stderr,
            "#### Note:  ICU Init without build-specific setDataDirectory() failed. %s\n", u_errorName(errorCode));
        defaultDataFound = FALSE;
    }
    u_cleanup();
#ifdef URES_DEBUG
    fprintf(stderr, "After initial u_cleanup: RB cache %s empty.\n", ures_dumpCacheContents()?"WAS NOT":"was");
#endif

    while (getTestOption(REPEAT_TESTS_OPTION) > 0) {   

        if (!initArgs(argc, argv, NULL, NULL)) {
            
            return -1;
        }
        errorCode = U_ZERO_ERROR;

        
        if (!defaultDataFound) {
            ctest_setICU_DATA();    
        }
        u_init(&errorCode);
        if (U_FAILURE(errorCode)) {
            fprintf(stderr,
                "#### ERROR! %s: u_init() failed with status = \"%s\".\n" 
                "*** Check the ICU_DATA environment variable and \n"
                "*** check that the data files are present.\n", argv[0], u_errorName(errorCode));
                if(!getTestOption(WARN_ON_MISSING_DATA_OPTION)) {
                    fprintf(stderr, "*** Exiting.  Use the '-w' option if data files were\n*** purposely removed, to continue test anyway.\n");
                    u_cleanup();
                    return 1;
                }
        }
        


        
        cnv = ucnv_open(TRY_CNV_2, &errorCode);
        if(cnv != 0) {
            
            ucnv_close(cnv);
        } else {
            fprintf(stderr,
                    "*** %s! The converter for " TRY_CNV_2 " cannot be opened.\n"
                    "*** Check the ICU_DATA environment variable and \n"
                    "*** check that the data files are present.\n", warnOrErr);
            if(!getTestOption(WARN_ON_MISSING_DATA_OPTION)) {
                fprintf(stderr, "*** Exitting.  Use the '-w' option if data files were\n*** purposely removed, to continue test anyway.\n");
                u_cleanup();
                return 1;
            }
        }

        rb = ures_open(NULL, "en", &errorCode);
        if(U_SUCCESS(errorCode)) {
            
            ures_close(rb);
        } else {
            fprintf(stderr,
                    "*** %s! The \"en\" locale resource bundle cannot be opened.\n"
                    "*** Check the ICU_DATA environment variable and \n"
                    "*** check that the data files are present.\n", warnOrErr);
            if(!getTestOption(WARN_ON_MISSING_DATA_OPTION)) {
                fprintf(stderr, "*** Exitting.  Use the '-w' option if data files were\n*** purposely removed, to continue test anyway.\n");
                u_cleanup();
                return 1;
            }
        }

        errorCode = U_ZERO_ERROR;
        rb = ures_open(NULL, NULL, &errorCode);
        if(U_SUCCESS(errorCode)) {
            
            if (errorCode == U_USING_DEFAULT_WARNING || errorCode == U_USING_FALLBACK_WARNING) {
                fprintf(stderr,
                        "#### Note: The default locale %s is not available\n", uloc_getDefault());
            }
            ures_close(rb);
        } else {
            fprintf(stderr,
                    "*** %s! Can not open a resource bundle for the default locale %s\n", warnOrErr, uloc_getDefault());
            if(!getTestOption(WARN_ON_MISSING_DATA_OPTION)) {
                fprintf(stderr, "*** Exitting.  Use the '-w' option if data files were\n"
                    "*** purposely removed, to continue test anyway.\n");
                u_cleanup();
                return 1;
            }
        }
        fprintf(stdout, "Default locale for this run is %s\n", uloc_getDefault());

        

        root = NULL;
        addAllTests(&root);

        
        nerrors = runTestRequest(root, argc, argv);

        setTestOption(REPEAT_TESTS_OPTION, DECREMENT_OPTION_VALUE);
        if (getTestOption(REPEAT_TESTS_OPTION) > 0) {
            printf("Repeating tests %d more time(s)\n", getTestOption(REPEAT_TESTS_OPTION));
        }
        cleanUpTestTree(root);

#ifdef CTST_LEAK_CHECK
        ctst_freeAll();
        
        u_cleanup(); 
        
        if(getTestOption(VERBOSITY_OPTION) && ctst_allocated_total>0) {
          fprintf(stderr,"ctst_freeAll():  cleaned up after %d allocations (queue of %d)\n", ctst_allocated_total, CTST_MAX_ALLOC);
        }
#ifdef URES_DEBUG
        if(ures_dumpCacheContents()) {
          fprintf(stderr, "Error: After final u_cleanup, RB cache was not empty.\n");
          nerrors++;
        } else {
          fprintf(stderr,"OK: After final u_cleanup, RB cache was empty.\n");
        }
#endif
#endif

    }  

    endTime = uprv_getRawUTCtime();
    diffTime = (int32_t)(endTime - startTime);
    printf("Elapsed Time: %02d:%02d:%02d.%03d\n",
        (int)((diffTime%U_MILLIS_PER_DAY)/U_MILLIS_PER_HOUR),
        (int)((diffTime%U_MILLIS_PER_HOUR)/U_MILLIS_PER_MINUTE),
        (int)((diffTime%U_MILLIS_PER_MINUTE)/U_MILLIS_PER_SECOND),
        (int)(diffTime%U_MILLIS_PER_SECOND));

    return nerrors ? 1 : 0;
}









































const char *  ctest_dataSrcDir()
{
    static const char *dataSrcDir = NULL;

    if(dataSrcDir) {
        return dataSrcDir;
    }

    







#if defined (U_TOPSRCDIR)
    {
        dataSrcDir = U_TOPSRCDIR  U_FILE_SEP_STRING "data" U_FILE_SEP_STRING;
    }
#else

    



    {
        static char p[sizeof(__FILE__) + 20];
        char *pBackSlash;
        int i;

        strcpy(p, __FILE__);
        
        
        for (i=1; i<=3; i++) {
            pBackSlash = strrchr(p, U_FILE_SEP_CHAR);
            if (pBackSlash != NULL) {
                *pBackSlash = 0;        
            }
        }

        if (pBackSlash != NULL) {
            


            strcpy(pBackSlash, U_FILE_SEP_STRING "data" U_FILE_SEP_STRING );
            dataSrcDir = p;
        }
        else {
            
            FILE *file = fopen(".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "Makefile.in", "r");
            if (file) {
                fclose(file);
                dataSrcDir = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING "data" U_FILE_SEP_STRING;
            }
            else {
                dataSrcDir = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING "data" U_FILE_SEP_STRING;
            }
        }
    }
#endif

    return dataSrcDir;

}


const char *ctest_dataOutDir()
{
    static const char *dataOutDir = NULL;

    if(dataOutDir) {
        return dataOutDir;
    }

    







#if defined (U_TOPBUILDDIR)
    {
        dataOutDir = U_TOPBUILDDIR "data"U_FILE_SEP_STRING"out"U_FILE_SEP_STRING;
    }
#else

    



    {
        static char p[sizeof(__FILE__) + 20];
        char *pBackSlash;
        int i;

        strcpy(p, __FILE__);
        
        
        for (i=1; i<=3; i++) {
            pBackSlash = strrchr(p, U_FILE_SEP_CHAR);
            if (pBackSlash != NULL) {
                *pBackSlash = 0;        
            }
        }

        if (pBackSlash != NULL) {
            


            strcpy(pBackSlash, U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "out" U_FILE_SEP_STRING);
            dataOutDir = p;
        }
        else {
            
            FILE *file = fopen(".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "Makefile.in", "r");
            if (file) {
                fclose(file);
                dataOutDir = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "out" U_FILE_SEP_STRING;
            }
            else {
                dataOutDir = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING "data" U_FILE_SEP_STRING "out" U_FILE_SEP_STRING;
            }
        }
    }
#endif

    return dataOutDir;
}










void ctest_setICU_DATA() {

    


    if (getenv("ICU_DATA") == NULL) {
        
        u_setDataDirectory(ctest_dataOutDir());
    }
}





static char *safeGetICUDataDirectory() {
    const char *dataDir = u_getDataDirectory();  
    char *retStr = NULL;
    if (dataDir != NULL) {
        retStr = (char *)malloc(strlen(dataDir)+1);
        strcpy(retStr, dataDir);
    }
    return retStr;
}

UBool ctest_resetICU() {
    UErrorCode   status = U_ZERO_ERROR;
    char         *dataDir = safeGetICUDataDirectory();

    u_cleanup();
    if (!initArgs(gOrigArgc, gOrigArgv, NULL, NULL)) {
        
        return FALSE;
    }
    u_setDataDirectory(dataDir);
    free(dataDir);
    u_init(&status);
    if (U_FAILURE(status)) {
        log_err_status(status, "u_init failed with %s\n", u_errorName(status));
        return FALSE;
    }
    return TRUE;
}

UChar* CharsToUChars(const char* str) {
    
    int32_t len = u_unescape(str, 0, 0); 
    
    UChar *buf = (UChar*) malloc(sizeof(UChar) * (len + 1));
    u_unescape(str, buf, len + 1);
    return buf;
}

char *austrdup(const UChar* unichars)
{
    int   length;
    char *newString;

    length    = u_strlen ( unichars );
     
    newString = (char*)ctst_malloc  ( sizeof( char ) * 4 * ( length + 1 ) ); 

    if ( newString == NULL )
        return NULL;

    u_austrcpy ( newString, unichars );

    return newString;
}

char *aescstrdup(const UChar* unichars,int32_t length){
    char *newString,*targetLimit,*target;
    UConverterFromUCallback cb;
    const void *p;
    UErrorCode errorCode = U_ZERO_ERROR;
#if U_CHARSET_FAMILY==U_EBCDIC_FAMILY
#   if U_PLATFORM == U_PF_OS390
        static const char convName[] = "ibm-1047";
#   else
        static const char convName[] = "ibm-37";
#   endif
#else
    static const char convName[] = "US-ASCII";
#endif
    UConverter* conv = ucnv_open(convName, &errorCode);
    if(length==-1){
        length = u_strlen( unichars);
    }
    newString = (char*)ctst_malloc ( sizeof(char) * 8 * (length +1));
    target = newString;
    targetLimit = newString+sizeof(char) * 8 * (length +1);
    ucnv_setFromUCallBack(conv, UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_C, &cb, &p, &errorCode);
    ucnv_fromUnicode(conv,&target,targetLimit, &unichars, (UChar*)(unichars+length),NULL,TRUE,&errorCode);
    ucnv_close(conv);
    *target = '\0';
    return newString;
}

const char* loadTestData(UErrorCode* err){
    if( _testDataPath == NULL){
        const char*      directory=NULL;
        UResourceBundle* test =NULL;
        char* tdpath=NULL;
        const char* tdrelativepath;
#if defined (U_TOPBUILDDIR)
        tdrelativepath = "test"U_FILE_SEP_STRING"testdata"U_FILE_SEP_STRING"out"U_FILE_SEP_STRING;
        directory = U_TOPBUILDDIR;
#else
        tdrelativepath = ".."U_FILE_SEP_STRING".."U_FILE_SEP_STRING"test"U_FILE_SEP_STRING"testdata"U_FILE_SEP_STRING"out"U_FILE_SEP_STRING;
        directory= ctest_dataOutDir();
#endif

        tdpath = (char*) ctst_malloc(sizeof(char) *(( strlen(directory) * strlen(tdrelativepath)) + 10));


        





        strcpy(tdpath, directory);
        strcat(tdpath, tdrelativepath);
        strcat(tdpath,"testdata");


        test=ures_open(tdpath, "testtypes", err);

        
        if(U_FAILURE(*err)){
            *err = U_FILE_ACCESS_ERROR;
            log_data_err("Could not load testtypes.res in testdata bundle with path %s - %s\n", tdpath, u_errorName(*err));
            return "";
        }
        ures_close(test);
        _testDataPath = tdpath;
        return _testDataPath;
    }
    return _testDataPath;
}

#define CTEST_MAX_TIMEZONE_SIZE 256
static UChar gOriginalTimeZone[CTEST_MAX_TIMEZONE_SIZE] = {0};






U_CFUNC void ctest_setTimeZone(const char *optionalTimeZone, UErrorCode *status) {
#if !UCONFIG_NO_FORMATTING
    UChar zoneID[CTEST_MAX_TIMEZONE_SIZE];

    if (optionalTimeZone == NULL) {
        optionalTimeZone = "America/Los_Angeles";
    }
    if (gOriginalTimeZone[0]) {
        log_data_err("*** Error: time zone saved twice. New value will be %s (Are you missing data?)\n",
               optionalTimeZone);
    }
    ucal_getDefaultTimeZone(gOriginalTimeZone, CTEST_MAX_TIMEZONE_SIZE, status);
    if (U_FAILURE(*status)) {
        log_err("*** Error: Failed to save default time zone: %s\n",
               u_errorName(*status));
        *status = U_ZERO_ERROR;
    }

    u_uastrncpy(zoneID, optionalTimeZone, CTEST_MAX_TIMEZONE_SIZE-1);
    zoneID[CTEST_MAX_TIMEZONE_SIZE-1] = 0;
    ucal_setDefaultTimeZone(zoneID, status);
    if (U_FAILURE(*status)) {
        log_err("*** Error: Failed to set default time zone to \"%s\": %s\n",
               optionalTimeZone, u_errorName(*status));
    }
#endif
}




U_CFUNC void ctest_resetTimeZone(void) {
#if !UCONFIG_NO_FORMATTING
    UErrorCode status = U_ZERO_ERROR;

    ucal_setDefaultTimeZone(gOriginalTimeZone, &status);
    if (U_FAILURE(status)) {
        log_err("*** Error: Failed to reset default time zone: %s\n",
               u_errorName(status));
    }
    
    gOriginalTimeZone[0] = 0;
#endif
}


void *ctst_malloc(size_t size) {
  ctst_allocated_total++;
    if(ctst_allocated >= CTST_MAX_ALLOC - 1) {
        ctst_allocated = 0;
        ctst_free = TRUE;
    }
    if(ctst_allocated_stuff[ctst_allocated]) {
        free(ctst_allocated_stuff[ctst_allocated]);
    }
    return ctst_allocated_stuff[ctst_allocated++] = malloc(size);
}

#ifdef CTST_LEAK_CHECK
static void ctst_freeAll() {
    int i;
    if(ctst_free == FALSE) { 
        for(i=0; i<ctst_allocated; i++) {
            free(ctst_allocated_stuff[i]);
            ctst_allocated_stuff[i] = NULL;
        }
    } else { 
        for(i=0; i<CTST_MAX_ALLOC; i++) {
            free(ctst_allocated_stuff[i]);
            ctst_allocated_stuff[i] = NULL;
        }
    }
    ctst_allocated = 0;
    _testDataPath=NULL;
}

#define VERBOSE_ASSERTIONS

U_CFUNC UBool assertSuccessCheck(const char* msg, UErrorCode* ec, UBool possibleDataError) {
    U_ASSERT(ec!=NULL);
    if (U_FAILURE(*ec)) {
        if (possibleDataError) {
            log_data_err("FAIL: %s (%s)\n", msg, u_errorName(*ec));
        } else {
            log_err_status(*ec, "FAIL: %s (%s)\n", msg, u_errorName(*ec));
        }
        return FALSE;
    }
    return TRUE;
}

U_CFUNC UBool assertSuccess(const char* msg, UErrorCode* ec) {
    U_ASSERT(ec!=NULL);
    return assertSuccessCheck(msg, ec, FALSE);
}



U_CFUNC UBool assertTrue(const char* msg, int  condition) { 
    if (!condition) {
        log_err("FAIL: assertTrue() failed: %s\n", msg);
    }
#ifdef VERBOSE_ASSERTIONS
    else {
        log_verbose("Ok: %s\n", msg);
    }
#endif
    return (UBool)condition;   
}

U_CFUNC UBool assertEquals(const char* message, const char* expected,
                           const char* actual) {
    if (uprv_strcmp(expected, actual) != 0) {
        log_err("FAIL: %s; got \"%s\"; expected \"%s\"\n",
                message, actual, expected);
        return FALSE;
    }
#ifdef VERBOSE_ASSERTIONS
    else {
        log_verbose("Ok: %s; got \"%s\"\n", message, actual);
    }
#endif
    return TRUE;
}

#endif
