



#ifdef FREEBL_NO_DEPEND
#include "stubs.h"
#endif
#include "blapit.h"
#include "blapii.h"
#include "cts.h"
#include "secerr.h"

struct CTSContextStr {
    freeblCipherFunc cipher;
    void *context;
    

    unsigned char iv[MAX_BLOCK_SIZE];
};

CTSContext *
CTS_CreateContext(void *context, freeblCipherFunc cipher,
		  const unsigned char *iv, unsigned int blocksize)
{
    CTSContext  *cts;

    if (blocksize > MAX_BLOCK_SIZE) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return NULL;
    }
    cts = PORT_ZNew(CTSContext);
    if (cts == NULL) {
	return NULL;
    }
    PORT_Memcpy(cts->iv, iv, blocksize);
    cts->cipher = cipher;
    cts->context = context;
    return cts;
}

void
CTS_DestroyContext(CTSContext *cts, PRBool freeit)
{
    if (freeit) {
	PORT_Free(cts);
    }
}
	









































SECStatus
CTS_EncryptUpdate(CTSContext *cts, unsigned char *outbuf,
		unsigned int *outlen, unsigned int maxout,
		const unsigned char *inbuf, unsigned int inlen,
		unsigned int blocksize)
{
    unsigned char lastBlock[MAX_BLOCK_SIZE];
    unsigned int tmp;
    int fullblocks;
    int written;
    SECStatus rv;

    if (inlen < blocksize) {
	PORT_SetError(SEC_ERROR_INPUT_LEN);
	return SECFailure;
    }

    if (maxout < inlen) {
	*outlen = inlen;
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	return SECFailure;
    }
    fullblocks = (inlen/blocksize)*blocksize;
    rv = (*cts->cipher)(cts->context, outbuf, outlen, maxout, inbuf,
	 fullblocks, blocksize);
    if (rv != SECSuccess) {
	return SECFailure;
    }
    *outlen = fullblocks; 
    inbuf += fullblocks;
    inlen -= fullblocks;
    if (inlen == 0) {
	return SECSuccess;
    }
    written = *outlen - (blocksize - inlen);
    outbuf += written;
    maxout -= written;

    








    PORT_Memcpy(lastBlock, inbuf, inlen);
    PORT_Memset(lastBlock + inlen, 0, blocksize - inlen);
    rv = (*cts->cipher)(cts->context, outbuf, &tmp, maxout, lastBlock,
			blocksize, blocksize);
    PORT_Memset(lastBlock, 0, blocksize);
    if (rv == SECSuccess) {
	*outlen = written + blocksize;
    }
    return rv;
}


#define XOR_BLOCK(x,y,count) for(i=0; i < count; i++) x[i] = x[i] ^ y[i]

























SECStatus
CTS_DecryptUpdate(CTSContext *cts, unsigned char *outbuf,
		unsigned int *outlen, unsigned int maxout,
		const unsigned char *inbuf, unsigned int inlen,
		unsigned int blocksize)
{
    unsigned char *Pn;
    unsigned char Cn_2[MAX_BLOCK_SIZE]; 
    unsigned char Cn_1[MAX_BLOCK_SIZE]; 
    unsigned char Cn[MAX_BLOCK_SIZE];   
    unsigned char lastBlock[MAX_BLOCK_SIZE];
    const unsigned char *tmp;
    unsigned int tmpLen;
    int fullblocks, pad;
    unsigned int i;
    SECStatus rv;

    if (inlen < blocksize) {
	PORT_SetError(SEC_ERROR_INPUT_LEN);
	return SECFailure;
    }

    if (maxout < inlen) {
	*outlen = inlen;
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	return SECFailure;
    }

    fullblocks = (inlen/blocksize)*blocksize;

    




    pad = inlen - fullblocks;
    if (pad != 0) {
	if (inbuf != outbuf) {
	    memcpy(outbuf, inbuf, inlen);
	    

	    inbuf = outbuf;
	}
	memcpy(lastBlock, inbuf+inlen-blocksize, blocksize);
	

	memcpy(outbuf+inlen-pad, inbuf+inlen-blocksize-pad, pad);
	memcpy(outbuf+inlen-blocksize-pad, lastBlock, blocksize);
    }
    

    tmp =  (fullblocks < blocksize*2) ? cts->iv :
			inbuf+fullblocks-blocksize*2;
    PORT_Memcpy(Cn_2, tmp, blocksize);
    PORT_Memcpy(Cn, inbuf+fullblocks-blocksize, blocksize);
    rv = (*cts->cipher)(cts->context, outbuf, outlen, maxout, inbuf,
	 fullblocks, blocksize);
    if (rv != SECSuccess) {
	return SECFailure;
    }
    *outlen = fullblocks; 
    inbuf += fullblocks;
    inlen -= fullblocks;
    if (inlen == 0) {
	return SECSuccess;
    }
    outbuf += fullblocks;

    
    PORT_Memset(lastBlock, 0, blocksize);
    PORT_Memcpy(lastBlock, inbuf, inlen);
    PORT_Memcpy(Cn_1, inbuf, inlen);
    Pn = outbuf-blocksize;
    
    



















    
    XOR_BLOCK(lastBlock, Cn_2, blocksize);
    XOR_BLOCK(lastBlock, Pn, blocksize);
    
    PORT_Memcpy(outbuf, lastBlock, inlen);
    *outlen += inlen;
    
    PORT_Memcpy(lastBlock, Cn_1, inlen);
    


    rv = (*cts->cipher)(cts->context, Pn, &tmpLen, blocksize, lastBlock,
	 blocksize, blocksize);
    if (rv != SECSuccess) {
	return SECFailure;
    }
    
    XOR_BLOCK(Pn, Cn_2, blocksize);
    XOR_BLOCK(Pn, Cn, blocksize);
    
    PORT_Memcpy(cts->iv, Cn, blocksize);
    


    (void) (*cts->cipher)(cts->context, lastBlock, &tmpLen, blocksize, Cn,
		blocksize, blocksize);
    


    PORT_Memset(lastBlock, 0, blocksize);
    
    return SECSuccess;
}
