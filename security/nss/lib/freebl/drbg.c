




#ifdef FREEBL_NO_DEPEND
#include "stubs.h"
#endif

#include "prerror.h"
#include "secerr.h"

#include "prtypes.h"
#include "prinit.h"
#include "blapi.h"
#include "blapii.h"
#include "nssilock.h"
#include "secitem.h"
#include "sha_fast.h"
#include "sha256.h"
#include "secrng.h"	
#include "secmpi.h"




#define PRNG_SEEDLEN      (440/PR_BITS_PER_BYTE)
static const PRInt64 PRNG_MAX_ADDITIONAL_BYTES = LL_INIT(0x1, 0x0);
						
#define PRNG_MAX_REQUEST_SIZE 0x10000		/* 2^19 bits or 2^16 bytes */
#define PRNG_ADDITONAL_DATA_CACHE_SIZE (8*1024) /* must be less than
						 *  PRNG_MAX_ADDITIONAL_BYTES
						 */










#define RESEED_BYTE 6
#define RESEED_VALUE 1

#define PRNG_RESET_RESEED_COUNT(rng) \
	PORT_Memset((rng)->reseed_counter, 0, sizeof (rng)->reseed_counter); \
	(rng)->reseed_counter[RESEED_BYTE] = 1;






typedef enum {
   prngCGenerateType = 0,   	
   prngReseedType = 1,	    	
   prngAdditionalDataType = 2,  
   prngGenerateByteType = 3	

} prngVTypes;



 
struct RNGContextStr {
    PZLock   *lock;        
    





    PRUint8  V_Data[PRNG_SEEDLEN+1]; 
#define  V_type  V_Data[0]
#define  V(rng)       (((rng)->V_Data)+1)
#define  VSize(rng)   ((sizeof (rng)->V_Data) -1)
    PRUint8  C[PRNG_SEEDLEN];        
    PRUint8  oldV[PRNG_SEEDLEN];     
    





    PRUint8  reseed_counter[RESEED_BYTE+1]; 



    PRUint8  data[SHA256_LENGTH];	


    PRUint8  dataAvail;            

    

    PRUint8  additionalDataCache[PRNG_ADDITONAL_DATA_CACHE_SIZE];
    PRUint32 additionalAvail;
    PRBool   isValid;          
};

typedef struct RNGContextStr RNGContext;
static RNGContext *globalrng = NULL;
static RNGContext theGlobalRng;

















static SECStatus
prng_Hash_df(PRUint8 *requested_bytes, unsigned int no_of_bytes_to_return, 
	const PRUint8 *input_string_1, unsigned int input_string_1_len, 
	const PRUint8 *input_string_2, unsigned int input_string_2_len)
{
    SHA256Context ctx;
    PRUint32 tmp;
    PRUint8 counter;

    tmp=SHA_HTONL(no_of_bytes_to_return*8);

    for (counter = 1 ; no_of_bytes_to_return > 0; counter++) {
	unsigned int hash_return_len;
 	SHA256_Begin(&ctx);
 	SHA256_Update(&ctx, &counter, 1);
 	SHA256_Update(&ctx, (unsigned char *)&tmp, sizeof tmp);
 	SHA256_Update(&ctx, input_string_1, input_string_1_len);
	if (input_string_2) {
 	    SHA256_Update(&ctx, input_string_2, input_string_2_len);
	}
	SHA256_End(&ctx, requested_bytes, &hash_return_len,
		no_of_bytes_to_return);
	requested_bytes += hash_return_len;
	no_of_bytes_to_return -= hash_return_len;
    }
    return SECSuccess;
}








static SECStatus
prng_instantiate(RNGContext *rng, const PRUint8 *bytes, unsigned int len)
{
    if (len < PRNG_SEEDLEN) {
	

	PORT_SetError(SEC_ERROR_NEED_RANDOM);
	return SECFailure;
    }
    prng_Hash_df(V(rng), VSize(rng), bytes, len, NULL, 0);
    rng->V_type = prngCGenerateType;
    prng_Hash_df(rng->C,sizeof rng->C,rng->V_Data,sizeof rng->V_Data,NULL,0);
    PRNG_RESET_RESEED_COUNT(rng)
    return SECSuccess;
}
    








