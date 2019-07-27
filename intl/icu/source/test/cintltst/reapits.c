


















#include "unicode/utypes.h"

#if !UCONFIG_NO_REGULAR_EXPRESSIONS

#include <stdlib.h>
#include <string.h>
#include "unicode/uloc.h"
#include "unicode/uregex.h"
#include "unicode/ustring.h"
#include "unicode/utext.h"
#include "cintltst.h"
#include "cmemory.h"

#define TEST_ASSERT_SUCCESS(status) {if (U_FAILURE(status)) { \
log_data_err("Failure at file %s:%d - error = %s (Are you missing data?)\n", __FILE__, __LINE__, u_errorName(status));}}

#define TEST_ASSERT(expr) {if ((expr)==FALSE) { \
log_err("Test Failure at file %s:%d - ASSERT(%s) failed.\n", __FILE__, __LINE__, #expr);}}












#define TEST_SETUP(pattern, testString, flags) {  \
    UChar   *srcString = NULL;  \
    status = U_ZERO_ERROR; \
    re = uregex_openC(pattern, flags, NULL, &status);  \
    TEST_ASSERT_SUCCESS(status);   \
    srcString = (UChar *)malloc((strlen(testString)+2)*sizeof(UChar)); \
    u_uastrncpy(srcString, testString,  strlen(testString)+1); \
    uregex_setText(re, srcString, -1, &status); \
    TEST_ASSERT_SUCCESS(status);  \
    if (U_SUCCESS(status)) {
    
#define TEST_TEARDOWN  \
    }  \
    TEST_ASSERT_SUCCESS(status);  \
    uregex_close(re);  \
    free(srcString);   \
    }





static void test_assert_string(const char *expected, const UChar *actual, UBool nulTerm, const char *file, int line) {
     char     buf_inside_macro[120];
     int32_t  len = (int32_t)strlen(expected);
     UBool    success;
     if (nulTerm) {
         u_austrncpy(buf_inside_macro, (actual), len+1);
         buf_inside_macro[len+2] = 0;
         success = (strcmp((expected), buf_inside_macro) == 0);
     } else {
         u_austrncpy(buf_inside_macro, (actual), len);
         buf_inside_macro[len+1] = 0;
         success = (strncmp((expected), buf_inside_macro, len) == 0);
     }
     if (success == FALSE) {
         log_err("Failure at file %s, line %d, expected \"%s\", got \"%s\"\n",
             file, line, (expected), buf_inside_macro);
     }
}

#define TEST_ASSERT_STRING(expected, actual, nulTerm) test_assert_string(expected, actual, nulTerm, __FILE__, __LINE__)
             

static UBool equals_utf8_utext(const char *utf8, UText *utext) {
    int32_t u8i = 0;
    UChar32 u8c = 0;
    UChar32 utc = 0;
    UBool   stringsEqual = TRUE;
    utext_setNativeIndex(utext, 0);
    for (;;) {
        U8_NEXT_UNSAFE(utf8, u8i, u8c);
        utc = utext_next32(utext);
        if (u8c == 0 && utc == U_SENTINEL) {
            break;
        }
        if (u8c != utc || u8c == 0) {
            stringsEqual = FALSE;
            break;
        }
    }
    return stringsEqual;
}


static void test_assert_utext(const char *expected, UText *actual, const char *file, int line) {
    utext_setNativeIndex(actual, 0);
    if (!equals_utf8_utext(expected, actual)) {
        UChar32 c;
        log_err("Failure at file %s, line %d, expected \"%s\", got \"", file, line, expected);
        c = utext_next32From(actual, 0);
        while (c != U_SENTINEL) {
            if (0x20<c && c <0x7e) {
                log_err("%c", c);
            } else {
                log_err("%#x", c);
            }
            c = UTEXT_NEXT32(actual);
        }
        log_err("\"\n");
    }
}





#define TEST_ASSERT_UTEXT(expected, actual) test_assert_utext(expected, actual, __FILE__, __LINE__)

static UBool testUTextEqual(UText *uta, UText *utb) {
    UChar32 ca = 0;
    UChar32 cb = 0;
    utext_setNativeIndex(uta, 0);
    utext_setNativeIndex(utb, 0);
    do {
        ca = utext_next32(uta);
        cb = utext_next32(utb);
        if (ca != cb) {
            break;
        }
    } while (ca != U_SENTINEL);
    return ca == cb;
}

    


static void TestRegexCAPI(void);
static void TestBug4315(void);
static void TestUTextAPI(void);
static void TestRefreshInput(void);
static void TestBug8421(void);
static void TestBug10815(void);

void addURegexTest(TestNode** root);

void addURegexTest(TestNode** root)
{
    addTest(root, &TestRegexCAPI, "regex/TestRegexCAPI");
    addTest(root, &TestBug4315,   "regex/TestBug4315");
    addTest(root, &TestUTextAPI,  "regex/TestUTextAPI");
    addTest(root, &TestRefreshInput, "regex/TestRefreshInput");
    addTest(root, &TestBug8421,   "regex/TestBug8421");
    addTest(root, &TestBug10815,   "regex/TestBug10815");
}






typedef struct callBackContext {
    int32_t          maxCalls;
    int32_t          numCalls;
    int32_t          lastSteps;
} callBackContext;

static UBool U_EXPORT2 U_CALLCONV
TestCallbackFn(const void *context, int32_t steps) {
  callBackContext  *info = (callBackContext *)context;
  if (info->lastSteps+1 != steps) {
      log_err("incorrect steps in callback.  Expected %d, got %d\n", info->lastSteps+1, steps);
  }
  info->lastSteps = steps;
  info->numCalls++;
  return (info->numCalls < info->maxCalls);
}




