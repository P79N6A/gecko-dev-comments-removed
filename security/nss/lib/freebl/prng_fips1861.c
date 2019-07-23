






































#ifdef FREEBL_NO_DEPEND
#include "stubs.h"
#endif

#include "prerr.h"
#include "secerr.h"

#include "prtypes.h"
#include "prinit.h"
#include "blapi.h"
#include "nssilock.h"
#include "secitem.h"
#include "sha_fast.h"
#include "sha256.h"
#include "secrng.h"	
#include "secmpi.h"













#define MIN_SEED_COUNT 1024











#define FIPS_B     256
#define BSIZE      (FIPS_B / PR_BITS_PER_BYTE)
#if BSIZE != SHA256_LENGTH
#error "this file requires that BSIZE and SHA256_LENGTH be equal"
#endif


#define FIPS_G     160
#define GSIZE      (FIPS_G / PR_BITS_PER_BYTE)






#define ADD_B_BIT_PLUS_CARRY(dest, add1, add2, cy) \
    carry = cy; \
    for (k=BSIZE-1; k>=0; --k) { \
	carry += add1[k] + add2[k]; \
	dest[k] = (PRUint8)carry; \
	carry >>= 8; \
    }

#define ADD_B_BIT_2(dest, add1, add2) \
	ADD_B_BIT_PLUS_CARRY(dest, add1, add2, 0)










SECStatus
FIPS186Change_ReduceModQForDSA(const unsigned char *w,
                               const unsigned char *q,
                               unsigned char *xj)
{
    mp_int W, Q, Xj;
    mp_err err;
    SECStatus rv = SECSuccess;

    
    MP_DIGITS(&W) = 0;
    MP_DIGITS(&Q) = 0;
    MP_DIGITS(&Xj) = 0;
    CHECK_MPI_OK( mp_init(&W) );
    CHECK_MPI_OK( mp_init(&Q) );
    CHECK_MPI_OK( mp_init(&Xj) );
    


    CHECK_MPI_OK( mp_read_unsigned_octets(&W, w, 2*GSIZE) );
    CHECK_MPI_OK( mp_read_unsigned_octets(&Q, q, DSA_SUBPRIME_LEN) );
    




    CHECK_MPI_OK( mp_mod(&W, &Q, &Xj) );
    CHECK_MPI_OK( mp_to_fixlen_octets(&Xj, xj, DSA_SUBPRIME_LEN) );
cleanup:
    mp_clear(&W);
    mp_clear(&Q);
    mp_clear(&Xj);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    return rv;
}






static void 
RNG_UpdateAndEnd_FIPS186_2(SHA1Context *ctx, 
                           unsigned char *input, unsigned int inputLen,
                           unsigned char *hashout, unsigned int *pDigestLen, 
                           unsigned int maxDigestLen);



 
struct RNGContextStr {
    PRUint8   XKEY[BSIZE]; 
    PRUint8   Xj[2*GSIZE]; 
    PZLock   *lock;        
    PRUint8   avail;       
    PRUint32  seedCount;   
    PRBool    isValid;     
};
typedef struct RNGContextStr RNGContext;
static RNGContext *globalrng = NULL;
static RNGContext theGlobalRng;




static void
freeRNGContext()
{
    unsigned char inputhash[BSIZE];
    SECStatus rv;

    
    PZ_DestroyLock(globalrng->lock);

    
    rv = SHA256_HashBuf(inputhash, globalrng->XKEY, BSIZE);
    PORT_Assert(SECSuccess == rv);
    memset(globalrng, 0, sizeof(*globalrng));
    memcpy(globalrng->XKEY, inputhash, BSIZE);

    globalrng = NULL;
}





















