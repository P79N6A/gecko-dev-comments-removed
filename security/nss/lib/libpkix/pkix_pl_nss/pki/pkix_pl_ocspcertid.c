










































#include "pkix_pl_ocspcertid.h"







static PKIX_Error *
pkix_pl_OcspCertID_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_OcspCertID *certID = NULL;

        PKIX_ENTER(OCSPCERTID, "pkix_pl_OcspCertID_Destroy");

        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_OCSPCERTID_TYPE, plContext),
                    PKIX_OBJECTNOTOCSPCERTID);

        certID = (PKIX_PL_OcspCertID *)object;

        if (certID->certID) {
                CERT_DestroyOCSPCertID(certID->certID);
        }

cleanup:

        PKIX_RETURN(OCSPCERTID);
}













PKIX_Error *
pkix_pl_OcspCertID_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(OCSPCERTID, "pkix_pl_OcspCertID_RegisterSelf");

        entry.description = "OcspCertID";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_OcspCertID);
        entry.destructor = pkix_pl_OcspCertID_Destroy;
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_duplicateImmutable;
        systemClasses[PKIX_OCSPCERTID_TYPE] = entry;

        PKIX_RETURN(OCSPCERTID);
}































PKIX_Error *
PKIX_PL_OcspCertID_Create(
        PKIX_PL_Cert *cert,
        PKIX_PL_Date *validity,
        PKIX_PL_OcspCertID **object,
        void *plContext)
{
        PKIX_PL_OcspCertID *cid = NULL;
        int64 time = 0;

        PKIX_ENTER(DATE, "PKIX_PL_OcspCertID_Create");
        PKIX_NULLCHECK_TWO(cert, object);
    
        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_OCSPCERTID_TYPE,
                    sizeof (PKIX_PL_OcspCertID),
                    (PKIX_PL_Object **)&cid,
                    plContext),
                    PKIX_COULDNOTCREATEOBJECT);

        if (validity != NULL) {
                PKIX_CHECK(pkix_pl_Date_GetPRTime(validity, &time, plContext),
                        PKIX_DATEGETPRTIMEFAILED);
        } else {
                time = PR_Now();
        }

        cid->certID = CERT_CreateOCSPCertID(cert->nssCert, time);
        if (!cid->certID) {
                PKIX_ERROR(PKIX_COULDNOTCREATEOBJECT);
        }

        *object = cid;
        cid = NULL;
cleanup:
        PKIX_DECREF(cid);
        PKIX_RETURN(OCSPCERTID);
}





























PKIX_Error *
PKIX_PL_OcspCertID_GetFreshCacheStatus(
        PKIX_PL_OcspCertID *cid, 
        PKIX_PL_Date *validity,
        PKIX_Boolean *hasFreshStatus,
        PKIX_Boolean *statusIsGood,
        SECErrorCodes *missingResponseError,
        void *plContext)
{
        int64 time = 0;
        SECStatus rv;
        SECStatus rvOcsp;

        PKIX_ENTER(DATE, "PKIX_PL_OcspCertID_GetFreshCacheStatus");
        PKIX_NULLCHECK_THREE(cid, hasFreshStatus, statusIsGood);

        if (validity != NULL) {
                PKIX_CHECK(pkix_pl_Date_GetPRTime(validity, &time, plContext),
                        PKIX_DATEGETPRTIMEFAILED);
        } else {
                time = PR_Now();
        }

        rv = ocsp_GetCachedOCSPResponseStatusIfFresh(
                cid->certID, time, PR_TRUE, 
                &rvOcsp, missingResponseError);

        *hasFreshStatus = (rv == SECSuccess);
        if (*hasFreshStatus) {
                *statusIsGood = (rvOcsp == SECSuccess);
        }
cleanup:
        PKIX_RETURN(OCSPCERTID);
}























PKIX_Error *
PKIX_PL_OcspCertID_RememberOCSPProcessingFailure(
        PKIX_PL_OcspCertID *cid, 
        void *plContext)
{
        PRBool certIDWasConsumed = PR_FALSE;

        PKIX_ENTER(DATE, "PKIX_PL_OcspCertID_RememberOCSPProcessingFailure");
        PKIX_NULLCHECK_TWO(cid, cid->certID);

        cert_RememberOCSPProcessingFailure(cid->certID, &certIDWasConsumed);

        if (certIDWasConsumed) {
                cid->certID = NULL;
        }

        PKIX_RETURN(OCSPCERTID);
}

