









































#include "cmslocal.h"

#include "cert.h"
#include "key.h"
#include "secasn1.h"
#include "secitem.h"
#include "secoid.h"
#include "prtime.h"
#include "secerr.h"

struct NSSCMSDecoderContextStr {
    SEC_ASN1DecoderContext *	dcx;		
    NSSCMSMessage *		cmsg;		
    SECOidTag			type;		
    NSSCMSContent		content;	
    NSSCMSDecoderContext *	childp7dcx;	
    PRBool			saw_contents;
    int				error;
    NSSCMSContentCallback	cb;
    void *			cb_arg;
};

struct NSSCMSDecoderDataStr {
    SECItem data; 	
    unsigned int totalBufferSize;
};

typedef struct NSSCMSDecoderDataStr NSSCMSDecoderData;

static void      nss_cms_decoder_update_filter (void *arg, const char *data, 
                 unsigned long len, int depth, SEC_ASN1EncodingPart data_kind);
static SECStatus nss_cms_before_data(NSSCMSDecoderContext *p7dcx);
static SECStatus nss_cms_after_data(NSSCMSDecoderContext *p7dcx);
static SECStatus nss_cms_after_end(NSSCMSDecoderContext *p7dcx);
static void      nss_cms_decoder_work_data(NSSCMSDecoderContext *p7dcx, 
		 const unsigned char *data, unsigned long len, PRBool final);
static NSSCMSDecoderData *nss_cms_create_decoder_data(PRArenaPool *poolp);

extern const SEC_ASN1Template NSSCMSMessageTemplate[];

static NSSCMSDecoderData *
nss_cms_create_decoder_data(PRArenaPool *poolp)
{
    NSSCMSDecoderData *decoderData = NULL;

    decoderData = (NSSCMSDecoderData *)
			PORT_ArenaAlloc(poolp,sizeof(NSSCMSDecoderData));
    if (!decoderData) {
	return NULL;
    }
    decoderData->data.data = NULL;
    decoderData->data.len = 0;
    decoderData->totalBufferSize = 0;
    return decoderData;
}








static void
nss_cms_decoder_notify(void *arg, PRBool before, void *dest, int depth)
{
    NSSCMSDecoderContext *p7dcx;
    NSSCMSContentInfo *rootcinfo, *cinfo;
    PRBool after = !before;

    p7dcx = (NSSCMSDecoderContext *)arg;
    rootcinfo = &(p7dcx->cmsg->contentInfo);

    

#ifdef CMSDEBUG 
    fprintf(stderr, "%6.6s, dest = 0x%08x, depth = %d\n", before ? "before" : "after", dest, depth);
#endif

    
    if (p7dcx->type == SEC_OID_UNKNOWN) {
	







	if (after && dest == &(rootcinfo->contentType)) {
	    p7dcx->type = NSS_CMSContentInfo_GetContentTypeTag(rootcinfo);
	    p7dcx->content = rootcinfo->content;	
	    
	    
	}
    } else if (NSS_CMSType_IsData(p7dcx->type)) {
	
	

	if (before && dest == &(rootcinfo->content)) {
	    



	    SEC_ASN1DecoderSetFilterProc(p7dcx->dcx,
					  nss_cms_decoder_update_filter,
					  p7dcx,
					  (PRBool)(p7dcx->cb != NULL));
	} else if (after && dest == &(rootcinfo->content.data)) {
	    
	    SEC_ASN1DecoderClearFilterProc(p7dcx->dcx);
	}
    } else if (NSS_CMSType_IsWrapper(p7dcx->type)) {
	if (!before || dest != &(rootcinfo->content)) {

	    if (p7dcx->content.pointer == NULL)
		p7dcx->content = rootcinfo->content;

	    
	    cinfo = NSS_CMSContent_GetContentInfo(p7dcx->content.pointer, 
	                                      p7dcx->type);

	    if (before && dest == &(cinfo->contentType)) {
	        
	        

		switch (p7dcx->type) {
		case SEC_OID_PKCS7_SIGNED_DATA:
		    p7dcx->content.signedData->cmsg = p7dcx->cmsg;
		    break;
		case SEC_OID_PKCS7_DIGESTED_DATA:
		    p7dcx->content.digestedData->cmsg = p7dcx->cmsg;
		    break;
		case SEC_OID_PKCS7_ENVELOPED_DATA:
		    p7dcx->content.envelopedData->cmsg = p7dcx->cmsg;
		    break;
		case SEC_OID_PKCS7_ENCRYPTED_DATA:
		    p7dcx->content.encryptedData->cmsg = p7dcx->cmsg;
		    break;
		default:
		    p7dcx->content.genericData->cmsg = p7dcx->cmsg;
		    break;
		}
	    }

	    if (before && dest == &(cinfo->rawContent)) {
		


		SEC_ASN1DecoderSetFilterProc(p7dcx->dcx, 
	                                 nss_cms_decoder_update_filter, 
					 p7dcx, (PRBool)(p7dcx->cb != NULL));


		
		if (nss_cms_before_data(p7dcx) != SECSuccess) {
		    SEC_ASN1DecoderClearFilterProc(p7dcx->dcx);	
		    
		    p7dcx->error = PORT_GetError();
		}
	    }
	    if (after && dest == &(cinfo->rawContent)) {
		
		if (nss_cms_after_data(p7dcx) != SECSuccess)
		    p7dcx->error = PORT_GetError();

		
		SEC_ASN1DecoderClearFilterProc(p7dcx->dcx);
	    }
	}
    } else {
	
	p7dcx->error = SEC_ERROR_UNSUPPORTED_MESSAGE_TYPE;
    }
}




