










































#include "cmslocal.h"

#include "cert.h"
#include "key.h"
#include "secasn1.h"
#include "secitem.h"
#include "secoid.h"
#include "pk11func.h"
#include "secerr.h"
#include "sechash.h"








SECStatus
NSS_CMSArray_SortByDER(void **objs, const SEC_ASN1Template *objtemplate, void **objs2)
{
    PRArenaPool *poolp;
    int num_objs;
    SECItem **enc_objs;
    SECStatus rv = SECFailure;
    int i;

    if (objs == NULL)					
	return SECSuccess;

    num_objs = NSS_CMSArray_Count((void **)objs);
    if (num_objs == 0 || num_objs == 1)		
	return SECSuccess;

    poolp = PORT_NewArena (1024);	
    if (poolp == NULL)
	return SECFailure;		

    



    enc_objs = (SECItem **)PORT_ArenaZAlloc(poolp, (num_objs + 1) * sizeof(SECItem *));
    if (enc_objs == NULL)
	goto loser;

    
    for (i = 0; i < num_objs; i++) {
	enc_objs[i] = SEC_ASN1EncodeItem(poolp, NULL, objs[i], objtemplate);
	if (enc_objs[i] == NULL)
	    goto loser;
    }
    enc_objs[num_objs] = NULL;

    
    NSS_CMSArray_Sort((void **)enc_objs, NSS_CMSUtil_DERCompare, objs, objs2);

    rv = SECSuccess;

loser:
    PORT_FreeArena (poolp, PR_FALSE);
    return rv;
}





int
NSS_CMSUtil_DERCompare(void *a, void *b)
{
    SECItem *der1 = (SECItem *)a;
    SECItem *der2 = (SECItem *)b;
    unsigned int j;

    










    if (der1->len != der2->len)
	return (der1->len < der2->len) ? -1 : 1;

    for (j = 0; j < der1->len; j++) {
	if (der1->data[j] == der2->data[j])
	    continue;
	return (der1->data[j] < der2->data[j]) ? -1 : 1;
    }
    return 0;
}












int
NSS_CMSAlgArray_GetIndexByAlgID(SECAlgorithmID **algorithmArray, SECAlgorithmID *algid)
{
    int i;

    if (algorithmArray == NULL || algorithmArray[0] == NULL)
	return -1;

    for (i = 0; algorithmArray[i] != NULL; i++) {
	if (SECOID_CompareAlgorithmID(algorithmArray[i], algid) == SECEqual)
	    break;	
    }

    if (algorithmArray[i] == NULL)
	return -1;	

    return i;
}












int
NSS_CMSAlgArray_GetIndexByAlgTag(SECAlgorithmID **algorithmArray, 
                                 SECOidTag algtag)
{
    SECOidData *algid;
    int i = -1;

    if (algorithmArray == NULL || algorithmArray[0] == NULL)
	return i;

#ifdef ORDER_N_SQUARED
    for (i = 0; algorithmArray[i] != NULL; i++) {
	algid = SECOID_FindOID(&(algorithmArray[i]->algorithm));
	if (algid->offset == algtag)
	    break;	
    }
#else
    algid = SECOID_FindOIDByTag(algtag);
    if (!algid) 
    	return i;
    for (i = 0; algorithmArray[i] != NULL; i++) {
	if (SECITEM_ItemsAreEqual(&algorithmArray[i]->algorithm, &algid->oid))
	    break;	
    }
#endif

    if (algorithmArray[i] == NULL)
	return -1;	

    return i;
}

const SECHashObject *
NSS_CMSUtil_GetHashObjByAlgID(SECAlgorithmID *algid)
{
    SECOidTag oidTag = SECOID_FindOIDTag(&(algid->algorithm));
    const SECHashObject *digobj = HASH_GetHashObjectByOidTag(oidTag);

    return digobj;
}

