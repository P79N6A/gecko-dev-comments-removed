









#include "testutil.h"
#include "testutil_nss.h"


static void *plContext = NULL;






static
void test_makeSubjectCertSelector(
        PKIX_PL_Cert *certNameToMatch,
        PKIX_PL_Date *validityDate,
        PKIX_CertSelector **pSelector,
        void *plContext)
{
        PKIX_CertSelector *selector = NULL;
        PKIX_ComCertSelParams *subjParams = NULL;
        PKIX_PL_X500Name *subjectName = NULL;

        PKIX_TEST_STD_VARS();

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_CertSelector_Create
                (NULL, NULL, &selector, plContext));
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ComCertSelParams_Create
                (&subjParams, plContext));
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_Cert_GetSubject
                (certNameToMatch, &subjectName, plContext));
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ComCertSelParams_SetSubject
                (subjParams, subjectName, plContext));
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ComCertSelParams_SetCertificateValid
                (subjParams, validityDate, plContext));
        PKIX_TEST_EXPECT_NO_ERROR
                (PKIX_CertSelector_SetCommonCertSelectorParams
                (selector, subjParams, plContext));
        *pSelector = selector;

cleanup:

        PKIX_TEST_DECREF_AC(subjParams);
        PKIX_TEST_DECREF_AC(subjectName);

        PKIX_TEST_RETURN();
}






static 
void test_makePathCertSelector(
        PKIX_Int32 minPathLength,
        PKIX_CertSelector **pSelector,
        void *plContext)
{
        PKIX_CertSelector *selector = NULL;
        PKIX_ComCertSelParams *pathParams = NULL;

        PKIX_TEST_STD_VARS();

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_CertSelector_Create
                (NULL, NULL, &selector, plContext));
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ComCertSelParams_Create
                (&pathParams, plContext));
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ComCertSelParams_SetBasicConstraints
                (pathParams, minPathLength, plContext));

        PKIX_TEST_EXPECT_NO_ERROR
                (PKIX_CertSelector_SetCommonCertSelectorParams
                (selector, pathParams, plContext));
        *pSelector = selector;

cleanup:

        PKIX_TEST_DECREF_AC(pathParams);

        PKIX_TEST_RETURN();
}







static
void testMatchCertSubject(
        char *crlDir,
        char *desiredSubjectCert,
        char *expectedAscii,
        PKIX_PL_Date *validityDate,
        void *plContext)
{
        PKIX_UInt32 numCert = 0;
        PKIX_PL_Cert *certWithDesiredSubject = NULL;
        PKIX_CertStore *certStore = NULL;
        PKIX_CertSelector *certSelector = NULL;
        PKIX_List *certList = NULL;
        PKIX_CertStore_CertCallback getCert = NULL;
	void *nbioContext = NULL;

        PKIX_TEST_STD_VARS();

        certWithDesiredSubject = createCert
                (crlDir, desiredSubjectCert, plContext);

        test_makeSubjectCertSelector
                (certWithDesiredSubject,
                validityDate,
                &certSelector,
                plContext);

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_Pk11CertStore_Create
                (&certStore, plContext));

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_CertStore_GetCertCallback
                (certStore, &getCert, plContext));

        PKIX_TEST_EXPECT_NO_ERROR(getCert
                (certStore,
		certSelector,
		&nbioContext,
		&certList,
		plContext));

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_List_GetLength
                (certList, &numCert, plContext));

        if (numCert > 0) {
                
                PKIX_TEST_EXPECT_ERROR(PKIX_List_DeleteItem
                (certList, 0, plContext));
        }

        if (expectedAscii) {
                testToStringHelper
                        ((PKIX_PL_Object *)certList, expectedAscii, plContext);
        }

cleanup:

        PKIX_TEST_DECREF_AC(certWithDesiredSubject);
        PKIX_TEST_DECREF_AC(certStore);
        PKIX_TEST_DECREF_AC(certSelector);
        PKIX_TEST_DECREF_AC(certList);

        PKIX_TEST_RETURN();
}