SECStatus
FIPS186Change_GenerateX(unsigned char *XKEY, const unsigned char *XSEEDj,
                        unsigned char *x_j)
{
    
    SHA1Context sha1cx;
    
    PRUint8 XKEY_1[BSIZE];
    const PRUint8 *XKEY_old;
    PRUint8 *XKEY_new;
    
    PRUint8 XVAL[BSIZE];
    
    int k, carry;
    
    PRUint8 w_i[BSIZE];
    int i;
    unsigned int len;
    SECStatus rv = SECSuccess;

#if GSIZE < BSIZE
    
    memset(w_i, 0, BSIZE - GSIZE);
#endif
    



 
    for (i = 0; i < 2; i++) {
	
	if (i == 0) {
	    
	    XKEY_old = XKEY;
	    XKEY_new = XKEY_1;
	} else {
	    
	    XKEY_old = XKEY_1;
	    XKEY_new = XKEY;
	}
	



	if (XSEEDj) {
	    
	    if (memcmp(XKEY_old, XSEEDj, BSIZE) == 0) {
		
		PORT_SetError(SEC_ERROR_INVALID_ARGS);
		rv = SECFailure;
		goto done;
	    }
	    ADD_B_BIT_2(XVAL, XKEY_old, XSEEDj);
	} else {
	    
	    memcpy(XVAL, XKEY_old, BSIZE);
	}
	




 
	SHA1_Begin(&sha1cx);
	RNG_UpdateAndEnd_FIPS186_2(&sha1cx, XVAL, BSIZE,
				   &w_i[BSIZE - GSIZE], &len, GSIZE);
	



	ADD_B_BIT_PLUS_CARRY(XKEY_new, XKEY_old, w_i, 1);
	


	memcpy(&x_j[i*GSIZE], &w_i[BSIZE - GSIZE], GSIZE);
    }

done:
    
    memset(&w_i[BSIZE - GSIZE], 0, GSIZE);
    memset(XVAL, 0, BSIZE);
    memset(XKEY_1, 0, BSIZE);
    return rv;
}









