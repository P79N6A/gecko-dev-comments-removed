









































#include "cmslocal.h"

#include "cert.h"
#include "key.h"
#include "secasn1.h"
#include "secoid.h"
#include "secitem.h"
#include "pk11func.h"
#include "secerr.h"

struct nss_cms_encoder_output {
    NSSCMSContentCallback outputfn;
    void *outputarg;
    PLArenaPool *destpoolp;
    SECItem *dest;
};

struct NSSCMSEncoderContextStr {
    SEC_ASN1EncoderContext *	ecx;		
    PRBool			ecxupdated;	
    NSSCMSMessage *		cmsg;		
    SECOidTag			type;		
    NSSCMSContent		content;	
    struct nss_cms_encoder_output output;	
    int				error;		
    NSSCMSEncoderContext *	childp7ecx;	
};

static SECStatus nss_cms_before_data(NSSCMSEncoderContext *p7ecx);
static SECStatus nss_cms_after_data(NSSCMSEncoderContext *p7ecx);
static SECStatus nss_cms_encoder_update(NSSCMSEncoderContext *p7ecx, const char *data, unsigned long len);
static SECStatus nss_cms_encoder_work_data(NSSCMSEncoderContext *p7ecx, SECItem *dest,
			     const unsigned char *data, unsigned long len,
			     PRBool final, PRBool innermost);

extern const SEC_ASN1Template NSSCMSMessageTemplate[];






static void
nss_cms_encoder_out(void *arg, const char *buf, unsigned long len,
		      int depth, SEC_ASN1EncodingPart data_kind)
{
    struct nss_cms_encoder_output *output = (struct nss_cms_encoder_output *)arg;
    unsigned char *dest;
    unsigned long offset;

#ifdef CMSDEBUG
    int i;
    const char *data_name = "unknown";

    switch (data_kind) {
    case SEC_ASN1_Identifier:
        data_name = "identifier";
        break;
    case SEC_ASN1_Length:
        data_name = "length";
        break;
    case SEC_ASN1_Contents:
        data_name = "contents";
        break;
    case SEC_ASN1_EndOfContents:
        data_name = "end-of-contents";
        break;
    }
    fprintf(stderr, "kind = %s, depth = %d, len = %d\n", data_name, depth, len);
    for (i=0; i < len; i++) {
	fprintf(stderr, " %02x%s", (unsigned int)buf[i] & 0xff, ((i % 16) == 15) ? "\n" : "");
    }
    if ((i % 16) != 0)
	fprintf(stderr, "\n");
#endif

    if (output->outputfn != NULL)
	
	output->outputfn(output->outputarg, buf, len);

    if (output->dest != NULL) {
	
	offset = output->dest->len;
	if (offset == 0) {
	    dest = (unsigned char *)PORT_ArenaAlloc(output->destpoolp, len);
	} else {
	    dest = (unsigned char *)PORT_ArenaGrow(output->destpoolp, 
				  output->dest->data,
				  output->dest->len,
				  output->dest->len + len);
	}
	if (dest == NULL)
	    
	    return;

	output->dest->data = dest;
	output->dest->len += len;

	
	PORT_Memcpy(output->dest->data + offset, buf, len);
    }
}