static 
void testMatchCertMinPath(
        PKIX_Int32 minPath,
        char *expectedAscii,
        void *plContext)
{
        PKIX_CertStore *certStore = NULL;
        PKIX_CertSelector *certSelector = NULL;
        PKIX_List *certList = NULL;
        PKIX_CertStore_CertCallback getCert = NULL;
	void *nbioContext = NULL;

        PKIX_TEST_STD_VARS();

        subTest("Searching Certs for minPath");

        test_makePathCertSelector
                (minPath, &certSelector, plContext);

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_Pk11CertStore_Create
                (&certStore, plContext));

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_CertStore_GetCertCallback
                (certStore, &getCert, plContext));

        PKIX_TEST_EXPECT_NO_ERROR(getCert
                (certStore,
		certSelector,
		&nbioContext,
		&certList,
		plContext));

        if (expectedAscii) {
                testToStringHelper
                        ((PKIX_PL_Object *)certList, expectedAscii, plContext);
        }

cleanup:

        PKIX_TEST_DECREF_AC(certStore);
        PKIX_TEST_DECREF_AC(certSelector);
        PKIX_TEST_DECREF_AC(certList);

        PKIX_TEST_RETURN();
}





static
void test_makeIssuerCRLSelector(
        PKIX_PL_CRL *crlNameToMatch,
        PKIX_CRLSelector **pSelector,
        void *plContext)
{
        PKIX_CRLSelector *selector = NULL;
        PKIX_ComCRLSelParams *issuerParams = NULL;
        PKIX_PL_X500Name *issuerName = NULL;
        PKIX_List *names = NULL;

        PKIX_TEST_STD_VARS();

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_CRLSelector_Create
                (NULL, NULL, &selector, plContext));
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ComCRLSelParams_Create
                (&issuerParams, plContext));
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_CRL_GetIssuer
                (crlNameToMatch, &issuerName, plContext));
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_List_Create(&names, plContext));
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_List_AppendItem
                (names, (PKIX_PL_Object *)issuerName, plContext));
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ComCRLSelParams_SetIssuerNames
                (issuerParams, names, plContext));

        PKIX_TEST_EXPECT_NO_ERROR
                (PKIX_CRLSelector_SetCommonCRLSelectorParams
                (selector, issuerParams, plContext));
        *pSelector = selector;

cleanup:

        PKIX_TEST_DECREF_AC(issuerParams);
        PKIX_TEST_DECREF_AC(issuerName);
        PKIX_TEST_DECREF_AC(names);

        PKIX_TEST_RETURN();
}






static 
void test_makeDateCRLSelector(
        PKIX_PL_Date *dateToMatch,
        PKIX_CRLSelector **pSelector,
        void *plContext)
{
        PKIX_CRLSelector *selector = NULL;
        PKIX_ComCRLSelParams *dateParams = NULL;

        PKIX_TEST_STD_VARS();

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_CRLSelector_Create
                (NULL, NULL, &selector, plContext));
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ComCRLSelParams_Create
                (&dateParams, plContext));
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_ComCRLSelParams_SetDateAndTime
                (dateParams, dateToMatch, plContext));
        PKIX_TEST_EXPECT_NO_ERROR
                (PKIX_CRLSelector_SetCommonCRLSelectorParams
                (selector, dateParams, plContext));
        *pSelector = selector;

cleanup:
        PKIX_TEST_DECREF_AC(dateParams);

        PKIX_TEST_RETURN();
}







