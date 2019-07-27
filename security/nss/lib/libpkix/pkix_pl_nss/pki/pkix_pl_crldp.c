









#include "pkix_pl_crldp.h"

static PKIX_Error *
pkix_pl_CrlDp_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
    pkix_pl_CrlDp *crldp = NULL;
    
    PKIX_ENTER(CRLCHECKER, "pkix_CrlDp_Destroy");
    PKIX_NULLCHECK_ONE(object);
    
    
    PKIX_CHECK(
        pkix_CheckType(object, PKIX_CRLDP_TYPE, plContext),
        PKIX_OBJECTNOTCRLCHECKER);
    
    crldp = (pkix_pl_CrlDp *)object;
    if (crldp->distPointType == relativeDistinguishedName) {
        CERT_DestroyName(crldp->name.issuerName);
        crldp->name.issuerName = NULL;
    }
    crldp->nssdp = NULL;
cleanup:
    PKIX_RETURN(CRLCHECKER);
}















PKIX_Error *
pkix_pl_CrlDp_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry* entry = &systemClasses[PKIX_CRLDP_TYPE];

        PKIX_ENTER(CRLCHECKER, "pkix_CrlDp_RegisterSelf");

        entry->description = "CrlDistPoint";
        entry->typeObjectSize = sizeof(pkix_pl_CrlDp);
        entry->destructor = pkix_pl_CrlDp_Destroy;
        entry->duplicateFunction = pkix_duplicateImmutable;

        PKIX_RETURN(CRLCHECKER);
}



PKIX_Error *
pkix_pl_CrlDp_Create(
    const CRLDistributionPoint *dp,
    const CERTName *certIssuerName,
    pkix_pl_CrlDp **pPkixDP,
    void *plContext)
{
    PLArenaPool *rdnArena = NULL;
    CERTName *issuerNameCopy = NULL;
    pkix_pl_CrlDp *dpl = NULL;

    





    PKIX_ENTER(CRLDP, "pkix_pl_CrlDp_Create");
    PKIX_NULLCHECK_ONE(dp);

    PKIX_CHECK(
        PKIX_PL_Object_Alloc(PKIX_CRLDP_TYPE,
                             sizeof (pkix_pl_CrlDp),
                             (PKIX_PL_Object **)&dpl,
                             plContext),
        PKIX_COULDNOTCREATEOBJECT);

    dpl->nssdp = dp;
    dpl->isPartitionedByReasonCode = PKIX_FALSE;
    if (dp->reasons.data) {
        dpl->isPartitionedByReasonCode = PKIX_TRUE;
    }
    if (dp->distPointType == generalName) {
        dpl->distPointType = generalName;
        dpl->name.fullName = dp->distPoint.fullName;
    } else {
        SECStatus rv;
        const CERTName *issuerName = NULL;
        const CERTRDN *relName = &dp->distPoint.relativeName;

        if (dp->crlIssuer) {
            if (dp->crlIssuer->l.next) {
                


                PKIX_ERROR(PKIX_NOTCONFORMINGCRLDP);
            }
            issuerName = &dp->crlIssuer->name.directoryName;
        } else {
            issuerName = certIssuerName;
        }
        rdnArena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
        if (!rdnArena) {
            PKIX_ERROR(PKIX_PORTARENAALLOCFAILED);
        }
        issuerNameCopy = (CERTName *)PORT_ArenaZNew(rdnArena, CERTName);
        if (!issuerNameCopy) {
            PKIX_ERROR(PKIX_ALLOCERROR);
        }
        rv = CERT_CopyName(rdnArena, issuerNameCopy, (CERTName*)issuerName);
        if (rv == SECFailure) {
            PKIX_ERROR(PKIX_ALLOCERROR);
        }
        rv = CERT_AddRDN(issuerNameCopy, (CERTRDN*)relName);
        if (rv == SECFailure) {
            PKIX_ERROR(PKIX_ALLOCERROR);
        }
        dpl->distPointType = relativeDistinguishedName;
        dpl->name.issuerName = issuerNameCopy;
        rdnArena = NULL;
    }
    *pPkixDP = dpl;
    dpl = NULL;

cleanup:
    if (rdnArena) {
        PORT_FreeArena(rdnArena, PR_FALSE);
    }
    PKIX_DECREF(dpl);

    PKIX_RETURN(CRLDP);
}