static SECStatus
prng_reseed(RNGContext *rng, const PRUint8 *entropy, unsigned int entropy_len,
	const PRUint8 *additional_input, unsigned int additional_input_len)
{
    PRUint8 noiseData[(sizeof rng->V_Data)+PRNG_SEEDLEN];
    PRUint8 *noise = &noiseData[0];

    
    if (entropy == NULL) {
    	entropy_len = (unsigned int) RNG_SystemRNG(
			&noiseData[sizeof rng->V_Data], PRNG_SEEDLEN);
    } else {
	
	
	if (entropy_len > PRNG_SEEDLEN) {
	    noise = PORT_Alloc(entropy_len + (sizeof rng->V_Data));
	    if (noise == NULL) {
		return SECFailure;
	    }
	}
	PORT_Memcpy(&noise[sizeof rng->V_Data],entropy, entropy_len);
    }

    if (entropy_len < 256/PR_BITS_PER_BYTE) {
	
	PORT_SetError(SEC_ERROR_NEED_RANDOM);
	return SECFailure;
    }

    rng->V_type = prngReseedType;
    PORT_Memcpy(noise, rng->V_Data, sizeof rng->V_Data);
    prng_Hash_df(V(rng), VSize(rng), noise, (sizeof rng->V_Data) + entropy_len,
		additional_input, additional_input_len);
    
    PORT_Memset(noise, 0, (sizeof rng->V_Data) + entropy_len); 
    rng->V_type = prngCGenerateType;
    prng_Hash_df(rng->C,sizeof rng->C,rng->V_Data,sizeof rng->V_Data,NULL,0);
    PRNG_RESET_RESEED_COUNT(rng)

    if (noise != &noiseData[0]) {
	PORT_Free(noise);
    }
    return SECSuccess;
}




static SECStatus
prng_reseed_test(RNGContext *rng, const PRUint8 *entropy, 
	unsigned int entropy_len, const PRUint8 *additional_input, 
	unsigned int additional_input_len)
{
    SECStatus rv;

    
    rv = PRNGTEST_RunHealthTests();
    if (rv != SECSuccess) {
	
	rng->isValid = PR_FALSE;
	return SECFailure;
    }
    return prng_reseed(rng, entropy, entropy_len, 
				additional_input, additional_input_len);
}




#define PRNG_ADD_CARRY_ONLY(dest, start, cy) \
   carry = cy; \
   for (k1=start; carry && k1 >=0 ; k1--) { \
	carry = !(++dest[k1]); \
   } 




#define PRNG_ADD_BITS(dest, dest_len, add, len) \
    carry = 0; \
    for (k1=dest_len -1, k2=len-1; k2 >= 0; --k1, --k2) { \
	carry += dest[k1]+ add[k2]; \
	dest[k1] = (PRUint8) carry; \
	carry >>= 8; \
    }

#define PRNG_ADD_BITS_AND_CARRY(dest, dest_len, add, len) \
    PRNG_ADD_BITS(dest, dest_len, add, len) \
    PRNG_ADD_CARRY_ONLY(dest, k1, carry)








static void
prng_Hashgen(RNGContext *rng, PRUint8 *returned_bytes, 
	     unsigned int no_of_returned_bytes)
{
    PRUint8 data[VSize(rng)];

    PORT_Memcpy(data, V(rng), VSize(rng));
    while (no_of_returned_bytes) {
	SHA256Context ctx;
	unsigned int len;
	unsigned int carry;
	int k1;

 	SHA256_Begin(&ctx);
 	SHA256_Update(&ctx, data, sizeof data);
	SHA256_End(&ctx, returned_bytes, &len, no_of_returned_bytes);
	returned_bytes += len;
	no_of_returned_bytes -= len;
	

	PRNG_ADD_CARRY_ONLY(data, (sizeof data)- 1, no_of_returned_bytes);
    }
    PORT_Memset(data, 0, sizeof data); 
}







