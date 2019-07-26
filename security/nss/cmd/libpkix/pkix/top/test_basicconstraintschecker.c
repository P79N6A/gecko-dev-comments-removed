









#include "testutil.h"
#include "testutil_nss.h"

#define PKIX_TEST_MAX_CERTS     10

static void *plContext = NULL;

static
void printUsage1(char *pName){
        printf("\nUSAGE: %s test-name [ENE|EE] ", pName);
        printf("cert [certs].\n");
}

static
void printUsageMax(PKIX_UInt32 numCerts){
        printf("\nUSAGE ERROR: number of certs %d exceed maximum %d\n",
                numCerts, PKIX_TEST_MAX_CERTS);
}

int test_basicconstraintschecker(int argc, char *argv[]){

        PKIX_List *chain = NULL;
        PKIX_ValidateParams *valParams = NULL;
        PKIX_ValidateResult *valResult = NULL;
        PKIX_UInt32 actualMinorVersion;
        char *certNames[PKIX_TEST_MAX_CERTS];
        PKIX_PL_Cert *certs[PKIX_TEST_MAX_CERTS];
	PKIX_VerifyNode *verifyTree = NULL;
	PKIX_PL_String *verifyString = NULL;
        PKIX_UInt32 chainLength = 0;
        PKIX_UInt32 i = 0;
        PKIX_UInt32 j = 0;
        PKIX_Boolean testValid = PKIX_FALSE;
        char *dirName = NULL;

        PKIX_TEST_STD_VARS();

        if (argc < 4){
                printUsage1(argv[0]);
                return (0);
        }

        startTests("BasicConstraintsChecker");

        PKIX_TEST_EXPECT_NO_ERROR(
            PKIX_PL_NssContext_Create(0, PKIX_FALSE, NULL, &plContext));

        
        if (PORT_Strcmp(argv[2+j], "ENE") == 0) {
                testValid = PKIX_TRUE;
        } else if (PORT_Strcmp(argv[2+j], "EE") == 0) {
                testValid = PKIX_FALSE;
        } else {
                printUsage1(argv[0]);
                return (0);
        }

        dirName = argv[3+j];

        chainLength = (argc - j) - 4;
        if (chainLength > PKIX_TEST_MAX_CERTS) {
                printUsageMax(chainLength);
        }

        for (i = 0; i < chainLength; i++) {
                certNames[i] = argv[(4+j)+i];
                certs[i] = NULL;
        }

        subTest(argv[1+j]);

        subTest("Basic-Constraints - Create Cert Chain");

        chain = createCertChainPlus
                (dirName, certNames, certs, chainLength, plContext);

        











        if (testValid == PKIX_FALSE && chain == NULL) {
                testErrorUndo("Cert Error - Create failed");
                goto cleanup;
        }

        subTest("Basic-Constraints - Create Params");

        valParams = createValidateParams
                (dirName,
                argv[4+j],
                NULL,
                NULL,
                NULL,
                PKIX_FALSE,
                PKIX_FALSE,
                PKIX_FALSE,
                PKIX_FALSE,
                chain,
                plContext);

        subTest("Basic-Constraints - Validate Chain");

        if (testValid == PKIX_TRUE) {
                PKIX_TEST_EXPECT_NO_ERROR(PKIX_ValidateChain
                        (valParams, &valResult, &verifyTree, plContext));
        } else {
                PKIX_TEST_EXPECT_ERROR(PKIX_ValidateChain
                        (valParams, &valResult, &verifyTree, plContext));
        }

cleanup:

        PKIX_TEST_DECREF_AC(verifyString);
        PKIX_TEST_DECREF_AC(verifyTree);
        PKIX_TEST_DECREF_AC(chain);
        PKIX_TEST_DECREF_AC(valParams);
        PKIX_TEST_DECREF_AC(valResult);

        PKIX_Shutdown(plContext);

        PKIX_TEST_RETURN();

        endTests("BasicConstraintsChecker");

        return (0);
}
