









































#include "cmslocal.h"

#include "pk11func.h"
#include "secitem.h"
#include "secoid.h"
#include "secerr.h"










void
NSS_CMSContentInfo_Destroy(NSSCMSContentInfo *cinfo)
{
    SECOidTag kind;

    kind = NSS_CMSContentInfo_GetContentTypeTag(cinfo);
    switch (kind) {
    case SEC_OID_PKCS7_ENVELOPED_DATA:
	NSS_CMSEnvelopedData_Destroy(cinfo->content.envelopedData);
	break;
      case SEC_OID_PKCS7_SIGNED_DATA:
	NSS_CMSSignedData_Destroy(cinfo->content.signedData);
	break;
      case SEC_OID_PKCS7_ENCRYPTED_DATA:
	NSS_CMSEncryptedData_Destroy(cinfo->content.encryptedData);
	break;
      case SEC_OID_PKCS7_DIGESTED_DATA:
	NSS_CMSDigestedData_Destroy(cinfo->content.digestedData);
	break;
      default:
	
	break;
    }
    if (cinfo->digcx) {
	
	NSS_CMSDigestContext_Cancel(cinfo->digcx);
	cinfo->digcx = NULL;
    }
    if (cinfo->bulkkey)
	PK11_FreeSymKey(cinfo->bulkkey);

    if (cinfo->ciphcx) {
	NSS_CMSCipherContext_Destroy(cinfo->ciphcx);
	cinfo->ciphcx = NULL;
    }
    
    
}




NSSCMSContentInfo *
NSS_CMSContentInfo_GetChildContentInfo(NSSCMSContentInfo *cinfo)
{
    void * ptr                  = NULL;
    NSSCMSContentInfo * ccinfo  = NULL;
    SECOidTag tag = NSS_CMSContentInfo_GetContentTypeTag(cinfo);
    switch (tag) {
    case SEC_OID_PKCS7_SIGNED_DATA:
	ptr    = (void *)cinfo->content.signedData;
	ccinfo = &(cinfo->content.signedData->contentInfo);
	break;
    case SEC_OID_PKCS7_ENVELOPED_DATA:
	ptr    = (void *)cinfo->content.envelopedData;
	ccinfo = &(cinfo->content.envelopedData->contentInfo);
	break;
    case SEC_OID_PKCS7_DIGESTED_DATA:
	ptr    = (void *)cinfo->content.digestedData;
	ccinfo = &(cinfo->content.digestedData->contentInfo);
	break;
    case SEC_OID_PKCS7_ENCRYPTED_DATA:
	ptr    = (void *)cinfo->content.encryptedData;
	ccinfo = &(cinfo->content.encryptedData->contentInfo);
	break;
    case SEC_OID_PKCS7_DATA:
    default:
	break;
    }
    return (ptr ? ccinfo : NULL);
}




SECStatus
NSS_CMSContentInfo_SetContent(NSSCMSMessage *cmsg, NSSCMSContentInfo *cinfo, SECOidTag type, void *ptr)
{
    SECStatus rv;

    cinfo->contentTypeTag = SECOID_FindOIDByTag(type);
    if (cinfo->contentTypeTag == NULL)
	return SECFailure;
    
    
    rv = SECITEM_CopyItem (cmsg->poolp, &(cinfo->contentType), &(cinfo->contentTypeTag->oid));
    if (rv != SECSuccess)
	return SECFailure;

    cinfo->content.pointer = ptr;

    if (type != SEC_OID_PKCS7_DATA) {
	


	cinfo->rawContent = SECITEM_AllocItem(cmsg->poolp, NULL, 1);
	if (cinfo->rawContent == NULL) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    return SECFailure;
	}
    }

    return SECSuccess;
}









SECStatus
NSS_CMSContentInfo_SetContent_Data(NSSCMSMessage *cmsg, NSSCMSContentInfo *cinfo, SECItem *data, PRBool detached)
{
    if (NSS_CMSContentInfo_SetContent(cmsg, cinfo, SEC_OID_PKCS7_DATA, (void *)data) != SECSuccess)
	return SECFailure;
    cinfo->rawContent = (detached) ? 
			    NULL : (data) ? 
				data : SECITEM_AllocItem(cmsg->poolp, NULL, 1);
    return SECSuccess;
}

SECStatus
NSS_CMSContentInfo_SetContent_SignedData(NSSCMSMessage *cmsg, NSSCMSContentInfo *cinfo, NSSCMSSignedData *sigd)
{
    return NSS_CMSContentInfo_SetContent(cmsg, cinfo, SEC_OID_PKCS7_SIGNED_DATA, (void *)sigd);
}

SECStatus
NSS_CMSContentInfo_SetContent_EnvelopedData(NSSCMSMessage *cmsg, NSSCMSContentInfo *cinfo, NSSCMSEnvelopedData *envd)
{
    return NSS_CMSContentInfo_SetContent(cmsg, cinfo, SEC_OID_PKCS7_ENVELOPED_DATA, (void *)envd);
}

SECStatus
NSS_CMSContentInfo_SetContent_DigestedData(NSSCMSMessage *cmsg, NSSCMSContentInfo *cinfo, NSSCMSDigestedData *digd)
{
    return NSS_CMSContentInfo_SetContent(cmsg, cinfo, SEC_OID_PKCS7_DIGESTED_DATA, (void *)digd);
}