static SECStatus
nss_cms_before_data(NSSCMSDecoderContext *p7dcx)
{
    SECStatus rv;
    SECOidTag childtype;
    PLArenaPool *poolp;
    NSSCMSDecoderContext *childp7dcx;
    NSSCMSContentInfo *cinfo;
    const SEC_ASN1Template *template;
    void *mark = NULL;
    size_t size;
    
    poolp = p7dcx->cmsg->poolp;

    
    switch (p7dcx->type) {
    case SEC_OID_PKCS7_SIGNED_DATA:
	
	rv = NSS_CMSSignedData_Decode_BeforeData(p7dcx->content.signedData);
	break;
    case SEC_OID_PKCS7_DIGESTED_DATA:
	
	rv = NSS_CMSDigestedData_Decode_BeforeData(p7dcx->content.digestedData);
	break;
    case SEC_OID_PKCS7_ENVELOPED_DATA:
	rv = NSS_CMSEnvelopedData_Decode_BeforeData(
	                             p7dcx->content.envelopedData);
	break;
    case SEC_OID_PKCS7_ENCRYPTED_DATA:
	rv = NSS_CMSEncryptedData_Decode_BeforeData(
	                             p7dcx->content.encryptedData);
	break;
    default:
	rv = NSS_CMSGenericWrapperData_Decode_BeforeData(p7dcx->type,
				p7dcx->content.genericData);
    }
    if (rv != SECSuccess)
	return SECFailure;

    
    
    
    cinfo = NSS_CMSContent_GetContentInfo(p7dcx->content.pointer, p7dcx->type);
    childtype = NSS_CMSContentInfo_GetContentTypeTag(cinfo);

    if (NSS_CMSType_IsData(childtype)) {
	cinfo->content.pointer = (void *) nss_cms_create_decoder_data(poolp);
	if (cinfo->content.pointer == NULL)
	    
	    return SECFailure;

	p7dcx->childp7dcx = NULL;
	return SECSuccess;
    }

    

    if ((template = NSS_CMSUtil_GetTemplateByTypeTag(childtype)) == NULL)
	return SECFailure;

    childp7dcx = PORT_ZNew(NSSCMSDecoderContext);
    if (childp7dcx == NULL)
	return SECFailure;

    mark = PORT_ArenaMark(poolp);

    
    size = NSS_CMSUtil_GetSizeByTypeTag(childtype);
    childp7dcx->content.pointer = (void *)PORT_ArenaZAlloc(poolp, size);
    if (childp7dcx->content.pointer == NULL)
	goto loser;

    
    cinfo->content.pointer = childp7dcx->content.pointer;

    
    childp7dcx->dcx = SEC_ASN1DecoderStart(poolp, childp7dcx->content.pointer, 
                                           template);
    if (childp7dcx->dcx == NULL)
	goto loser;

    
    SEC_ASN1DecoderSetNotifyProc(childp7dcx->dcx, nss_cms_decoder_notify, 
                                 childp7dcx);

    
    p7dcx->childp7dcx = childp7dcx;

    childp7dcx->type = childtype;	

    childp7dcx->cmsg = p7dcx->cmsg;	

    


    childp7dcx->cb = p7dcx->cb;
    childp7dcx->cb_arg = p7dcx->cb_arg;

    
    p7dcx->cb = (NSSCMSContentCallback)NSS_CMSDecoder_Update;
    p7dcx->cb_arg = childp7dcx;

    PORT_ArenaUnmark(poolp, mark);

    return SECSuccess;

loser:
    if (mark)
	PORT_ArenaRelease(poolp, mark);
    if (childp7dcx)
	PORT_Free(childp7dcx);
    p7dcx->childp7dcx = NULL;
    return SECFailure;
}

