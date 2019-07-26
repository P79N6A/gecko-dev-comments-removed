


#include "cryptohi.h"
#include "secasn1.h"
#include "secitem.h"
#include "prerr.h"

#ifndef DSA1_SUBPRIME_LEN
#define DSA1_SUBPRIME_LEN 20	/* bytes */
#endif

typedef struct {
    SECItem r;
    SECItem s;
} DSA_ASN1Signature;

const SEC_ASN1Template DSA_SignatureTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(DSA_ASN1Signature) },
    { SEC_ASN1_INTEGER, offsetof(DSA_ASN1Signature,r) },
    { SEC_ASN1_INTEGER, offsetof(DSA_ASN1Signature,s) },
    { 0, }
};










void
DSAU_ConvertUnsignedToSigned(SECItem *dest, SECItem *src)
{
    unsigned char *pSrc = src->data;
    unsigned char *pDst = dest->data;
    unsigned int   cntSrc = src->len;

    
    while (cntSrc && !(*pSrc)) { 
    	pSrc++; 
	cntSrc--;
    }
    if (!cntSrc) {
    	*pDst = 0; 
	dest->len = 1; 
	return; 
    }

    if (*pSrc & 0x80)
    	*pDst++ = 0;

    PORT_Memcpy(pDst, pSrc, cntSrc);
    dest->len = (pDst - dest->data) + cntSrc;
}










SECStatus
DSAU_ConvertSignedToFixedUnsigned(SECItem *dest, SECItem *src)
{
    unsigned char *pSrc = src->data;
    unsigned char *pDst = dest->data;
    unsigned int   cntSrc = src->len;
    unsigned int   cntDst = dest->len;
    int            zCount = cntDst - cntSrc;

    if (zCount > 0) {
    	PORT_Memset(pDst, 0, zCount);
	PORT_Memcpy(pDst + zCount, pSrc, cntSrc);
	return SECSuccess;
    }
    if (zCount <= 0) {
	
	while (zCount++ < 0) {
	    if (*pSrc++ != 0)
		goto loser;
	}
    }
    PORT_Memcpy(pDst, pSrc, cntDst);
    return SECSuccess;

loser:
    PORT_SetError( PR_INVALID_ARGUMENT_ERROR );
    return SECFailure;
}




static SECStatus
common_EncodeDerSig(SECItem *dest, SECItem *src)
{
    SECItem *         item;
    SECItem           srcItem;
    DSA_ASN1Signature sig;
    unsigned char     *signedR;
    unsigned char     *signedS;
    unsigned int len;

    



    len = src->len/2;
    signedR = (unsigned char *) PORT_Alloc(len + 1);
    if (!signedR) return SECFailure;
    signedS = (unsigned char *) PORT_ZAlloc(len + 1);
    if (!signedS) {
        if (signedR) PORT_Free(signedR);
	return SECFailure;
    }

    PORT_Memset(&sig, 0, sizeof(sig));

    




    sig.r.type = siUnsignedInteger;
    sig.r.data = signedR;
    sig.r.len  = sizeof signedR;
    sig.s.type = siUnsignedInteger;
    sig.s.data = signedS;
    sig.s.len  = sizeof signedR;

    srcItem.data = src->data;
    srcItem.len  = len;

    DSAU_ConvertUnsignedToSigned(&sig.r, &srcItem);
    srcItem.data += len;
    DSAU_ConvertUnsignedToSigned(&sig.s, &srcItem);

    item = SEC_ASN1EncodeItem(NULL, dest, &sig, DSA_SignatureTemplate);
    if (signedR) PORT_Free(signedR);
    if (signedS) PORT_Free(signedS);
    if (item == NULL)
	return SECFailure;

    
    return SECSuccess;
}







static SECItem *
common_DecodeDerSig(const SECItem *item, unsigned int len)
{
    SECItem *         result = NULL;
    SECStatus         status;
    DSA_ASN1Signature sig;
    SECItem           dst;

    PORT_Memset(&sig, 0, sizeof(sig));

    result = PORT_ZNew(SECItem);
    if (result == NULL)
	goto loser;

    result->len  = 2 * len;
    result->data = (unsigned char*)PORT_Alloc(2 * len);
    if (result->data == NULL)
	goto loser;

    sig.r.type = siUnsignedInteger;
    sig.s.type = siUnsignedInteger;
    status = SEC_ASN1DecodeItem(NULL, &sig, DSA_SignatureTemplate, item);
    if (status != SECSuccess)
	goto loser;

    


    dst.data = result->data;
    dst.len  = len;
    status = DSAU_ConvertSignedToFixedUnsigned(&dst, &sig.r);
    if (status != SECSuccess)
    	goto loser;

    dst.data += len;
    status = DSAU_ConvertSignedToFixedUnsigned(&dst, &sig.s);
    if (status != SECSuccess)
    	goto loser;

done:
    if (sig.r.data != NULL)
	PORT_Free(sig.r.data);
    if (sig.s.data != NULL)
	PORT_Free(sig.s.data);

    return result;

loser:
    if (result != NULL) {
	SECITEM_FreeItem(result, PR_TRUE);
	result = NULL;
    }
    goto done;
}




SECStatus
DSAU_EncodeDerSig(SECItem *dest, SECItem *src)
{
    PORT_Assert(src->len == 2 * DSA1_SUBPRIME_LEN);
    if (src->len != 2 * DSA1_SUBPRIME_LEN) {
    	PORT_SetError( PR_INVALID_ARGUMENT_ERROR );
	return SECFailure;
    }

    return common_EncodeDerSig(dest, src);
}




SECStatus
DSAU_EncodeDerSigWithLen(SECItem *dest, SECItem *src, unsigned int len)
{

    PORT_Assert((src->len == len) && (len % 2 == 0));
    if ((src->len != len) || (src->len % 2 != 0)) {
    	PORT_SetError( PR_INVALID_ARGUMENT_ERROR );
	return SECFailure;
    }

    return common_EncodeDerSig(dest, src);
}






SECItem *
DSAU_DecodeDerSig(const SECItem *item)
{
    return common_DecodeDerSig(item, DSA1_SUBPRIME_LEN);
}






SECItem *
DSAU_DecodeDerSigToLen(const SECItem *item, unsigned int len)
{
    return common_DecodeDerSig(item, len/2);
}