static void
nss_cms_encoder_notify(void *arg, PRBool before, void *dest, int depth)
{
    NSSCMSEncoderContext *p7ecx;
    NSSCMSContentInfo *rootcinfo, *cinfo;
    PRBool after = !before;
    PLArenaPool *poolp;
    SECOidTag childtype;
    SECItem *item;

    p7ecx = (NSSCMSEncoderContext *)arg;
    PORT_Assert(p7ecx != NULL);

    rootcinfo = &(p7ecx->cmsg->contentInfo);
    poolp = p7ecx->cmsg->poolp;

#ifdef CMSDEBUG
    fprintf(stderr, "%6.6s, dest = 0x%08x, depth = %d\n", before ? "before" : "after", dest, depth);
#endif

    



    if (NSS_CMSType_IsData(p7ecx->type)) {
	cinfo = NSS_CMSContent_GetContentInfo(p7ecx->content.pointer, p7ecx->type);
	if (before && dest == &(cinfo->rawContent)) {
	    
	    if ((item = cinfo->content.data) != NULL)
		(void)nss_cms_encoder_work_data(p7ecx, NULL, item->data, item->len, PR_TRUE, PR_TRUE);
	    else
		SEC_ASN1EncoderSetTakeFromBuf(p7ecx->ecx);
	    SEC_ASN1EncoderClearNotifyProc(p7ecx->ecx);	
	}
    } else if (NSS_CMSType_IsWrapper(p7ecx->type)) {
	
	cinfo = NSS_CMSContent_GetContentInfo(p7ecx->content.pointer, p7ecx->type);
	childtype = NSS_CMSContentInfo_GetContentTypeTag(cinfo);

	if (after && dest == &(cinfo->contentType)) {
	    
	    
	    
	    if (nss_cms_before_data(p7ecx) != SECSuccess)
		p7ecx->error = PORT_GetError();
	}
	if (before && dest == &(cinfo->rawContent)) {
	    if (p7ecx->childp7ecx == NULL) {
		if ((NSS_CMSType_IsData(childtype) && (item = cinfo->content.data) != NULL)) {
		    
		    (void)nss_cms_encoder_work_data(p7ecx, NULL, item->data, item->len, PR_TRUE, PR_TRUE);
	        } else {
		    
		    SEC_ASN1EncoderSetTakeFromBuf(p7ecx->ecx);
		}
	    } else {
	        
		SEC_ASN1EncoderSetTakeFromBuf(p7ecx->ecx);
	    }
	}
	if (after && dest == &(cinfo->rawContent)) {
	    if (nss_cms_after_data(p7ecx) != SECSuccess)
		p7ecx->error = PORT_GetError();
	    SEC_ASN1EncoderClearNotifyProc(p7ecx->ecx);	
	}
    } else {
	
	if (after && dest == &(rootcinfo->contentType)) {
	    
	    p7ecx->type = NSS_CMSContentInfo_GetContentTypeTag(rootcinfo);
	    
	    p7ecx->content = rootcinfo->content;
	}
    }
}




static SECStatus
nss_cms_before_data(NSSCMSEncoderContext *p7ecx)
{
    SECStatus rv;
    SECOidTag childtype;
    NSSCMSContentInfo *cinfo;
    PLArenaPool *poolp;
    NSSCMSEncoderContext *childp7ecx;
    const SEC_ASN1Template *template;

    poolp = p7ecx->cmsg->poolp;

    
    switch (p7ecx->type) {
    case SEC_OID_PKCS7_SIGNED_DATA:
	
	rv = NSS_CMSSignedData_Encode_BeforeData(p7ecx->content.signedData);
	break;
    case SEC_OID_PKCS7_DIGESTED_DATA:
	
	rv = NSS_CMSDigestedData_Encode_BeforeData(p7ecx->content.digestedData);
	break;
    case SEC_OID_PKCS7_ENVELOPED_DATA:
	rv = NSS_CMSEnvelopedData_Encode_BeforeData(p7ecx->content.envelopedData);
	break;
    case SEC_OID_PKCS7_ENCRYPTED_DATA:
	rv = NSS_CMSEncryptedData_Encode_BeforeData(p7ecx->content.encryptedData);
	break;
    default:
        if (NSS_CMSType_IsWrapper(p7ecx->type)) {
	    rv = NSS_CMSGenericWrapperData_Encode_BeforeData(p7ecx->type, p7ecx->content.genericData);
	} else {
	    rv = SECFailure;
	}
    }
    if (rv != SECSuccess)
	return SECFailure;

    
    
    
    cinfo = NSS_CMSContent_GetContentInfo(p7ecx->content.pointer, p7ecx->type);
    childtype = NSS_CMSContentInfo_GetContentTypeTag(cinfo);

    if (NSS_CMSType_IsWrapper(childtype)) {
	
	
	childp7ecx = PORT_ZAlloc(sizeof(NSSCMSEncoderContext));
	if (childp7ecx == NULL)
	    return SECFailure;

	



	childp7ecx->type = childtype;
	childp7ecx->content = cinfo->content;
	
	childp7ecx->output.outputfn = (NSSCMSContentCallback)nss_cms_encoder_update;
	childp7ecx->output.outputarg = p7ecx;
	childp7ecx->output.destpoolp = NULL;
	childp7ecx->output.dest = NULL;
	childp7ecx->cmsg = p7ecx->cmsg;
	childp7ecx->ecxupdated = PR_FALSE;
	childp7ecx->childp7ecx = NULL;

	template = NSS_CMSUtil_GetTemplateByTypeTag(childtype);
	if (template == NULL)
	    goto loser;		

	
	switch (childp7ecx->type) {
	case SEC_OID_PKCS7_SIGNED_DATA:
	    rv = NSS_CMSSignedData_Encode_BeforeStart(cinfo->content.signedData);
	    break;
	case SEC_OID_PKCS7_ENVELOPED_DATA:
	    rv = NSS_CMSEnvelopedData_Encode_BeforeStart(cinfo->content.envelopedData);
	    break;
	case SEC_OID_PKCS7_DIGESTED_DATA:
	    rv = NSS_CMSDigestedData_Encode_BeforeStart(cinfo->content.digestedData);
	    break;
	case SEC_OID_PKCS7_ENCRYPTED_DATA:
	    rv = NSS_CMSEncryptedData_Encode_BeforeStart(cinfo->content.encryptedData);
	    break;
	default:
	    rv = NSS_CMSGenericWrapperData_Encode_BeforeStart(childp7ecx->type, cinfo->content.genericData);
	    break;
	}
	if (rv != SECSuccess)
	    goto loser;

	


	childp7ecx->ecx = SEC_ASN1EncoderStart(cinfo->content.pointer, template,
					   nss_cms_encoder_out, &(childp7ecx->output));
	if (childp7ecx->ecx == NULL)
	    goto loser;

	



        if (!cinfo->privateInfo || !cinfo->privateInfo->dontStream)
	    SEC_ASN1EncoderSetStreaming(childp7ecx->ecx);

	


	p7ecx->childp7ecx = childp7ecx;
	SEC_ASN1EncoderSetNotifyProc(childp7ecx->ecx, nss_cms_encoder_notify, childp7ecx);

	
	
	
	

    } else if (NSS_CMSType_IsData(childtype)) {
	p7ecx->childp7ecx = NULL;
    } else {
	
	p7ecx->error = SEC_ERROR_BAD_DER;
    }

    return SECSuccess;

loser:
    if (childp7ecx) {
	if (childp7ecx->ecx)
	    SEC_ASN1EncoderFinish(childp7ecx->ecx);
	PORT_Free(childp7ecx);
	p7ecx->childp7ecx = NULL;
    }
    return SECFailure;
}

