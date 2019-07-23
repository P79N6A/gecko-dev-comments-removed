








































#include "pkix_pl_ocsprequest.h"







static PKIX_Error *
pkix_pl_OcspRequest_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_OcspRequest *ocspReq = NULL;

        PKIX_ENTER(OCSPREQUEST, "pkix_pl_OcspRequest_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_OCSPREQUEST_TYPE, plContext),
                    PKIX_OBJECTNOTOCSPREQUEST);

        ocspReq = (PKIX_PL_OcspRequest *)object;

        if (ocspReq->decoded != NULL) {
                CERT_DestroyOCSPRequest(ocspReq->decoded);
        }

        if (ocspReq->encoded != NULL) {
                SECITEM_FreeItem(ocspReq->encoded, PR_TRUE);
        }

        if (ocspReq->location != NULL) {
                PORT_Free(ocspReq->location);
        }

        PKIX_DECREF(ocspReq->cert);
        PKIX_DECREF(ocspReq->validity);
        PKIX_DECREF(ocspReq->signerCert);

cleanup:

        PKIX_RETURN(OCSPREQUEST);
}





static PKIX_Error *
pkix_pl_OcspRequest_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_UInt32 certHash = 0;
        PKIX_UInt32 dateHash = 0;
        PKIX_UInt32 extensionHash = 0;
        PKIX_UInt32 signerHash = 0;
        PKIX_PL_OcspRequest *ocspRq = NULL;

        PKIX_ENTER(OCSPREQUEST, "pkix_pl_OcspRequest_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType(object, PKIX_OCSPREQUEST_TYPE, plContext),
                    PKIX_OBJECTNOTOCSPREQUEST);

        ocspRq = (PKIX_PL_OcspRequest *)object;

        *pHashcode = 0;

        PKIX_HASHCODE(ocspRq->cert, &certHash, plContext,
                PKIX_CERTHASHCODEFAILED);

        PKIX_HASHCODE(ocspRq->validity, &dateHash, plContext,
                PKIX_DATEHASHCODEFAILED);

        if (ocspRq->addServiceLocator == PKIX_TRUE) {
                extensionHash = 0xff;
        }

        PKIX_HASHCODE(ocspRq->signerCert, &signerHash, plContext,
                PKIX_CERTHASHCODEFAILED);

        *pHashcode = (((((extensionHash << 8) || certHash) << 8) ||
                dateHash) << 8) || signerHash;

cleanup:

        PKIX_RETURN(OCSPREQUEST);

}





static PKIX_Error *
pkix_pl_OcspRequest_Equals(
        PKIX_PL_Object *firstObj,
        PKIX_PL_Object *secondObj,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_Boolean match = PKIX_FALSE;
        PKIX_UInt32 secondType = 0;
        PKIX_PL_OcspRequest *firstReq = NULL;
        PKIX_PL_OcspRequest *secondReq = NULL;

        PKIX_ENTER(OCSPREQUEST, "pkix_pl_OcspRequest_Equals");
        PKIX_NULLCHECK_THREE(firstObj, secondObj, pResult);

        
        PKIX_CHECK(pkix_CheckType(firstObj, PKIX_OCSPREQUEST_TYPE, plContext),
                    PKIX_FIRSTOBJARGUMENTNOTOCSPREQUEST);

        



        if (firstObj == secondObj){
                match = PKIX_TRUE;
                goto cleanup;
        }

        



        PKIX_CHECK(PKIX_PL_Object_GetType
                    (secondObj, &secondType, plContext),
                    PKIX_COULDNOTGETTYPEOFSECONDARGUMENT);
        if (secondType != PKIX_OCSPREQUEST_TYPE) {
                goto cleanup;
        }

        firstReq = (PKIX_PL_OcspRequest *)firstObj;
        secondReq = (PKIX_PL_OcspRequest *)secondObj;

        if (firstReq->addServiceLocator != secondReq->addServiceLocator) {
                goto cleanup;
        }

        PKIX_EQUALS(firstReq->cert, secondReq->cert, &match, plContext,
                PKIX_CERTEQUALSFAILED);

        if (match == PKIX_FALSE) {
                goto cleanup;
        }

        PKIX_EQUALS(firstReq->validity, secondReq->validity, &match, plContext,
                PKIX_DATEEQUALSFAILED);

        if (match == PKIX_FALSE) {
                goto cleanup;
        }

        PKIX_EQUALS
                (firstReq->signerCert, secondReq->signerCert, &match, plContext,
                PKIX_CERTEQUALSFAILED);

cleanup:

        *pResult = match;

        PKIX_RETURN(OCSPREQUEST);
}
















PKIX_Error *
pkix_pl_OcspRequest_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(OCSPREQUEST, "pkix_pl_OcspRequest_RegisterSelf");

        entry.description = "OcspRequest";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_OcspRequest);
        entry.destructor = pkix_pl_OcspRequest_Destroy;
        entry.equalsFunction = pkix_pl_OcspRequest_Equals;
        entry.hashcodeFunction = pkix_pl_OcspRequest_Hashcode;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_duplicateImmutable;

        systemClasses[PKIX_OCSPREQUEST_TYPE] = entry;

        PKIX_RETURN(OCSPREQUEST);
}







































