









#include "testutil.h"





static int errCount = 0;


















void
startTests(char *testName)
{
        (void) printf("*START OF TESTS FOR %s:\n", testName);
        errCount = 0;
}


















void
endTests(char *testName)
{
        char plural = ' ';

        (void) printf("*END OF TESTS FOR %s: ", testName);
        if (errCount > 0) {
                if (errCount > 1) plural = 's';
                (void) printf("%d SUBTEST%c FAILED.\n\n", errCount, plural);
        } else {
                (void) printf("ALL TESTS COMPLETED SUCCESSFULLY.\n\n");
        }
}

















void
subTest(char *subTestName)
{
        (void) printf("TESTING: %s ...\n", subTestName);
}




















void
testErrorUndo(char *msg)
{
        --errCount;
        (void) printf("TEST FAILURE *** EXPECTED *** :%s\n", msg);
}



















void
testError(char *msg)
{
        ++errCount;
        (void) printf("TEST FAILURE: %s\n", msg);
}





















char *
PKIX_String2ASCII(PKIX_PL_String *string, void *plContext)
{
        PKIX_UInt32 length;
        char *asciiString = NULL;
        PKIX_Error *errorResult;

        errorResult = PKIX_PL_String_GetEncoded
                (string,
                PKIX_ESCASCII,
                (void **)&asciiString,
                &length,
                plContext);

        if (errorResult) goto cleanup;

cleanup:

        if (errorResult){
                return (NULL);
        }

        return (asciiString);

}



















char *
PKIX_Error2ASCII(PKIX_Error *error, void *plContext)
{
        PKIX_UInt32 length;
        char *asciiString = NULL;
        PKIX_PL_String *pkixString = NULL;
        PKIX_Error *errorResult = NULL;

        errorResult = PKIX_PL_Object_ToString
                ((PKIX_PL_Object*)error, &pkixString, plContext);
        if (errorResult) goto cleanup;

        errorResult = PKIX_PL_String_GetEncoded
                (pkixString,
                PKIX_ESCASCII,
                (void **)&asciiString,
                &length,
                plContext);

cleanup:

        if (pkixString){
                if (PKIX_PL_Object_DecRef
                    ((PKIX_PL_Object*)pkixString, plContext)){
                        return (NULL);
                }
        }

        if (errorResult){
                return (NULL);
        }

        return (asciiString);
}

















char *
PKIX_Object2ASCII(PKIX_PL_Object *object)
{
        PKIX_UInt32 length;
        char *asciiString = NULL;
        PKIX_PL_String *pkixString = NULL;
        PKIX_Error *errorResult = NULL;

        errorResult = PKIX_PL_Object_ToString
                (object, &pkixString, NULL);
        if (errorResult) goto cleanup;

        errorResult = PKIX_PL_String_GetEncoded
            (pkixString, PKIX_ESCASCII, (void **)&asciiString, &length, NULL);

cleanup:

        if (pkixString){
                if (PKIX_PL_Object_DecRef((PKIX_PL_Object*)pkixString, NULL)){
                        return (NULL);
                }
        }

        if (errorResult){
                return (NULL);
        }

        return (asciiString);
}

















char *
PKIX_Cert2ASCII(PKIX_PL_Cert *cert)
{
        PKIX_PL_X500Name *issuer = NULL;
        void *issuerAscii = NULL;
        PKIX_PL_X500Name *subject = NULL;
        void *subjectAscii = NULL;
        void *asciiString = NULL;
        PKIX_Error *errorResult = NULL;
        PKIX_UInt32 numChars;

        
        errorResult = PKIX_PL_Cert_GetIssuer(cert, &issuer, NULL);
        if (errorResult) goto cleanup;

        issuerAscii = PKIX_Object2ASCII((PKIX_PL_Object*)issuer);

        
        errorResult = PKIX_PL_Cert_GetSubject(cert, &subject, NULL);
        if (errorResult) goto cleanup;

        if (subject){
                subjectAscii = PKIX_Object2ASCII((PKIX_PL_Object*)subject);
        }

        errorResult = PKIX_PL_Malloc(200, &asciiString, NULL);
        if (errorResult) goto cleanup;

        numChars =
                PR_snprintf
                (asciiString,
                200,
                "Issuer=%s\nSubject=%s\n",
                issuerAscii,
                subjectAscii);

        if (!numChars) goto cleanup;

cleanup:

        if (issuer){
                if (PKIX_PL_Object_DecRef((PKIX_PL_Object*)issuer, NULL)){
                        return (NULL);
                }
        }

        if (subject){
                if (PKIX_PL_Object_DecRef((PKIX_PL_Object*)subject, NULL)){
                        return (NULL);
                }
        }

        if (PKIX_PL_Free((PKIX_PL_Object*)issuerAscii, NULL)){
                return (NULL);
        }

        if (PKIX_PL_Free((PKIX_PL_Object*)subjectAscii, NULL)){
                return (NULL);
        }

        if (errorResult){
                return (NULL);
        }

        return (asciiString);
}
























