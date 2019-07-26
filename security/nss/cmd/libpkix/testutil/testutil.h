









#ifndef _TESTUTIL_H
#define _TESTUTIL_H

#include "pkix.h"
#include "plstr.h"
#include "prprf.h"
#include "prlong.h"
#include "pkix_pl_common.h"
#include "secutil.h"
#include <stdio.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif


























#define PKIX_TEST_STD_VARS() \
        PKIX_Error *pkixTestErrorResult = NULL; \
        char *pkixTestErrorMsg = NULL;











#define PKIX_TEST_EXPECT_NO_ERROR(func) \
        do { \
                pkixTestErrorResult = (func); \
                if (pkixTestErrorResult) { \
                        goto cleanup; \
                } \
        } while (0)












#define PKIX_TEST_EXPECT_ERROR(func) \
        do { \
                pkixTestErrorResult = (func); \
                if (!pkixTestErrorResult){ \
                        pkixTestErrorMsg = \
                                "Should have thrown an error here."; \
                        goto cleanup; \
                } \
                PKIX_TEST_DECREF_BC(pkixTestErrorResult); \
        } while (0)









#define PKIX_TEST_DECREF_BC(obj) \
        do { \
                if (obj){ \
                        PKIX_TEST_EXPECT_NO_ERROR \
                        (PKIX_PL_Object_DecRef \
                                ((PKIX_PL_Object*)(obj), plContext)); \
                obj = NULL; \
                } \
        } while (0)












#define PKIX_TEST_DECREF_AC(obj) \
        do { \
                if (obj){ \
                        PKIX_Error *pkixTestTempResult = NULL; \
                        pkixTestTempResult = \
                        PKIX_PL_Object_DecRef \
                                ((PKIX_PL_Object*)(obj), plContext); \
                        if (pkixTestTempResult) \
                                pkixTestErrorResult = pkixTestTempResult; \
                        obj = NULL; \
                } \
        } while (0)










#define PKIX_TEST_RETURN() \
        { \
                if (pkixTestErrorMsg){ \
                        testError(pkixTestErrorMsg); \
                } else if (pkixTestErrorResult){ \
                        pkixTestErrorMsg = \
                                PKIX_Error2ASCII \
                                        (pkixTestErrorResult, plContext); \
                        if (pkixTestErrorMsg) { \
                                testError(pkixTestErrorMsg); \
                                PKIX_PL_Free \
                                        ((PKIX_PL_Object *)pkixTestErrorMsg, \
                                        plContext); \
                        } else { \
                                testError("PKIX_Error2ASCII Failed"); \
                        } \
                        if (pkixTestErrorResult != PKIX_ALLOC_ERROR()){ \
                                PKIX_PL_Object_DecRef \
                                ((PKIX_PL_Object*)pkixTestErrorResult, \
                                plContext); \
                                pkixTestErrorResult = NULL; \
                        } \
                } \
        }

















#define PKIX_TEST_EQ_HASH_TOSTR_DUP(goodObj, equalObj, diffObj, \
                                        expAscii, type, checkDuplicate) \
        do { \
                subTest("PKIX_PL_" #type "_Equals   <match>"); \
                testEqualsHelper \
                        ((PKIX_PL_Object *)(goodObj), \
                        (PKIX_PL_Object *)(equalObj), \
                        PKIX_TRUE, \
                        plContext); \
                subTest("PKIX_PL_" #type "_Hashcode <match>"); \
                testHashcodeHelper \
                        ((PKIX_PL_Object *)(goodObj), \
                        (PKIX_PL_Object *)(equalObj), \
                        PKIX_TRUE, \
                        plContext); \
                subTest("PKIX_PL_" #type "_Equals   <non-match>"); \
                testEqualsHelper \
                        ((PKIX_PL_Object *)(goodObj), \
                        (PKIX_PL_Object *)(diffObj), \
                        PKIX_FALSE, \
                        plContext); \
                subTest("PKIX_PL_" #type "_Hashcode <non-match>"); \
                testHashcodeHelper \
                        ((PKIX_PL_Object *)(goodObj), \
                        (PKIX_PL_Object *)(diffObj), \
                        PKIX_FALSE, \
                        plContext); \
                if (expAscii){ \
                        subTest("PKIX_PL_" #type "_ToString"); \
                        testToStringHelper \
                                ((PKIX_PL_Object *)(goodObj), \
                                (expAscii), \
                                plContext); } \
                if (checkDuplicate){ \
                        subTest("PKIX_PL_" #type "_Duplicate"); \
                        testDuplicateHelper \
                                ((PKIX_PL_Object *)goodObj, plContext); } \
        } while (0)









#define PKIX_TEST_ABORT_ON_NULL(obj) \
        do { \
                if (!obj){ \
                        goto cleanup; \
                } \
        } while (0)

#define PKIX_TEST_ARENAS_ARG(arena) \
        (arena? \
        (PORT_Strcmp(arena, "arenas") ? PKIX_FALSE : (j++, PKIX_TRUE)): \
        PKIX_FALSE)

#define PKIX_TEST_ERROR_RECEIVED (pkixTestErrorMsg || pkixTestErrorResult)



void startTests(char *testName);

void endTests(char *testName);

void subTest(char *subTestName);

void testError(char *msg);

extern PKIX_Error *
_ErrorCheck(PKIX_Error *errorResult);

extern PKIX_Error *
_OutputError(PKIX_Error *errorResult);

char* PKIX_String2ASCII(PKIX_PL_String *string, void *plContext);

char* PKIX_Error2ASCII(PKIX_Error *error, void *plContext);

char* PKIX_Object2ASCII(PKIX_PL_Object *object);

char *PKIX_Cert2ASCII(PKIX_PL_Cert *cert);

void
testHashcodeHelper(
        PKIX_PL_Object *goodObject,
        PKIX_PL_Object *otherObject,
        PKIX_Boolean match,
        void *plContext);

void
testToStringHelper(
        PKIX_PL_Object *goodObject,
        char *expected,
        void *plContext);

void
testEqualsHelper(
        PKIX_PL_Object *goodObject,
        PKIX_PL_Object *otherObject,
        PKIX_Boolean match,
        void *plContext);

void
testDuplicateHelper(
        PKIX_PL_Object *object,
        void *plContext);
void
testErrorUndo(char *msg);

#ifdef __cplusplus
}
#endif

#endif