static SECStatus
nss_cms_after_data(NSSCMSEncoderContext *p7ecx)
{
    SECStatus rv = SECFailure;

    switch (p7ecx->type) {
    case SEC_OID_PKCS7_SIGNED_DATA:
	
	rv = NSS_CMSSignedData_Encode_AfterData(p7ecx->content.signedData);
	break;
    case SEC_OID_PKCS7_ENVELOPED_DATA:
	rv = NSS_CMSEnvelopedData_Encode_AfterData(p7ecx->content.envelopedData);
	break;
    case SEC_OID_PKCS7_DIGESTED_DATA:
	rv = NSS_CMSDigestedData_Encode_AfterData(p7ecx->content.digestedData);
	break;
    case SEC_OID_PKCS7_ENCRYPTED_DATA:
	rv = NSS_CMSEncryptedData_Encode_AfterData(p7ecx->content.encryptedData);
	break;
    default:
        if (NSS_CMSType_IsWrapper(p7ecx->type)) {
	    rv = NSS_CMSGenericWrapperData_Encode_AfterData(p7ecx->type, p7ecx->content.genericData);
	} else {
	    rv = SECFailure;
	}
	break;
    }
    return rv;
}







static SECStatus
nss_cms_encoder_work_data(NSSCMSEncoderContext *p7ecx, SECItem *dest,
			     const unsigned char *data, unsigned long len,
			     PRBool final, PRBool innermost)
{
    unsigned char *buf = NULL;
    SECStatus rv;
    NSSCMSContentInfo *cinfo;

    rv = SECSuccess;		

    






    PORT_Assert ((data != NULL && len) || final);

    
    cinfo = NSS_CMSContent_GetContentInfo(p7ecx->content.pointer, p7ecx->type);
    if (!cinfo) {
	
	p7ecx->error = SEC_ERROR_LIBRARY_FAILURE;
	return SECFailure;
    }

    
    if (len && cinfo->privateInfo && cinfo->privateInfo->digcx != NULL)
	NSS_CMSDigestContext_Update(cinfo->privateInfo->digcx, data, len);

    
    if (cinfo->privateInfo && cinfo->privateInfo->ciphcx != NULL) {
	unsigned int inlen;	
	unsigned int outlen;	
	unsigned int buflen;	

	inlen = len;
	buflen = NSS_CMSCipherContext_EncryptLength(cinfo->privateInfo->ciphcx, inlen, final);
	if (buflen == 0) {
	    



	    rv = NSS_CMSCipherContext_Encrypt(cinfo->privateInfo->ciphcx, NULL, NULL, 0,
				   data, inlen, final);
	    if (final) {
		len = 0;
		goto done;
	    }
	    return rv;
	}

	if (dest != NULL)
	    buf = (unsigned char*)PORT_ArenaAlloc(p7ecx->cmsg->poolp, buflen);
	else
	    buf = (unsigned char*)PORT_Alloc(buflen);

	if (buf == NULL) {
	    rv = SECFailure;
	} else {
	    rv = NSS_CMSCipherContext_Encrypt(cinfo->privateInfo->ciphcx, buf, &outlen, buflen,
				   data, inlen, final);
	    data = buf;
	    len = outlen;
	}
	if (rv != SECSuccess)
	    
	    return rv;
    }


    




    if (p7ecx->ecx != NULL && len && (!innermost || cinfo->rawContent != cinfo->content.pointer))
	rv = SEC_ASN1EncoderUpdate(p7ecx->ecx, (const char *)data, len);

done:

    if (cinfo->privateInfo && cinfo->privateInfo->ciphcx != NULL) {
	if (dest != NULL) {
	    dest->data = buf;
	    dest->len = len;
	} else if (buf != NULL) {
	    PORT_Free (buf);
	}
    }
    return rv;
}