PKIX_Error *
pkix_pl_OcspRequest_Create(
        PKIX_PL_Cert *cert,
        PKIX_PL_OcspCertID *cid,
        PKIX_PL_Date *validity,
        PKIX_PL_Cert *signerCert,
        PKIX_UInt32 methodFlags,
        PKIX_Boolean *pURIFound,
        PKIX_PL_OcspRequest **pRequest,
        void *plContext)
{
        PKIX_PL_OcspRequest *ocspRequest = NULL;

        CERTCertDBHandle *handle = NULL;
        SECStatus rv = SECFailure;
        SECItem *encoding = NULL;
        CERTOCSPRequest *certRequest = NULL;
        int64 time = 0;
        PRBool addServiceLocatorExtension = PR_FALSE;
        CERTCertificate *nssCert = NULL;
        CERTCertificate *nssSignerCert = NULL;
        char *location = NULL;
        PRErrorCode locError = 0;
        PKIX_Boolean canUseDefaultSource = PKIX_FALSE;

        PKIX_ENTER(OCSPREQUEST, "pkix_pl_OcspRequest_Create");
        PKIX_NULLCHECK_TWO(cert, pRequest);

        
        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_OCSPREQUEST_TYPE,
                    sizeof (PKIX_PL_OcspRequest),
                    (PKIX_PL_Object **)&ocspRequest,
                    plContext),
                    PKIX_COULDNOTCREATEOBJECT);

        PKIX_INCREF(cert);
        ocspRequest->cert = cert;

        PKIX_INCREF(validity);
        ocspRequest->validity = validity;

        PKIX_INCREF(signerCert);
        ocspRequest->signerCert = signerCert;

        ocspRequest->decoded = NULL;
        ocspRequest->encoded = NULL;

        ocspRequest->location = NULL;

        nssCert = cert->nssCert;

        



        handle = CERT_GetDefaultCertDB();

        if (!(methodFlags & PKIX_REV_M_IGNORE_IMPLICIT_DEFAULT_SOURCE)) {
            canUseDefaultSource = PKIX_TRUE;
        }
        location = ocsp_GetResponderLocation(handle, nssCert,
                                             canUseDefaultSource,
                                             &addServiceLocatorExtension);
        if (location == NULL) {
                locError = PORT_GetError();
                if (locError == SEC_ERROR_EXTENSION_NOT_FOUND ||
                    locError == SEC_ERROR_CERT_BAD_ACCESS_LOCATION) {
                    PORT_SetError(0);
                    *pURIFound = PKIX_FALSE;
                    goto cleanup;
                }
                PKIX_ERROR(PKIX_ERRORFINDINGORPROCESSINGURI);
        }

        ocspRequest->location = location;
        *pURIFound = PKIX_TRUE;

        if (signerCert != NULL) {
                nssSignerCert = signerCert->nssCert;
        }

        if (validity != NULL) {
		PKIX_CHECK(pkix_pl_Date_GetPRTime(validity, &time, plContext),
			PKIX_DATEGETPRTIMEFAILED);
        } else {
                time = PR_Now();
	}

        certRequest = cert_CreateSingleCertOCSPRequest(
                cid->certID, cert->nssCert, time, 
                addServiceLocatorExtension, nssSignerCert);

        ocspRequest->decoded = certRequest;

        if (certRequest == NULL) {
                PKIX_ERROR(PKIX_UNABLETOCREATECERTOCSPREQUEST);
        }

        rv = CERT_AddOCSPAcceptableResponses(
                certRequest, SEC_OID_PKIX_OCSP_BASIC_RESPONSE);

        if (rv == SECFailure) {
                PKIX_ERROR(PKIX_UNABLETOADDACCEPTABLERESPONSESTOREQUEST);
        }

        encoding = CERT_EncodeOCSPRequest(NULL, certRequest, NULL);

        ocspRequest->encoded = encoding;

        *pRequest = ocspRequest;
        ocspRequest = NULL;

cleanup:
        PKIX_DECREF(ocspRequest);

        PKIX_RETURN(OCSPREQUEST);
}























PKIX_Error *
pkix_pl_OcspRequest_GetEncoded(
        PKIX_PL_OcspRequest *request,
        SECItem **pRequest,
        void *plContext)
{
        PKIX_ENTER(OCSPREQUEST, "pkix_pl_OcspRequest_GetEncoded");
        PKIX_NULLCHECK_TWO(request, pRequest);

        *pRequest = request->encoded;

        PKIX_RETURN(OCSPREQUEST);
}























PKIX_Error *
pkix_pl_OcspRequest_GetLocation(
        PKIX_PL_OcspRequest *request,
        char **pLocation,
        void *plContext)
{
        PKIX_ENTER(OCSPREQUEST, "pkix_pl_OcspRequest_GetLocation");
        PKIX_NULLCHECK_TWO(request, pLocation);

        *pLocation = request->location;

        PKIX_RETURN(OCSPREQUEST);
}
