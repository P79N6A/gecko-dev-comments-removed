









































#include "cmslocal.h"

#include "key.h"
#include "secasn1.h"
#include "secitem.h"
#include "secoid.h"
#include "pk11func.h"
#include "prtime.h"
#include "secerr.h"
#include "secpkcs5.h"










NSSCMSEncryptedData *
NSS_CMSEncryptedData_Create(NSSCMSMessage *cmsg, SECOidTag algorithm, 
			    int keysize)
{
    void *mark;
    NSSCMSEncryptedData *encd;
    PLArenaPool *poolp;
    SECAlgorithmID *pbe_algid;
    SECStatus rv;

    poolp = cmsg->poolp;

    mark = PORT_ArenaMark(poolp);

    encd = PORT_ArenaZNew(poolp, NSSCMSEncryptedData);
    if (encd == NULL)
	goto loser;

    encd->cmsg = cmsg;

    

    if (!SEC_PKCS5IsAlgorithmPBEAlgTag(algorithm)) {
	rv = NSS_CMSContentInfo_SetContentEncAlg(poolp, &(encd->contentInfo), 
						 algorithm, NULL, keysize);
    } else {
	






	pbe_algid = PK11_CreatePBEAlgorithmID(algorithm, 1, NULL);
	if (pbe_algid == NULL) {
	    rv = SECFailure;
	} else {
	    rv = NSS_CMSContentInfo_SetContentEncAlgID(poolp, 
				&(encd->contentInfo), pbe_algid, keysize);
	    SECOID_DestroyAlgorithmID (pbe_algid, PR_TRUE);
	}
    }
    if (rv != SECSuccess)
	goto loser;

    PORT_ArenaUnmark(poolp, mark);
    return encd;

loser:
    PORT_ArenaRelease(poolp, mark);
    return NULL;
}




void
NSS_CMSEncryptedData_Destroy(NSSCMSEncryptedData *encd)
{
    
    NSS_CMSContentInfo_Destroy(&(encd->contentInfo));
    return;
}




NSSCMSContentInfo *
NSS_CMSEncryptedData_GetContentInfo(NSSCMSEncryptedData *encd)
{
    return &(encd->contentInfo);
}









SECStatus
NSS_CMSEncryptedData_Encode_BeforeStart(NSSCMSEncryptedData *encd)
{
    int version;
    PK11SymKey *bulkkey = NULL;
    SECItem *dummy;
    NSSCMSContentInfo *cinfo = &(encd->contentInfo);

    if (NSS_CMSArray_IsEmpty((void **)encd->unprotectedAttr))
	version = NSS_CMS_ENCRYPTED_DATA_VERSION;
    else
	version = NSS_CMS_ENCRYPTED_DATA_VERSION_UPATTR;
    
    dummy = SEC_ASN1EncodeInteger (encd->cmsg->poolp, &(encd->version), version);
    if (dummy == NULL)
	return SECFailure;

    
    if (encd->cmsg->decrypt_key_cb)
	bulkkey = (*encd->cmsg->decrypt_key_cb)(encd->cmsg->decrypt_key_cb_arg, 
		    NSS_CMSContentInfo_GetContentEncAlg(cinfo));
    if (bulkkey == NULL)
	return SECFailure;

    
    NSS_CMSContentInfo_SetBulkKey(cinfo, bulkkey);
    PK11_FreeSymKey (bulkkey);

    return SECSuccess;
}




SECStatus
NSS_CMSEncryptedData_Encode_BeforeData(NSSCMSEncryptedData *encd)
{
    NSSCMSContentInfo *cinfo;
    PK11SymKey *bulkkey;
    SECAlgorithmID *algid;
    SECStatus rv;

    cinfo = &(encd->contentInfo);

    
    bulkkey = NSS_CMSContentInfo_GetBulkKey(cinfo);
    if (bulkkey == NULL)
	return SECFailure;
    algid = NSS_CMSContentInfo_GetContentEncAlg(cinfo);
    if (algid == NULL)
	return SECFailure;

    rv = NSS_CMSContentInfo_Private_Init(cinfo);
    if (rv != SECSuccess) {
	return SECFailure;
    }
    


    cinfo->privateInfo->ciphcx = NSS_CMSCipherContext_StartEncrypt(encd->cmsg->poolp, bulkkey, algid);
    PK11_FreeSymKey(bulkkey);
    if (cinfo->privateInfo->ciphcx == NULL)
	return SECFailure;

    return SECSuccess;
}




SECStatus
NSS_CMSEncryptedData_Encode_AfterData(NSSCMSEncryptedData *encd)
{
    if (encd->contentInfo.privateInfo && encd->contentInfo.privateInfo->ciphcx) {
	NSS_CMSCipherContext_Destroy(encd->contentInfo.privateInfo->ciphcx);
	encd->contentInfo.privateInfo->ciphcx = NULL;
    }

    
    return SECSuccess;
}





SECStatus
NSS_CMSEncryptedData_Decode_BeforeData(NSSCMSEncryptedData *encd)
{
    PK11SymKey *bulkkey = NULL;
    NSSCMSContentInfo *cinfo;
    SECAlgorithmID *bulkalg;
    SECStatus rv = SECFailure;

    cinfo = &(encd->contentInfo);

    bulkalg = NSS_CMSContentInfo_GetContentEncAlg(cinfo);

    if (encd->cmsg->decrypt_key_cb == NULL)	
	goto loser;

    bulkkey = (*encd->cmsg->decrypt_key_cb)(encd->cmsg->decrypt_key_cb_arg, bulkalg);
    if (bulkkey == NULL)
	
	goto loser;

    NSS_CMSContentInfo_SetBulkKey(cinfo, bulkkey);

    rv = NSS_CMSContentInfo_Private_Init(cinfo);
    if (rv != SECSuccess) {
	goto loser;
    }
    rv = SECFailure;

    cinfo->privateInfo->ciphcx = NSS_CMSCipherContext_StartDecrypt(bulkkey, bulkalg);
    if (cinfo->privateInfo->ciphcx == NULL)
	goto loser;		


    
    PK11_FreeSymKey(bulkkey);

    rv = SECSuccess;

loser:
    return rv;
}




SECStatus
NSS_CMSEncryptedData_Decode_AfterData(NSSCMSEncryptedData *encd)
{
    if (encd->contentInfo.privateInfo && encd->contentInfo.privateInfo->ciphcx) {
	NSS_CMSCipherContext_Destroy(encd->contentInfo.privateInfo->ciphcx);
	encd->contentInfo.privateInfo->ciphcx = NULL;
    }

    return SECSuccess;
}




SECStatus
NSS_CMSEncryptedData_Decode_AfterEnd(NSSCMSEncryptedData *encd)
{
    
    return SECSuccess;
}