static SECStatus
nss_cms_encoder_update(NSSCMSEncoderContext *p7ecx, const char *data, unsigned long len)
{
    
    return nss_cms_encoder_work_data (p7ecx, NULL, (const unsigned char *)data, len, PR_FALSE, PR_FALSE);
}













NSSCMSEncoderContext *
NSS_CMSEncoder_Start(NSSCMSMessage *cmsg,
			NSSCMSContentCallback outputfn, void *outputarg,
			SECItem *dest, PLArenaPool *destpoolp,
			PK11PasswordFunc pwfn, void *pwfn_arg,
			NSSCMSGetDecryptKeyCallback decrypt_key_cb, void *decrypt_key_cb_arg,
			SECAlgorithmID **detached_digestalgs, SECItem **detached_digests)
{
    NSSCMSEncoderContext *p7ecx;
    SECStatus rv;
    NSSCMSContentInfo *cinfo;
    SECOidTag tag;

    NSS_CMSMessage_SetEncodingParams(cmsg, pwfn, pwfn_arg, decrypt_key_cb, decrypt_key_cb_arg,
					detached_digestalgs, detached_digests);

    p7ecx = (NSSCMSEncoderContext *)PORT_ZAlloc(sizeof(NSSCMSEncoderContext));
    if (p7ecx == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    p7ecx->cmsg = cmsg;
    p7ecx->output.outputfn = outputfn;
    p7ecx->output.outputarg = outputarg;
    p7ecx->output.dest = dest;
    p7ecx->output.destpoolp = destpoolp;
    p7ecx->type = SEC_OID_UNKNOWN;

    cinfo = NSS_CMSMessage_GetContentInfo(cmsg);

    tag = NSS_CMSContentInfo_GetContentTypeTag(cinfo);
    switch (tag) {
    case SEC_OID_PKCS7_SIGNED_DATA:
	rv = NSS_CMSSignedData_Encode_BeforeStart(cinfo->content.signedData);
	break;
    case SEC_OID_PKCS7_ENVELOPED_DATA:
	rv = NSS_CMSEnvelopedData_Encode_BeforeStart(cinfo->content.envelopedData);
	break;
    case SEC_OID_PKCS7_DIGESTED_DATA:
	rv = NSS_CMSDigestedData_Encode_BeforeStart(cinfo->content.digestedData);
	break;
    case SEC_OID_PKCS7_ENCRYPTED_DATA:
	rv = NSS_CMSEncryptedData_Encode_BeforeStart(cinfo->content.encryptedData);
	break;
    default:
        if (NSS_CMSType_IsWrapper(tag)) {
	    rv = NSS_CMSGenericWrapperData_Encode_BeforeStart(tag, 
						p7ecx->content.genericData);
	} else {
	    rv = SECFailure;
	}
	break;
    }
    if (rv != SECSuccess) {
	PORT_Free(p7ecx);
	return NULL;
    }

    

    p7ecx->ecx = SEC_ASN1EncoderStart(cmsg, NSSCMSMessageTemplate,
				       nss_cms_encoder_out, &(p7ecx->output));
    if (p7ecx->ecx == NULL) {
	PORT_Free (p7ecx);
	return NULL;
    }
    p7ecx->ecxupdated = PR_FALSE;

    



    if (!cinfo->privateInfo || !cinfo->privateInfo->dontStream)
	SEC_ASN1EncoderSetStreaming(p7ecx->ecx);

    


    SEC_ASN1EncoderSetNotifyProc(p7ecx->ecx, nss_cms_encoder_notify, p7ecx);

    


    p7ecx->ecxupdated = PR_TRUE;
    if (SEC_ASN1EncoderUpdate(p7ecx->ecx, NULL, 0) != SECSuccess) {
	PORT_Free (p7ecx);
	return NULL;
    }

    return p7ecx;
}











SECStatus
NSS_CMSEncoder_Update(NSSCMSEncoderContext *p7ecx, const char *data, unsigned long len)
{
    SECStatus rv;
    NSSCMSContentInfo *cinfo;
    SECOidTag childtype;

    if (p7ecx->error)
	return SECFailure;

    
    if (p7ecx->childp7ecx) {
	

	if (!p7ecx->childp7ecx->ecxupdated) {
	    p7ecx->childp7ecx->ecxupdated = PR_TRUE;
	    if (SEC_ASN1EncoderUpdate(p7ecx->childp7ecx->ecx, NULL, 0) != SECSuccess)
	        return SECFailure;
	}
	
	rv = NSS_CMSEncoder_Update(p7ecx->childp7ecx, data, len);
    } else {
	
	
	cinfo = NSS_CMSContent_GetContentInfo(p7ecx->content.pointer, p7ecx->type);
	if (!cinfo) {
	    
	    p7ecx->error = SEC_ERROR_LIBRARY_FAILURE;
	    return SECFailure;
	}

	childtype = NSS_CMSContentInfo_GetContentTypeTag(cinfo);
	if (!NSS_CMSType_IsData(childtype))
	    return SECFailure;
	
	if (cinfo->content.data != NULL)
	    return SECFailure;

	
	rv = nss_cms_encoder_work_data(p7ecx, NULL, (const unsigned char *)data, len, PR_FALSE, PR_TRUE);
    }
    return rv;
}






SECStatus
NSS_CMSEncoder_Cancel(NSSCMSEncoderContext *p7ecx)
{
    SECStatus rv = SECFailure;

    

    





    if (p7ecx->childp7ecx) {
	rv = NSS_CMSEncoder_Cancel(p7ecx->childp7ecx); 
	
    }

    




    rv = nss_cms_encoder_work_data(p7ecx, NULL, NULL, 0, PR_TRUE, (p7ecx->childp7ecx == NULL));
    if (rv != SECSuccess)
	goto loser;

    p7ecx->childp7ecx = NULL;

    



    SEC_ASN1EncoderClearTakeFromBuf(p7ecx->ecx);
    SEC_ASN1EncoderClearStreaming(p7ecx->ecx);

    
    rv = SEC_ASN1EncoderUpdate(p7ecx->ecx, NULL, 0);

loser:
    SEC_ASN1EncoderFinish(p7ecx->ecx);
    PORT_Free (p7ecx);
    return rv;
}






SECStatus
NSS_CMSEncoder_Finish(NSSCMSEncoderContext *p7ecx)
{
    SECStatus rv = SECFailure;
    NSSCMSContentInfo *cinfo;
    SECOidTag childtype;

    





    if (p7ecx->childp7ecx) {
	

	if (!p7ecx->childp7ecx->ecxupdated) {
	    p7ecx->childp7ecx->ecxupdated = PR_TRUE;
	    rv = SEC_ASN1EncoderUpdate(p7ecx->childp7ecx->ecx, NULL, 0);
	    if (rv != SECSuccess) {
		NSS_CMSEncoder_Finish(p7ecx->childp7ecx); 
		goto loser;
	    }
	}
	rv = NSS_CMSEncoder_Finish(p7ecx->childp7ecx); 
	if (rv != SECSuccess)
	    goto loser;
    }

    




    rv = nss_cms_encoder_work_data(p7ecx, NULL, NULL, 0, PR_TRUE, (p7ecx->childp7ecx == NULL));
    if (rv != SECSuccess)
	goto loser;

    p7ecx->childp7ecx = NULL;

    cinfo = NSS_CMSContent_GetContentInfo(p7ecx->content.pointer, p7ecx->type);
    if (!cinfo) {
	
	p7ecx->error = SEC_ERROR_LIBRARY_FAILURE;
	rv = SECFailure;
	goto loser;
    }
    SEC_ASN1EncoderClearTakeFromBuf(p7ecx->ecx);
    SEC_ASN1EncoderClearStreaming(p7ecx->ecx);
    
    rv = SEC_ASN1EncoderUpdate(p7ecx->ecx, NULL, 0);

    if (p7ecx->error)
	rv = SECFailure;

loser:
    SEC_ASN1EncoderFinish(p7ecx->ecx);
    PORT_Free (p7ecx);
    return rv;
}