const SEC_ASN1Template *
NSS_CMSUtil_GetTemplateByTypeTag(SECOidTag type)
{
    const SEC_ASN1Template *template;
    extern const SEC_ASN1Template NSSCMSSignedDataTemplate[];
    extern const SEC_ASN1Template NSSCMSEnvelopedDataTemplate[];
    extern const SEC_ASN1Template NSSCMSEncryptedDataTemplate[];
    extern const SEC_ASN1Template NSSCMSDigestedDataTemplate[];

    switch (type) {
    case SEC_OID_PKCS7_SIGNED_DATA:
	template = NSSCMSSignedDataTemplate;
	break;
    case SEC_OID_PKCS7_ENVELOPED_DATA:
	template = NSSCMSEnvelopedDataTemplate;
	break;
    case SEC_OID_PKCS7_ENCRYPTED_DATA:
	template = NSSCMSEncryptedDataTemplate;
	break;
    case SEC_OID_PKCS7_DIGESTED_DATA:
	template = NSSCMSDigestedDataTemplate;
	break;
    default:
	template = NSS_CMSType_GetTemplate(type);
	break;
    }
    return template;
}

size_t
NSS_CMSUtil_GetSizeByTypeTag(SECOidTag type)
{
    size_t size;

    switch (type) {
    case SEC_OID_PKCS7_SIGNED_DATA:
	size = sizeof(NSSCMSSignedData);
	break;
    case SEC_OID_PKCS7_ENVELOPED_DATA:
	size = sizeof(NSSCMSEnvelopedData);
	break;
    case SEC_OID_PKCS7_ENCRYPTED_DATA:
	size = sizeof(NSSCMSEncryptedData);
	break;
    case SEC_OID_PKCS7_DIGESTED_DATA:
	size = sizeof(NSSCMSDigestedData);
	break;
    default:
	size = NSS_CMSType_GetContentSize(type);
	break;
    }
    return size;
}

NSSCMSContentInfo *
NSS_CMSContent_GetContentInfo(void *msg, SECOidTag type)
{
    NSSCMSContent c;
    NSSCMSContentInfo *cinfo = NULL;

    if (!msg)
	return cinfo;
    c.pointer = msg;
    switch (type) {
    case SEC_OID_PKCS7_SIGNED_DATA:
	cinfo = &(c.signedData->contentInfo);
	break;
    case SEC_OID_PKCS7_ENVELOPED_DATA:
	cinfo = &(c.envelopedData->contentInfo);
	break;
    case SEC_OID_PKCS7_ENCRYPTED_DATA:
	cinfo = &(c.encryptedData->contentInfo);
	break;
    case SEC_OID_PKCS7_DIGESTED_DATA:
	cinfo = &(c.digestedData->contentInfo);
	break;
    default:
	cinfo = NULL;
	if (NSS_CMSType_IsWrapper(type)) {
	    cinfo = &(c.genericData->contentInfo);
	}
    }
    return cinfo;
}

const char *
NSS_CMSUtil_VerificationStatusToString(NSSCMSVerificationStatus vs)
{
    switch (vs) {
    case NSSCMSVS_Unverified:			return "Unverified";
    case NSSCMSVS_GoodSignature:		return "GoodSignature";
    case NSSCMSVS_BadSignature:			return "BadSignature";
    case NSSCMSVS_DigestMismatch:		return "DigestMismatch";
    case NSSCMSVS_SigningCertNotFound:		return "SigningCertNotFound";
    case NSSCMSVS_SigningCertNotTrusted:	return "SigningCertNotTrusted";
    case NSSCMSVS_SignatureAlgorithmUnknown:	return "SignatureAlgorithmUnknown";
    case NSSCMSVS_SignatureAlgorithmUnsupported: return "SignatureAlgorithmUnsupported";
    case NSSCMSVS_MalformedSignature:		return "MalformedSignature";
    case NSSCMSVS_ProcessingError:		return "ProcessingError";
    default:					return "Unknown";
    }
}

SECStatus
NSS_CMSDEREncode(NSSCMSMessage *cmsg, SECItem *input, SECItem *derOut, 
                 PLArenaPool *arena)
{
    NSSCMSEncoderContext *ecx;
    SECStatus rv = SECSuccess;
    if (!cmsg || !derOut || !arena) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    ecx = NSS_CMSEncoder_Start(cmsg, 0, 0, derOut, arena, 0, 0, 0, 0, 0, 0);
    if (!ecx) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    if (input) {
	rv = NSS_CMSEncoder_Update(ecx, (const char*)input->data, input->len);
	if (rv) {
	    PORT_SetError(SEC_ERROR_BAD_DATA);
	}
    }
    rv |= NSS_CMSEncoder_Finish(ecx);
    if (rv) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
    }
    return rv;
}