void
testHashcodeHelper(
        PKIX_PL_Object *goodObject,
        PKIX_PL_Object *otherObject,
        PKIX_Boolean match,
        void *plContext)
{

        PKIX_UInt32 goodHash;
        PKIX_UInt32 otherHash;
        PKIX_Boolean cmpResult;
        PKIX_TEST_STD_VARS();

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_Object_Hashcode
                        ((PKIX_PL_Object *)goodObject, &goodHash, plContext));

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_Object_Hashcode
                        ((PKIX_PL_Object *)otherObject, &otherHash, plContext));

        cmpResult = (goodHash == otherHash);

        if ((match && !cmpResult) || (!match && cmpResult)){
                testError("unexpected mismatch");
                (void) printf("Hash1:\t%d\n", goodHash);
                (void) printf("Hash2:\t%d\n", otherHash);
        }

cleanup:

        PKIX_TEST_RETURN();

}





















void
testToStringHelper(
        PKIX_PL_Object *goodObject,
        char *expected,
        void *plContext)
{
        PKIX_PL_String *stringRep = NULL;
        char *actual = NULL;
        PKIX_TEST_STD_VARS();

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_Object_ToString
                        (goodObject, &stringRep, plContext));

        actual = PKIX_String2ASCII(stringRep, plContext);
        if (actual == NULL){
                pkixTestErrorMsg = "PKIX_String2ASCII Failed";
                goto cleanup;
        }

        




        






        if (PL_strcmp(actual, expected) != 0){
                testError("unexpected mismatch");
                (void) printf("Actual value:\t%s\n", actual);
                (void) printf("Expected value:\t%s\n", expected);
        }

cleanup:

        PKIX_PL_Free(actual, plContext);

        PKIX_TEST_DECREF_AC(stringRep);

        PKIX_TEST_RETURN();
}























void
testEqualsHelper(
        PKIX_PL_Object *goodObject,
        PKIX_PL_Object *otherObject,
        PKIX_Boolean match,
        void *plContext)
{

        PKIX_Boolean cmpResult;
        PKIX_TEST_STD_VARS();

        PKIX_TEST_EXPECT_NO_ERROR
                (PKIX_PL_Object_Equals
                (goodObject, otherObject, &cmpResult, plContext));

        if ((match && !cmpResult) || (!match && cmpResult)){
                testError("unexpected mismatch");
                (void) printf("Actual value:\t%d\n", cmpResult);
                (void) printf("Expected value:\t%d\n", match);
        }

cleanup:

        PKIX_TEST_RETURN();
}

















void
testDuplicateHelper(PKIX_PL_Object *object, void *plContext)
{
        PKIX_PL_Object *newObject = NULL;
        PKIX_Boolean cmpResult;

        PKIX_TEST_STD_VARS();

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_Object_Duplicate
                                    (object, &newObject, plContext));

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_Object_Equals
                                    (object, newObject, &cmpResult, plContext));

        if (!cmpResult){
                testError("unexpected mismatch");
                (void) printf("Actual value:\t%d\n", cmpResult);
                (void) printf("Expected value:\t%d\n", PKIX_TRUE);
        }

cleanup:

        PKIX_TEST_DECREF_AC(newObject);

        PKIX_TEST_RETURN();
}