static 
void testMatchCrlIssuer(
        char *crlDir,
        char *desiredIssuerCrl,
        char *expectedAscii,
        void *plContext)
{
        PKIX_UInt32 numCrl = 0;
        PKIX_PL_CRL *crlWithDesiredIssuer = NULL;
        PKIX_CertStore *crlStore = NULL;
        PKIX_CRLSelector *crlSelector = NULL;
        PKIX_List *crlList = NULL;
        PKIX_CertStore_CRLCallback getCrl = NULL;
	void *nbioContext = NULL;

        PKIX_TEST_STD_VARS();

        subTest("Searching CRLs for matching Issuer");

        crlWithDesiredIssuer = createCRL(crlDir, desiredIssuerCrl, plContext);

        test_makeIssuerCRLSelector
                (crlWithDesiredIssuer, &crlSelector, plContext);

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_Pk11CertStore_Create
                (&crlStore, plContext));

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_CertStore_GetCRLCallback
                (crlStore, &getCrl, plContext));

        PKIX_TEST_EXPECT_NO_ERROR(getCrl
                (crlStore,
		crlSelector,
                &nbioContext,
		&crlList,
		plContext));

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_List_GetLength
                (crlList, &numCrl, plContext));

        if (numCrl > 0) {
                
                PKIX_TEST_EXPECT_ERROR(PKIX_List_DeleteItem
                (crlList, 0, plContext));
        }

        if (expectedAscii) {
                testToStringHelper
                        ((PKIX_PL_Object *)crlList, expectedAscii, plContext);
        }

cleanup:

        PKIX_TEST_DECREF_AC(crlWithDesiredIssuer);
        PKIX_TEST_DECREF_AC(crlStore);
        PKIX_TEST_DECREF_AC(crlSelector);
        PKIX_TEST_DECREF_AC(crlList);

        PKIX_TEST_RETURN();
}








static 
void testMatchCrlDate(
        char *dateMatch,
        char *expectedAscii,
        void *plContext)
{
        PKIX_PL_Date *dateCriterion = NULL;
        PKIX_CertStore *crlStore = NULL;
        PKIX_CRLSelector *crlSelector = NULL;
        PKIX_List *crlList = NULL;
        PKIX_CertStore_CRLCallback getCrl = NULL;

        PKIX_TEST_STD_VARS();

        subTest("Searching CRLs for matching Date");

        dateCriterion = createDate(dateMatch, plContext);
        test_makeDateCRLSelector(dateCriterion, &crlSelector, plContext);

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_Pk11CertStore_Create
                (&crlStore, plContext));

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_CertStore_GetCRLCallback
                (crlStore, &getCrl, plContext));

        PKIX_TEST_EXPECT_ERROR(getCrl
                (crlStore, crlSelector, NULL, &crlList, plContext));

cleanup:

        PKIX_TEST_DECREF_AC(dateCriterion);
        PKIX_TEST_DECREF_AC(crlStore);
        PKIX_TEST_DECREF_AC(crlSelector);
        PKIX_TEST_DECREF_AC(crlList);

        PKIX_TEST_RETURN();
}

static 
void printUsage(char *pName){
        printf("\nUSAGE: %s <-d data-dir> <database-dir>\n\n", pName);
}



