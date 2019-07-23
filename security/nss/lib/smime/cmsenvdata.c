









































#include "cmslocal.h"

#include "cert.h"
#include "key.h"
#include "secasn1.h"
#include "secitem.h"
#include "secoid.h"
#include "pk11func.h"
#include "secerr.h"
#include "secpkcs5.h"




NSSCMSEnvelopedData *
NSS_CMSEnvelopedData_Create(NSSCMSMessage *cmsg, SECOidTag algorithm, int keysize)
{
    void *mark;
    NSSCMSEnvelopedData *envd;
    PLArenaPool *poolp;
    SECStatus rv;

    poolp = cmsg->poolp;

    mark = PORT_ArenaMark(poolp);

    envd = (NSSCMSEnvelopedData *)PORT_ArenaZAlloc(poolp, sizeof(NSSCMSEnvelopedData));
    if (envd == NULL)
	goto loser;

    envd->cmsg = cmsg;

    

    rv = NSS_CMSContentInfo_SetContentEncAlg(poolp, &(envd->contentInfo), algorithm, NULL, keysize);
    if (rv != SECSuccess)
	goto loser;

    PORT_ArenaUnmark(poolp, mark);
    return envd;

loser:
    PORT_ArenaRelease(poolp, mark);
    return NULL;
}




void
NSS_CMSEnvelopedData_Destroy(NSSCMSEnvelopedData *edp)
{
    NSSCMSRecipientInfo **recipientinfos;
    NSSCMSRecipientInfo *ri;

    if (edp == NULL)
	return;

    recipientinfos = edp->recipientInfos;
    if (recipientinfos == NULL)
	return;

    while ((ri = *recipientinfos++) != NULL)
	NSS_CMSRecipientInfo_Destroy(ri);

   NSS_CMSContentInfo_Destroy(&(edp->contentInfo));

}




NSSCMSContentInfo *
NSS_CMSEnvelopedData_GetContentInfo(NSSCMSEnvelopedData *envd)
{
    return &(envd->contentInfo);
}






SECStatus
NSS_CMSEnvelopedData_AddRecipient(NSSCMSEnvelopedData *edp, NSSCMSRecipientInfo *rip)
{
    void *mark;
    SECStatus rv;

    

    PR_ASSERT(edp != NULL);
    PR_ASSERT(rip != NULL);

    mark = PORT_ArenaMark(edp->cmsg->poolp);

    rv = NSS_CMSArray_Add(edp->cmsg->poolp, (void ***)&(edp->recipientInfos), (void *)rip);
    if (rv != SECSuccess) {
	PORT_ArenaRelease(edp->cmsg->poolp, mark);
	return SECFailure;
    }

    PORT_ArenaUnmark (edp->cmsg->poolp, mark);
    return SECSuccess;
}













SECStatus
NSS_CMSEnvelopedData_Encode_BeforeStart(NSSCMSEnvelopedData *envd)
{
    int version;
    NSSCMSRecipientInfo **recipientinfos;
    NSSCMSContentInfo *cinfo;
    PK11SymKey *bulkkey = NULL;
    SECOidTag bulkalgtag;
    CK_MECHANISM_TYPE type;
    PK11SlotInfo *slot;
    SECStatus rv;
    SECItem *dummy;
    PLArenaPool *poolp;
    extern const SEC_ASN1Template NSSCMSRecipientInfoTemplate[];
    void *mark = NULL;
    int i;

    poolp = envd->cmsg->poolp;
    cinfo = &(envd->contentInfo);

    recipientinfos = envd->recipientInfos;
    if (recipientinfos == NULL) {
	PORT_SetError(SEC_ERROR_BAD_DATA);
#if 0
	PORT_SetErrorString("Cannot find recipientinfos to encode.");
#endif
	goto loser;
    }

    version = NSS_CMS_ENVELOPED_DATA_VERSION_REG;
    if (envd->originatorInfo != NULL || envd->unprotectedAttr != NULL) {
	version = NSS_CMS_ENVELOPED_DATA_VERSION_ADV;
    } else {
	for (i = 0; recipientinfos[i] != NULL; i++) {
	    if (NSS_CMSRecipientInfo_GetVersion(recipientinfos[i]) != 0) {
		version = NSS_CMS_ENVELOPED_DATA_VERSION_ADV;
		break;
	    }
	}
    }
    dummy = SEC_ASN1EncodeInteger(poolp, &(envd->version), version);
    if (dummy == NULL)
	goto loser;

    



    if ((bulkalgtag = NSS_CMSContentInfo_GetContentEncAlgTag(cinfo)) == SEC_OID_UNKNOWN) {
	rv = NSS_CMSContentInfo_SetContentEncAlg(poolp, cinfo, SEC_OID_DES_EDE3_CBC, NULL, 168);
	if (rv != SECSuccess)
	    goto loser;
	bulkalgtag = SEC_OID_DES_EDE3_CBC;
    } 

    
    type = PK11_AlgtagToMechanism(bulkalgtag);
    slot = PK11_GetBestSlot(type, envd->cmsg->pwfn_arg);
    if (slot == NULL)
	goto loser;	

    
    bulkkey = PK11_KeyGen(slot, type, NULL, NSS_CMSContentInfo_GetBulkKeySize(cinfo) / 8, envd->cmsg->pwfn_arg);
    PK11_FreeSlot(slot);
    if (bulkkey == NULL)
	goto loser;	

    mark = PORT_ArenaMark(poolp);

    
    for (i = 0; recipientinfos[i] != NULL; i++) {
	rv = NSS_CMSRecipientInfo_WrapBulkKey(recipientinfos[i], bulkkey, bulkalgtag);
	if (rv != SECSuccess)
	    goto loser;	
	    		
    }

    
    rv = NSS_CMSArray_SortByDER((void **)envd->recipientInfos, NSSCMSRecipientInfoTemplate, NULL);
    if (rv != SECSuccess)
	goto loser;	

    
    NSS_CMSContentInfo_SetBulkKey(cinfo, bulkkey);

    PORT_ArenaUnmark(poolp, mark);

    PK11_FreeSymKey(bulkkey);

    return SECSuccess;

loser:
    if (mark != NULL)
	PORT_ArenaRelease (poolp, mark);
    if (bulkkey)
	PK11_FreeSymKey(bulkkey);

    return SECFailure;
}







