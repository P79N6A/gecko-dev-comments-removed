






#include "pkcs11i.h"
#include "blapi.h"

#define SFTK_OFFSETOF(str, memb) ((PRPtrdiff)(&(((str *)0)->memb)))

static void sftk_TLSPRFNull(void *data, PRBool freeit)
{
    return;
} 

typedef struct {
    PRUint32	   cxSize;	
    PRUint32       cxBufSize;   
    unsigned char *cxBufPtr;	
    PRUint32	   cxKeyLen;	
    PRUint32	   cxDataLen;	
    SECStatus	   cxRv;	
    PRBool	   cxIsFIPS;	
    unsigned char  cxBuf[512];	
} TLSPRFContext;

static void
sftk_TLSPRFHashUpdate(TLSPRFContext *cx, const unsigned char *data, 
                        unsigned int data_len)
{
    PRUint32 bytesUsed = cx->cxKeyLen + cx->cxDataLen;

    if (cx->cxRv != SECSuccess)	
    	return;
    if (bytesUsed + data_len > cx->cxBufSize) {
	



	PRUint32 newBufSize = bytesUsed + data_len + 512;
    	unsigned char * newBuf = (unsigned char *)PORT_Alloc(newBufSize);
	if (!newBuf) {
	   cx->cxRv = SECFailure;
	   return;
	}
	PORT_Memcpy(newBuf, cx->cxBufPtr, bytesUsed);
	if (cx->cxBufPtr != cx->cxBuf) {
	    PORT_ZFree(cx->cxBufPtr, bytesUsed);
	}
	cx->cxBufPtr  = newBuf;
	cx->cxBufSize = newBufSize;
    }
    PORT_Memcpy(cx->cxBufPtr + bytesUsed, data, data_len);
    cx->cxDataLen += data_len;
}

static void 
sftk_TLSPRFEnd(TLSPRFContext *ctx, unsigned char *hashout,
	 unsigned int *pDigestLen, unsigned int maxDigestLen)
{
    *pDigestLen = 0; 
}


static SECStatus
sftk_TLSPRFUpdate(TLSPRFContext *cx, 
                  unsigned char *sig,		
		  unsigned int * sigLen, 	
		  unsigned int   maxLen, 	
		  unsigned char *hash, 		
		  unsigned int   hashLen)	
{
    SECStatus rv;
    SECItem sigItem;
    SECItem seedItem;
    SECItem secretItem;

    if (cx->cxRv != SECSuccess)
    	return cx->cxRv;

    secretItem.data = cx->cxBufPtr;
    secretItem.len  = cx->cxKeyLen;

    seedItem.data = cx->cxBufPtr + cx->cxKeyLen;
    seedItem.len  = cx->cxDataLen;

    sigItem.data = sig;
    sigItem.len  = maxLen;

    rv = TLS_PRF(&secretItem, NULL, &seedItem, &sigItem, cx->cxIsFIPS);
    if (rv == SECSuccess && sigLen != NULL)
    	*sigLen = sigItem.len;
    return rv;

}

static SECStatus
sftk_TLSPRFVerify(TLSPRFContext *cx, 
                  unsigned char *sig, 		
		  unsigned int   sigLen,	
		  unsigned char *hash, 		
		  unsigned int   hashLen)	
{
    unsigned char * tmp    = (unsigned char *)PORT_Alloc(sigLen);
    unsigned int    tmpLen = sigLen;
    SECStatus       rv;

    if (!tmp)
    	return SECFailure;
    if (hashLen) {
    	


    	sftk_TLSPRFHashUpdate(cx, hash, hashLen);
    }
    rv = sftk_TLSPRFUpdate(cx, tmp, &tmpLen, sigLen, NULL, 0);
    if (rv == SECSuccess) {
    	rv = (SECStatus)(1 - !PORT_Memcmp(tmp, sig, sigLen));
    }
    PORT_ZFree(tmp, sigLen);
    return rv;
}

static void
sftk_TLSPRFHashDestroy(TLSPRFContext *cx, PRBool freeit)
{
    if (freeit) {
	if (cx->cxBufPtr != cx->cxBuf) 
	    PORT_ZFree(cx->cxBufPtr, cx->cxBufSize);
	PORT_ZFree(cx, cx->cxSize);
    }
}

CK_RV
sftk_TLSPRFInit(SFTKSessionContext *context, 
		  SFTKObject *        key, 
		  CK_KEY_TYPE         key_type)
{
    SFTKAttribute * keyVal;
    TLSPRFContext * prf_cx;
    CK_RV           crv = CKR_HOST_MEMORY;
    PRUint32        keySize;
    PRUint32        blockSize;

    if (key_type != CKK_GENERIC_SECRET)
    	return CKR_KEY_TYPE_INCONSISTENT; 

    context->multi = PR_TRUE;

    keyVal = sftk_FindAttribute(key, CKA_VALUE);
    keySize = (!keyVal) ? 0 : keyVal->attrib.ulValueLen;
    blockSize = keySize + sizeof(TLSPRFContext);
    prf_cx = (TLSPRFContext *)PORT_Alloc(blockSize);
    if (!prf_cx) 
    	goto done;
    prf_cx->cxSize    = blockSize;
    prf_cx->cxKeyLen  = keySize;
    prf_cx->cxDataLen = 0;
    prf_cx->cxBufSize = blockSize - SFTK_OFFSETOF(TLSPRFContext, cxBuf);
    prf_cx->cxRv      = SECSuccess;
    prf_cx->cxIsFIPS  = (key->slot->slotID == FIPS_SLOT_ID);
    prf_cx->cxBufPtr  = prf_cx->cxBuf;
    if (keySize)
	PORT_Memcpy(prf_cx->cxBufPtr, keyVal->attrib.pValue, keySize);

    context->hashInfo    = (void *) prf_cx;
    context->cipherInfo  = (void *) prf_cx;
    context->hashUpdate  = (SFTKHash)    sftk_TLSPRFHashUpdate;
    context->end         = (SFTKEnd)     sftk_TLSPRFEnd;
    context->update      = (SFTKCipher)  sftk_TLSPRFUpdate;
    context->verify      = (SFTKVerify)  sftk_TLSPRFVerify;
    context->destroy     = (SFTKDestroy) sftk_TLSPRFNull;
    context->hashdestroy = (SFTKDestroy) sftk_TLSPRFHashDestroy;
    crv = CKR_OK;

done:
    if (keyVal) 
	sftk_FreeAttribute(keyVal);
    return crv;
}

