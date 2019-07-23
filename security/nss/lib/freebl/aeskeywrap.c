









































#include "prcpucfg.h"
#if defined(IS_LITTLE_ENDIAN) || defined(SHA_NO_LONG_LONG)
#define BIG_ENDIAN_WITH_64_BIT_REGISTERS 0
#else
#define BIG_ENDIAN_WITH_64_BIT_REGISTERS 1
#endif
#include "prtypes.h"	
#include "secport.h"	
#include "secerr.h"
#include "blapi.h"	
#include "rijndael.h"

struct AESKeyWrapContextStr {
     unsigned char iv[AES_KEY_WRAP_IV_BYTES];
     AESContext    aescx;
};






AESKeyWrapContext * 
AESKeyWrap_AllocateContext(void)
{
    AESKeyWrapContext * cx = PORT_New(AESKeyWrapContext);
    return cx;
}

SECStatus  
AESKeyWrap_InitContext(AESKeyWrapContext *cx, 
		       const unsigned char *key, 
		       unsigned int keylen,
		       const unsigned char *iv, 
		       int x1,
		       unsigned int encrypt,
		       unsigned int x2)
{
    SECStatus rv = SECFailure;
    if (!cx) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
    	return SECFailure;
    }
    if (iv) {
    	memcpy(cx->iv, iv, sizeof cx->iv);
    } else {
	memset(cx->iv, 0xA6, sizeof cx->iv);
    }
    rv = AES_InitContext(&cx->aescx, key, keylen, NULL, NSS_AES, encrypt, 
                                  AES_BLOCK_SIZE);
    return rv;
}






extern AESKeyWrapContext *
AESKeyWrap_CreateContext(const unsigned char *key, const unsigned char *iv, 
                         int encrypt, unsigned int keylen)
{
    SECStatus rv;
    AESKeyWrapContext * cx = AESKeyWrap_AllocateContext();
    if (!cx) 
    	return NULL;	
    rv = AESKeyWrap_InitContext(cx, key, keylen, iv, 0, encrypt, 0);
    if (rv != SECSuccess) {
        PORT_Free(cx);
	cx = NULL; 	
    }
    return cx;
}






extern void 
AESKeyWrap_DestroyContext(AESKeyWrapContext *cx, PRBool freeit)
{
    if (cx) {
	AES_DestroyContext(&cx->aescx, PR_FALSE);

	if (freeit)
	    PORT_Free(cx);
    }
}

#if !BIG_ENDIAN_WITH_64_BIT_REGISTERS












 
static void
increment_and_xor(unsigned char *A, unsigned char *T)
{
    if (!++T[7])
        if (!++T[6])
	    if (!++T[5])
		if (!++T[4])
		    if (!++T[3])
			if (!++T[2])
			    if (!++T[1])
				 ++T[0];

    A[0] ^= T[0];
    A[1] ^= T[1];
    A[2] ^= T[2];
    A[3] ^= T[3];
    A[4] ^= T[4];
    A[5] ^= T[5];
    A[6] ^= T[6];
    A[7] ^= T[7];
}




 
static void
xor_and_decrement(unsigned char *A, unsigned char *T)
{
    A[0] ^= T[0];
    A[1] ^= T[1];
    A[2] ^= T[2];
    A[3] ^= T[3];
    A[4] ^= T[4];
    A[5] ^= T[5];
    A[6] ^= T[6];
    A[7] ^= T[7];

    if (!T[7]--)
        if (!T[6]--)
	    if (!T[5]--)
		if (!T[4]--)
		    if (!T[3]--)
			if (!T[2]--)
			    if (!T[1]--)
				 T[0]--;

}




static void
set_t(unsigned char *pt, unsigned long t)
{
    pt[7] = (unsigned char)t; t >>= 8;
    pt[6] = (unsigned char)t; t >>= 8;
    pt[5] = (unsigned char)t; t >>= 8;
    pt[4] = (unsigned char)t; t >>= 8;
    pt[3] = (unsigned char)t; t >>= 8;
    pt[2] = (unsigned char)t; t >>= 8;
    pt[1] = (unsigned char)t; t >>= 8;
    pt[0] = (unsigned char)t;
}

#endif












