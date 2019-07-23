











































#include "pkix_expirationchecker.h"







PKIX_Error *
pkix_ExpirationChecker_Check(
        PKIX_CertChainChecker *checker,
        PKIX_PL_Cert *cert,
        PKIX_List *unresolvedCriticalExtensions,
        void **pNBIOContext,
        void *plContext)
{
        PKIX_PL_Date *testDate = NULL;

        PKIX_ENTER(CERTCHAINCHECKER, "pkix_ExpirationChecker_Check");
        PKIX_NULLCHECK_THREE(checker, cert, pNBIOContext);

        *pNBIOContext = NULL; 

        PKIX_CHECK(PKIX_CertChainChecker_GetCertChainCheckerState
                    (checker, (PKIX_PL_Object **)&testDate, plContext),
                    PKIX_CERTCHAINCHECKERGETCERTCHAINCHECKERSTATEFAILED);

        PKIX_CHECK(PKIX_PL_Cert_CheckValidity(cert, testDate, plContext),
                    PKIX_CERTCHECKVALIDITYFAILED);

cleanup:

        PKIX_DECREF(testDate);

        PKIX_RETURN(CERTCHAINCHECKER);

}


























PKIX_Error *
pkix_ExpirationChecker_Initialize(
        PKIX_PL_Date *testDate,
        PKIX_CertChainChecker **pChecker,
        void *plContext)
{
        PKIX_PL_Date *myDate = NULL;
        PKIX_PL_Date *nowDate = NULL;

        PKIX_ENTER(CERTCHAINCHECKER, "pkix_ExpirationChecker_Initialize");
        PKIX_NULLCHECK_ONE(pChecker);

        
        if (!testDate){
                PKIX_CHECK(PKIX_PL_Date_Create_UTCTime
                            (NULL, &nowDate, plContext),
                            PKIX_DATECREATEUTCTIMEFAILED);
                myDate = nowDate;
        } else {
                myDate = testDate;
        }

        PKIX_CHECK(PKIX_CertChainChecker_Create
                    (pkix_ExpirationChecker_Check,
                    PKIX_TRUE,
                    PKIX_FALSE,
                    NULL,
                    (PKIX_PL_Object *)myDate,
                    pChecker,
                    plContext),
                    PKIX_CERTCHAINCHECKERCREATEFAILED);

cleanup:

        PKIX_DECREF(nowDate);

        PKIX_RETURN(CERTCHAINCHECKER);

}