static SECStatus
prng_generateNewBytes(RNGContext *rng, 
		PRUint8 *returned_bytes, unsigned int no_of_returned_bytes,
		const PRUint8 *additional_input,
		unsigned int additional_input_len)
{
    PRUint8 H[SHA256_LENGTH]; 

    unsigned int carry;
    int k1, k2;

    if (!rng->isValid) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    

    if (additional_input){
	SHA256Context ctx;
	




#define w H
	rng->V_type = prngAdditionalDataType;
 	SHA256_Begin(&ctx);
 	SHA256_Update(&ctx, rng->V_Data, sizeof rng->V_Data);
 	SHA256_Update(&ctx, additional_input, additional_input_len);
	SHA256_End(&ctx, w, NULL, sizeof w);
	PRNG_ADD_BITS_AND_CARRY(V(rng), VSize(rng), w, sizeof w)
	PORT_Memset(w, 0, sizeof w);
#undef w 
    }

    if (no_of_returned_bytes == SHA256_LENGTH) {
	
	SHA256_HashBuf(returned_bytes, V(rng), VSize(rng) );
    } else {
    	prng_Hashgen(rng, returned_bytes, no_of_returned_bytes);
    }
    
    rng->V_type = prngGenerateByteType;
    SHA256_HashBuf(H, rng->V_Data, sizeof rng->V_Data);
    PRNG_ADD_BITS_AND_CARRY(V(rng), VSize(rng), H, sizeof H)
    PRNG_ADD_BITS(V(rng), VSize(rng), rng->C, sizeof rng->C);
    PRNG_ADD_BITS_AND_CARRY(V(rng), VSize(rng), rng->reseed_counter, 
					sizeof rng->reseed_counter)
    PRNG_ADD_CARRY_ONLY(rng->reseed_counter,(sizeof rng->reseed_counter)-1, 1);

    
    if (memcmp(V(rng), rng->oldV, sizeof rng->oldV) == 0) {
	rng->isValid = PR_FALSE;
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    PORT_Memcpy(rng->oldV, V(rng), sizeof rng->oldV);
    return SECSuccess;
}




static const PRCallOnceType pristineCallOnce;
static PRCallOnceType coRNGInit;
static PRStatus rng_init(void)
{
    PRUint8 bytes[PRNG_SEEDLEN*2]; 
    unsigned int numBytes;
    SECStatus rv = SECSuccess;

    if (globalrng == NULL) {
	

	PR_STATIC_ASSERT(sizeof(bytes) >= SHA256_LENGTH);
	
	globalrng = &theGlobalRng;
        PORT_Assert(NULL == globalrng->lock);
	
	globalrng->lock = PZ_NewLock(nssILockOther);
	if (globalrng->lock == NULL) {
	    globalrng = NULL;
	    PORT_SetError(PR_OUT_OF_MEMORY_ERROR);
	    return PR_FAILURE;
	}

	
	numBytes = (unsigned int) RNG_SystemRNG(bytes, sizeof bytes);
	PORT_Assert(numBytes == 0 || numBytes == sizeof bytes);
	if (numBytes != 0) {
	    


	    if (V(globalrng)[0] == 0) {
		rv = prng_instantiate(globalrng, bytes, numBytes);
	    } else {
		rv = prng_reseed_test(globalrng, bytes, numBytes, NULL, 0);
	    }
	    memset(bytes, 0, numBytes);
	} else {
	    PZ_DestroyLock(globalrng->lock);
	    globalrng->lock = NULL;
	    globalrng = NULL;
	    return PR_FAILURE;
	}
 
	if (rv != SECSuccess) {
	    return PR_FAILURE;
	}
	
	globalrng->isValid = PR_TRUE;

	

	prng_generateNewBytes(globalrng, bytes, SHA256_LENGTH, NULL, 0);

	
	RNG_SystemInfoForRNG();
    }
    return PR_SUCCESS;
}




static void
prng_freeRNGContext(RNGContext *rng)
{
    PRUint8 inputhash[VSize(rng) + (sizeof rng->C)];

    
    SKIP_AFTER_FORK(PZ_DestroyLock(globalrng->lock));

    
    prng_Hash_df(inputhash, sizeof rng->C, rng->C, sizeof rng->C, NULL, 0); 
    prng_Hash_df(&inputhash[sizeof rng->C], VSize(rng), V(rng), VSize(rng), 
								  NULL, 0); 
    memset(rng, 0, sizeof *rng);
    memcpy(rng->C, inputhash, sizeof rng->C); 
    memcpy(V(rng), &inputhash[sizeof rng->C], VSize(rng)); 

    memset(inputhash, 0, sizeof inputhash);
}














SECStatus 
RNG_RNGInit(void)
{
    
    PR_CallOnce(&coRNGInit, rng_init);
    
    return (globalrng != NULL) ? PR_SUCCESS : PR_FAILURE;
}





SECStatus 
RNG_RandomUpdate(const void *data, size_t bytes)
{
    SECStatus rv;

    
    PR_STATIC_ASSERT(((size_t)-1) > (size_t)1);

#if defined(NS_PTR_GT_32) || (defined(NSS_USE_64) && !defined(NS_PTR_LE_32))
    






















    PR_STATIC_ASSERT(sizeof(size_t) > 4);

    if (bytes > PRNG_MAX_ADDITIONAL_BYTES) {
	bytes = PRNG_MAX_ADDITIONAL_BYTES;
    }
#else
    PR_STATIC_ASSERT(sizeof(size_t) <= 4);
#endif

    PZ_Lock(globalrng->lock);
    

    if (bytes > sizeof (globalrng->additionalDataCache)) {
	rv = prng_reseed_test(globalrng, NULL, 0, data, (unsigned int) bytes);
    
    } else if (bytes < ((sizeof globalrng->additionalDataCache)
				- globalrng->additionalAvail)) {
	PORT_Memcpy(globalrng->additionalDataCache+globalrng->additionalAvail,
		    data, bytes);
	globalrng->additionalAvail += (PRUint32) bytes;
	rv = SECSuccess;
    } else {
	




	size_t bufRemain = (sizeof globalrng->additionalDataCache) 
					- globalrng->additionalAvail;
	
	if (bufRemain) {
	    PORT_Memcpy(globalrng->additionalDataCache
			+globalrng->additionalAvail, 
			data, bufRemain);
	    data = ((unsigned char *)data) + bufRemain;
	    bytes -= bufRemain;
	}
	
	rv = prng_reseed_test(globalrng, NULL, 0, 
				        globalrng->additionalDataCache, 
					sizeof globalrng->additionalDataCache);

	
	PORT_Memcpy(globalrng->additionalDataCache, data, bytes);
	globalrng->additionalAvail = (PRUint32) bytes;
    }
		
    PZ_Unlock(globalrng->lock);
    return rv;
}





static SECStatus 
prng_GenerateGlobalRandomBytes(RNGContext *rng,
                               void *dest, size_t len)
{
    SECStatus rv = SECSuccess;
    PRUint8 *output = dest;
    
    PORT_Assert(rng != NULL);
    if (rng == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    
    if (len > PRNG_MAX_REQUEST_SIZE) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    
    PZ_Lock(rng->lock);
    


    if (rng->reseed_counter[0] >= RESEED_VALUE) {
	rv = prng_reseed_test(rng, NULL, 0, NULL, 0);
	PZ_Unlock(rng->lock);
	if (rv != SECSuccess) {
	    return rv;
	}
	RNG_SystemInfoForRNG();
	PZ_Lock(rng->lock);
    }
    


    if (len <= rng->dataAvail) {
	memcpy(output, rng->data + ((sizeof rng->data) - rng->dataAvail), len);
	memset(rng->data + ((sizeof rng->data) - rng->dataAvail), 0, len);
	rng->dataAvail -= len;
	rv = SECSuccess;
    

    } else if (len < sizeof rng->data) {
	rv = prng_generateNewBytes(rng, rng->data, sizeof rng->data, 
			rng->additionalAvail ? rng->additionalDataCache : NULL,
			rng->additionalAvail);
	rng->additionalAvail = 0;
	if (rv == SECSuccess) {
	    memcpy(output, rng->data, len);
	    memset(rng->data, 0, len); 
	    rng->dataAvail = (sizeof rng->data) - len;
	}
    
    } else {
	rv = prng_generateNewBytes(rng, output, len,
			rng->additionalAvail ? rng->additionalDataCache : NULL,
			rng->additionalAvail);
	rng->additionalAvail = 0;
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
	
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return;
    }
    
    prng_freeRNGContext(globalrng);
    globalrng = NULL;
    
    coRNGInit = pristineCallOnce;
}




 


RNGContext testContext;





SECStatus
PRNGTEST_Instantiate(const PRUint8 *entropy, unsigned int entropy_len, 
		const PRUint8 *nonce, unsigned int nonce_len,
		const PRUint8 *personal_string, unsigned int ps_len)
{
   int bytes_len = entropy_len + nonce_len + ps_len;
   PRUint8 *bytes = NULL;
   SECStatus rv;

   if (entropy_len < 256/PR_BITS_PER_BYTE) {
	PORT_SetError(SEC_ERROR_NEED_RANDOM);
	return SECFailure;
   }

   bytes = PORT_Alloc(bytes_len);
   if (bytes == NULL) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
   }
   

   PORT_Memcpy(bytes, entropy, entropy_len);
   if (nonce) {
	PORT_Memcpy(&bytes[entropy_len], nonce, nonce_len);
   } else {
	PORT_Assert(nonce_len == 0);
   }
   if (personal_string) {
       PORT_Memcpy(&bytes[entropy_len+nonce_len], personal_string, ps_len);
   } else {
	PORT_Assert(ps_len == 0);
   }
   rv = prng_instantiate(&testContext, bytes, bytes_len);
   PORT_ZFree(bytes, bytes_len);
   if (rv == SECFailure) {
	return SECFailure;
   }
   testContext.isValid = PR_TRUE;
   return SECSuccess;
}

SECStatus
PRNGTEST_Reseed(const PRUint8 *entropy, unsigned int entropy_len, 
		  const PRUint8 *additional, unsigned int additional_len)
{
    if (!testContext.isValid) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
   

    if ((entropy == NULL) && (entropy_len == 0) && 
		(additional == NULL) && (additional_len == 0)) {
	testContext.reseed_counter[0] = RESEED_VALUE;
	return SECSuccess;
    }
    return prng_reseed(&testContext, entropy, entropy_len, additional,
			additional_len);

}

SECStatus
PRNGTEST_Generate(PRUint8 *bytes, unsigned int bytes_len, 
		  const PRUint8 *additional, unsigned int additional_len)
{
    SECStatus rv;
    if (!testContext.isValid) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    
    if (testContext.reseed_counter[0] >= RESEED_VALUE) {
	rv = prng_reseed(&testContext, NULL, 0, NULL, 0);
	if (rv != SECSuccess) {
	    return rv;
	}
    }
    return prng_generateNewBytes(&testContext, bytes, bytes_len,
			additional, additional_len);

}

SECStatus
PRNGTEST_Uninstantiate()
{
    if (!testContext.isValid) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
   PORT_Memset(&testContext, 0, sizeof testContext);
   return SECSuccess;
}

SECStatus
PRNGTEST_RunHealthTests()
{
   static const PRUint8 entropy[] = {
			0x8e,0x9c,0x0d,0x25,0x75,0x22,0x04,0xf9,
			0xc5,0x79,0x10,0x8b,0x23,0x79,0x37,0x14,
			0x9f,0x2c,0xc7,0x0b,0x39,0xf8,0xee,0xef,
			0x95,0x0c,0x97,0x59,0xfc,0x0a,0x85,0x41,
			0x76,0x9d,0x6d,0x67,0x00,0x4e,0x19,0x12,
			0x02,0x16,0x53,0xea,0xf2,0x73,0xd7,0xd6,
			0x7f,0x7e,0xc8,0xae,0x9c,0x09,0x99,0x7d,
			0xbb,0x9e,0x48,0x7f,0xbb,0x96,0x46,0xb3,
			0x03,0x75,0xf8,0xc8,0x69,0x45,0x3f,0x97,
			0x5e,0x2e,0x48,0xe1,0x5d,0x58,0x97,0x4c };
   static const PRUint8 rng_known_result[] = {
			0x16,0xe1,0x8c,0x57,0x21,0xd8,0xf1,0x7e,
			0x5a,0xa0,0x16,0x0b,0x7e,0xa6,0x25,0xb4,
			0x24,0x19,0xdb,0x54,0xfa,0x35,0x13,0x66,
			0xbb,0xaa,0x2a,0x1b,0x22,0x33,0x2e,0x4a,
			0x14,0x07,0x9d,0x52,0xfc,0x73,0x61,0x48,
			0xac,0xc1,0x22,0xfc,0xa4,0xfc,0xac,0xa4,
			0xdb,0xda,0x5b,0x27,0x33,0xc4,0xb3 };
   static const PRUint8 reseed_entropy[] = {
			0xc6,0x0b,0x0a,0x30,0x67,0x07,0xf4,0xe2,
			0x24,0xa7,0x51,0x6f,0x5f,0x85,0x3e,0x5d,
			0x67,0x97,0xb8,0x3b,0x30,0x9c,0x7a,0xb1,
			0x52,0xc6,0x1b,0xc9,0x46,0xa8,0x62,0x79 };
   static const PRUint8 additional_input[] = {
			0x86,0x82,0x28,0x98,0xe7,0xcb,0x01,0x14,
			0xae,0x87,0x4b,0x1d,0x99,0x1b,0xc7,0x41,
			0x33,0xff,0x33,0x66,0x40,0x95,0x54,0xc6,
			0x67,0x4d,0x40,0x2a,0x1f,0xf9,0xeb,0x65 };
   static const PRUint8 rng_reseed_result[] = {
			0x02,0x0c,0xc6,0x17,0x86,0x49,0xba,0xc4,
			0x7b,0x71,0x35,0x05,0xf0,0xdb,0x4a,0xc2,
			0x2c,0x38,0xc1,0xa4,0x42,0xe5,0x46,0x4a,
			0x7d,0xf0,0xbe,0x47,0x88,0xb8,0x0e,0xc6,
			0x25,0x2b,0x1d,0x13,0xef,0xa6,0x87,0x96,
			0xa3,0x7d,0x5b,0x80,0xc2,0x38,0x76,0x61,
			0xc7,0x80,0x5d,0x0f,0x05,0x76,0x85 };
   static const PRUint8 rng_no_reseed_result[] = {
			0xc4,0x40,0x41,0x8c,0xbf,0x2f,0x70,0x23,
			0x88,0xf2,0x7b,0x30,0xc3,0xca,0x1e,0xf3,
			0xef,0x53,0x81,0x5d,0x30,0xed,0x4c,0xf1,
			0xff,0x89,0xa5,0xee,0x92,0xf8,0xc0,0x0f,
			0x88,0x53,0xdf,0xb6,0x76,0xf0,0xaa,0xd3,
			0x2e,0x1d,0x64,0x37,0x3e,0xe8,0x4a,0x02,
			0xff,0x0a,0x7f,0xe5,0xe9,0x2b,0x6d };

   SECStatus rng_status = SECSuccess;
   PR_STATIC_ASSERT(sizeof(rng_known_result) >= sizeof(rng_reseed_result));
   PRUint8 result[sizeof(rng_known_result)];

   
   
   
   
   
   
   
   rng_status = PRNGTEST_Instantiate(entropy, 256/PR_BITS_PER_BYTE, 
				     NULL, 0, NULL, 0);
   if (rng_status == SECSuccess) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
   }
   if (PORT_GetError() != SEC_ERROR_NEED_RANDOM) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
   }
   

   
   
   
   rng_status = PRNGTEST_Instantiate(entropy, sizeof entropy, 
				     NULL, 0, NULL, 0);
   if (rng_status != SECSuccess) {
	
	return SECFailure;
   }
   rng_status = PRNGTEST_Generate(result, sizeof rng_known_result, NULL, 0);
   if ( ( rng_status != SECSuccess)  ||
        ( PORT_Memcmp( result, rng_known_result,
                       sizeof rng_known_result ) != 0 ) ) {
	PRNGTEST_Uninstantiate();
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
   }
   rng_status = PRNGTEST_Reseed(reseed_entropy, sizeof reseed_entropy,
				additional_input, sizeof additional_input);
   if (rng_status != SECSuccess) {
	
	PRNGTEST_Uninstantiate();
	return SECFailure;
   }
   rng_status = PRNGTEST_Generate(result, sizeof rng_reseed_result, NULL, 0);
   if ( ( rng_status != SECSuccess)  ||
        ( PORT_Memcmp( result, rng_reseed_result,
                       sizeof rng_reseed_result ) != 0 ) ) {
	PRNGTEST_Uninstantiate();
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
   }
   

   rng_status = PRNGTEST_Reseed(NULL, 0, NULL, 0);
   if (rng_status != SECSuccess) {
	PRNGTEST_Uninstantiate();
	
	return SECFailure;
   }
   
   rng_status = PRNGTEST_Generate(result, sizeof rng_reseed_result, NULL, 0);
   if ( ( rng_status != SECSuccess)  ||
	


        ( PORT_Memcmp( result, rng_no_reseed_result,
                       sizeof rng_no_reseed_result ) == 0 ) ) {
	PRNGTEST_Uninstantiate();
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
   }
   
   rng_status = PRNGTEST_Reseed(reseed_entropy, 4, NULL, 0);
   if (rng_status == SECSuccess) {
	PRNGTEST_Uninstantiate();
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
   }
   if (PORT_GetError() != SEC_ERROR_NEED_RANDOM) {
	PRNGTEST_Uninstantiate();
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
   }
   rng_status = PRNGTEST_Uninstantiate();
   if (rng_status != SECSuccess) {
	
	return rng_status;
   }
   

   rng_status = PRNGTEST_Uninstantiate();
   if (rng_status == SECSuccess) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
   }
   if (PORT_GetError() != SEC_ERROR_LIBRARY_FAILURE) {
	return rng_status;
   }
  
   return SECSuccess;
}