static SECStatus
nss_cms_after_data(NSSCMSDecoderContext *p7dcx)
{
    NSSCMSDecoderContext *childp7dcx;
    SECStatus rv = SECFailure;

    

    nss_cms_decoder_work_data(p7dcx, NULL, 0, PR_TRUE);

    
    if (p7dcx->childp7dcx != NULL) {
	childp7dcx = p7dcx->childp7dcx;
	if (childp7dcx->dcx != NULL) {
	    if (SEC_ASN1DecoderFinish(childp7dcx->dcx) != SECSuccess) {
		
		rv = SECFailure;
	    } else {
		rv = nss_cms_after_end(childp7dcx);
	    }
	    if (rv != SECSuccess)
		goto done;
	}
	PORT_Free(p7dcx->childp7dcx);
	p7dcx->childp7dcx = NULL;
    }

    switch (p7dcx->type) {
    case SEC_OID_PKCS7_SIGNED_DATA:
	
	rv = NSS_CMSSignedData_Decode_AfterData(p7dcx->content.signedData);
	break;
    case SEC_OID_PKCS7_ENVELOPED_DATA:
	rv = NSS_CMSEnvelopedData_Decode_AfterData(
	                            p7dcx->content.envelopedData);
	break;
    case SEC_OID_PKCS7_DIGESTED_DATA:
	rv = NSS_CMSDigestedData_Decode_AfterData(
	                           p7dcx->content.digestedData);
	break;
    case SEC_OID_PKCS7_ENCRYPTED_DATA:
	rv = NSS_CMSEncryptedData_Decode_AfterData(
	                            p7dcx->content.encryptedData);
	break;
    case SEC_OID_PKCS7_DATA:
	
	break;
    default:
	rv = NSS_CMSGenericWrapperData_Decode_AfterData(p7dcx->type,
	                            p7dcx->content.genericData);
	break;
    }
done:
    return rv;
}

static SECStatus
nss_cms_after_end(NSSCMSDecoderContext *p7dcx)
{
    SECStatus rv = SECSuccess;

    switch (p7dcx->type) {
    case SEC_OID_PKCS7_SIGNED_DATA:
	if (p7dcx->content.signedData)
	    rv = NSS_CMSSignedData_Decode_AfterEnd(p7dcx->content.signedData);
	break;
    case SEC_OID_PKCS7_ENVELOPED_DATA:
	if (p7dcx->content.envelopedData)
	    rv = NSS_CMSEnvelopedData_Decode_AfterEnd(
	                               p7dcx->content.envelopedData);
	break;
    case SEC_OID_PKCS7_DIGESTED_DATA:
	if (p7dcx->content.digestedData)
	    rv = NSS_CMSDigestedData_Decode_AfterEnd(
	                              p7dcx->content.digestedData);
	break;
    case SEC_OID_PKCS7_ENCRYPTED_DATA:
	if (p7dcx->content.encryptedData)
	    rv = NSS_CMSEncryptedData_Decode_AfterEnd(
	                               p7dcx->content.encryptedData);
	break;
    case SEC_OID_PKCS7_DATA:
	break;
    default:
	rv = NSS_CMSGenericWrapperData_Decode_AfterEnd(p7dcx->type,
	                               p7dcx->content.genericData);
	break;
    }
    return rv;
}