SECStatus
NSS_CMSContentInfo_SetContent_EncryptedData(NSSCMSMessage *cmsg, NSSCMSContentInfo *cinfo, NSSCMSEncryptedData *encd)
{
    return NSS_CMSContentInfo_SetContent(cmsg, cinfo, SEC_OID_PKCS7_ENCRYPTED_DATA, (void *)encd);
}






void *
NSS_CMSContentInfo_GetContent(NSSCMSContentInfo *cinfo)
{
    SECOidTag tag = (cinfo && cinfo->contentTypeTag) 
	                ? cinfo->contentTypeTag->offset 
	                : SEC_OID_UNKNOWN;
    switch (tag) {
    case SEC_OID_PKCS7_DATA:
    case SEC_OID_PKCS7_SIGNED_DATA:
    case SEC_OID_PKCS7_ENVELOPED_DATA:
    case SEC_OID_PKCS7_DIGESTED_DATA:
    case SEC_OID_PKCS7_ENCRYPTED_DATA:
	return cinfo->content.pointer;
    default:
	return NULL;
    }
}






SECItem *
NSS_CMSContentInfo_GetInnerContent(NSSCMSContentInfo *cinfo)
{
    NSSCMSContentInfo *ccinfo;
    SECOidTag          tag;
    SECItem           *pItem = NULL;

    tag = NSS_CMSContentInfo_GetContentTypeTag(cinfo);
    switch (tag) {
    case SEC_OID_PKCS7_DATA:
	
	pItem = cinfo->content.data; 
	break;
    case SEC_OID_PKCS7_DIGESTED_DATA:
    case SEC_OID_PKCS7_ENCRYPTED_DATA:
    case SEC_OID_PKCS7_ENVELOPED_DATA:
    case SEC_OID_PKCS7_SIGNED_DATA:
	ccinfo = NSS_CMSContentInfo_GetChildContentInfo(cinfo);
	if (ccinfo != NULL)
	    pItem = NSS_CMSContentInfo_GetContent(ccinfo);
	break;
    default:
	PORT_Assert(0);
	break;
    }
    return pItem;
}





SECOidTag
NSS_CMSContentInfo_GetContentTypeTag(NSSCMSContentInfo *cinfo)
{
    if (cinfo->contentTypeTag == NULL)
	cinfo->contentTypeTag = SECOID_FindOID(&(cinfo->contentType));

    if (cinfo->contentTypeTag == NULL)
	return SEC_OID_UNKNOWN;

    return cinfo->contentTypeTag->offset;
}

SECItem *
NSS_CMSContentInfo_GetContentTypeOID(NSSCMSContentInfo *cinfo)
{
    if (cinfo->contentTypeTag == NULL)
	cinfo->contentTypeTag = SECOID_FindOID(&(cinfo->contentType));

    if (cinfo->contentTypeTag == NULL)
	return NULL;

    return &(cinfo->contentTypeTag->oid);
}





SECOidTag
NSS_CMSContentInfo_GetContentEncAlgTag(NSSCMSContentInfo *cinfo)
{
    if (cinfo->contentEncAlgTag == SEC_OID_UNKNOWN)
	cinfo->contentEncAlgTag = SECOID_GetAlgorithmTag(&(cinfo->contentEncAlg));

    return cinfo->contentEncAlgTag;
}




SECAlgorithmID *
NSS_CMSContentInfo_GetContentEncAlg(NSSCMSContentInfo *cinfo)
{
    return &(cinfo->contentEncAlg);
}

SECStatus
NSS_CMSContentInfo_SetContentEncAlg(PLArenaPool *poolp, NSSCMSContentInfo *cinfo,
				    SECOidTag bulkalgtag, SECItem *parameters, int keysize)
{
    SECStatus rv;

    rv = SECOID_SetAlgorithmID(poolp, &(cinfo->contentEncAlg), bulkalgtag, parameters);
    if (rv != SECSuccess)
	return SECFailure;
    cinfo->keysize = keysize;
    return SECSuccess;
}

SECStatus
NSS_CMSContentInfo_SetContentEncAlgID(PLArenaPool *poolp, NSSCMSContentInfo *cinfo,
				    SECAlgorithmID *algid, int keysize)
{
    SECStatus rv;

    rv = SECOID_CopyAlgorithmID(poolp, &(cinfo->contentEncAlg), algid);
    if (rv != SECSuccess)
	return SECFailure;
    if (keysize >= 0)
	cinfo->keysize = keysize;
    return SECSuccess;
}

void
NSS_CMSContentInfo_SetBulkKey(NSSCMSContentInfo *cinfo, PK11SymKey *bulkkey)
{
    cinfo->bulkkey = PK11_ReferenceSymKey(bulkkey);
    cinfo->keysize = PK11_GetKeyStrength(cinfo->bulkkey, &(cinfo->contentEncAlg));
}

PK11SymKey *
NSS_CMSContentInfo_GetBulkKey(NSSCMSContentInfo *cinfo)
{
    if (cinfo->bulkkey == NULL)
	return NULL;

    return PK11_ReferenceSymKey(cinfo->bulkkey);
}

int
NSS_CMSContentInfo_GetBulkKeySize(NSSCMSContentInfo *cinfo)
{
    return cinfo->keysize;
}
