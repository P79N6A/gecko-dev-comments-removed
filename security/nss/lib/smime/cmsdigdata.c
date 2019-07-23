









































#include "cmslocal.h"

#include "secitem.h"
#include "secasn1.h"
#include "secoid.h"
#include "secerr.h"









NSSCMSDigestedData *
NSS_CMSDigestedData_Create(NSSCMSMessage *cmsg, SECAlgorithmID *digestalg)
{
    void *mark;
    NSSCMSDigestedData *digd;
    PLArenaPool *poolp;

    poolp = cmsg->poolp;

    mark = PORT_ArenaMark(poolp);

    digd = (NSSCMSDigestedData *)PORT_ArenaZAlloc(poolp, sizeof(NSSCMSDigestedData));
    if (digd == NULL)
	goto loser;

    digd->cmsg = cmsg;

    if (SECOID_CopyAlgorithmID (poolp, &(digd->digestAlg), digestalg) != SECSuccess)
	goto loser;

    PORT_ArenaUnmark(poolp, mark);
    return digd;

loser:
    PORT_ArenaRelease(poolp, mark);
    return NULL;
}




void
NSS_CMSDigestedData_Destroy(NSSCMSDigestedData *digd)
{
    
    NSS_CMSContentInfo_Destroy(&(digd->contentInfo));
    return;
}




NSSCMSContentInfo *
NSS_CMSDigestedData_GetContentInfo(NSSCMSDigestedData *digd)
{
    return &(digd->contentInfo);
}








SECStatus
NSS_CMSDigestedData_Encode_BeforeStart(NSSCMSDigestedData *digd)
{
    unsigned long version;
    SECItem *dummy;

    version = NSS_CMS_DIGESTED_DATA_VERSION_DATA;
    if (NSS_CMSContentInfo_GetContentTypeTag(&(digd->contentInfo)) != SEC_OID_PKCS7_DATA)
	version = NSS_CMS_DIGESTED_DATA_VERSION_ENCAP;

    dummy = SEC_ASN1EncodeInteger(digd->cmsg->poolp, &(digd->version), version);
    return (dummy == NULL) ? SECFailure : SECSuccess;
}








SECStatus
NSS_CMSDigestedData_Encode_BeforeData(NSSCMSDigestedData *digd)
{
    
    if (digd->digestAlg.algorithm.len != 0 && digd->digest.len == 0) {
	
	digd->contentInfo.digcx = NSS_CMSDigestContext_StartSingle(&(digd->digestAlg));
	if (digd->contentInfo.digcx == NULL)
	    return SECFailure;
    }
    return SECSuccess;
}








SECStatus
NSS_CMSDigestedData_Encode_AfterData(NSSCMSDigestedData *digd)
{
    SECStatus rv = SECSuccess;
    
    if (digd->contentInfo.digcx) {
	rv = NSS_CMSDigestContext_FinishSingle(digd->contentInfo.digcx,
				               digd->cmsg->poolp, 
					       &(digd->digest));
	
	digd->contentInfo.digcx = NULL;
    }

    return rv;
}








SECStatus
NSS_CMSDigestedData_Decode_BeforeData(NSSCMSDigestedData *digd)
{
    
    if (digd->digestAlg.algorithm.len == 0)
	return SECFailure;

    digd->contentInfo.digcx = NSS_CMSDigestContext_StartSingle(&(digd->digestAlg));
    if (digd->contentInfo.digcx == NULL)
	return SECFailure;

    return SECSuccess;
}








SECStatus
NSS_CMSDigestedData_Decode_AfterData(NSSCMSDigestedData *digd)
{
    SECStatus rv = SECSuccess;
    
    if (digd->contentInfo.digcx) {
	rv = NSS_CMSDigestContext_FinishSingle(digd->contentInfo.digcx,
				               digd->cmsg->poolp, 
					       &(digd->cdigest));
	
	digd->contentInfo.digcx = NULL;
    }

    return rv;
}







SECStatus
NSS_CMSDigestedData_Decode_AfterEnd(NSSCMSDigestedData *digd)
{
    
    if (digd->cdigest.len != 0) {
	
	
	
    }

    return SECSuccess;
}
