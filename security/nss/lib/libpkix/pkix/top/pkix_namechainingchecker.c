











































#include "pkix_namechainingchecker.h"







PKIX_Error *
pkix_NameChainingChecker_Check(
        PKIX_CertChainChecker *checker,
        PKIX_PL_Cert *cert,
        PKIX_List *unresolvedCriticalExtensions,
        void **pNBIOContext,
        void *plContext)
{
        PKIX_PL_X500Name *prevSubject = NULL;
        PKIX_PL_X500Name *currIssuer = NULL;
        PKIX_PL_X500Name *currSubject = NULL;
        PKIX_Boolean result;

        PKIX_ENTER(CERTCHAINCHECKER, "pkix_NameChainingChecker_Check");
        PKIX_NULLCHECK_THREE(checker, cert, pNBIOContext);

        *pNBIOContext = NULL; 

        PKIX_CHECK(PKIX_CertChainChecker_GetCertChainCheckerState
                    (checker, (PKIX_PL_Object **)&prevSubject, plContext),
                    PKIX_CERTCHAINCHECKERGETCERTCHAINCHECKERSTATEFAILED);

        PKIX_CHECK(PKIX_PL_Cert_GetIssuer(cert, &currIssuer, plContext),
                    PKIX_CERTGETISSUERFAILED);

        if (prevSubject){
                PKIX_CHECK(PKIX_PL_X500Name_Match
                            (prevSubject, currIssuer, &result, plContext),
                            PKIX_X500NAMEMATCHFAILED);
                if (!result){
                        PKIX_ERROR(PKIX_NAMECHAININGCHECKFAILED);
                }
        } else {
                PKIX_ERROR(PKIX_NAMECHAININGCHECKFAILED);
        }

        PKIX_CHECK(PKIX_PL_Cert_GetSubject(cert, &currSubject, plContext),
                    PKIX_CERTGETSUBJECTFAILED);

        PKIX_CHECK(PKIX_CertChainChecker_SetCertChainCheckerState
                    (checker, (PKIX_PL_Object *)currSubject, plContext),
                    PKIX_CERTCHAINCHECKERSETCERTCHAINCHECKERSTATEFAILED);

cleanup:

        PKIX_DECREF(prevSubject);
        PKIX_DECREF(currIssuer);
        PKIX_DECREF(currSubject);

        PKIX_RETURN(CERTCHAINCHECKER);

}


























PKIX_Error *
pkix_NameChainingChecker_Initialize(
        PKIX_PL_X500Name *trustedCAName,
        PKIX_CertChainChecker **pChecker,
        void *plContext)
{
        PKIX_ENTER(CERTCHAINCHECKER, "PKIX_NameChainingChecker_Initialize");
        PKIX_NULLCHECK_TWO(pChecker, trustedCAName);

        PKIX_CHECK(PKIX_CertChainChecker_Create
                    (pkix_NameChainingChecker_Check,
                    PKIX_FALSE,
                    PKIX_FALSE,
                    NULL,
                    (PKIX_PL_Object *)trustedCAName,
                    pChecker,
                    plContext),
                    PKIX_CERTCHAINCHECKERCREATEFAILED);

cleanup:

        PKIX_RETURN(CERTCHAINCHECKER);

}
