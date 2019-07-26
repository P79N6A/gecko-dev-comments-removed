



#ifdef FREEBL_NO_DEPEND
#include "stubs.h"
#endif
#include "prtypes.h"
#include "blapit.h"
#include "blapii.h"
#include "ctr.h"
#include "pkcs11t.h"
#include "secerr.h"

SECStatus
CTR_InitContext(CTRContext *ctr, void *context, freeblCipherFunc cipher,
		const unsigned char *param, unsigned int blocksize)
{
    const CK_AES_CTR_PARAMS *ctrParams = (const CK_AES_CTR_PARAMS *)param;

    if (ctrParams->ulCounterBits == 0 ||
	ctrParams->ulCounterBits > blocksize * PR_BITS_PER_BYTE) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    
    ctr->bufPtr = blocksize; 
    ctr->cipher = cipher;
    ctr->context = context;
    ctr->counterBits = ctrParams->ulCounterBits;
    if (blocksize > sizeof(ctr->counter) ||
	blocksize > sizeof(ctrParams->cb)) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    PORT_Memcpy(ctr->counter, ctrParams->cb, blocksize);
    return SECSuccess;
}

CTRContext *
CTR_CreateContext(void *context, freeblCipherFunc cipher,
		  const unsigned char *param, unsigned int blocksize)
{
    CTRContext *ctr;
    SECStatus rv;

    
    ctr = PORT_ZNew(CTRContext);
    if (ctr == NULL) {
	return NULL;
    }
    rv = CTR_InitContext(ctr, context, cipher, param, blocksize);
    if (rv != SECSuccess) {
	CTR_DestroyContext(ctr, PR_TRUE);
	ctr = NULL;
    }
    return ctr;
}

void
CTR_DestroyContext(CTRContext *ctr, PRBool freeit)
{
    PORT_Memset(ctr, 0, sizeof(CTRContext));
    if (freeit) {
	PORT_Free(ctr);
    }
}









static void
ctr_GetNextCtr(unsigned char *counter, unsigned int counterBits,
		unsigned int blocksize)
{
    unsigned char *counterPtr = counter + blocksize - 1;
    unsigned char mask, count;

    PORT_Assert(counterBits <= blocksize*PR_BITS_PER_BYTE);
    while (counterBits >= PR_BITS_PER_BYTE) {
	if (++(*(counterPtr--))) {
	    return;
	}
	counterBits -= PR_BITS_PER_BYTE;
    }
    if (counterBits == 0) {
	return;
    }
    
    mask = (1 << counterBits)-1;
    count = ++(*counterPtr) & mask;
    *counterPtr = ((*counterPtr) & ~mask) | count;
    return;
}

static void
ctr_xor(unsigned char *target, const unsigned char *x,
	 const unsigned char *y, unsigned int count)
{
    unsigned int i;
    for (i=0; i < count; i++) {
	*target++ = *x++ ^ *y++;
    }
}

SECStatus
CTR_Update(CTRContext *ctr, unsigned char *outbuf,
		unsigned int *outlen, unsigned int maxout,
		const unsigned char *inbuf, unsigned int inlen,
		unsigned int blocksize)
{
    unsigned int tmp;
    SECStatus rv;

    if (maxout < inlen) {
	*outlen = inlen;
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	return SECFailure;
    }
    *outlen = 0;
    if (ctr->bufPtr != blocksize) {
	unsigned int needed = PR_MIN(blocksize-ctr->bufPtr, inlen);
	ctr_xor(outbuf, inbuf, ctr->buffer+ctr->bufPtr, needed);
	ctr->bufPtr += needed;
	outbuf += needed;
	inbuf += needed;
	*outlen += needed;
	inlen -= needed;
	if (inlen == 0) {
	    return SECSuccess;
	}
	PORT_Assert(ctr->bufPtr == blocksize);
    }
	
    while (inlen >= blocksize) {
	rv = (*ctr->cipher)(ctr->context, ctr->buffer, &tmp, blocksize,
			ctr->counter, blocksize, blocksize);
	ctr_GetNextCtr(ctr->counter, ctr->counterBits, blocksize);
	if (rv != SECSuccess) {
	    return SECFailure;
	}
	ctr_xor(outbuf, inbuf, ctr->buffer, blocksize);
	outbuf += blocksize;
	inbuf += blocksize;
	*outlen += blocksize;
	inlen -= blocksize;
    }
    if (inlen == 0) {
	return SECSuccess;
    }
    rv = (*ctr->cipher)(ctr->context, ctr->buffer, &tmp, blocksize,
			ctr->counter, blocksize, blocksize);
    ctr_GetNextCtr(ctr->counter, ctr->counterBits, blocksize);
    if (rv != SECSuccess) {
	return SECFailure;
    }
    ctr_xor(outbuf, inbuf, ctr->buffer, inlen);
    ctr->bufPtr = inlen;
    *outlen += inlen;
    return SECSuccess;
}