int test_pk11certstore(int argc, char *argv[]) {

        PKIX_UInt32 j = 0;
        PKIX_UInt32 actualMinorVersion;
        PKIX_PL_Date *validityDate = NULL;
        PKIX_PL_Date *betweenDate = NULL;
        char *crlDir = NULL;
        char *expectedProfAscii = "([\n"
                "\tVersion:         v3\n"
                "\tSerialNumber:    00ca\n"
                "\tIssuer:          CN=chemistry,O=mit,C=us\n"
                "\tSubject:         CN=prof noall,O=mit,C=us\n"
                "\tValidity: [From: Fri Feb 11 14:14:06 2005\n"
                "\t           To:   Mon Jan 18, 2105]\n"
                "\tSubjectAltNames: (null)\n"
                "\tAuthorityKeyId:  (null)\n"
                "\tSubjectKeyId:    (null)\n"
                "\tSubjPubKeyAlgId: ANSI X9.57 DSA Signature\n"
                "\tCritExtOIDs:     (2.5.29.15, 2.5.29.19)\n"
                "\tExtKeyUsages:    (null)\n"
                "\tBasicConstraint: CA(6)\n"
                "\tCertPolicyInfo:  (null)\n"
                "\tPolicyMappings:  (null)\n"
                "\tExplicitPolicy:  -1\n"
                "\tInhibitMapping:  -1\n"
                "\tInhibitAnyPolicy:-1\n"
                "\tNameConstraints: (null)\n"
                "]\n"
                ", [\n"
                "\tVersion:         v3\n"
                "\tSerialNumber:    03\n"
                "\tIssuer:          CN=physics,O=mit,C=us\n"
                "\tSubject:         CN=prof noall,O=mit,C=us\n"
                "\tValidity: [From: Fri Feb 11 12:52:26 2005\n"
                "\t           To:   Mon Jan 18, 2105]\n"
                "\tSubjectAltNames: (null)\n"
                "\tAuthorityKeyId:  (null)\n"
                "\tSubjectKeyId:    (null)\n"
                "\tSubjPubKeyAlgId: ANSI X9.57 DSA Signature\n"
                "\tCritExtOIDs:     (2.5.29.15, 2.5.29.19)\n"
                "\tExtKeyUsages:    (null)\n"
                "\tBasicConstraint: CA(0)\n"
                "\tCertPolicyInfo:  (null)\n"
                "\tPolicyMappings:  (null)\n"
                "\tExplicitPolicy:  -1\n"
                "\tInhibitMapping:  -1\n"
                "\tInhibitAnyPolicy:-1\n"
                "\tNameConstraints: (null)\n"
                "]\n"
                ")";
        char *expectedValidityAscii = "([\n"
                "\tVersion:         v3\n"
                "\tSerialNumber:    03\n"
                "\tIssuer:          CN=physics,O=mit,C=us\n"
                "\tSubject:         CN=prof noall,O=mit,C=us\n"
                "\tValidity: [From: Fri Feb 11 12:52:26 2005\n"
                "\t           To:   Mon Jan 18, 2105]\n"
                "\tSubjectAltNames: (null)\n"
                "\tAuthorityKeyId:  (null)\n"
                "\tSubjectKeyId:    (null)\n"
                "\tSubjPubKeyAlgId: ANSI X9.57 DSA Signature\n"
                "\tCritExtOIDs:     (2.5.29.15, 2.5.29.19)\n"
                "\tExtKeyUsages:    (null)\n"
                "\tBasicConstraint: CA(0)\n"
                "\tCertPolicyInfo:  (null)\n"
                "\tPolicyMappings:  (null)\n"
                "\tExplicitPolicy:  -1\n"
                "\tInhibitMapping:  -1\n"
                "\tInhibitAnyPolicy:-1\n"
                "\tNameConstraints: (null)\n"
                "]\n"
                ")";
        char *expectedMinPathAscii = "([\n"
                "\tVersion:         v3\n"
                "\tSerialNumber:    01\n"
                "\tIssuer:          CN=science,O=mit,C=us\n"
                "\tSubject:         CN=science,O=mit,C=us\n"
                "\tValidity: [From: Fri Feb 11 12:47:58 2005\n"
                "\t           To:   Mon Jan 18, 2105]\n"
                "\tSubjectAltNames: (null)\n"
                "\tAuthorityKeyId:  (null)\n"
                "\tSubjectKeyId:    (null)\n"
                "\tSubjPubKeyAlgId: ANSI X9.57 DSA Signature\n"
                "\tCritExtOIDs:     (2.5.29.15, 2.5.29.19)\n"
                "\tExtKeyUsages:    (null)\n"
                "\tBasicConstraint: CA(10)\n"
                "\tCertPolicyInfo:  (null)\n"
                "\tPolicyMappings:  (null)\n"
                "\tExplicitPolicy:  -1\n"
                "\tInhibitMapping:  -1\n"
                "\tInhibitAnyPolicy:-1\n"
                "\tNameConstraints: (null)\n"
                "]\n"
                ")";
        char *expectedIssuerAscii = "([\n"
                "\tVersion:         v2\n"
                "\tIssuer:          CN=physics,O=mit,C=us\n"
                "\tUpdate:   [Last: Fri Feb 11 13:51:38 2005\n"
                "\t           Next: Mon Jan 18, 2105]\n"
                "\tSignatureAlgId:  1.2.840.10040.4.3\n"
                "\tCRL Number     : (null)\n"
                "\n"
                "\tEntry List:      (\n"
                "\t[\n"
                "\tSerialNumber:    67\n"
                "\tReasonCode:      257\n"
                "\tRevocationDate:  Fri Feb 11 13:51:38 2005\n"
                "\tCritExtOIDs:     (EMPTY)\n"
                "\t]\n"
                "\t)\n"
                "\n"
                "\tCritExtOIDs:     (EMPTY)\n"
                "]\n"
                ")";
        char *expectedDateAscii = "([\n"
                "\tVersion:         v2\n"
                "\tIssuer:          CN=science,O=mit,C=us\n"
                "\tUpdate:   [Last: Fri Feb 11 13:34:40 2005\n"
                "\t           Next: Mon Jan 18, 2105]\n"
                "\tSignatureAlgId:  1.2.840.10040.4.3\n"
                "\tCRL Number     : (null)\n"
                "\n"
                "\tEntry List:      (\n"
                "\t[\n"
                "\tSerialNumber:    65\n"
                "\tReasonCode:      260\n"
                "\tRevocationDate:  Fri Feb 11 13:34:40 2005\n"
                "\tCritExtOIDs:     (EMPTY)\n"
                "\t]\n"
                "\t)\n"
                "\n"
                "\tCritExtOIDs:     (EMPTY)\n"
                "]\n"
                ", [\n"
                "\tVersion:         v2\n"
                "\tIssuer:          CN=testing CRL,O=test,C=us\n"
                "\tUpdate:   [Last: Fri Feb 11 13:14:38 2005\n"
                "\t           Next: Mon Jan 18, 2105]\n"
                "\tSignatureAlgId:  1.2.840.10040.4.3\n"
                "\tCRL Number     : (null)\n"
                "\n"
                "\tEntry List:      (\n"
                "\t[\n"
                "\tSerialNumber:    67\n"
                "\tReasonCode:      258\n"
                "\tRevocationDate:  Fri Feb 11 13:14:38 2005\n"
                "\tCritExtOIDs:     (EMPTY)\n"
                "\t]\n"
                "\t)\n"
                "\n"
                "\tCritExtOIDs:     (EMPTY)\n"
                "]\n"
                ")";

        PKIX_TEST_STD_VARS();

        startTests("Pk11CertStore");

        if (argc < 3) {
                printUsage(argv[0]);
                return (0);
        }

        PKIX_TEST_EXPECT_NO_ERROR(
            PKIX_PL_NssContext_Create(0, PKIX_FALSE, NULL, &plContext));

        crlDir = argv[j+2];

        
        PKIX_TEST_EXPECT_NO_ERROR(pkix_pl_Date_CreateFromPRTime
                (PR_Now(), &validityDate, plContext));

        subTest("Searching Certs for Subject");

        testMatchCertSubject
                (crlDir,
                "phy2prof.crt",
                NULL, 
                validityDate,
                plContext);

        
        betweenDate = createDate("050210184000Z", plContext);

        subTest("Searching Certs for Subject and Validity");

        testMatchCertSubject
                (crlDir,
                "phy2prof.crt",
                NULL, 
                betweenDate,
                plContext);

        testMatchCertMinPath
                (9,
                NULL, 
        	plContext);

        testMatchCrlIssuer
                (crlDir,
                "phys.crl",
                NULL, 
                plContext);

        testMatchCrlDate
                ("050211184000Z",
                NULL, 
                plContext);

cleanup:

        PKIX_TEST_DECREF_AC(validityDate);
        PKIX_TEST_DECREF_AC(betweenDate);

        PKIX_TEST_RETURN();

        endTests("Pk11CertStore");

        return (0);
}