static void
nss_cms_decoder_work_data(NSSCMSDecoderContext *p7dcx, 
			     const unsigned char *data, unsigned long len,
			     PRBool final)
{
    NSSCMSContentInfo *cinfo;
    unsigned char *buf = NULL;
    unsigned char *dest;
    unsigned int offset;
    SECStatus rv;

    






    PORT_Assert ((data != NULL && len) || final);

    cinfo = NSS_CMSContent_GetContentInfo(p7dcx->content.pointer, p7dcx->type);
    if (!cinfo) {
	
	p7dcx->error = SEC_ERROR_LIBRARY_FAILURE;
	goto loser;
    }

    if (cinfo->privateInfo && cinfo->privateInfo->ciphcx != NULL) {
	








	unsigned int outlen = 0;	
	unsigned int buflen;		

	
	buflen = NSS_CMSCipherContext_DecryptLength(cinfo->privateInfo->ciphcx, len, final);

	




	
	if (buflen == 0 && len == 0)
	    goto loser;	

	



	if (buflen != 0) {
	    
	    
	    buf = (unsigned char *)PORT_Alloc(buflen);
	    if (buf == NULL) {
		p7dcx->error = SEC_ERROR_NO_MEMORY;
		goto loser;
	    }
	}

	





	rv = NSS_CMSCipherContext_Decrypt(cinfo->privateInfo->ciphcx, buf, &outlen, buflen,
			       data, len, final);
	if (rv != SECSuccess) {
	    p7dcx->error = PORT_GetError();
	    goto loser;
	}

	PORT_Assert (final || outlen == buflen);
	
	
	data = buf;
	len = outlen;
    }

    if (len == 0)
	goto done;		

    


    if (cinfo->privateInfo && cinfo->privateInfo->digcx)
	NSS_CMSDigestContext_Update(cinfo->privateInfo->digcx, data, len);

    




    
    
    if (p7dcx->cb != NULL) {
	(*p7dcx->cb)(p7dcx->cb_arg, (const char *)data, len);
    }
#if 1
    else
#endif
    if (NSS_CMSContentInfo_GetContentTypeTag(cinfo) == SEC_OID_PKCS7_DATA) {
	
	
	NSSCMSDecoderData *decoderData = 
				(NSSCMSDecoderData *)cinfo->content.pointer;
	SECItem *dataItem = &decoderData->data;

	offset = dataItem->len;
	if (dataItem->len+len > decoderData->totalBufferSize) {
	    int needLen = (dataItem->len+len) * 2;
	    dest = (unsigned char *)
				PORT_ArenaAlloc(p7dcx->cmsg->poolp, needLen);
	    if (dest == NULL) {
		p7dcx->error = SEC_ERROR_NO_MEMORY;
		goto loser;
	    }

	    if (dataItem->len) {
		PORT_Memcpy(dest, dataItem->data, dataItem->len);
	    }
	    decoderData->totalBufferSize = needLen;
	    dataItem->data = dest;
	}

	
	PORT_Memcpy(dataItem->data + offset, data, len);
	dataItem->len += len;
    }

done:
loser:
    if (buf)
	PORT_Free (buf);
}









static void
nss_cms_decoder_update_filter (void *arg, const char *data, unsigned long len,
			  int depth, SEC_ASN1EncodingPart data_kind)
{
    NSSCMSDecoderContext *p7dcx;

    PORT_Assert (len);	
    if (len == 0)
	return;

    p7dcx = (NSSCMSDecoderContext*)arg;

    p7dcx->saw_contents = PR_TRUE;

    
    if (data_kind == SEC_ASN1_Contents)
	nss_cms_decoder_work_data(p7dcx, (const unsigned char *) data, len, 
	                          PR_FALSE);
}