SECStatus
NSS_CMSEnvelopedData_Encode_BeforeData(NSSCMSEnvelopedData *envd)
{
    NSSCMSContentInfo *cinfo;
    PK11SymKey *bulkkey;
    SECAlgorithmID *algid;

    cinfo = &(envd->contentInfo);

    
    bulkkey = NSS_CMSContentInfo_GetBulkKey(cinfo);
    if (bulkkey == NULL)
	return SECFailure;
    algid = NSS_CMSContentInfo_GetContentEncAlg(cinfo);
    if (algid == NULL)
	return SECFailure;

    


    cinfo->ciphcx = NSS_CMSCipherContext_StartEncrypt(envd->cmsg->poolp, bulkkey, algid);
    PK11_FreeSymKey(bulkkey);
    if (cinfo->ciphcx == NULL)
	return SECFailure;

    return SECSuccess;
}




SECStatus
NSS_CMSEnvelopedData_Encode_AfterData(NSSCMSEnvelopedData *envd)
{
    if (envd->contentInfo.ciphcx) {
	NSS_CMSCipherContext_Destroy(envd->contentInfo.ciphcx);
	envd->contentInfo.ciphcx = NULL;
    }

    
    return SECSuccess;
}





SECStatus
NSS_CMSEnvelopedData_Decode_BeforeData(NSSCMSEnvelopedData *envd)
{
    NSSCMSRecipientInfo *ri;
    PK11SymKey *bulkkey = NULL;
    SECOidTag bulkalgtag;
    SECAlgorithmID *bulkalg;
    SECStatus rv = SECFailure;
    NSSCMSContentInfo *cinfo;
    NSSCMSRecipient **recipient_list = NULL;
    NSSCMSRecipient *recipient;
    int rlIndex;

    if (NSS_CMSArray_Count((void **)envd->recipientInfos) == 0) {
	PORT_SetError(SEC_ERROR_BAD_DATA);
#if 0
	PORT_SetErrorString("No recipient data in envelope.");
#endif
	goto loser;
    }

    
    
    recipient_list = nss_cms_recipient_list_create(envd->recipientInfos);
    if (recipient_list == NULL)
	goto loser;

    



    rlIndex = PK11_FindCertAndKeyByRecipientListNew(recipient_list, envd->cmsg->pwfn_arg);

    
    if (rlIndex < 0) {
	PORT_SetError(SEC_ERROR_NOT_A_RECIPIENT);
#if 0
	PORT_SetErrorString("Cannot decrypt data because proper key cannot be found.");
#endif
	goto loser;
    }

    recipient = recipient_list[rlIndex];
    if (!recipient->cert || !recipient->privkey) {
	
	goto loser;
    }
    
    ri = envd->recipientInfos[recipient->riIndex];

    cinfo = &(envd->contentInfo);
    bulkalgtag = NSS_CMSContentInfo_GetContentEncAlgTag(cinfo);
    if (bulkalgtag == SEC_OID_UNKNOWN) {
	PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
    } else 
	bulkkey = 
	    NSS_CMSRecipientInfo_UnwrapBulkKey(ri,recipient->subIndex,
						    recipient->cert,
						    recipient->privkey,
						    bulkalgtag);
    if (bulkkey == NULL) {
	
	goto loser;
    }

    NSS_CMSContentInfo_SetBulkKey(cinfo, bulkkey);

    bulkalg = NSS_CMSContentInfo_GetContentEncAlg(cinfo);

    cinfo->ciphcx = NSS_CMSCipherContext_StartDecrypt(bulkkey, bulkalg);
    if (cinfo->ciphcx == NULL)
	goto loser;		


    rv = SECSuccess;

loser:
    if (bulkkey)
	PK11_FreeSymKey(bulkkey);
    if (recipient_list != NULL)
	nss_cms_recipient_list_destroy(recipient_list);
    return rv;
}




SECStatus
NSS_CMSEnvelopedData_Decode_AfterData(NSSCMSEnvelopedData *envd)
{
    if (envd && envd->contentInfo.ciphcx) {
	NSS_CMSCipherContext_Destroy(envd->contentInfo.ciphcx);
	envd->contentInfo.ciphcx = NULL;
    }

    return SECSuccess;
}




SECStatus
NSS_CMSEnvelopedData_Decode_AfterEnd(NSSCMSEnvelopedData *envd)
{
    
    return SECSuccess;
}