static SECStatus
alg_fips186_2_cn_1(RNGContext *rng, const unsigned char *XSEEDj)
{
    
    PRUint8 x_j[2*GSIZE];
    SECStatus rv;

    if (!rng->isValid) {
	
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    rv = FIPS186Change_GenerateX(rng->XKEY, XSEEDj, x_j);
    if (rv != SECSuccess) {
	goto done;
    }
    
    if (memcmp(x_j, rng->Xj, 2*GSIZE) == 0) {
	
	rng->isValid = PR_FALSE;
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	rv = SECFailure;
	goto done;
    }
    
    memcpy(rng->Xj, x_j, 2*GSIZE);
    
    rng->avail = 2*GSIZE;

done:
    
    memset(x_j, 0, 2*GSIZE);
    return rv;
}




static const PRCallOnceType pristineCallOnce;
static PRCallOnceType coRNGInit;
static PRStatus rng_init(void)
{
    unsigned char bytes[SYSTEM_RNG_SEED_COUNT];
    unsigned int numBytes;
    if (globalrng == NULL) {
	
	globalrng = &theGlobalRng;
        PORT_Assert(NULL == globalrng->lock);
	
	globalrng->lock = PZ_NewLock(nssILockOther);
	if (globalrng->lock == NULL) {
	    globalrng = NULL;
	    PORT_SetError(PR_OUT_OF_MEMORY_ERROR);
	    return PR_FAILURE;
	}
	
	globalrng->isValid = PR_TRUE;
	
	numBytes = RNG_SystemRNG(bytes, sizeof bytes);
	PORT_Assert(numBytes == 0 || numBytes == sizeof bytes);
	if (numBytes != 0) {
	    RNG_RandomUpdate(bytes, numBytes);
	    memset(bytes, 0, numBytes);
	} else if (PORT_GetError() != PR_NOT_IMPLEMENTED_ERROR) {
	    PZ_DestroyLock(globalrng->lock);
	    globalrng->lock = NULL;
	    globalrng->isValid = PR_FALSE;
	    globalrng = NULL;
	    return PR_FAILURE;
	}
	numBytes = RNG_GetNoise(bytes, sizeof bytes);
	RNG_RandomUpdate(bytes, numBytes);
    }
    return PR_SUCCESS;
}










SECStatus 
RNG_RNGInit(void)
{
    
    PR_CallOnce(&coRNGInit, rng_init);
    
    return (globalrng != NULL) ? PR_SUCCESS : PR_FAILURE;
}





static SECStatus 
prng_RandomUpdate(RNGContext *rng, const void *data, size_t bytes)
{
    SECStatus rv = SECSuccess;
    
    PORT_Assert(rng != NULL);
    if (rng == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    
    if (bytes == 0)
	return SECSuccess;
    
    PZ_Lock(rng->lock);
    









    if (rng->seedCount == 0) {
	




	SHA256_HashBuf(rng->XKEY, data, bytes);
	
	rv = alg_fips186_2_cn_1(rng, NULL);
	




	rng->avail = 0;
    } else if (bytes == BSIZE && memcmp(rng->XKEY, data, BSIZE) == 0) {
	
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	rv = SECFailure;
    } else {
	





	SHA256Context ctx;
	SHA256_Begin(&ctx);
	SHA256_Update(&ctx, rng->XKEY, BSIZE);
	SHA256_Update(&ctx, data, bytes);
	SHA256_End(&ctx, rng->XKEY, NULL, BSIZE);
    }
    
    if (rv == SECSuccess)
	rng->seedCount += bytes;
    PZ_Unlock(rng->lock);
    
    return rv;
}





SECStatus 
RNG_RandomUpdate(const void *data, size_t bytes)
{
    return prng_RandomUpdate(globalrng, data, bytes);
}





static SECStatus 
prng_GenerateGlobalRandomBytes(RNGContext *rng,
                               void *dest, size_t len)
{
    PRUint8 num;
    SECStatus rv = SECSuccess;
    unsigned char *output = dest;
    
    PORT_Assert(rng != NULL);
    if (rng == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    
    PZ_Lock(rng->lock);
    


    if (rng->seedCount < MIN_SEED_COUNT) {
	PZ_Unlock(rng->lock);
	PORT_SetError(SEC_ERROR_NEED_RANDOM);
	return SECFailure;
    }
    



    while (len > 0 && rv == SECSuccess) {
	if (rng->avail == 0) {
	    
	    rv = alg_fips186_2_cn_1(rng, NULL);
	}
	
	num = PR_MIN(rng->avail, len);
	



	if (num) {
	    memcpy(output, rng->Xj + (2*GSIZE - rng->avail), num);
	    rng->avail -= num;
	    len -= num;
	    output += num;
	}
    }
    PZ_Unlock(rng->lock);
    
    return rv;
}





SECStatus 
RNG_GenerateGlobalRandomBytes(void *dest, size_t len)
{
    return prng_GenerateGlobalRandomBytes(globalrng, dest, len);
}

void
RNG_RNGShutdown(void)
{
    
    PORT_Assert(globalrng != NULL);
    if (globalrng == NULL) {
	
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return;
    }
    
    freeRNGContext();
    
    coRNGInit = pristineCallOnce;
}










static void 
RNG_UpdateAndEnd_FIPS186_2(SHA1Context *ctx, 
                           unsigned char *input, unsigned int inputLen,
                           unsigned char *hashout, unsigned int *pDigestLen, 
                           unsigned int maxDigestLen)
{
#if defined(SHA_NEED_TMP_VARIABLE)
    register PRUint32 tmp;
#endif
    static const unsigned char bulk_pad0[64] = { 0,0,0,0,0,0,0,0,0,0,
               0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
               0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0  };

    PORT_Assert(maxDigestLen >= SHA1_LENGTH);
    PORT_Assert(inputLen <= SHA1_INPUT_LEN);

    


    SHA1_Update(ctx, input, inputLen);
    




    SHA1_Update(ctx, bulk_pad0, SHA1_INPUT_LEN - inputLen);

    


    SHA_STORE_RESULT;
    *pDigestLen = SHA1_LENGTH;
}
















SECStatus 
DSA_GenerateGlobalRandomBytes(void *dest, size_t len, const unsigned char *q)
{
    SECStatus rv;
    unsigned char w[2*GSIZE];

    PORT_Assert(q && len == DSA_SUBPRIME_LEN);
    if (len != DSA_SUBPRIME_LEN) {
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	return SECFailure;
    }
    if (*q == 0) {
        ++q;
    }
    rv = prng_GenerateGlobalRandomBytes(globalrng, w, 2*GSIZE);
    if (rv != SECSuccess) {
	return rv;
    }
    FIPS186Change_ReduceModQForDSA(w, q, (unsigned char *)dest);
    return rv;
}