NSSCMSDecoderContext *
NSS_CMSDecoder_Start(PRArenaPool *poolp,
		      NSSCMSContentCallback cb, void *cb_arg,
		      PK11PasswordFunc pwfn, void *pwfn_arg,
		      NSSCMSGetDecryptKeyCallback decrypt_key_cb, 
		      void *decrypt_key_cb_arg)
{
    NSSCMSDecoderContext *p7dcx;
    NSSCMSMessage *cmsg;

    cmsg = NSS_CMSMessage_Create(poolp);
    if (cmsg == NULL)
	return NULL;

    NSS_CMSMessage_SetEncodingParams(cmsg, pwfn, pwfn_arg, decrypt_key_cb, 
                                     decrypt_key_cb_arg, NULL, NULL);

    p7dcx = PORT_ZNew(NSSCMSDecoderContext);
    if (p7dcx == NULL) {
	NSS_CMSMessage_Destroy(cmsg);
	return NULL;
    }

    p7dcx->dcx = SEC_ASN1DecoderStart(cmsg->poolp, cmsg, NSSCMSMessageTemplate);
    if (p7dcx->dcx == NULL) {
	PORT_Free (p7dcx);
	NSS_CMSMessage_Destroy(cmsg);
	return NULL;
    }

    SEC_ASN1DecoderSetNotifyProc (p7dcx->dcx, nss_cms_decoder_notify, p7dcx);

    p7dcx->cmsg = cmsg;
    p7dcx->type = SEC_OID_UNKNOWN;

    p7dcx->cb = cb;
    p7dcx->cb_arg = cb_arg;

    return p7dcx;
}




SECStatus
NSS_CMSDecoder_Update(NSSCMSDecoderContext *p7dcx, const char *buf, 
                      unsigned long len)
{
    SECStatus rv;
    if (p7dcx->dcx != NULL && p7dcx->error == 0) {	
    	
	rv = SEC_ASN1DecoderUpdate(p7dcx->dcx, buf, len);
	if (rv != SECSuccess) {
	    p7dcx->error = PORT_GetError();
	    PORT_Assert (p7dcx->error);
	    if (p7dcx->error == 0)
		p7dcx->error = -1;
	}
    }

    if (p7dcx->error == 0)
	return SECSuccess;

    
    if (p7dcx->dcx != NULL) {
	(void) SEC_ASN1DecoderFinish (p7dcx->dcx);
	p7dcx->dcx = NULL;
    }
    PORT_SetError (p7dcx->error);

    return SECFailure;
}




void
NSS_CMSDecoder_Cancel(NSSCMSDecoderContext *p7dcx)
{
    if (p7dcx->dcx != NULL)
	(void)SEC_ASN1DecoderFinish(p7dcx->dcx);
    NSS_CMSMessage_Destroy(p7dcx->cmsg);
    PORT_Free(p7dcx);
}




NSSCMSMessage *
NSS_CMSDecoder_Finish(NSSCMSDecoderContext *p7dcx)
{
    NSSCMSMessage *cmsg;

    cmsg = p7dcx->cmsg;

    if (p7dcx->dcx == NULL || 
        SEC_ASN1DecoderFinish(p7dcx->dcx) != SECSuccess ||
	nss_cms_after_end(p7dcx) != SECSuccess)
    {
	NSS_CMSMessage_Destroy(cmsg);	
	cmsg = NULL;
    }

    PORT_Free(p7dcx);
    return cmsg;
}

NSSCMSMessage *
NSS_CMSMessage_CreateFromDER(SECItem *DERmessage,
		    NSSCMSContentCallback cb, void *cb_arg,
		    PK11PasswordFunc pwfn, void *pwfn_arg,
		    NSSCMSGetDecryptKeyCallback decrypt_key_cb, 
		    void *decrypt_key_cb_arg)
{
    NSSCMSDecoderContext *p7dcx;

    
    p7dcx = NSS_CMSDecoder_Start(NULL, cb, cb_arg, pwfn, pwfn_arg, 
                                 decrypt_key_cb, decrypt_key_cb_arg);
    if (p7dcx == NULL)
	return NULL;
    NSS_CMSDecoder_Update(p7dcx, (char *)DERmessage->data, DERmessage->len);
    return NSS_CMSDecoder_Finish(p7dcx);
}