extern SECStatus 
AESKeyWrap_Encrypt(AESKeyWrapContext *cx, unsigned char *output,
            unsigned int *pOutputLen, unsigned int maxOutputLen,
            const unsigned char *input, unsigned int inputLen)
{
    PRUint64 *     R          = NULL;
    unsigned int   nBlocks;
    unsigned int   i, j;
    unsigned int   aesLen     = AES_BLOCK_SIZE;
    unsigned int   outLen     = inputLen + AES_KEY_WRAP_BLOCK_SIZE;
    SECStatus      s          = SECFailure;
    
    PRUint64       t;
    PRUint64       B[2];

#define A B[0]

    
    if (!inputLen || 0 != inputLen % AES_KEY_WRAP_BLOCK_SIZE) {
	PORT_SetError(SEC_ERROR_INPUT_LEN);
	return s;
    }
#ifdef maybe
    if (!output && pOutputLen) {	
    	*pOutputLen = outLen;
	return SECSuccess;
    }
#endif
    if (maxOutputLen < outLen) {
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	return s;
    }
    if (cx == NULL || output == NULL || input == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return s;
    }
    nBlocks = inputLen / AES_KEY_WRAP_BLOCK_SIZE;
    R = PORT_NewArray(PRUint64, nBlocks + 1);
    if (!R)
    	return s;	
    


    memcpy(&A, cx->iv, AES_KEY_WRAP_IV_BYTES);
    memcpy(&R[1], input, inputLen);
#if BIG_ENDIAN_WITH_64_BIT_REGISTERS
    t = 0;
#else
    memset(&t, 0, sizeof t);
#endif
    


    for (j = 0; j < 6; ++j) {
    	for (i = 1; i <= nBlocks; ++i) {
	    B[1] = R[i];
	    s = AES_Encrypt(&cx->aescx, (unsigned char *)B, &aesLen, 
	                    sizeof B,  (unsigned char *)B, sizeof B);
	    if (s != SECSuccess) 
	        break;
	    R[i] = B[1];
	    
#if BIG_ENDIAN_WITH_64_BIT_REGISTERS
   	    A ^= ++t; 
#else
	    increment_and_xor((unsigned char *)&A, (unsigned char *)&t);
#endif
	}
    }
    


    if (s == SECSuccess) {
    	R[0] =  A;
	memcpy(output, &R[0], outLen);
	if (pOutputLen)
	    *pOutputLen = outLen;
    } else if (pOutputLen) {
    	*pOutputLen = 0;
    }
    PORT_ZFree(R, outLen);
    return s;
}
#undef A












extern SECStatus 
AESKeyWrap_Decrypt(AESKeyWrapContext *cx, unsigned char *output,
            unsigned int *pOutputLen, unsigned int maxOutputLen,
            const unsigned char *input, unsigned int inputLen)
{
    PRUint64 *     R          = NULL;
    unsigned int   nBlocks;
    unsigned int   i, j;
    unsigned int   aesLen     = AES_BLOCK_SIZE;
    unsigned int   outLen;
    SECStatus      s          = SECFailure;
    
    PRUint64       t;
    PRUint64       B[2];

#define A B[0]

    
    if (inputLen < 3 * AES_KEY_WRAP_BLOCK_SIZE || 
        0 != inputLen % AES_KEY_WRAP_BLOCK_SIZE) {
	PORT_SetError(SEC_ERROR_INPUT_LEN);
	return s;
    }
    outLen = inputLen - AES_KEY_WRAP_BLOCK_SIZE;
#ifdef maybe
    if (!output && pOutputLen) {	
    	*pOutputLen = outLen;
	return SECSuccess;
    }
#endif
    if (maxOutputLen < outLen) {
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	return s;
    }
    if (cx == NULL || output == NULL || input == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return s;
    }
    nBlocks = inputLen / AES_KEY_WRAP_BLOCK_SIZE;
    R = PORT_NewArray(PRUint64, nBlocks);
    if (!R)
    	return s;	
    nBlocks--;
    


    memcpy(&R[0], input, inputLen);
    A = R[0];
#if BIG_ENDIAN_WITH_64_BIT_REGISTERS
    t = 6UL * nBlocks;
#else
    set_t((unsigned char *)&t, 6UL * nBlocks);
#endif
    


    for (j = 0; j < 6; ++j) {
    	for (i = nBlocks; i; --i) {
	    
#if BIG_ENDIAN_WITH_64_BIT_REGISTERS
   	    A ^= t--; 
#else
	    xor_and_decrement((unsigned char *)&A, (unsigned char *)&t);
#endif
	    B[1] = R[i];
	    s = AES_Decrypt(&cx->aescx, (unsigned char *)B, &aesLen, 
	                    sizeof B,  (unsigned char *)B, sizeof B);
	    if (s != SECSuccess) 
	        break;
	    R[i] = B[1];
	}
    }
    


    if (s == SECSuccess) {
	int bad = memcmp(&A, cx->iv, AES_KEY_WRAP_IV_BYTES);
	if (!bad) {
	    memcpy(output, &R[1], outLen);
	    if (pOutputLen)
		*pOutputLen = outLen;
	} else {
	    PORT_SetError(SEC_ERROR_BAD_DATA);
	    if (pOutputLen) 
		*pOutputLen = 0;
    	}
    } else if (pOutputLen) {
    	*pOutputLen = 0;
    }
    PORT_ZFree(R, inputLen);
    return s;
}
#undef A
