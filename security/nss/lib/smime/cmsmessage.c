







#include "cmslocal.h"

#include "cert.h"
#include "secasn1.h"
#include "secitem.h"
#include "secoid.h"
#include "pk11func.h"
#include "secerr.h"






NSSCMSMessage *
NSS_CMSMessage_Create(PLArenaPool *poolp)
{
    void *mark = NULL;
    NSSCMSMessage *cmsg;
    PRBool poolp_is_ours = PR_FALSE;

    if (poolp == NULL) {
        poolp = PORT_NewArena (1024);           
        if (poolp == NULL)
            return NULL;
        poolp_is_ours = PR_TRUE;
    }

    if (!poolp_is_ours)
        mark = PORT_ArenaMark(poolp);

    cmsg = (NSSCMSMessage *)PORT_ArenaZAlloc(poolp, sizeof(NSSCMSMessage));
    if (cmsg == NULL ||
        NSS_CMSContentInfo_Private_Init(&(cmsg->contentInfo)) != SECSuccess) {
        if (!poolp_is_ours) {
            if (mark) {
                PORT_ArenaRelease(poolp, mark);
            }
        } else
            PORT_FreeArena(poolp, PR_FALSE);
        return NULL;
    }

    cmsg->poolp = poolp;
    cmsg->poolp_is_ours = poolp_is_ours;
    cmsg->refCount = 1;

    if (mark)
	PORT_ArenaUnmark(poolp, mark);

    return cmsg;
}









void
NSS_CMSMessage_SetEncodingParams(NSSCMSMessage *cmsg,
			PK11PasswordFunc pwfn, void *pwfn_arg,
			NSSCMSGetDecryptKeyCallback decrypt_key_cb, void *decrypt_key_cb_arg,
			SECAlgorithmID **detached_digestalgs, SECItem **detached_digests)
{
    if (pwfn)
	PK11_SetPasswordFunc(pwfn);
    cmsg->pwfn_arg = pwfn_arg;
    cmsg->decrypt_key_cb = decrypt_key_cb;
    cmsg->decrypt_key_cb_arg = decrypt_key_cb_arg;
    cmsg->detached_digestalgs = detached_digestalgs;
    cmsg->detached_digests = detached_digests;
}




void
NSS_CMSMessage_Destroy(NSSCMSMessage *cmsg)
{
    PORT_Assert (cmsg->refCount > 0);
    if (cmsg->refCount <= 0)	
	return;

    cmsg->refCount--;		
    if (cmsg->refCount > 0)
	return;

    NSS_CMSContentInfo_Destroy(&(cmsg->contentInfo));

    
    if (cmsg->poolp_is_ours)
	PORT_FreeArena (cmsg->poolp, PR_FALSE);	
}







NSSCMSMessage *
NSS_CMSMessage_Copy(NSSCMSMessage *cmsg)
{
    if (cmsg == NULL)
	return NULL;

    PORT_Assert (cmsg->refCount > 0);

    cmsg->refCount++; 
    return cmsg;
}




PLArenaPool *
NSS_CMSMessage_GetArena(NSSCMSMessage *cmsg)
{
    return cmsg->poolp;
}




NSSCMSContentInfo *
NSS_CMSMessage_GetContentInfo(NSSCMSMessage *cmsg)
{
    return &(cmsg->contentInfo);
}






SECItem *
NSS_CMSMessage_GetContent(NSSCMSMessage *cmsg)
{
    
    NSSCMSContentInfo * cinfo = NSS_CMSMessage_GetContentInfo(cmsg);
    SECItem           * pItem = NSS_CMSContentInfo_GetInnerContent(cinfo);
    return pItem;
}






int
NSS_CMSMessage_ContentLevelCount(NSSCMSMessage *cmsg)
{
    int count = 0;
    NSSCMSContentInfo *cinfo;

    
    for (cinfo = &(cmsg->contentInfo); cinfo != NULL; ) {
	count++;
	cinfo = NSS_CMSContentInfo_GetChildContentInfo(cinfo);
    }
    return count;
}






NSSCMSContentInfo *
NSS_CMSMessage_ContentLevel(NSSCMSMessage *cmsg, int n)
{
    int count = 0;
    NSSCMSContentInfo *cinfo;

    
    for (cinfo = &(cmsg->contentInfo); cinfo != NULL && count < n; cinfo = NSS_CMSContentInfo_GetChildContentInfo(cinfo)) {
	count++;
    }

    return cinfo;
}




PRBool
NSS_CMSMessage_ContainsCertsOrCrls(NSSCMSMessage *cmsg)
{
    NSSCMSContentInfo *cinfo;

    
    for (cinfo = &(cmsg->contentInfo); cinfo != NULL; cinfo = NSS_CMSContentInfo_GetChildContentInfo(cinfo)) {
	if (!NSS_CMSType_IsData(NSS_CMSContentInfo_GetContentTypeTag(cinfo)))
	    continue;	
	
	if (NSS_CMSSignedData_ContainsCertsOrCrls(cinfo->content.signedData))
	    return PR_TRUE;
	
    }
    return PR_FALSE;
}




PRBool
NSS_CMSMessage_IsEncrypted(NSSCMSMessage *cmsg)
{
    NSSCMSContentInfo *cinfo;

    
    for (cinfo = &(cmsg->contentInfo); cinfo != NULL; cinfo = NSS_CMSContentInfo_GetChildContentInfo(cinfo))
    {
	switch (NSS_CMSContentInfo_GetContentTypeTag(cinfo)) {
	case SEC_OID_PKCS7_ENVELOPED_DATA:
	case SEC_OID_PKCS7_ENCRYPTED_DATA:
	    return PR_TRUE;
	default:
	    
	    break;
	}
    }
    return PR_FALSE;
}











PRBool
NSS_CMSMessage_IsSigned(NSSCMSMessage *cmsg)
{
    NSSCMSContentInfo *cinfo;

    
    for (cinfo = &(cmsg->contentInfo); cinfo != NULL; cinfo = NSS_CMSContentInfo_GetChildContentInfo(cinfo))
    {
	switch (NSS_CMSContentInfo_GetContentTypeTag(cinfo)) {
	case SEC_OID_PKCS7_SIGNED_DATA:
	    if (!NSS_CMSArray_IsEmpty((void **)cinfo->content.signedData->signerInfos))
		return PR_TRUE;
	    break;
	default:
	    
	    break;
	}
    }
    return PR_FALSE;
}







PRBool
NSS_CMSMessage_IsContentEmpty(NSSCMSMessage *cmsg, unsigned int minLen)
{
    SECItem *item = NULL;

    if (cmsg == NULL)
	return PR_TRUE;

    item = NSS_CMSContentInfo_GetContent(NSS_CMSMessage_GetContentInfo(cmsg));

    if (!item) {
	return PR_TRUE;
    } else if(item->len <= minLen) {
	return PR_TRUE;
    }

    return PR_FALSE;
}