static void TestRegexCAPI(void) {
    UErrorCode           status = U_ZERO_ERROR;
    URegularExpression  *re;
    UChar                pat[200];
    UChar               *minus1;

    memset(&minus1, -1, sizeof(minus1));

    
    u_uastrncpy(pat, "abc*", UPRV_LENGTHOF(pat));
    re = uregex_open(pat, -1, 0, 0, &status);
    if (U_FAILURE(status)) {
         log_data_err("Failed to open regular expression, %s:%d, error is \"%s\" (Are you missing data?)\n", __FILE__, __LINE__, u_errorName(status));
         return;
    }
    uregex_close(re);

    
    status = U_ZERO_ERROR;
    re = uregex_open(pat, -1, 
        UREGEX_CASE_INSENSITIVE | UREGEX_COMMENTS | UREGEX_DOTALL | UREGEX_MULTILINE | UREGEX_UWORD | UREGEX_LITERAL,
        0, &status);
    TEST_ASSERT_SUCCESS(status);
    uregex_close(re);

    
    status = U_ZERO_ERROR;
    re = uregex_open(pat, -1, 0x40000000, 0, &status);
    TEST_ASSERT(status == U_REGEX_INVALID_FLAG);
    uregex_close(re);

    
    status = U_ZERO_ERROR;
    re = uregex_open(pat, -1, UREGEX_CANON_EQ, 0, &status);
    TEST_ASSERT(status == U_REGEX_UNIMPLEMENTED);
    uregex_close(re);

    
    status = U_ZERO_ERROR;
    re = uregex_openC(NULL,
        UREGEX_CASE_INSENSITIVE | UREGEX_COMMENTS | UREGEX_DOTALL | UREGEX_MULTILINE | UREGEX_UWORD, 0, &status);
    TEST_ASSERT(status == U_ILLEGAL_ARGUMENT_ERROR && re == NULL);

    
    status = U_USELESS_COLLATOR_ERROR;
    re = uregex_openC(NULL,
        UREGEX_CASE_INSENSITIVE | UREGEX_COMMENTS | UREGEX_DOTALL | UREGEX_MULTILINE | UREGEX_UWORD, 0, &status);
    TEST_ASSERT(status == U_USELESS_COLLATOR_ERROR && re == NULL);

    
    {
        const UChar   *p;
        int32_t  len;
        status = U_ZERO_ERROR;
        re = uregex_openC("abc*", 0, 0, &status);
        TEST_ASSERT_SUCCESS(status);
        p = uregex_pattern(re, &len, &status);
        TEST_ASSERT_SUCCESS(status);

        
        if(U_SUCCESS(status)) {
            u_uastrncpy(pat, "abc*", UPRV_LENGTHOF(pat));
            TEST_ASSERT(u_strcmp(pat, p) == 0);
            TEST_ASSERT(len==(int32_t)strlen("abc*"));
        }

        uregex_close(re);

        
    }

    


    {
        URegularExpression *clone1;
        URegularExpression *clone2;
        URegularExpression *clone3;
        UChar  testString1[30];
        UChar  testString2[30];
        UBool  result;


        status = U_ZERO_ERROR;
        re = uregex_openC("abc*", 0, 0, &status);
        TEST_ASSERT_SUCCESS(status);
        clone1 = uregex_clone(re, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(clone1 != NULL);

        status = U_ZERO_ERROR;
        clone2 = uregex_clone(re, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(clone2 != NULL);
        uregex_close(re);

        status = U_ZERO_ERROR;
        clone3 = uregex_clone(clone2, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(clone3 != NULL);

        u_uastrncpy(testString1, "abcccd", UPRV_LENGTHOF(pat));
        u_uastrncpy(testString2, "xxxabcccd", UPRV_LENGTHOF(pat));

        status = U_ZERO_ERROR;
        uregex_setText(clone1, testString1, -1, &status);
        TEST_ASSERT_SUCCESS(status);
        result = uregex_lookingAt(clone1, 0, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(result==TRUE);
        
        status = U_ZERO_ERROR;
        uregex_setText(clone2, testString2, -1, &status);
        TEST_ASSERT_SUCCESS(status);
        result = uregex_lookingAt(clone2, 0, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(result==FALSE);
        result = uregex_find(clone2, 0, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(result==TRUE);

        uregex_close(clone1);
        uregex_close(clone2);
        uregex_close(clone3);

    }

    


    {
        const UChar  *resultPat;
        int32_t       resultLen;
        u_uastrncpy(pat, "hello", UPRV_LENGTHOF(pat));
        status = U_ZERO_ERROR;
        re = uregex_open(pat, -1, 0, NULL, &status);
        resultPat = uregex_pattern(re, &resultLen, &status);
        TEST_ASSERT_SUCCESS(status);

        
        if (U_SUCCESS(status)) {
            TEST_ASSERT(resultLen == -1);
            TEST_ASSERT(u_strcmp(resultPat, pat) == 0);
        }

        uregex_close(re);

        status = U_ZERO_ERROR;
        re = uregex_open(pat, 3, 0, NULL, &status);
        resultPat = uregex_pattern(re, &resultLen, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_SUCCESS(status);

        
        if (U_SUCCESS(status)) {
            TEST_ASSERT(resultLen == 3);
            TEST_ASSERT(u_strncmp(resultPat, pat, 3) == 0);
            TEST_ASSERT(u_strlen(resultPat) == 3);
        }

        uregex_close(re);
    }

    


    {
        int32_t  t;

        status = U_ZERO_ERROR;
        re = uregex_open(pat, -1, 0, NULL, &status);
        t  = uregex_flags(re, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(t == 0);
        uregex_close(re);

        status = U_ZERO_ERROR;
        re = uregex_open(pat, -1, 0, NULL, &status);
        t  = uregex_flags(re, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(t == 0);
        uregex_close(re);

        status = U_ZERO_ERROR;
        re = uregex_open(pat, -1, UREGEX_CASE_INSENSITIVE | UREGEX_DOTALL, NULL, &status);
        t  = uregex_flags(re, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(t == (UREGEX_CASE_INSENSITIVE | UREGEX_DOTALL));
        uregex_close(re);
    }

    


    {
        UChar  text1[50];
        UChar  text2[50];
        UBool  result;

        u_uastrncpy(text1, "abcccd",  UPRV_LENGTHOF(text1));
        u_uastrncpy(text2, "abcccxd", UPRV_LENGTHOF(text2));
        status = U_ZERO_ERROR;
        u_uastrncpy(pat, "abc*d", UPRV_LENGTHOF(pat));
        re = uregex_open(pat, -1, 0, NULL, &status);
        TEST_ASSERT_SUCCESS(status);

        
        status = U_ZERO_ERROR;
        uregex_lookingAt(re, 0, &status);
        TEST_ASSERT( status== U_REGEX_INVALID_STATE);

        status = U_ZERO_ERROR;
        uregex_setText(re, text1, -1, &status);
        result = uregex_lookingAt(re, 0, &status);
        TEST_ASSERT(result == TRUE);
        TEST_ASSERT_SUCCESS(status);

        status = U_ZERO_ERROR;
        uregex_setText(re, text2, -1, &status);
        result = uregex_lookingAt(re, 0, &status);
        TEST_ASSERT(result == FALSE);
        TEST_ASSERT_SUCCESS(status);

        status = U_ZERO_ERROR;
        uregex_setText(re, text1, -1, &status);
        result = uregex_lookingAt(re, 0, &status);
        TEST_ASSERT(result == TRUE);
        TEST_ASSERT_SUCCESS(status);

        status = U_ZERO_ERROR;
        uregex_setText(re, text1, 5, &status);
        result = uregex_lookingAt(re, 0, &status);
        TEST_ASSERT(result == FALSE);
        TEST_ASSERT_SUCCESS(status);

        status = U_ZERO_ERROR;
        uregex_setText(re, text1, 6, &status);
        result = uregex_lookingAt(re, 0, &status);
        TEST_ASSERT(result == TRUE);
        TEST_ASSERT_SUCCESS(status);

        uregex_close(re);
    }


    


    {
        UChar    text1[50];
        UChar    text2[50];
        const UChar   *result;
        int32_t  textLength;

        u_uastrncpy(text1, "abcccd",  UPRV_LENGTHOF(text1));
        u_uastrncpy(text2, "abcccxd", UPRV_LENGTHOF(text2));
        status = U_ZERO_ERROR;
        u_uastrncpy(pat, "abc*d", UPRV_LENGTHOF(pat));
        re = uregex_open(pat, -1, 0, NULL, &status);

        uregex_setText(re, text1, -1, &status);
        result = uregex_getText(re, &textLength, &status);
        TEST_ASSERT(result == text1);
        TEST_ASSERT(textLength == -1);
        TEST_ASSERT_SUCCESS(status);

        status = U_ZERO_ERROR;
        uregex_setText(re, text2, 7, &status);
        result = uregex_getText(re, &textLength, &status);
        TEST_ASSERT(result == text2);
        TEST_ASSERT(textLength == 7);
        TEST_ASSERT_SUCCESS(status);

        status = U_ZERO_ERROR;
        uregex_setText(re, text2, 4, &status);
        result = uregex_getText(re, &textLength, &status);
        TEST_ASSERT(result == text2);
        TEST_ASSERT(textLength == 4);
        TEST_ASSERT_SUCCESS(status);
        uregex_close(re);
    }

    


    {
        UChar   text1[50];
        UBool   result;
        int     len;
        UChar   nullString[] = {0,0,0};

        u_uastrncpy(text1, "abcccde",  UPRV_LENGTHOF(text1));
        status = U_ZERO_ERROR;
        u_uastrncpy(pat, "abc*d", UPRV_LENGTHOF(pat));
        re = uregex_open(pat, -1, 0, NULL, &status);

        uregex_setText(re, text1, -1, &status);
        result = uregex_matches(re, 0, &status);
        TEST_ASSERT(result == FALSE);
        TEST_ASSERT_SUCCESS(status);

        status = U_ZERO_ERROR;
        uregex_setText(re, text1, 6, &status);
        result = uregex_matches(re, 0, &status);
        TEST_ASSERT(result == TRUE);
        TEST_ASSERT_SUCCESS(status);

        status = U_ZERO_ERROR;
        uregex_setText(re, text1, 6, &status);
        result = uregex_matches(re, 1, &status);
        TEST_ASSERT(result == FALSE);
        TEST_ASSERT_SUCCESS(status);
        uregex_close(re);

        status = U_ZERO_ERROR;
        re = uregex_openC(".?", 0, NULL, &status);
        uregex_setText(re, text1, -1, &status);
        len = u_strlen(text1);
        result = uregex_matches(re, len, &status);
        TEST_ASSERT(result == TRUE);
        TEST_ASSERT_SUCCESS(status);

        status = U_ZERO_ERROR;
        uregex_setText(re, nullString, -1, &status);
        TEST_ASSERT_SUCCESS(status);
        result = uregex_matches(re, 0, &status);
        TEST_ASSERT(result == TRUE);
        TEST_ASSERT_SUCCESS(status);
        uregex_close(re);
    }


    




    


    {
        UChar    text1[50];
        UBool    result;
        u_uastrncpy(text1, "012rx5rx890rxrx...",  UPRV_LENGTHOF(text1));
        status = U_ZERO_ERROR;
        re = uregex_openC("rx", 0, NULL, &status);

        uregex_setText(re, text1, -1, &status);
        result = uregex_find(re, 0, &status);
        TEST_ASSERT(result == TRUE);
        TEST_ASSERT(uregex_start(re, 0, &status) == 3);
        TEST_ASSERT(uregex_end(re, 0, &status) == 5);
        TEST_ASSERT_SUCCESS(status);

        result = uregex_find(re, 9, &status);
        TEST_ASSERT(result == TRUE);
        TEST_ASSERT(uregex_start(re, 0, &status) == 11);
        TEST_ASSERT(uregex_end(re, 0, &status) == 13);
        TEST_ASSERT_SUCCESS(status);

        result = uregex_find(re, 14, &status);
        TEST_ASSERT(result == FALSE);
        TEST_ASSERT_SUCCESS(status);

        status = U_ZERO_ERROR;
        uregex_reset(re, 0, &status);

        result = uregex_findNext(re, &status);
        TEST_ASSERT(result == TRUE);
        TEST_ASSERT(uregex_start(re, 0, &status) == 3);
        TEST_ASSERT(uregex_end(re, 0, &status) == 5);
        TEST_ASSERT_SUCCESS(status);

        result = uregex_findNext(re, &status);
        TEST_ASSERT(result == TRUE);
        TEST_ASSERT(uregex_start(re, 0, &status) == 6);
        TEST_ASSERT(uregex_end(re, 0, &status) == 8);
        TEST_ASSERT_SUCCESS(status);

        status = U_ZERO_ERROR;
        uregex_reset(re, 12, &status);

        result = uregex_findNext(re, &status);
        TEST_ASSERT(result == TRUE);
        TEST_ASSERT(uregex_start(re, 0, &status) == 13);
        TEST_ASSERT(uregex_end(re, 0, &status) == 15);
        TEST_ASSERT_SUCCESS(status);

        result = uregex_findNext(re, &status);
        TEST_ASSERT(result == FALSE);
        TEST_ASSERT_SUCCESS(status);

        uregex_close(re);
    }

    


    {
        int32_t result;

        status = U_ZERO_ERROR;
        re = uregex_openC("abc", 0, NULL, &status);
        result = uregex_groupCount(re, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(result == 0);
        uregex_close(re);

        status = U_ZERO_ERROR;
        re = uregex_openC("abc(def)(ghi(j))", 0, NULL, &status);
        result = uregex_groupCount(re, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(result == 3);
        uregex_close(re);

    }


    


    {
        UChar    text1[80];
        UChar    buf[80];
        UBool    result;
        int32_t  resultSz;
        u_uastrncpy(text1, "noise abc interior def, and this is off the end",  UPRV_LENGTHOF(text1));

        status = U_ZERO_ERROR;
        re = uregex_openC("abc(.*?)def", 0, NULL, &status);
        TEST_ASSERT_SUCCESS(status);


        uregex_setText(re, text1, -1, &status);
        result = uregex_find(re, 0, &status);
        TEST_ASSERT(result==TRUE);

        
        status = U_ZERO_ERROR;
        resultSz = uregex_group(re, 0, buf, UPRV_LENGTHOF(buf), &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_STRING("abc interior def", buf, TRUE);
        TEST_ASSERT(resultSz == (int32_t)strlen("abc interior def"));

        
        status = U_ZERO_ERROR;
        resultSz = uregex_group(re, 1, buf, UPRV_LENGTHOF(buf), &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_STRING(" interior ", buf, TRUE);
        TEST_ASSERT(resultSz == (int32_t)strlen(" interior "));

        
        status = U_ZERO_ERROR;
        uregex_group(re, 2, buf, UPRV_LENGTHOF(buf), &status);
        TEST_ASSERT(status == U_INDEX_OUTOFBOUNDS_ERROR);

        
        status = U_ZERO_ERROR;
        resultSz = uregex_group(re, 0, NULL, 0, &status);
        TEST_ASSERT(status == U_BUFFER_OVERFLOW_ERROR);
        TEST_ASSERT(resultSz == (int32_t)strlen("abc interior def"));

        
        status = U_ZERO_ERROR;
        memset(buf, -1, sizeof(buf));
        resultSz = uregex_group(re, 0, buf, 5, &status);
        TEST_ASSERT(status == U_BUFFER_OVERFLOW_ERROR);
        TEST_ASSERT_STRING("abc i", buf, FALSE);
        TEST_ASSERT(buf[5] == (UChar)0xffff);
        TEST_ASSERT(resultSz == (int32_t)strlen("abc interior def"));

        
        status = U_ZERO_ERROR;
        resultSz = uregex_group(re, 0, buf, (int32_t)strlen("abc interior def"), &status);
        TEST_ASSERT(status == U_STRING_NOT_TERMINATED_WARNING);
        TEST_ASSERT_STRING("abc interior def", buf, FALSE);
        TEST_ASSERT(resultSz == (int32_t)strlen("abc interior def"));
        TEST_ASSERT(buf[strlen("abc interior def")] == (UChar)0xffff);
        
        uregex_close(re);

    }
    
    


        
        
        
        TEST_SETUP(".*", "0123456789ABCDEF", 0)
        UChar resultString[40];
        TEST_ASSERT(uregex_regionStart(re, &status) == 0);
        TEST_ASSERT(uregex_regionEnd(re, &status) == 16);
        uregex_setRegion(re, 3, 6, &status);
        TEST_ASSERT(uregex_regionStart(re, &status) == 3);
        TEST_ASSERT(uregex_regionEnd(re, &status) == 6);
        TEST_ASSERT(uregex_findNext(re, &status));
        TEST_ASSERT(uregex_group(re, 0, resultString, UPRV_LENGTHOF(resultString), &status) == 3)
        TEST_ASSERT_STRING("345", resultString, TRUE);
        TEST_TEARDOWN;
        
        
        TEST_SETUP(".*", "0123456789ABCDEF", 0);
        uregex_setRegion(re, 4, 6, &status);
        TEST_ASSERT(uregex_find(re, -1, &status) == TRUE);
        TEST_ASSERT(uregex_start(re, 0, &status) == 4);
        TEST_ASSERT(uregex_end(re, 0, &status) == 6);
        TEST_TEARDOWN;
        
        
        TEST_SETUP(".*", "0123456789ABCDEF", 0);
        uregex_setRegion(re, 4, 6, &status);
        TEST_ASSERT(uregex_find(re, 0, &status) == TRUE);
        TEST_ASSERT(uregex_start(re, 0, &status) == 0);
        TEST_ASSERT(uregex_end(re, 0, &status) == 16);
        TEST_TEARDOWN;
         
        
        TEST_SETUP(".", "0123456789ABCDEF", 0);
        uregex_setRegion(re, 4, 6, &status);
        TEST_ASSERT(uregex_findNext(re,&status) == TRUE);
        TEST_ASSERT(uregex_start(re, 0, &status) == 4);
        TEST_ASSERT(uregex_findNext(re, &status) == TRUE);
        TEST_ASSERT(uregex_start(re, 0, &status) == 5);
        TEST_ASSERT(uregex_findNext(re, &status) == FALSE);
        TEST_TEARDOWN;

        
        
        TEST_SETUP(".*?", "0123456789ABCDEF", 0);
        uregex_setRegion(re, 4, 6, &status);
        TEST_ASSERT(uregex_matches(re, -1, &status) == TRUE);
        TEST_ASSERT(uregex_start(re, 0, &status) == 4);
        TEST_ASSERT(uregex_end(re, 0, &status) == 6);
        TEST_TEARDOWN;
        
        
        TEST_SETUP(".*?", "0123456789ABCDEF", 0);
        uregex_setRegion(re, 4, 6, &status);
        TEST_ASSERT(uregex_matches(re, 0, &status) == TRUE);
        TEST_ASSERT(uregex_start(re, 0, &status) == 0);
        TEST_ASSERT(uregex_end(re, 0, &status) == 16);
        TEST_TEARDOWN;
        
        
        
        TEST_SETUP(".*?", "0123456789ABCDEF", 0);
        uregex_setRegion(re, 4, 6, &status);
        TEST_ASSERT(uregex_lookingAt(re, -1, &status) == TRUE);
        TEST_ASSERT(uregex_start(re, 0, &status) == 4);
        TEST_ASSERT(uregex_end(re, 0, &status) == 4);
        TEST_TEARDOWN;
        
        
        TEST_SETUP(".*?", "0123456789ABCDEF", 0);
        uregex_setRegion(re, 4, 6, &status);
        TEST_ASSERT(uregex_lookingAt(re, 0, &status) == TRUE);
        TEST_ASSERT(uregex_start(re, 0, &status) == 0);
        TEST_ASSERT(uregex_end(re, 0, &status) == 0);
        TEST_TEARDOWN;

        
        TEST_SETUP("[a-f]*", "abcdefghij", 0);
        TEST_ASSERT(uregex_find(re, 0, &status) == TRUE);
        TEST_ASSERT(uregex_hitEnd(re, &status) == FALSE);
        TEST_TEARDOWN;

        TEST_SETUP("[a-f]*", "abcdef", 0);
        TEST_ASSERT(uregex_find(re, 0, &status) == TRUE);
        TEST_ASSERT(uregex_hitEnd(re, &status) == TRUE);
        TEST_TEARDOWN;

        
        TEST_SETUP("abcd", "abcd", 0);
        TEST_ASSERT(uregex_find(re, 0, &status) == TRUE);
        TEST_ASSERT(uregex_requireEnd(re, &status) == FALSE);
        TEST_TEARDOWN;

        TEST_SETUP("abcd$", "abcd", 0);
        TEST_ASSERT(uregex_find(re, 0, &status) == TRUE);
        TEST_ASSERT(uregex_requireEnd(re, &status) == TRUE);
        TEST_TEARDOWN;
        
        
        TEST_SETUP("abc$", "abcdef", 0);
        TEST_ASSERT(uregex_hasAnchoringBounds(re, &status) == TRUE);
        uregex_useAnchoringBounds(re, FALSE, &status);
        TEST_ASSERT(uregex_hasAnchoringBounds(re, &status) == FALSE);
        
        TEST_ASSERT(uregex_find(re, -1, &status) == FALSE);
        uregex_useAnchoringBounds(re, TRUE, &status);
        uregex_setRegion(re, 0, 3, &status);
        TEST_ASSERT(uregex_find(re, -1, &status) == TRUE);
        TEST_ASSERT(uregex_end(re, 0, &status) == 3);
        TEST_TEARDOWN;
        
        
        TEST_SETUP("abc(?=def)", "abcdef", 0);
        TEST_ASSERT(uregex_hasTransparentBounds(re, &status) == FALSE);
        uregex_useTransparentBounds(re, TRUE, &status);
        TEST_ASSERT(uregex_hasTransparentBounds(re, &status) == TRUE);
        
        uregex_useTransparentBounds(re, FALSE, &status);
        TEST_ASSERT(uregex_find(re, -1, &status) == TRUE);    
        uregex_setRegion(re, 0, 3, &status);
        TEST_ASSERT(uregex_find(re, -1, &status) == FALSE);   
        uregex_useTransparentBounds(re, TRUE, &status);
        TEST_ASSERT(uregex_find(re, -1, &status) == TRUE);    
        TEST_ASSERT(uregex_end(re, 0, &status) == 3);
        TEST_TEARDOWN;
        

    


    {
        UChar    text1[80];
        UChar    text2[80];
        UChar    replText[80];
        UChar    buf[80];
        int32_t  resultSz;
        u_uastrncpy(text1, "Replace xaax x1x x...x.",  UPRV_LENGTHOF(text1));
        u_uastrncpy(text2, "No match here.",  UPRV_LENGTHOF(text2));
        u_uastrncpy(replText, "<$1>", UPRV_LENGTHOF(replText));

        status = U_ZERO_ERROR;
        re = uregex_openC("x(.*?)x", 0, NULL, &status);
        TEST_ASSERT_SUCCESS(status);

        
        uregex_setText(re, text1, -1, &status);
        resultSz = uregex_replaceFirst(re, replText, -1, buf, UPRV_LENGTHOF(buf), &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_STRING("Replace <aa> x1x x...x.", buf, TRUE);
        TEST_ASSERT(resultSz == (int32_t)strlen("Replace xaax x1x x...x."));

        
        status = U_ZERO_ERROR;
        uregex_setText(re, text2, -1, &status);
        resultSz = uregex_replaceFirst(re, replText, -1, buf, UPRV_LENGTHOF(buf), &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_STRING("No match here.", buf, TRUE);
        TEST_ASSERT(resultSz == (int32_t)strlen("No match here."));

        
        status = U_ZERO_ERROR;
        uregex_setText(re, text1, -1, &status);
        memset(buf, -1, sizeof(buf));
        resultSz = uregex_replaceFirst(re, replText, -1, buf, strlen("Replace <aa> x1x x...x."), &status);
        TEST_ASSERT(status == U_STRING_NOT_TERMINATED_WARNING);
        TEST_ASSERT_STRING("Replace <aa> x1x x...x.", buf, FALSE);
        TEST_ASSERT(resultSz == (int32_t)strlen("Replace xaax x1x x...x."));
        TEST_ASSERT(buf[resultSz] == (UChar)0xffff);

        


        status = U_ZERO_ERROR;
        memset(buf, -1, sizeof(buf));
        resultSz = uregex_replaceFirst(re, replText, -1, buf, strlen("Replace <aa> x1x x...x."), &status);
        TEST_ASSERT(status == U_STRING_NOT_TERMINATED_WARNING);
        TEST_ASSERT_STRING("Replace <aa> x1x x...x.", buf, FALSE);
        TEST_ASSERT(resultSz == (int32_t)strlen("Replace xaax x1x x...x."));
        TEST_ASSERT(buf[resultSz] == (UChar)0xffff);

        
        status = U_ZERO_ERROR;
        resultSz = uregex_replaceFirst(re, replText, -1, NULL, 0, &status);
        TEST_ASSERT(status == U_BUFFER_OVERFLOW_ERROR);
        TEST_ASSERT(resultSz == (int32_t)strlen("Replace xaax x1x x...x."));

        
        status = U_ZERO_ERROR;
        memset(buf, -1, sizeof(buf));
        resultSz = uregex_replaceFirst(re, replText, -1, buf, strlen("Replace <aa> x1x x...x.")-1, &status);
        TEST_ASSERT(status == U_BUFFER_OVERFLOW_ERROR);
        TEST_ASSERT_STRING("Replace <aa> x1x x...x", buf, FALSE);
        TEST_ASSERT(resultSz == (int32_t)strlen("Replace xaax x1x x...x."));
        TEST_ASSERT(buf[resultSz] == (UChar)0xffff);

        uregex_close(re);
    }


    


    {
        UChar    text1[80];          
        UChar    text2[80];          
        UChar    replText[80];       
        UChar    replText2[80];      
        const char * pattern = "x(.*?)x";
        const char * expectedResult = "Replace <aa> <1> <...>.";
        const char * expectedResult2 = "Replace <<aa>> <<1>> <<...>>.";
        UChar    buf[80];
        int32_t  resultSize;
        int32_t  expectedResultSize;
        int32_t  expectedResultSize2;
        int32_t  i;

        u_uastrncpy(text1, "Replace xaax x1x x...x.",  UPRV_LENGTHOF(text1));
        u_uastrncpy(text2, "No match here.",  UPRV_LENGTHOF(text2));
        u_uastrncpy(replText, "<$1>", UPRV_LENGTHOF(replText));
        u_uastrncpy(replText2, "<<$1>>", UPRV_LENGTHOF(replText2));
        expectedResultSize = strlen(expectedResult);
        expectedResultSize2 = strlen(expectedResult2);

        status = U_ZERO_ERROR;
        re = uregex_openC(pattern, 0, NULL, &status);
        TEST_ASSERT_SUCCESS(status);

        
        uregex_setText(re, text1, -1, &status);
        resultSize = uregex_replaceAll(re, replText, -1, buf, UPRV_LENGTHOF(buf), &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_STRING(expectedResult, buf, TRUE);
        TEST_ASSERT(resultSize == expectedResultSize);

        
        status = U_ZERO_ERROR;
        uregex_setText(re, text2, -1, &status);
        resultSize = uregex_replaceAll(re, replText, -1, buf, UPRV_LENGTHOF(buf), &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_STRING("No match here.", buf, TRUE);
        TEST_ASSERT(resultSize == u_strlen(text2));

        
        status = U_ZERO_ERROR;
        uregex_setText(re, text1, -1, &status);
        memset(buf, -1, sizeof(buf));
        resultSize = uregex_replaceAll(re, replText, -1, buf, expectedResultSize, &status);
        TEST_ASSERT(status == U_STRING_NOT_TERMINATED_WARNING);
        TEST_ASSERT_STRING(expectedResult, buf, FALSE);
        TEST_ASSERT(resultSize == expectedResultSize);
        TEST_ASSERT(buf[resultSize] == (UChar)0xffff);

        


        status = U_ZERO_ERROR;
        memset(buf, -1, sizeof(buf));
        resultSize = uregex_replaceAll(re, replText, -1, buf, strlen("Replace xaax x1x x...x."), &status);
        TEST_ASSERT(status == U_STRING_NOT_TERMINATED_WARNING);
        TEST_ASSERT_STRING("Replace <aa> <1> <...>.", buf, FALSE);
        TEST_ASSERT(resultSize == (int32_t)strlen("Replace <aa> <1> <...>."));
        TEST_ASSERT(buf[resultSize] == (UChar)0xffff);

        
        status = U_ZERO_ERROR;
        resultSize = uregex_replaceAll(re, replText, -1, NULL, 0, &status);
        TEST_ASSERT(status == U_BUFFER_OVERFLOW_ERROR);
        TEST_ASSERT(resultSize == (int32_t)strlen("Replace <aa> <1> <...>."));

        

        for (i=0; i<expectedResultSize; i++) {
            char  expected[80];
            status = U_ZERO_ERROR;
            memset(buf, -1, sizeof(buf));
            resultSize = uregex_replaceAll(re, replText, -1, buf, i, &status);
            TEST_ASSERT(status == U_BUFFER_OVERFLOW_ERROR);
            strcpy(expected, expectedResult);
            expected[i] = 0;
            TEST_ASSERT_STRING(expected, buf, FALSE);
            TEST_ASSERT(resultSize == expectedResultSize);
            TEST_ASSERT(buf[i] == (UChar)0xffff);
        }

        



        for (i=0; i<expectedResultSize2; i++) {
            char  expected[80];
            status = U_ZERO_ERROR;
            memset(buf, -1, sizeof(buf));
            resultSize = uregex_replaceAll(re, replText2, -1, buf, i, &status);
            TEST_ASSERT(status == U_BUFFER_OVERFLOW_ERROR);
            strcpy(expected, expectedResult2);
            expected[i] = 0;
            TEST_ASSERT_STRING(expected, buf, FALSE);
            TEST_ASSERT(resultSize == expectedResultSize2);
            TEST_ASSERT(buf[i] == (UChar)0xffff);
        }


        uregex_close(re);
    }


    


    {
        UChar    text[100];
        UChar    repl[100];
        UChar    buf[100];
        UChar   *bufPtr;
        int32_t  bufCap;


        status = U_ZERO_ERROR;
        re = uregex_openC(".*", 0, 0, &status);
        TEST_ASSERT_SUCCESS(status);

        u_uastrncpy(text, "whatever",  UPRV_LENGTHOF(text));
        u_uastrncpy(repl, "some other", UPRV_LENGTHOF(repl));
        uregex_setText(re, text, -1, &status);

        
        uregex_find(re, 0, &status);
        TEST_ASSERT_SUCCESS(status);
        bufPtr = buf;
        bufCap = UPRV_LENGTHOF(buf);
        uregex_appendReplacement(re, repl, -1, &bufPtr, &bufCap, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_STRING("some other", buf, TRUE);

        
        uregex_find(re, 0, &status);
        TEST_ASSERT_SUCCESS(status);
        bufPtr = buf;
        bufCap = UPRV_LENGTHOF(buf);
        u_uastrncpy(repl, "abc\\u0041\\U00000042 \\\\ \\$ \\abc", UPRV_LENGTHOF(repl));
        uregex_appendReplacement(re, repl, -1, &bufPtr, &bufCap, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_STRING("abcAB \\ $ abc", buf, TRUE); 

        
        status = U_ZERO_ERROR;
        uregex_find(re, 0, &status);
        TEST_ASSERT_SUCCESS(status);
        bufPtr = buf;
        status = U_BUFFER_OVERFLOW_ERROR;
        uregex_appendReplacement(re, repl, -1, &bufPtr, NULL, &status);
        TEST_ASSERT(status == U_BUFFER_OVERFLOW_ERROR);

        uregex_close(re);
    }


    



    


    {
        UChar    textToSplit[80];
        UChar    text2[80];
        UChar    buf[200];
        UChar    *fields[10];
        int32_t  numFields;
        int32_t  requiredCapacity;
        int32_t  spaceNeeded;
        int32_t  sz;

        u_uastrncpy(textToSplit, "first : second:  third",  UPRV_LENGTHOF(textToSplit));
        u_uastrncpy(text2, "No match here.",  UPRV_LENGTHOF(text2));

        status = U_ZERO_ERROR;
        re = uregex_openC(":", 0, NULL, &status);


         

        uregex_setText(re, textToSplit, -1, &status);
        TEST_ASSERT_SUCCESS(status);

        
        if (U_SUCCESS(status)) {
            memset(fields, -1, sizeof(fields));
            numFields = 
                uregex_split(re, buf, UPRV_LENGTHOF(buf), &requiredCapacity, fields, 10, &status);
            TEST_ASSERT_SUCCESS(status);

            
            if(U_SUCCESS(status)) {
                TEST_ASSERT(numFields == 3);
                TEST_ASSERT_STRING("first ",  fields[0], TRUE);
                TEST_ASSERT_STRING(" second", fields[1], TRUE);
                TEST_ASSERT_STRING("  third", fields[2], TRUE);
                TEST_ASSERT(fields[3] == NULL);

                spaceNeeded = u_strlen(textToSplit) -
                            (numFields - 1)  +  
                            numFields;           

                TEST_ASSERT(spaceNeeded == requiredCapacity);
            }
        }

        uregex_close(re);

    
        
        status = U_ZERO_ERROR;
        re = uregex_openC(":", 0, NULL, &status);
        uregex_setText(re, textToSplit, -1, &status);
        TEST_ASSERT_SUCCESS(status);

        
        if(U_SUCCESS(status)) {
            memset(fields, -1, sizeof(fields));
            numFields = 
                uregex_split(re, buf, UPRV_LENGTHOF(buf), &requiredCapacity, fields, 2, &status);
            TEST_ASSERT_SUCCESS(status);

            
            if(U_SUCCESS(status)) {
                TEST_ASSERT(numFields == 2);
                TEST_ASSERT_STRING("first ",  fields[0], TRUE);
                TEST_ASSERT_STRING(" second:  third", fields[1], TRUE);
                TEST_ASSERT(!memcmp(&fields[2],&minus1,sizeof(UChar*)));

                spaceNeeded = u_strlen(textToSplit) -
                            (numFields - 1)  +  
                            numFields;           

                TEST_ASSERT(spaceNeeded == requiredCapacity);

                
                spaceNeeded = u_strlen(textToSplit) -
                    (numFields - 1)  +  
                    numFields;           
                        
                for (sz=0; sz < spaceNeeded+1; sz++) {
                    memset(fields, -1, sizeof(fields));
                    status = U_ZERO_ERROR;
                    numFields = 
                        uregex_split(re, buf, sz, &requiredCapacity, fields, 10, &status);
                    if (sz >= spaceNeeded) {
                        TEST_ASSERT_SUCCESS(status);
                        TEST_ASSERT_STRING("first ",  fields[0], TRUE);
                        TEST_ASSERT_STRING(" second", fields[1], TRUE);
                        TEST_ASSERT_STRING("  third", fields[2], TRUE);
                    } else {
                        TEST_ASSERT(status == U_BUFFER_OVERFLOW_ERROR);
                    }
                    TEST_ASSERT(numFields == 3);
                    TEST_ASSERT(fields[3] == NULL);
                    TEST_ASSERT(spaceNeeded == requiredCapacity);
                }
            }
        }

        uregex_close(re);
    }




    

    {
        UChar    textToSplit[80];
        UChar    buf[200];
        UChar    *fields[10];
        int32_t  numFields;
        int32_t  requiredCapacity;
        int32_t  spaceNeeded;
        int32_t  sz;

        u_uastrncpy(textToSplit, "first <tag-a> second<tag-b>  third",  UPRV_LENGTHOF(textToSplit));

        status = U_ZERO_ERROR;
        re = uregex_openC("<(.*?)>", 0, NULL, &status);

        uregex_setText(re, textToSplit, -1, &status);
        TEST_ASSERT_SUCCESS(status);

        
        if(U_SUCCESS(status)) {
            memset(fields, -1, sizeof(fields));
            numFields = 
                uregex_split(re, buf, UPRV_LENGTHOF(buf), &requiredCapacity, fields, 10, &status);
            TEST_ASSERT_SUCCESS(status);

            
            if(U_SUCCESS(status)) {
                TEST_ASSERT(numFields == 5);
                TEST_ASSERT_STRING("first ",  fields[0], TRUE);
                TEST_ASSERT_STRING("tag-a",   fields[1], TRUE);
                TEST_ASSERT_STRING(" second", fields[2], TRUE);
                TEST_ASSERT_STRING("tag-b",   fields[3], TRUE);
                TEST_ASSERT_STRING("  third", fields[4], TRUE);
                TEST_ASSERT(fields[5] == NULL);
                spaceNeeded = strlen("first .tag-a. second.tag-b.  third.");  
                TEST_ASSERT(spaceNeeded == requiredCapacity);
            }
        }
    
        
        status = U_ZERO_ERROR;
        memset(fields, -1, sizeof(fields));
        numFields = 
            uregex_split(re, buf, UPRV_LENGTHOF(buf), &requiredCapacity, fields, 2, &status);
        TEST_ASSERT_SUCCESS(status);

        
        if(U_SUCCESS(status)) {
            TEST_ASSERT(numFields == 2);
            TEST_ASSERT_STRING("first ",  fields[0], TRUE);
            TEST_ASSERT_STRING(" second<tag-b>  third", fields[1], TRUE);
            TEST_ASSERT(!memcmp(&fields[2],&minus1,sizeof(UChar*)));

            spaceNeeded = strlen("first . second<tag-b>  third.");  
            TEST_ASSERT(spaceNeeded == requiredCapacity);
        }

        
        status = U_ZERO_ERROR;
        memset(fields, -1, sizeof(fields));
        numFields = 
            uregex_split(re, buf, UPRV_LENGTHOF(buf), &requiredCapacity, fields, 3, &status);
        TEST_ASSERT_SUCCESS(status);

        
        if(U_SUCCESS(status)) {
            TEST_ASSERT(numFields == 3);
            TEST_ASSERT_STRING("first ",  fields[0], TRUE);
            TEST_ASSERT_STRING("tag-a",   fields[1], TRUE);
            TEST_ASSERT_STRING(" second<tag-b>  third", fields[2], TRUE);
            TEST_ASSERT(!memcmp(&fields[3],&minus1,sizeof(UChar*)));

            spaceNeeded = strlen("first .tag-a. second<tag-b>  third.");  
            TEST_ASSERT(spaceNeeded == requiredCapacity);
        }

        
        status = U_ZERO_ERROR;
        memset(fields, -1, sizeof(fields));
        numFields = 
            uregex_split(re, buf, UPRV_LENGTHOF(buf), &requiredCapacity, fields, 5, &status);
        TEST_ASSERT_SUCCESS(status);

        
        if(U_SUCCESS(status)) {
            TEST_ASSERT(numFields == 5);
            TEST_ASSERT_STRING("first ",  fields[0], TRUE);
            TEST_ASSERT_STRING("tag-a",   fields[1], TRUE);
            TEST_ASSERT_STRING(" second", fields[2], TRUE);
            TEST_ASSERT_STRING("tag-b",   fields[3], TRUE);
            TEST_ASSERT_STRING("  third", fields[4], TRUE);
            TEST_ASSERT(!memcmp(&fields[5],&minus1,sizeof(UChar*)));

            spaceNeeded = strlen("first .tag-a. second.tag-b.  third.");  
            TEST_ASSERT(spaceNeeded == requiredCapacity);
        }

        
        status = U_ZERO_ERROR;
        sz = strlen("first <tag-a> second<tag-b>");
        uregex_setText(re, textToSplit, sz, &status);
        TEST_ASSERT_SUCCESS(status);

        
        if(U_SUCCESS(status)) {
            memset(fields, -1, sizeof(fields));
            numFields = 
                uregex_split(re, buf, UPRV_LENGTHOF(buf), &requiredCapacity, fields, 9, &status);
            TEST_ASSERT_SUCCESS(status);

            
            if(U_SUCCESS(status)) {
                TEST_ASSERT(numFields == 5);
                TEST_ASSERT_STRING("first ",  fields[0], TRUE);
                TEST_ASSERT_STRING("tag-a",   fields[1], TRUE);
                TEST_ASSERT_STRING(" second", fields[2], TRUE);
                TEST_ASSERT_STRING("tag-b",   fields[3], TRUE);
                TEST_ASSERT_STRING("",        fields[4], TRUE);
                TEST_ASSERT(fields[5] == NULL);
                TEST_ASSERT(fields[8] == NULL);
                TEST_ASSERT(!memcmp(&fields[9],&minus1,sizeof(UChar*)));
                spaceNeeded = strlen("first .tag-a. second.tag-b..");  
                TEST_ASSERT(spaceNeeded == requiredCapacity);
            }
        }

        uregex_close(re);
    }

    


     TEST_SETUP("abc$", "abcdef", 0);
     TEST_ASSERT(uregex_getTimeLimit(re, &status) == 0);
     uregex_setTimeLimit(re, 1000, &status);
     TEST_ASSERT(uregex_getTimeLimit(re, &status) == 1000);
     TEST_ASSERT_SUCCESS(status);
     uregex_setTimeLimit(re, -1, &status);
     TEST_ASSERT(status == U_ILLEGAL_ARGUMENT_ERROR);
     status = U_ZERO_ERROR;
     TEST_ASSERT(uregex_getTimeLimit(re, &status) == 1000);
     TEST_TEARDOWN;

     


     TEST_SETUP("abc$", "abcdef", 0);
     TEST_ASSERT(uregex_getStackLimit(re, &status) == 8000000);
     uregex_setStackLimit(re, 40000, &status);
     TEST_ASSERT(uregex_getStackLimit(re, &status) == 40000);
     TEST_ASSERT_SUCCESS(status);
     uregex_setStackLimit(re, -1, &status);
     TEST_ASSERT(status == U_ILLEGAL_ARGUMENT_ERROR);
     status = U_ZERO_ERROR;
     TEST_ASSERT(uregex_getStackLimit(re, &status) == 40000);
     TEST_TEARDOWN;
     
     
     






     TEST_SETUP("((.)+\\2)+x", "aaaaaaaaaaaaaaaaaaab", 0)
     callBackContext cbInfo = {4, 0, 0};
     const void     *pContext   = &cbInfo;
     URegexMatchCallback    *returnedFn = &TestCallbackFn;
     
     
     uregex_getMatchCallback(re, &returnedFn, &pContext, &status);
     TEST_ASSERT_SUCCESS(status);
     TEST_ASSERT(returnedFn == NULL);
     TEST_ASSERT(pContext == NULL);
     
     
     
     uregex_setMatchCallback(re, &TestCallbackFn, &cbInfo, &status);
     TEST_ASSERT_SUCCESS(status);
     TEST_ASSERT(cbInfo.numCalls == 0);
     TEST_ASSERT(uregex_matches(re, -1, &status) == FALSE);
     TEST_ASSERT_SUCCESS(status);
     TEST_ASSERT(cbInfo.numCalls > 0);
     
     
     uregex_getMatchCallback(re, &returnedFn, &pContext, &status);
     TEST_ASSERT(returnedFn == &TestCallbackFn);
     TEST_ASSERT(pContext == &cbInfo);

     TEST_TEARDOWN;
}



static void TestBug4315(void) {
    UErrorCode      theICUError = U_ZERO_ERROR;
    URegularExpression *theRegEx;
    UChar           *textBuff;
    const char      *thePattern;
    UChar            theString[100];
    UChar           *destFields[24];
    int32_t         neededLength1;
    int32_t         neededLength2;

    int32_t         wordCount = 0;
    int32_t         destFieldsSize = 24;

    thePattern  = "ck ";
    u_uastrcpy(theString, "The quick brown fox jumped over the slow black turtle.");

    
    theRegEx = uregex_openC(thePattern, 0, NULL, &theICUError);
    TEST_ASSERT_SUCCESS(theICUError);

    
    uregex_setText(theRegEx, theString, u_strlen(theString), &theICUError);
    TEST_ASSERT_SUCCESS(theICUError);

    
    

    wordCount = uregex_split(theRegEx, NULL, 0, &neededLength1, destFields,
        destFieldsSize, &theICUError);

    TEST_ASSERT(theICUError == U_BUFFER_OVERFLOW_ERROR);
    TEST_ASSERT(wordCount==3);

    if(theICUError == U_BUFFER_OVERFLOW_ERROR)
    {
        theICUError = U_ZERO_ERROR;
        textBuff = (UChar *) malloc(sizeof(UChar) * (neededLength1 + 1));
        wordCount = uregex_split(theRegEx, textBuff, neededLength1+1, &neededLength2,
            destFields, destFieldsSize, &theICUError);
        TEST_ASSERT(wordCount==3);
        TEST_ASSERT_SUCCESS(theICUError);
        TEST_ASSERT(neededLength1 == neededLength2);
        TEST_ASSERT_STRING("The qui", destFields[0], TRUE);
        TEST_ASSERT_STRING("brown fox jumped over the slow bla", destFields[1], TRUE);
        TEST_ASSERT_STRING("turtle.", destFields[2], TRUE);
        TEST_ASSERT(destFields[3] == NULL);
        free(textBuff);
    }
    uregex_close(theRegEx);
}


static void TestUTextAPI(void) {
    UErrorCode           status = U_ZERO_ERROR;
    URegularExpression  *re;
    UText                patternText = UTEXT_INITIALIZER;
    UChar                pat[200];
    const char           patternTextUTF8[5] = { 0x61, 0x62, 0x63, 0x2a, 0x00 };

    
    utext_openUTF8(&patternText, patternTextUTF8, -1, &status);
    re = uregex_openUText(&patternText, 0, 0, &status);
    if (U_FAILURE(status)) {
         log_data_err("Failed to open regular expression, %s:%d, error is \"%s\" (Are you missing data?)\n", __FILE__, __LINE__, u_errorName(status));
         utext_close(&patternText);
         return;
    }
    uregex_close(re);

    
    status = U_ZERO_ERROR;
    re = uregex_openUText(&patternText, 
        UREGEX_CASE_INSENSITIVE | UREGEX_COMMENTS | UREGEX_DOTALL | UREGEX_MULTILINE | UREGEX_UWORD,
        0, &status);
    TEST_ASSERT_SUCCESS(status);
    uregex_close(re);

    
    status = U_ZERO_ERROR;
    re = uregex_openUText(&patternText, 0x40000000, 0, &status);
    TEST_ASSERT(status == U_REGEX_INVALID_FLAG);
    uregex_close(re);

    
    status = U_ZERO_ERROR;
    re = uregex_openUText(NULL,
        UREGEX_CASE_INSENSITIVE | UREGEX_COMMENTS | UREGEX_DOTALL | UREGEX_MULTILINE | UREGEX_UWORD, 0, &status);
    TEST_ASSERT(status == U_ILLEGAL_ARGUMENT_ERROR && re == NULL);

    


    {
        URegularExpression *clone1;
        URegularExpression *clone2;
        URegularExpression *clone3;
        UChar  testString1[30];
        UChar  testString2[30];
        UBool  result;


        status = U_ZERO_ERROR;
        re = uregex_openUText(&patternText, 0, 0, &status);
        TEST_ASSERT_SUCCESS(status);
        clone1 = uregex_clone(re, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(clone1 != NULL);

        status = U_ZERO_ERROR;
        clone2 = uregex_clone(re, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(clone2 != NULL);
        uregex_close(re);

        status = U_ZERO_ERROR;
        clone3 = uregex_clone(clone2, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(clone3 != NULL);

        u_uastrncpy(testString1, "abcccd", UPRV_LENGTHOF(pat));
        u_uastrncpy(testString2, "xxxabcccd", UPRV_LENGTHOF(pat));

        status = U_ZERO_ERROR;
        uregex_setText(clone1, testString1, -1, &status);
        TEST_ASSERT_SUCCESS(status);
        result = uregex_lookingAt(clone1, 0, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(result==TRUE);
        
        status = U_ZERO_ERROR;
        uregex_setText(clone2, testString2, -1, &status);
        TEST_ASSERT_SUCCESS(status);
        result = uregex_lookingAt(clone2, 0, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(result==FALSE);
        result = uregex_find(clone2, 0, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(result==TRUE);

        uregex_close(clone1);
        uregex_close(clone2);
        uregex_close(clone3);

    }

    


    {
        const UChar  *resultPat;
        int32_t       resultLen;
        UText        *resultText;
        const char str_hello[] = { 0x68, 0x65, 0x6c, 0x6c, 0x6f, 0x00 }; 
        const char str_hel[] = { 0x68, 0x65, 0x6c, 0x00 }; 
        u_uastrncpy(pat, "hello", UPRV_LENGTHOF(pat)); 
        status = U_ZERO_ERROR;
        
        utext_openUTF8(&patternText, str_hello, -1, &status);
        re = uregex_open(pat, -1, 0, NULL, &status);
        resultPat = uregex_pattern(re, &resultLen, &status);
        TEST_ASSERT_SUCCESS(status);

        
        if (U_SUCCESS(status)) {
            TEST_ASSERT(resultLen == -1);
            TEST_ASSERT(u_strcmp(resultPat, pat) == 0);
        }
        
        resultText = uregex_patternUText(re, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_UTEXT(str_hello, resultText);

        uregex_close(re);

        status = U_ZERO_ERROR;
        re = uregex_open(pat, 3, 0, NULL, &status);
        resultPat = uregex_pattern(re, &resultLen, &status);
        TEST_ASSERT_SUCCESS(status);

        
        if (U_SUCCESS(status)) {
            TEST_ASSERT(resultLen == 3);
            TEST_ASSERT(u_strncmp(resultPat, pat, 3) == 0);
            TEST_ASSERT(u_strlen(resultPat) == 3);
        }
        
        resultText = uregex_patternUText(re, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_UTEXT(str_hel, resultText);

        uregex_close(re);
    }

    


    {
        UText  text1 = UTEXT_INITIALIZER;
        UText  text2 = UTEXT_INITIALIZER;
        UBool  result;
        const char str_abcccd[] = { 0x62, 0x63, 0x64, 0x64, 0x64, 0x65, 0x00 }; 
        const char str_abcccxd[] = { 0x62, 0x63, 0x64, 0x64, 0x64, 0x79, 0x65, 0x00 }; 
        const char str_abcd[] = { 0x62, 0x63, 0x64, 0x2b, 0x65, 0x00 }; 
        status = U_ZERO_ERROR;
        utext_openUTF8(&text1, str_abcccd, -1, &status);
        utext_openUTF8(&text2, str_abcccxd, -1, &status);
        
        utext_openUTF8(&patternText, str_abcd, -1, &status);
        re = uregex_openUText(&patternText, 0, NULL, &status);
        TEST_ASSERT_SUCCESS(status);

        
        status = U_ZERO_ERROR;
        uregex_lookingAt(re, 0, &status);
        TEST_ASSERT( status== U_REGEX_INVALID_STATE);

        status = U_ZERO_ERROR;
        uregex_setUText(re, &text1, &status);
        result = uregex_lookingAt(re, 0, &status);
        TEST_ASSERT(result == TRUE);
        TEST_ASSERT_SUCCESS(status);

        status = U_ZERO_ERROR;
        uregex_setUText(re, &text2, &status);
        result = uregex_lookingAt(re, 0, &status);
        TEST_ASSERT(result == FALSE);
        TEST_ASSERT_SUCCESS(status);

        status = U_ZERO_ERROR;
        uregex_setUText(re, &text1, &status);
        result = uregex_lookingAt(re, 0, &status);
        TEST_ASSERT(result == TRUE);
        TEST_ASSERT_SUCCESS(status);

        uregex_close(re);
        utext_close(&text1);
        utext_close(&text2);
    }


    


    {
        UText  text1 = UTEXT_INITIALIZER;
        UText  text2 = UTEXT_INITIALIZER;
        UChar  text2Chars[20];
        UText  *resultText;
        const UChar   *result;
        int32_t  textLength;
        const char str_abcccd[] = { 0x62, 0x63, 0x64, 0x64, 0x64, 0x65, 0x00 }; 
        const char str_abcccxd[] = { 0x62, 0x63, 0x64, 0x64, 0x64, 0x79, 0x65, 0x00 }; 
        const char str_abcd[] = { 0x62, 0x63, 0x64, 0x2b, 0x65, 0x00 }; 


        status = U_ZERO_ERROR;
        utext_openUTF8(&text1, str_abcccd, -1, &status);
        u_uastrncpy(text2Chars, str_abcccxd, UPRV_LENGTHOF(text2Chars));
        utext_openUChars(&text2, text2Chars, -1, &status);
        
        utext_openUTF8(&patternText, str_abcd, -1, &status);
        re = uregex_openUText(&patternText, 0, NULL, &status);

        
        uregex_setUText(re, &text1, &status);
        resultText = uregex_getUText(re, NULL, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(resultText != &text1);
        utext_setNativeIndex(resultText, 0);
        utext_setNativeIndex(&text1, 0);
        TEST_ASSERT(testUTextEqual(resultText, &text1));
        utext_close(resultText);
        
        result = uregex_getText(re, &textLength, &status); 
        (void)result;    
        TEST_ASSERT(textLength == -1 || textLength == 6);
        resultText = uregex_getUText(re, NULL, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(resultText != &text1);
        utext_setNativeIndex(resultText, 0);
        utext_setNativeIndex(&text1, 0);
        TEST_ASSERT(testUTextEqual(resultText, &text1));
        utext_close(resultText);

        
        uregex_setText(re, text2Chars, 7, &status);
        resultText = uregex_getUText(re, NULL, &status);
        TEST_ASSERT_SUCCESS(status);
        utext_setNativeIndex(resultText, 0);
        utext_setNativeIndex(&text2, 0);
        TEST_ASSERT(testUTextEqual(resultText, &text2));
        utext_close(resultText);
        result = uregex_getText(re, &textLength, &status);
        TEST_ASSERT(textLength == 7);
        
        uregex_close(re);
        utext_close(&text1);
        utext_close(&text2);
    }

    


    {
        UText   text1 = UTEXT_INITIALIZER;
        UBool   result;
        UText   nullText = UTEXT_INITIALIZER;
        const char str_abcccde[] = { 0x61, 0x62, 0x63, 0x63, 0x63, 0x64, 0x65, 0x00 }; 
        const char str_abcd[] = { 0x61, 0x62, 0x63, 0x2a, 0x64, 0x00 }; 

        status = U_ZERO_ERROR;
        utext_openUTF8(&text1, str_abcccde, -1, &status);
        utext_openUTF8(&patternText, str_abcd, -1, &status);
        re = uregex_openUText(&patternText, 0, NULL, &status);

        uregex_setUText(re, &text1, &status);
        result = uregex_matches(re, 0, &status);
        TEST_ASSERT(result == FALSE);
        TEST_ASSERT_SUCCESS(status);
        uregex_close(re);

        status = U_ZERO_ERROR;
        re = uregex_openC(".?", 0, NULL, &status);
        uregex_setUText(re, &text1, &status);
        result = uregex_matches(re, 7, &status);
        TEST_ASSERT(result == TRUE);
        TEST_ASSERT_SUCCESS(status);

        status = U_ZERO_ERROR;
        utext_openUTF8(&nullText, "", -1, &status);
        uregex_setUText(re, &nullText, &status);
        TEST_ASSERT_SUCCESS(status);
        result = uregex_matches(re, 0, &status);
        TEST_ASSERT(result == TRUE);
        TEST_ASSERT_SUCCESS(status);
        
        uregex_close(re);
        utext_close(&text1);
        utext_close(&nullText);
    }


    




    


    {
        UChar    text1[50];
        UBool    result;
        u_uastrncpy(text1, "012rx5rx890rxrx...",  UPRV_LENGTHOF(text1));
        status = U_ZERO_ERROR;
        re = uregex_openC("rx", 0, NULL, &status);

        uregex_setText(re, text1, -1, &status);
        result = uregex_find(re, 0, &status);
        TEST_ASSERT(result == TRUE);
        TEST_ASSERT(uregex_start(re, 0, &status) == 3);
        TEST_ASSERT(uregex_end(re, 0, &status) == 5);
        TEST_ASSERT_SUCCESS(status);

        result = uregex_find(re, 9, &status);
        TEST_ASSERT(result == TRUE);
        TEST_ASSERT(uregex_start(re, 0, &status) == 11);
        TEST_ASSERT(uregex_end(re, 0, &status) == 13);
        TEST_ASSERT_SUCCESS(status);

        result = uregex_find(re, 14, &status);
        TEST_ASSERT(result == FALSE);
        TEST_ASSERT_SUCCESS(status);

        status = U_ZERO_ERROR;
        uregex_reset(re, 0, &status);

        result = uregex_findNext(re, &status);
        TEST_ASSERT(result == TRUE);
        TEST_ASSERT(uregex_start(re, 0, &status) == 3);
        TEST_ASSERT(uregex_end(re, 0, &status) == 5);
        TEST_ASSERT_SUCCESS(status);

        result = uregex_findNext(re, &status);
        TEST_ASSERT(result == TRUE);
        TEST_ASSERT(uregex_start(re, 0, &status) == 6);
        TEST_ASSERT(uregex_end(re, 0, &status) == 8);
        TEST_ASSERT_SUCCESS(status);

        status = U_ZERO_ERROR;
        uregex_reset(re, 12, &status);

        result = uregex_findNext(re, &status);
        TEST_ASSERT(result == TRUE);
        TEST_ASSERT(uregex_start(re, 0, &status) == 13);
        TEST_ASSERT(uregex_end(re, 0, &status) == 15);
        TEST_ASSERT_SUCCESS(status);

        result = uregex_findNext(re, &status);
        TEST_ASSERT(result == FALSE);
        TEST_ASSERT_SUCCESS(status);

        uregex_close(re);
    }

    


    {
        UChar    text1[80];
        UText   *actual;
        UBool    result;
        int64_t  groupLen = 0;
        UChar    groupBuf[20];

        u_uastrncpy(text1, "noise abc interior def, and this is off the end",  UPRV_LENGTHOF(text1));

        status = U_ZERO_ERROR;
        re = uregex_openC("abc(.*?)def", 0, NULL, &status);
        TEST_ASSERT_SUCCESS(status);

        uregex_setText(re, text1, -1, &status);
        result = uregex_find(re, 0, &status);
        TEST_ASSERT(result==TRUE);

        
        status = U_ZERO_ERROR;
        actual = uregex_groupUText(re, 0, NULL, &groupLen, &status);
        TEST_ASSERT_SUCCESS(status);

        TEST_ASSERT(utext_getNativeIndex(actual) == 6);  
        TEST_ASSERT(groupLen == 16);   
        utext_extract(actual, 6 , 6+16 , groupBuf, sizeof(groupBuf), &status);

        TEST_ASSERT_STRING("abc interior def", groupBuf, TRUE);
        utext_close(actual);

        
        status = U_ZERO_ERROR;

        actual = uregex_groupUText(re, 1, NULL, &groupLen, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT(9 == utext_getNativeIndex(actual));    
                                                           
        TEST_ASSERT(10 == groupLen);                       
        utext_extract(actual, 9 , 9+10 , groupBuf, sizeof(groupBuf), &status);
        TEST_ASSERT_STRING(" interior ", groupBuf, TRUE);

        utext_close(actual);

        
        status = U_ZERO_ERROR;
        actual = uregex_groupUText(re, 2, NULL, &groupLen, &status);
        TEST_ASSERT(status == U_INDEX_OUTOFBOUNDS_ERROR);
        utext_close(actual);

        uregex_close(re);
    }
    
    


    {
        UChar    text1[80];
        UChar    text2[80];
        UText    replText = UTEXT_INITIALIZER;
        UText   *result;
        const char str_Replxxx[] = { 0x52, 0x65, 0x70, 0x6c, 0x61, 0x63, 0x65, 0x20, 0x3c, 0x61, 0x61, 0x3e, 0x20, 0x78, 0x31, 0x78, 0x20, 0x78, 0x2e, 0x2e, 0x2e, 0x78, 0x2e, 0x00 }; 
        const char str_Nomatchhere[] = { 0x4e, 0x6f, 0x20, 0x6d, 0x61, 0x74, 0x63, 0x68, 0x20, 0x68, 0x65, 0x72, 0x65, 0x2e, 0x00 }; 
        const char str_u00411U00000042a[] =  { 0x5c, 0x5c, 0x5c, 0x75, 0x30, 0x30, 0x34, 0x31, 0x24, 0x31, 
               0x5c, 0x55, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x34, 0x32, 0x5c, 0x24, 0x5c, 0x61, 0x00 }; 
        const char str_1x[] = { 0x3c, 0x24, 0x31, 0x3e, 0x00 }; 
        const char str_ReplaceAaaBax1xxx[] = { 0x52, 0x65, 0x70, 0x6c, 0x61, 0x63, 0x65, 0x20, 0x5c, 0x41, 0x61, 0x61, 0x42, 0x24, 0x61, 0x20, 0x78, 0x31, 0x78, 0x20, 0x78, 0x2e, 0x2e, 0x2e, 0x78, 0x2e, 0x00 }; 
        status = U_ZERO_ERROR;
        u_uastrncpy(text1, "Replace xaax x1x x...x.",  UPRV_LENGTHOF(text1));
        u_uastrncpy(text2, "No match here.",  UPRV_LENGTHOF(text2));
        utext_openUTF8(&replText, str_1x, -1, &status);

        re = uregex_openC("x(.*?)x", 0, NULL, &status);
        TEST_ASSERT_SUCCESS(status);

        
        uregex_setText(re, text1, -1, &status);
        result = uregex_replaceFirstUText(re, &replText, NULL, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_UTEXT(str_Replxxx, result);
        utext_close(result);

        
        uregex_setText(re, text2, -1, &status);
        result = uregex_replaceFirstUText(re, &replText, NULL, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_UTEXT(str_Nomatchhere, result);
        utext_close(result);
        
        
        uregex_setText(re, text1, -1, &status);
        utext_openUTF8(&replText, str_u00411U00000042a, -1, &status);
        result = uregex_replaceFirstUText(re, &replText, NULL, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_UTEXT(str_ReplaceAaaBax1xxx, result);
        utext_close(result);

        uregex_close(re);
        utext_close(&replText);
    }


    


    {
        UChar    text1[80];
        UChar    text2[80];
        UText    replText = UTEXT_INITIALIZER;
        UText   *result;
        const char str_1[] = { 0x3c, 0x24, 0x31, 0x3e, 0x00 }; 
        const char str_Replaceaa1[] = { 0x52, 0x65, 0x70, 0x6c, 0x61, 0x63, 0x65, 0x20, 0x3c, 0x61, 0x61, 0x3e, 0x20, 0x3c, 0x31, 0x3e, 0x20, 0x3c, 0x2e, 0x2e, 0x2e, 0x3e, 0x2e, 0x00 }; 
        const char str_Nomatchhere[] = { 0x4e, 0x6f, 0x20, 0x6d, 0x61, 0x74, 0x63, 0x68, 0x20, 0x68, 0x65, 0x72, 0x65, 0x2e, 0x00 }; 
        status = U_ZERO_ERROR;
        u_uastrncpy(text1, "Replace xaax x1x x...x.",  UPRV_LENGTHOF(text1));
        u_uastrncpy(text2, "No match here.",  UPRV_LENGTHOF(text2));
        utext_openUTF8(&replText, str_1, -1, &status);

        re = uregex_openC("x(.*?)x", 0, NULL, &status);
        TEST_ASSERT_SUCCESS(status);

        
        uregex_setText(re, text1, -1, &status);
        result = uregex_replaceAllUText(re, &replText, NULL, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_UTEXT(str_Replaceaa1, result);
        utext_close(result);

        
        uregex_setText(re, text2, -1, &status);
        result = uregex_replaceAllUText(re, &replText, NULL, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_UTEXT(str_Nomatchhere, result);
        utext_close(result);

        uregex_close(re);
        utext_close(&replText);
    }


    


    {
        UChar    text[100];
        UChar    repl[100];
        UChar    buf[100];
        UChar   *bufPtr;
        int32_t  bufCap;

        status = U_ZERO_ERROR;
        re = uregex_openC(".*", 0, 0, &status);
        TEST_ASSERT_SUCCESS(status);

        u_uastrncpy(text, "whatever",  UPRV_LENGTHOF(text));
        u_uastrncpy(repl, "some other", UPRV_LENGTHOF(repl));
        uregex_setText(re, text, -1, &status);

        
        uregex_find(re, 0, &status);
        TEST_ASSERT_SUCCESS(status);
        bufPtr = buf;
        bufCap = UPRV_LENGTHOF(buf);
        uregex_appendReplacement(re, repl, -1, &bufPtr, &bufCap, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_STRING("some other", buf, TRUE);

        
        uregex_find(re, 0, &status);
        TEST_ASSERT_SUCCESS(status);
        bufPtr = buf;
        bufCap = UPRV_LENGTHOF(buf);
        u_uastrncpy(repl, "abc\\u0041\\U00000042 \\\\ \\$ \\abc", UPRV_LENGTHOF(repl));
        uregex_appendReplacement(re, repl, -1, &bufPtr, &bufCap, &status);
        TEST_ASSERT_SUCCESS(status);
        TEST_ASSERT_STRING("abcAB \\ $ abc", buf, TRUE); 

        uregex_close(re);
    }


    



    


    {
        UChar    textToSplit[80];
        UChar    text2[80];
        UText    *fields[10];
        int32_t  numFields;
        int32_t i;

        u_uastrncpy(textToSplit, "first : second:  third",  UPRV_LENGTHOF(textToSplit));
        u_uastrncpy(text2, "No match here.",  UPRV_LENGTHOF(text2));

        status = U_ZERO_ERROR;
        re = uregex_openC(":", 0, NULL, &status);


         

        uregex_setText(re, textToSplit, -1, &status);
        TEST_ASSERT_SUCCESS(status);

        
        if (U_SUCCESS(status)) {
            memset(fields, 0, sizeof(fields));
            numFields = uregex_splitUText(re, fields, 10, &status);
            TEST_ASSERT_SUCCESS(status);

            
            if(U_SUCCESS(status)) {
              const char str_first[] = { 0x66, 0x69, 0x72, 0x73, 0x74, 0x20, 0x00 }; 
              const char str_second[] = { 0x20, 0x73, 0x65, 0x63, 0x6f, 0x6e, 0x64, 0x00 }; 
              const char str_third[] = { 0x20, 0x20, 0x74, 0x68, 0x69, 0x72, 0x64, 0x00 }; 
                TEST_ASSERT(numFields == 3);
                TEST_ASSERT_UTEXT(str_first,  fields[0]);
                TEST_ASSERT_UTEXT(str_second, fields[1]);
                TEST_ASSERT_UTEXT(str_third, fields[2]);
                TEST_ASSERT(fields[3] == NULL);
            }
            for(i = 0; i < numFields; i++) {
                utext_close(fields[i]);
            }
        }

        uregex_close(re);

    
        
        status = U_ZERO_ERROR;
        re = uregex_openC(":", 0, NULL, &status);
        uregex_setText(re, textToSplit, -1, &status);
        TEST_ASSERT_SUCCESS(status);

        
        if(U_SUCCESS(status)) {
            fields[0] = NULL;
            fields[1] = NULL;
            fields[2] = &patternText;
            numFields = uregex_splitUText(re, fields, 2, &status);
            TEST_ASSERT_SUCCESS(status);

            
            if(U_SUCCESS(status)) {
                const char str_first[] = { 0x66, 0x69, 0x72, 0x73, 0x74, 0x20, 0x00 }; 
                const char str_secondthird[] = { 0x20, 0x73, 0x65, 0x63, 0x6f, 0x6e, 0x64, 0x3a, 0x20, 0x20, 0x74, 0x68, 0x69, 0x72, 0x64, 0x00 }; 
                TEST_ASSERT(numFields == 2);
                TEST_ASSERT_UTEXT(str_first,  fields[0]);
                TEST_ASSERT_UTEXT(str_secondthird, fields[1]);
                TEST_ASSERT(fields[2] == &patternText);
            }
            for(i = 0; i < numFields; i++) {
                utext_close(fields[i]);
            }
        }

        uregex_close(re);
    }

    

    {
        UChar    textToSplit[80];
        UText    *fields[10];
        int32_t  numFields;
        int32_t i;

        u_uastrncpy(textToSplit, "first <tag-a> second<tag-b>  third",  UPRV_LENGTHOF(textToSplit));

        status = U_ZERO_ERROR;
        re = uregex_openC("<(.*?)>", 0, NULL, &status);

        uregex_setText(re, textToSplit, -1, &status);
        TEST_ASSERT_SUCCESS(status);

        
        if(U_SUCCESS(status)) {
            memset(fields, 0, sizeof(fields));
            numFields = uregex_splitUText(re, fields, 10, &status);
            TEST_ASSERT_SUCCESS(status);

            
            if(U_SUCCESS(status)) {
                const char str_first[] = { 0x66, 0x69, 0x72, 0x73, 0x74, 0x20, 0x00 }; 
                const char str_taga[] = { 0x74, 0x61, 0x67, 0x2d, 0x61, 0x00 }; 
                const char str_second[] = { 0x20, 0x73, 0x65, 0x63, 0x6f, 0x6e, 0x64, 0x00 }; 
                const char str_tagb[] = { 0x74, 0x61, 0x67, 0x2d, 0x62, 0x00 }; 
                const char str_third[] = { 0x20, 0x20, 0x74, 0x68, 0x69, 0x72, 0x64, 0x00 }; 

                TEST_ASSERT(numFields == 5);
                TEST_ASSERT_UTEXT(str_first,  fields[0]);
                TEST_ASSERT_UTEXT(str_taga,   fields[1]);
                TEST_ASSERT_UTEXT(str_second, fields[2]);
                TEST_ASSERT_UTEXT(str_tagb,   fields[3]);
                TEST_ASSERT_UTEXT(str_third, fields[4]);
                TEST_ASSERT(fields[5] == NULL);
            }
            for(i = 0; i < numFields; i++) {
                utext_close(fields[i]);
            }
        }
    
        
        status = U_ZERO_ERROR;
        fields[0] = NULL;
        fields[1] = NULL;
        fields[2] = &patternText;
        numFields = uregex_splitUText(re, fields, 2, &status);
        TEST_ASSERT_SUCCESS(status);

        
        if(U_SUCCESS(status)) {
            const char str_first[] = { 0x66, 0x69, 0x72, 0x73, 0x74, 0x20, 0x00 }; 
            const char str_secondtagbthird[] = { 0x20, 0x73, 0x65, 0x63, 0x6f, 0x6e, 0x64, 0x3c, 0x74, 0x61, 0x67, 0x2d, 0x62, 0x3e, 0x20, 0x20, 0x74, 0x68, 0x69, 0x72, 0x64, 0x00 }; 
            TEST_ASSERT(numFields == 2);
            TEST_ASSERT_UTEXT(str_first,  fields[0]);
            TEST_ASSERT_UTEXT(str_secondtagbthird, fields[1]);
            TEST_ASSERT(fields[2] == &patternText);
        }
        for(i = 0; i < numFields; i++) {
            utext_close(fields[i]);
        }


        
        status = U_ZERO_ERROR;
        fields[0] = NULL;
        fields[1] = NULL;
        fields[2] = NULL;
        fields[3] = &patternText;
        numFields = uregex_splitUText(re, fields, 3, &status);
        TEST_ASSERT_SUCCESS(status);

        
        if(U_SUCCESS(status)) {
            const char str_first[] = { 0x66, 0x69, 0x72, 0x73, 0x74, 0x20, 0x00 }; 
            const char str_taga[] = { 0x74, 0x61, 0x67, 0x2d, 0x61, 0x00 }; 
            const char str_secondtagbthird[] = { 0x20, 0x73, 0x65, 0x63, 0x6f, 0x6e, 0x64, 0x3c, 0x74, 0x61, 0x67, 0x2d, 0x62, 0x3e, 0x20, 0x20, 0x74, 0x68, 0x69, 0x72, 0x64, 0x00 }; 
            TEST_ASSERT(numFields == 3);
            TEST_ASSERT_UTEXT(str_first,  fields[0]);
            TEST_ASSERT_UTEXT(str_taga,   fields[1]);
            TEST_ASSERT_UTEXT(str_secondtagbthird, fields[2]);
            TEST_ASSERT(fields[3] == &patternText);
        }
        for(i = 0; i < numFields; i++) {
            utext_close(fields[i]);
        }

        
        status = U_ZERO_ERROR;
        fields[0] = NULL;
        fields[1] = NULL;
        fields[2] = NULL;
        fields[3] = NULL;
        fields[4] = NULL;
        fields[5] = &patternText;
        numFields = uregex_splitUText(re, fields, 5, &status);
        TEST_ASSERT_SUCCESS(status);

        
        if(U_SUCCESS(status)) {
            const char str_first[] = { 0x66, 0x69, 0x72, 0x73, 0x74, 0x20, 0x00 }; 
            const char str_taga[] = { 0x74, 0x61, 0x67, 0x2d, 0x61, 0x00 }; 
            const char str_second[] = { 0x20, 0x73, 0x65, 0x63, 0x6f, 0x6e, 0x64, 0x00 }; 
            const char str_tagb[] = { 0x74, 0x61, 0x67, 0x2d, 0x62, 0x00 }; 
            const char str_third[] = { 0x20, 0x20, 0x74, 0x68, 0x69, 0x72, 0x64, 0x00 }; 

            TEST_ASSERT(numFields == 5);
            TEST_ASSERT_UTEXT(str_first,  fields[0]);
            TEST_ASSERT_UTEXT(str_taga,   fields[1]);
            TEST_ASSERT_UTEXT(str_second, fields[2]);
            TEST_ASSERT_UTEXT(str_tagb,   fields[3]);
            TEST_ASSERT_UTEXT(str_third, fields[4]);
            TEST_ASSERT(fields[5] == &patternText);
        }
        for(i = 0; i < numFields; i++) {
            utext_close(fields[i]);
        }

        
        status = U_ZERO_ERROR;
        uregex_setText(re, textToSplit, strlen("first <tag-a> second<tag-b>"), &status);
        TEST_ASSERT_SUCCESS(status);

        
        if(U_SUCCESS(status)) {
            memset(fields, 0, sizeof(fields));
            fields[9] = &patternText;
            numFields = uregex_splitUText(re, fields, 9, &status);
            TEST_ASSERT_SUCCESS(status);

            
            if(U_SUCCESS(status)) {
                const char str_first[] = { 0x66, 0x69, 0x72, 0x73, 0x74, 0x20, 0x00 }; 
                const char str_taga[] = { 0x74, 0x61, 0x67, 0x2d, 0x61, 0x00 }; 
                const char str_second[] = { 0x20, 0x73, 0x65, 0x63, 0x6f, 0x6e, 0x64, 0x00 }; 
                const char str_tagb[] = { 0x74, 0x61, 0x67, 0x2d, 0x62, 0x00 }; 
                const char str_empty[] = { 0x00 };

                TEST_ASSERT(numFields == 5);
                TEST_ASSERT_UTEXT(str_first,  fields[0]);
                TEST_ASSERT_UTEXT(str_taga,   fields[1]);
                TEST_ASSERT_UTEXT(str_second, fields[2]);
                TEST_ASSERT_UTEXT(str_tagb,   fields[3]);
                TEST_ASSERT_UTEXT(str_empty,  fields[4]);
                TEST_ASSERT(fields[5] == NULL);
                TEST_ASSERT(fields[8] == NULL);
                TEST_ASSERT(fields[9] == &patternText);
            }
            for(i = 0; i < numFields; i++) {
                utext_close(fields[i]);
            }
        }

        uregex_close(re);
    }
    utext_close(&patternText);
}


static void TestRefreshInput(void) {
    






    UChar testStr[]  = {0x41, 0x20, 0x42, 0x20, 0x43, 0x0};  
    UChar movedStr[] = {   0,    0,    0,    0,    0,   0};
    UErrorCode status = U_ZERO_ERROR;
    URegularExpression *re;
    UText ut1 = UTEXT_INITIALIZER;
    UText ut2 = UTEXT_INITIALIZER;
    
    re = uregex_openC("[ABC]", 0, 0, &status);
    TEST_ASSERT_SUCCESS(status);

    utext_openUChars(&ut1, testStr, -1, &status);
    TEST_ASSERT_SUCCESS(status);
    uregex_setUText(re, &ut1, &status);
    TEST_ASSERT_SUCCESS(status);

    
    TEST_ASSERT(uregex_findNext(re, &status));
    TEST_ASSERT(uregex_start(re, 0, &status) == 0);
    
    
    u_strcpy(movedStr, testStr);
    u_memset(testStr, 0, u_strlen(testStr));
    utext_openUChars(&ut2, movedStr, -1, &status);
    TEST_ASSERT_SUCCESS(status);
    uregex_refreshUText(re, &ut2, &status);
    TEST_ASSERT_SUCCESS(status);

    
    TEST_ASSERT(uregex_findNext(re, &status));
    TEST_ASSERT(uregex_start(re, 0, &status) == 2);
    TEST_ASSERT(uregex_findNext(re, &status));
    TEST_ASSERT(uregex_start(re, 0, &status) == 4);
    TEST_ASSERT(FALSE == uregex_findNext(re, &status));

    uregex_close(re);
}


static void TestBug8421(void) {
    


    URegularExpression *re;
    UErrorCode status = U_ZERO_ERROR;
    int32_t  limit = -1;

    re = uregex_openC("abc", 0, 0, &status);
    TEST_ASSERT_SUCCESS(status);

    limit = uregex_getTimeLimit(re, &status);
    TEST_ASSERT_SUCCESS(status);
    TEST_ASSERT(limit == 0);

    uregex_setTimeLimit(re, 100, &status);
    TEST_ASSERT_SUCCESS(status);
    limit = uregex_getTimeLimit(re, &status);
    TEST_ASSERT_SUCCESS(status);
    TEST_ASSERT(limit == 100);

    uregex_close(re);
}

static UBool U_CALLCONV FindCallback(const void* context , int64_t matchIndex) {
    return FALSE;
}

static UBool U_CALLCONV MatchCallback(const void *context, int32_t steps) {
    return FALSE;
}

static void TestBug10815() {
  


    URegularExpression *re;
    UErrorCode status = U_ZERO_ERROR;
    UChar    text[100];


    

    re = uregex_openC(".z", 0, 0, &status);
    TEST_ASSERT_SUCCESS(status);

    u_uastrncpy(text, "Hello, World.",  UPRV_LENGTHOF(text));
    uregex_setText(re, text, -1, &status);
    TEST_ASSERT_SUCCESS(status);

    uregex_setFindProgressCallback(re, FindCallback, NULL, &status);
    TEST_ASSERT_SUCCESS(status);

    uregex_findNext(re, &status);
    TEST_ASSERT(status == U_REGEX_STOPPED_BY_CALLER);

    uregex_close(re);

    

    status = U_ZERO_ERROR;
    re = uregex_openC("((xxx)*)*y", 0, 0, &status);
    TEST_ASSERT_SUCCESS(status);

    
    
    u_uastrncpy(text, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",  UPRV_LENGTHOF(text));
    uregex_setText(re, text, -1, &status);
    TEST_ASSERT_SUCCESS(status);

    uregex_setMatchCallback(re, MatchCallback, NULL, &status);
    TEST_ASSERT_SUCCESS(status);

    uregex_findNext(re, &status);
    TEST_ASSERT(status == U_REGEX_STOPPED_BY_CALLER);

    uregex_close(re);
}

    
#endif   
