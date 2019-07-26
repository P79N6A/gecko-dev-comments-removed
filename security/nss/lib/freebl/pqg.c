








#ifdef FREEBL_NO_DEPEND
#include "stubs.h"
#endif

#include "prerr.h"
#include "secerr.h"

#include "prtypes.h"
#include "blapi.h"
#include "secitem.h"
#include "mpi.h"
#include "mpprime.h"
#include "mplogic.h"
#include "secmpi.h"

#define MAX_ITERATIONS 1000  /* Maximum number of iterations of primegen */

typedef enum {
    FIPS186_1_TYPE,		
    FIPS186_3_TYPE,		
    FIPS186_3_ST_TYPE		
} pqgGenType;
















int prime_testcount_p(int L, int N)
{
    switch (L) {
    case 1024:
	return 40;
    case 2048:
	return 56;
    case 3072:
	return 64;
    default:
 	break;
    }
    return 50; 
}




int prime_testcount_q(int L, int N)
{
    return prime_testcount_p(L,N);
}






SECStatus static
pqg_validate_dsa2(unsigned int L, unsigned int N)
{

    switch (L) {
    case 1024:
	if (N != DSA1_Q_BITS) {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    return SECFailure;
	}
	break;
    case 2048:
	if ((N != 224) && (N != 256)) {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    return SECFailure;
	}
	break;
    case 3072:
	if (N != 256) {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    return SECFailure;
	}
	break;
    default:
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    return SECSuccess;
}




static HASH_HashType
getFirstHash(unsigned int L, unsigned int N)
{
    if (N < 224) {
	return HASH_AlgSHA1;
    }
    if (N < 256) {
	return HASH_AlgSHA224;
    }
    if (N < 384) {
	return HASH_AlgSHA256;
    }
    if (N < 512) {
	return HASH_AlgSHA384;
    }
    return HASH_AlgSHA512;
}




static HASH_HashType
getNextHash(HASH_HashType hashtype)
{
    switch (hashtype) {
    case HASH_AlgSHA1:
	hashtype = HASH_AlgSHA224;
	break;
    case HASH_AlgSHA224:
	hashtype = HASH_AlgSHA256;
	break;
    case HASH_AlgSHA256:
	hashtype = HASH_AlgSHA384;
	break;
    case HASH_AlgSHA384:
	hashtype = HASH_AlgSHA512;
	break;
    case HASH_AlgSHA512:
    default:
	hashtype = HASH_AlgTOTAL;
	break;
    }
    return hashtype;
}

static unsigned int
HASH_ResultLen(HASH_HashType type)
{
    const SECHashObject *hash_obj = HASH_GetRawHashObject(type);
    if (hash_obj == NULL) {
	return 0;
    }
    return hash_obj->length;
}

static SECStatus
HASH_HashBuf(HASH_HashType type, unsigned char *dest,
	     const unsigned char *src, PRUint32 src_len)
{
    const SECHashObject *hash_obj = HASH_GetRawHashObject(type);
    void *hashcx = NULL;
    unsigned int dummy;

    if (hash_obj == NULL) {
	return SECFailure;
    }

    hashcx = hash_obj->create();
    if (hashcx == NULL) {
	return SECFailure;
    }
    hash_obj->begin(hashcx);
    hash_obj->update(hashcx,src,src_len);
    hash_obj->end(hashcx,dest, &dummy, hash_obj->length);
    hash_obj->destroy(hashcx, PR_TRUE);
    return SECSuccess;
}

unsigned int
PQG_GetLength(const SECItem *obj)
{
    unsigned int len = obj->len;

    if (obj->data == NULL) {
	return 0;
    }
    if (len > 1 && obj->data[0] == 0) {
	len--;
    }
    return len;
}

SECStatus
PQG_Check(const PQGParams *params)
{
    unsigned int L,N;
    SECStatus rv = SECSuccess;

    if (params == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    L = PQG_GetLength(&params->prime)*BITS_PER_BYTE;
    N = PQG_GetLength(&params->subPrime)*BITS_PER_BYTE;

    if (L < 1024) {
	int j;

	
	if ( N != DSA1_Q_BITS ) {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    return SECFailure;
	}
	j = PQG_PBITS_TO_INDEX(L);
	if ( j >= 0 && j <= 8 ) {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    rv = SECFailure;
	}
    } else {
	
	rv = pqg_validate_dsa2(L, N);
    }
    return rv;
}

HASH_HashType
PQG_GetHashType(const PQGParams *params)
{
    unsigned int L,N;

    if (params == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    L = PQG_GetLength(&params->prime)*BITS_PER_BYTE;
    N = PQG_GetLength(&params->subPrime)*BITS_PER_BYTE;
    return getFirstHash(L, N);
}





static SECStatus
getPQseed(SECItem *seed, PRArenaPool* arena)
{
    SECStatus rv;

    if (!seed->data) {
        seed->data = (unsigned char*)PORT_ArenaZAlloc(arena, seed->len);
    }
    if (!seed->data) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
    }
    rv = RNG_GenerateGlobalRandomBytes(seed->data, seed->len);
    





    seed->data[0] |= 0x80;
    return rv;
}





static SECStatus
generate_h_candidate(SECItem *hit, mp_int *H)
{
    SECStatus rv = SECSuccess;
    mp_err   err = MP_OKAY;
#ifdef FIPS_186_1_A5_TEST
    memset(hit->data, 0, hit->len);
    hit->data[hit->len-1] = 0x02;
#else
    rv = RNG_GenerateGlobalRandomBytes(hit->data, hit->len);
#endif
    if (rv)
	return SECFailure;
    err = mp_read_unsigned_octets(H, hit->data, hit->len);
    if (err) {
	MP_TO_SEC_ERROR(err);
	return SECFailure;
    }
    return SECSuccess;
}

static SECStatus
addToSeed(const SECItem * seed,
          unsigned long   addend,
          int             seedlen, 
          SECItem * seedout)
{
    mp_int s, sum, modulus, tmp;
    mp_err    err = MP_OKAY;
    SECStatus rv  = SECSuccess;
    MP_DIGITS(&s)       = 0;
    MP_DIGITS(&sum)     = 0;
    MP_DIGITS(&modulus) = 0;
    MP_DIGITS(&tmp)     = 0;
    CHECK_MPI_OK( mp_init(&s) );
    CHECK_MPI_OK( mp_init(&sum) );
    CHECK_MPI_OK( mp_init(&modulus) );
    SECITEM_TO_MPINT(*seed, &s); 
    
    if (addend < MP_DIGIT_MAX) {
	CHECK_MPI_OK( mp_add_d(&s, (mp_digit)addend, &s) );
    } else {
	CHECK_MPI_OK( mp_init(&tmp) );
	CHECK_MPI_OK( mp_set_ulong(&tmp, addend) );
	CHECK_MPI_OK( mp_add(&s, &tmp, &s) );
    }
    
    CHECK_MPI_OK( mp_div_2d(&s, (mp_digit)seedlen, NULL, &sum) );
    if (seedout->data != NULL) {
	SECITEM_ZfreeItem(seedout, PR_FALSE);
    }
    MPINT_TO_SECITEM(&sum, seedout, NULL);
cleanup:
    mp_clear(&s);
    mp_clear(&sum);
    mp_clear(&modulus);
    mp_clear(&tmp);
    if (err) {
	MP_TO_SEC_ERROR(err);
	return SECFailure;
    }
    return rv;
}






static SECStatus
addToSeedThenHash(HASH_HashType   hashtype,
                  const SECItem * seed,
                  unsigned long   addend,
                  int             seedlen, 
                  unsigned char * hashOutBuf)
{
    SECItem str = { 0, 0, 0 };
    SECStatus rv;
    rv = addToSeed(seed, addend, seedlen, &str);
    if (rv != SECSuccess) {
	return rv;
    }
    rv = HASH_HashBuf(hashtype, hashOutBuf, str.data, str.len);
    if (str.data)
	SECITEM_ZfreeItem(&str, PR_FALSE);
    return rv;
}





static SECStatus
makeQfromSeed(
      unsigned int  g,          
const SECItem   *   seed,       
      mp_int    *   Q)          
{
    unsigned char sha1[SHA1_LENGTH];
    unsigned char sha2[SHA1_LENGTH];
    unsigned char U[SHA1_LENGTH];
    SECStatus rv  = SECSuccess;
    mp_err    err = MP_OKAY;
    int i;
    



    CHECK_SEC_OK( SHA1_HashBuf(sha1, seed->data, seed->len) );
    CHECK_SEC_OK( addToSeedThenHash(HASH_AlgSHA1, seed, 1, g, sha2) );
    for (i=0; i<SHA1_LENGTH; ++i) 
	U[i] = sha1[i] ^ sha2[i];
    





    U[0]             |= 0x80;  
    U[SHA1_LENGTH-1] |= 0x01;
    err = mp_read_unsigned_octets(Q, U, SHA1_LENGTH);
cleanup:
     memset(U, 0, SHA1_LENGTH);
     memset(sha1, 0, SHA1_LENGTH);
     memset(sha2, 0, SHA1_LENGTH);
     if (err) {
	MP_TO_SEC_ERROR(err);
	return SECFailure;
     }
     return rv;
}





static SECStatus
makeQ2fromSeed(
      HASH_HashType hashtype,	
      unsigned int  N,          
const SECItem   *   seed,       
      mp_int    *   Q)          
{
    unsigned char U[HASH_LENGTH_MAX];
    SECStatus rv  = SECSuccess;
    mp_err    err = MP_OKAY;
    int N_bytes = N/BITS_PER_BYTE; 
    int hashLen = HASH_ResultLen(hashtype);
    int offset = 0;

    



    CHECK_SEC_OK( HASH_HashBuf(hashtype, U, seed->data, seed->len) );
    

    if 	(hashLen > N_bytes) {
	offset = hashLen - N_bytes;
    }
    






    U[offset]    |= 0x80;  
    U[hashLen-1] |= 0x01;
    err = mp_read_unsigned_octets(Q, &U[offset], N_bytes);
cleanup:
     memset(U, 0, HASH_LENGTH_MAX);
     if (err) {
	MP_TO_SEC_ERROR(err);
	return SECFailure;
     }
     return rv;
}










#define MAX_ST_SEED_BITS HASH_LENGTH_MAX*BITS_PER_BYTE
SECStatus
makePrimefromPrimesShaweTaylor(
      HASH_HashType hashtype,	
      unsigned int  length,     
      mp_int    *   c0,         
      mp_int    *   q,          
      mp_int    *   prime,      
      SECItem   *   prime_seed, 
      int       *   prime_gen_counter) 
{
    mp_int c;
    mp_int c0_2;
    mp_int t;
    mp_int a;
    mp_int z;
    mp_int two_length_minus_1;
    SECStatus rv = SECFailure;
    int hashlen = HASH_ResultLen(hashtype);
    int outlen = hashlen*BITS_PER_BYTE;
    int offset;
    unsigned char bit, mask;
    


    unsigned char x[DSA_MAX_P_BITS/8+HASH_LENGTH_MAX];
    mp_err err = MP_OKAY;
    int i;
    int iterations;
    int old_counter;

    MP_DIGITS(&c) = 0;
    MP_DIGITS(&c0_2) = 0;
    MP_DIGITS(&t) = 0;
    MP_DIGITS(&a) = 0;
    MP_DIGITS(&z) = 0;
    MP_DIGITS(&two_length_minus_1) = 0;
    CHECK_MPI_OK( mp_init(&c) );
    CHECK_MPI_OK( mp_init(&c0_2) );
    CHECK_MPI_OK( mp_init(&t) );
    CHECK_MPI_OK( mp_init(&a) );
    CHECK_MPI_OK( mp_init(&z) );
    CHECK_MPI_OK( mp_init(&two_length_minus_1) );


    















    
    iterations = (length+outlen-1)/outlen;  
    
    old_counter = *prime_gen_counter;
    





    PORT_Memset(x, 0, sizeof(x));
    



    for (i=0; i < iterations; i++) {
	
	CHECK_SEC_OK( addToSeedThenHash(hashtype, prime_seed, i, 
		MAX_ST_SEED_BITS,&x[(iterations - i - 1)*hashlen]));
    }
    
    CHECK_SEC_OK(addToSeed(prime_seed, iterations, MAX_ST_SEED_BITS, 
					prime_seed));
    










    offset = (outlen*iterations - length)/BITS_PER_BYTE;
    

    bit = 1 << ((length-1) & 0x7); 
    
    mask = (bit-1);
    
    x[offset] = (mask & x[offset]) | bit;
    






    CHECK_MPI_OK( mp_read_unsigned_octets(&t, &x[offset], 
			hashlen*iterations - offset ) ); 
    CHECK_MPI_OK( mp_mul(c0, q, &c0_2) );        
    CHECK_MPI_OK( mp_add(&c0_2, &c0_2, &c0_2) ); 
    CHECK_MPI_OK( mp_add(&t, &c0_2, &t) );       
    CHECK_MPI_OK( mp_sub_d(&t, (mp_digit) 1, &t) ); 
    
    CHECK_MPI_OK( mp_div(&t, &c0_2, &t, NULL) );
    






    CHECK_MPI_OK( mp_2expt(&two_length_minus_1, length-1) );
step_23:
    CHECK_MPI_OK( mp_mul(&t, &c0_2, &c) );               
    CHECK_MPI_OK( mp_add_d(&c, (mp_digit)1, &c) );       
    if (mpl_significant_bits(&c) > length) {     
	    CHECK_MPI_OK( mp_sub_d(&c0_2, (mp_digit) 1, &t) ); 
	    
	    CHECK_MPI_OK( mp_add(&two_length_minus_1,&t, &t) );
	    

	    CHECK_MPI_OK( mp_div(&t, &c0_2, &t, NULL) );
	    CHECK_MPI_OK( mp_mul(&t, &c0_2, &c) );         
	    CHECK_MPI_OK( mp_add_d(&c, (mp_digit)1, &c) );  
    }
    
    (*prime_gen_counter)++;
    





    PORT_Memset(x, 0, sizeof(x));    
    





    for (i=0; i < iterations; i++) {
	
	CHECK_SEC_OK(addToSeedThenHash(hashtype, prime_seed, i, 
			MAX_ST_SEED_BITS,&x[(iterations - i - 1)*hashlen]));
    }
    
    CHECK_SEC_OK(addToSeed(prime_seed, iterations, MAX_ST_SEED_BITS, 
					prime_seed));
    
    CHECK_MPI_OK( mp_read_unsigned_octets(&a, x, iterations*hashlen) );
    CHECK_MPI_OK( mp_sub_d(&c, (mp_digit) 3, &z) ); 
    CHECK_MPI_OK( mp_mod(&a, &z, &a) );             
    CHECK_MPI_OK( mp_add_d(&a, (mp_digit) 2, &a) ); 
    



    CHECK_MPI_OK( mp_mul(&t, q, &z) );              
    CHECK_MPI_OK( mp_add(&z, &z, &z) );             
    CHECK_MPI_OK( mp_exptmod(&a, &z, &c, &z) );     
    



    CHECK_MPI_OK( mp_sub_d(&z, (mp_digit) 1, &a) );
    CHECK_MPI_OK( mp_gcd(&a,&c,&a ));
    if (mp_cmp_d(&a, (mp_digit)1) == 0) {
	CHECK_MPI_OK( mp_exptmod(&z, c0, &c, &a) );
	if (mp_cmp_d(&a, (mp_digit)1) == 0) {
	    
	    CHECK_MPI_OK( mp_copy(&c, prime) );
	    



	    rv = SECSuccess;
	    goto cleanup;
	}
    }
    





    if (*prime_gen_counter < (4*length + old_counter)) {
	
	CHECK_MPI_OK( mp_add_d(&t, (mp_digit) 1, &t) );
	
	goto step_23;
    }

    
    rv = SECFailure; 
	
cleanup:
    mp_clear(&c);
    mp_clear(&c0_2);
    mp_clear(&t);
    mp_clear(&a);
    mp_clear(&z);
    mp_clear(&two_length_minus_1);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    if (rv == SECFailure) {
	mp_zero(prime);
	if (prime_seed->data) {
	    SECITEM_FreeItem(prime_seed, PR_FALSE);
	}
	*prime_gen_counter = 0;
    }
    return rv;
}






SECStatus
makePrimefromSeedShaweTaylor(
      HASH_HashType hashtype,	
      unsigned int  length,     
const SECItem   *   input_seed,       
      mp_int    *   prime,      
      SECItem   *   prime_seed, 
      int       *   prime_gen_counter) 
{
    mp_int c;
    mp_int c0;
    mp_int one;
    SECStatus rv = SECFailure;
    int hashlen = HASH_ResultLen(hashtype);
    int outlen = hashlen*BITS_PER_BYTE;
    int offset;
    unsigned char bit, mask;
    unsigned char x[HASH_LENGTH_MAX*2];
    mp_digit dummy;
    mp_err err = MP_OKAY;
    int i;

    MP_DIGITS(&c) = 0;
    MP_DIGITS(&c0) = 0;
    MP_DIGITS(&one) = 0;
    CHECK_MPI_OK( mp_init(&c) );
    CHECK_MPI_OK( mp_init(&c0) );
    CHECK_MPI_OK( mp_init(&one) );

    
    if (length < 2) {
	rv = SECFailure;
	goto cleanup;
    }
    
    if (length >= 33) {
	mp_zero(&one);
	CHECK_MPI_OK( mp_add_d(&one, (mp_digit) 1, &one) );

	


	rv = makePrimefromSeedShaweTaylor(hashtype, (length+1)/2+1,
			input_seed, &c0, prime_seed, prime_gen_counter);
	
	if (rv != SECSuccess) {
	    goto cleanup;
	}
	
	rv = makePrimefromPrimesShaweTaylor(hashtype,length, &c0, &one,
		prime, prime_seed, prime_gen_counter);
	goto cleanup; 
    }
    
    CHECK_SEC_OK(SECITEM_CopyItem(NULL, prime_seed, input_seed));
    
    *prime_gen_counter = 0;

step_5:
    
    CHECK_SEC_OK(HASH_HashBuf(hashtype, x, prime_seed->data, prime_seed->len) );
    CHECK_SEC_OK(addToSeedThenHash(hashtype, prime_seed, 1, 
					MAX_ST_SEED_BITS, &x[hashlen]) );
    for (i=0; i < hashlen; i++) {
	x[i] = x[i] ^ x[i+hashlen];
    }
    
    







    offset = (outlen - length)/BITS_PER_BYTE;
    

    bit = 1 << ((length-1) & 0x7); 
    
    mask = (bit-1);
    
    x[offset] = (mask & x[offset]) | bit;
    
    
    x[hashlen-1] |= 1;
    
    CHECK_MPI_OK( mp_read_unsigned_octets(&c, &x[offset], hashlen-offset) );
    
    (*prime_gen_counter)++;
    
    CHECK_SEC_OK(addToSeed(prime_seed, 2, MAX_ST_SEED_BITS, prime_seed));
    






    if (prime_tab[prime_tab_size-1] < 0xFFF1) {
 	

	rv = SECFailure;
	goto cleanup;
    }
    dummy = prime_tab_size;
    err = mpp_divis_primes(&c, &dummy);
    
    if (err == MP_NO) {
	
	CHECK_MPI_OK( mp_copy(&c, prime) );
	
	err = MP_OKAY;
	rv = SECSuccess;
	goto cleanup;
    } else if (err != MP_YES) {
	goto cleanup;  
    } else {
	
	err = MP_OKAY;
    }
    




    if (*prime_gen_counter <= (4*length)) {
	goto step_5;
    }
    
    rv = SECFailure; 
	
cleanup:
    mp_clear(&c);
    mp_clear(&c0);
    mp_clear(&one);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    if (rv == SECFailure) {
	mp_zero(prime);
	if (prime_seed->data) {
	    SECITEM_FreeItem(prime_seed, PR_FALSE);
	}
	*prime_gen_counter = 0;
    }
    return rv;
}





static SECStatus
findQfromSeed(
      unsigned int  L,          
      unsigned int  N,          
      unsigned int  g,          
const SECItem   *   seed,       
      mp_int    *   Q,          
      mp_int    *   Q_,         
      int       *  qseed_len,   
      HASH_HashType *hashtypePtr,  
      pqgGenType    *typePtr)      
{
    HASH_HashType hashtype;
    SECItem  firstseed = { 0, 0, 0 };
    SECItem  qseed = { 0, 0, 0 };
    SECStatus rv;

    *qseed_len = 0; 

    
    if (L < 1024) {
	rv =makeQfromSeed(g,seed,Q_);
	if ((rv == SECSuccess) && (mp_cmp(Q,Q_) == 0)) {
	    *hashtypePtr = HASH_AlgSHA1;
	    *typePtr = FIPS186_1_TYPE;
	    return SECSuccess;
	}
	return SECFailure;
    } 
    

    if (L == 1024) {
	rv = makeQfromSeed(g,seed,Q_);
	if (rv == SECSuccess) {
	    if (mp_cmp(Q,Q_) == 0) {
		*hashtypePtr = HASH_AlgSHA1;
		*typePtr = FIPS186_1_TYPE;
		return SECSuccess;
	    }
	}
	
    }
    

    for (hashtype = getFirstHash(L,N); hashtype != HASH_AlgTOTAL; 
					hashtype=getNextHash(hashtype)) {
	rv = makeQ2fromSeed(hashtype, N, seed, Q_);
	if (rv != SECSuccess) {
	    continue;
	}
	if (mp_cmp(Q,Q_) == 0) {
	    *hashtypePtr = hashtype;
	    *typePtr = FIPS186_3_TYPE;
	    return SECSuccess;
	}
    }
    


    firstseed = *seed;
    firstseed.len = seed->len/3;
    for (hashtype = getFirstHash(L,N); hashtype != HASH_AlgTOTAL; 
					hashtype=getNextHash(hashtype)) {
	int count;

	rv = makePrimefromSeedShaweTaylor(hashtype, N, &firstseed, Q_, 
		&qseed, &count);
	if (rv != SECSuccess) {
	    continue;
	}
	if (mp_cmp(Q,Q_) == 0) {
	    
	    int offset = seed->len - qseed.len;
	    if ((offset < 0) || 
	       (PORT_Memcmp(&seed->data[offset],qseed.data,qseed.len) != 0)) {
		


		SECITEM_FreeItem(&qseed,PR_FALSE);
		return SECFailure;
	    }
	    *qseed_len = qseed.len;
	    *hashtypePtr = hashtype;
	    *typePtr = FIPS186_3_ST_TYPE;
	    SECITEM_FreeItem(&qseed, PR_FALSE);
	    return SECSuccess;
	}
	SECITEM_FreeItem(&qseed, PR_FALSE);
    }
    
    return SECFailure;
}
	







static SECStatus
makePfromQandSeed(
      HASH_HashType hashtype,	
      unsigned int  L,          
      unsigned int  N,          
      unsigned int  offset,     
      unsigned int  seedlen,    
const SECItem   *   seed,       
const mp_int    *   Q,          
      mp_int    *   P)          
{
    unsigned int  j;            
    unsigned int  n;            
    mp_digit      b;            
    unsigned int outlen;        
    unsigned int hashlen;       
    unsigned char V_j[HASH_LENGTH_MAX];
    mp_int        W, X, c, twoQ, V_n, tmp;
    mp_err    err = MP_OKAY;
    SECStatus rv  = SECSuccess;
    
    MP_DIGITS(&W)     = 0;
    MP_DIGITS(&X)     = 0;
    MP_DIGITS(&c)     = 0;
    MP_DIGITS(&twoQ)  = 0;
    MP_DIGITS(&V_n)   = 0;
    MP_DIGITS(&tmp)   = 0;
    CHECK_MPI_OK( mp_init(&W)    );
    CHECK_MPI_OK( mp_init(&X)    );
    CHECK_MPI_OK( mp_init(&c)    );
    CHECK_MPI_OK( mp_init(&twoQ) );
    CHECK_MPI_OK( mp_init(&tmp)  );
    CHECK_MPI_OK( mp_init(&V_n)  );

    hashlen = HASH_ResultLen(hashtype);
    outlen = hashlen*BITS_PER_BYTE; 

    
    n = (L - 1) / outlen;
    b = (L - 1) % outlen;

    








    for (j=0; j<n; ++j) { 
	


	CHECK_SEC_OK( addToSeedThenHash(hashtype,seed,offset+j, seedlen, V_j) );
	


	OCTETS_TO_MPINT(V_j, &tmp, hashlen);          
	CHECK_MPI_OK( mpl_lsh(&tmp, &tmp, j*outlen) );
	CHECK_MPI_OK( mp_add(&W, &tmp, &W) );         
    }
    


    CHECK_SEC_OK( addToSeedThenHash(hashtype, seed, offset + n, seedlen, V_j) );
    OCTETS_TO_MPINT(V_j, &V_n, hashlen);          
    CHECK_MPI_OK( mp_div_2d(&V_n, b, NULL, &tmp) ); 
    CHECK_MPI_OK( mpl_lsh(&tmp, &tmp, n*outlen) );  
    CHECK_MPI_OK( mp_add(&W, &tmp, &W) );           
    



    CHECK_MPI_OK( mpl_set_bit(&X, (mp_size)(L-1), 1) );    
    CHECK_MPI_OK( mp_add(&X, &W, &X) );                    
    



    CHECK_MPI_OK( mp_mul_2(Q, &twoQ) );                    
    CHECK_MPI_OK( mp_mod(&X, &twoQ, &c) );                 
    




    CHECK_MPI_OK( mp_sub_d(&c, 1, &c) );                   
    CHECK_MPI_OK( mp_sub(&X, &c, P) );                     
cleanup:
    mp_clear(&W);
    mp_clear(&X);
    mp_clear(&c);
    mp_clear(&twoQ);
    mp_clear(&V_n);
    mp_clear(&tmp);
    if (err) {
	MP_TO_SEC_ERROR(err);
	return SECFailure;
    }
    return rv;
}




static SECStatus
makeGfromH(const mp_int *P,     
           const mp_int *Q,     
                 mp_int *H,     
                 mp_int *G,     
                 PRBool *passed)
{
    mp_int exp, pm1;
    mp_err err = MP_OKAY;
    SECStatus rv = SECSuccess;
    *passed = PR_FALSE;
    MP_DIGITS(&exp) = 0;
    MP_DIGITS(&pm1) = 0;
    CHECK_MPI_OK( mp_init(&exp) );
    CHECK_MPI_OK( mp_init(&pm1) );
    CHECK_MPI_OK( mp_sub_d(P, 1, &pm1) );        
    if ( mp_cmp(H, &pm1) >= 0)                   
	CHECK_MPI_OK( mp_sub(H, &pm1, H) );      
    



    
    if (mp_cmp_d(H, 1) <= 0) {
	rv = SECFailure;
	goto cleanup;
    }
    
    CHECK_MPI_OK( mp_div(&pm1, Q, &exp, NULL) );  
    CHECK_MPI_OK( mp_exptmod(H, &exp, P, G) );    
    
    if (mp_cmp_d(G, 1) <= 0) {
	rv = SECFailure;
	goto cleanup;
    }
    *passed = PR_TRUE;
cleanup:
    mp_clear(&exp);
    mp_clear(&pm1);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    return rv;
}




static SECStatus
makeGfromIndex(HASH_HashType hashtype,
		const mp_int *P,	
           	const mp_int *Q,	
                const SECItem *seed,	
		unsigned char index,	
		mp_int *G)		
{
    mp_int e, pm1, W;
    unsigned int count;
    unsigned char data[HASH_LENGTH_MAX];
    unsigned int len;
    mp_err err = MP_OKAY;
    SECStatus rv = SECSuccess;
    const SECHashObject *hashobj;
    void *hashcx = NULL;

    MP_DIGITS(&e) = 0;
    MP_DIGITS(&pm1) = 0;
    MP_DIGITS(&W) = 0;
    CHECK_MPI_OK( mp_init(&e) );
    CHECK_MPI_OK( mp_init(&pm1) );
    CHECK_MPI_OK( mp_init(&W) );

    
    hashobj = HASH_GetRawHashObject(hashtype);
    if (hashobj == NULL) {
	
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	rv = SECFailure;
	goto cleanup;
    }
    hashcx = hashobj->create();
    if (hashcx == NULL) {
	rv = SECFailure;
	goto cleanup;
    }

    CHECK_MPI_OK( mp_sub_d(P, 1, &pm1) );        
    
    CHECK_MPI_OK( mp_div(&pm1, Q, &e, NULL) );  
    
    

#define MAX_COUNT 0x10000
    for (count = 1; count < MAX_COUNT; count++) {
	




	hashobj->begin(hashcx);
	hashobj->update(hashcx,seed->data,seed->len);
	hashobj->update(hashcx, (unsigned char *)"ggen", 4);
	hashobj->update(hashcx,&index, 1);
	data[0] = (count >> 8) & 0xff;
	data[1] = count & 0xff;
	hashobj->update(hashcx, data, 2);
	hashobj->end(hashcx, data, &len, sizeof(data));
	OCTETS_TO_MPINT(data, &W, len);
	
	CHECK_MPI_OK( mp_exptmod(&W, &e, P, G) );
	
	

    	if (mp_cmp_d(G, 2) < 0) {
	     continue;
	}
	break; 
    }
    if (count >= MAX_COUNT) { 
	rv = SECFailure; 
    }
    

cleanup:
    PORT_Memset(data, 0, sizeof(data));
    if (hashcx) {
	hashobj->destroy(hashcx, PR_TRUE);
    }
    mp_clear(&e);
    mp_clear(&pm1);
    mp_clear(&W);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    return rv;
}






static SECStatus
pqg_ParamGen(unsigned int L, unsigned int N, pqgGenType type,
	 unsigned int seedBytes, PQGParams **pParams, PQGVerify **pVfy)
{
    unsigned int  n;        
    unsigned int  b;        
    unsigned int  seedlen;  
    unsigned int  counter;  
    unsigned int  offset;   
    unsigned int  outlen;   
    unsigned int  maxCount;
    HASH_HashType hashtype;
    SECItem      *seed;     
    PRArenaPool  *arena  = NULL;
    PQGParams    *params = NULL;
    PQGVerify    *verify = NULL;
    PRBool passed;
    SECItem hit = { 0, 0, 0 };
    mp_int P, Q, G, H, l;
    mp_err    err = MP_OKAY;
    SECStatus rv  = SECFailure;
    int iterations = 0;


    
    
    if (seedBytes < N/BITS_PER_BYTE || !pParams || !pVfy) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    
    arena = PORT_NewArena(NSS_FREEBL_DEFAULT_CHUNKSIZE);
    if (!arena) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
    }
    params = (PQGParams *)PORT_ArenaZAlloc(arena, sizeof(PQGParams));
    if (!params) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	PORT_FreeArena(arena, PR_TRUE);
	return SECFailure;
    }
    params->arena = arena;
    
    arena = PORT_NewArena(NSS_FREEBL_DEFAULT_CHUNKSIZE);
    if (!arena) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	PORT_FreeArena(params->arena, PR_TRUE);
	return SECFailure;
    }
    verify = (PQGVerify *)PORT_ArenaZAlloc(arena, sizeof(PQGVerify));
    if (!verify) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	PORT_FreeArena(arena, PR_TRUE);
	PORT_FreeArena(params->arena, PR_TRUE);
	return SECFailure;
    }
    verify->arena = arena;
    seed = &verify->seed;
    arena = NULL;
    
    MP_DIGITS(&P) = 0;
    MP_DIGITS(&Q) = 0;
    MP_DIGITS(&G) = 0;
    MP_DIGITS(&H) = 0;
    MP_DIGITS(&l) = 0;
    CHECK_MPI_OK( mp_init(&P) );
    CHECK_MPI_OK( mp_init(&Q) );
    CHECK_MPI_OK( mp_init(&G) );
    CHECK_MPI_OK( mp_init(&H) );
    CHECK_MPI_OK( mp_init(&l) );

    
    

    hashtype = getFirstHash(L,N);
    outlen = HASH_ResultLen(hashtype)*BITS_PER_BYTE;

    
    n = (L - 1) / outlen; 
    
    b = (L - 1) % outlen;
    seedlen = seedBytes * BITS_PER_BYTE;    
step_5:
    




    if (++iterations > MAX_ITERATIONS) {        
        PORT_SetError(SEC_ERROR_NEED_RANDOM);
        goto cleanup;
    }
    seed->len = seedBytes;
    CHECK_SEC_OK( getPQseed(seed, verify->arena) );
    














    if (type == FIPS186_1_TYPE) {
	CHECK_SEC_OK( makeQfromSeed(seedlen, seed, &Q) );
    } else {
	CHECK_SEC_OK( makeQ2fromSeed(hashtype, N, seed, &Q) );
    }
    






    
    err = mpp_pprime(&Q, prime_testcount_q(L,N));
    passed = (err == MP_YES) ? SECSuccess : SECFailure;
    


    if (passed != SECSuccess)
        goto step_5;
    




    offset = (type == FIPS186_1_TYPE) ? 2 : 1;
    




    maxCount = L >= 1024 ? (4*L - 1) : 4095;
    for (counter = 0; counter <= maxCount; counter++) {
	


















	CHECK_SEC_OK( makePfromQandSeed(hashtype, L, N, offset, seedlen, 
					seed, &Q, &P) );
	



	CHECK_MPI_OK( mpl_set_bit(&l, (mp_size)(L-1), 1) ); 
	if (mp_cmp(&P, &l) < 0)
            goto step_11_9;
	



	
	err = mpp_pprime(&P, prime_testcount_p(L, N));
	passed = (err == MP_YES) ? SECSuccess : SECFailure;
	



	if (passed == SECSuccess)
	    break;
step_11_9:
	


	offset += n + 1;
    }
    





    if (counter > maxCount) 
	     goto step_5;
    


    if (type == FIPS186_1_TYPE) {
	


	SECITEM_AllocItem(NULL, &hit, L/8); 
	if (!hit.data) goto cleanup;
 	do {
	    
	    CHECK_SEC_OK( generate_h_candidate(&hit, &H) );
            CHECK_SEC_OK( makeGfromH(&P, &Q, &H, &G, &passed) );
	} while (passed != PR_TRUE);
        MPINT_TO_SECITEM(&H, &verify->h,        verify->arena);
    } else {
	unsigned char index = 1; 
	verify->h.data = (unsigned char *)PORT_ArenaZAlloc(verify->arena, 1);
	if (verify->h.data == NULL) { goto cleanup; }
	verify->h.len = 1;
	verify->h.data[0] = index;
	
	CHECK_SEC_OK(makeGfromIndex(hashtype, &P, &Q, seed, index, &G) );
    }
    
    MPINT_TO_SECITEM(&P, &params->prime,    params->arena);
    MPINT_TO_SECITEM(&Q, &params->subPrime, params->arena);
    MPINT_TO_SECITEM(&G, &params->base,     params->arena);
    verify->counter = counter;
    *pParams = params;
    *pVfy = verify;
cleanup:
    mp_clear(&P);
    mp_clear(&Q);
    mp_clear(&G);
    mp_clear(&H);
    mp_clear(&l);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    if (rv) {
	PORT_FreeArena(params->arena, PR_TRUE);
	PORT_FreeArena(verify->arena, PR_TRUE);
    }
    if (hit.data) {
        SECITEM_FreeItem(&hit, PR_FALSE);
    }
    return rv;
}

SECStatus
PQG_ParamGen(unsigned int j, PQGParams **pParams, PQGVerify **pVfy)
{
    unsigned int L;            
    unsigned int seedBytes;

    if (j > 8 || !pParams || !pVfy) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    L = 512 + (j * 64);         
    seedBytes = L/8;
    return pqg_ParamGen(L, DSA1_Q_BITS, FIPS186_1_TYPE, seedBytes, 
                        pParams, pVfy);
}

SECStatus
PQG_ParamGenSeedLen(unsigned int j, unsigned int seedBytes,
                    PQGParams **pParams, PQGVerify **pVfy)
{
    unsigned int L;            

    if (j > 8 || !pParams || !pVfy) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    L = 512 + (j * 64);         
    return pqg_ParamGen(L, DSA1_Q_BITS, FIPS186_1_TYPE, seedBytes,
                        pParams, pVfy);
}

SECStatus
PQG_ParamGenV2(unsigned int L, unsigned int N, unsigned int seedBytes,
                    PQGParams **pParams, PQGVerify **pVfy)
{
    if (pqg_validate_dsa2(L,N) != SECSuccess) {
	
	return SECFailure;
    }
    return pqg_ParamGen(L, N, FIPS186_3_TYPE, seedBytes, pParams, pVfy);
}







SECStatus   
PQG_VerifyParams(const PQGParams *params, 
                 const PQGVerify *vfy, SECStatus *result)
{
    SECStatus rv = SECSuccess;
    unsigned int g, n, L, N, offset, outlen;
    mp_int p0, P, Q, G, P_, Q_, G_, r, h;
    mp_err err = MP_OKAY;
    int j;
    unsigned int counter_max = 0; 
    int qseed_len;
    SECItem pseed_ = {0, 0, 0};
    HASH_HashType hashtype;
    pqgGenType type;

#define CHECKPARAM(cond)      \
    if (!(cond)) {            \
	*result = SECFailure; \
	goto cleanup;         \
    }
    if (!params || !vfy || !result) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    
    if ((params->prime.len == 0) || (params->subPrime.len == 0) ||
        (vfy->seed.len == 0)) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    

    if ((params->base.len == 0) && (vfy->counter == -1)) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    MP_DIGITS(&p0) = 0;
    MP_DIGITS(&P) = 0;
    MP_DIGITS(&Q) = 0;
    MP_DIGITS(&G) = 0;
    MP_DIGITS(&P_) = 0;
    MP_DIGITS(&Q_) = 0;
    MP_DIGITS(&G_) = 0;
    MP_DIGITS(&r) = 0;
    MP_DIGITS(&h) = 0;
    CHECK_MPI_OK( mp_init(&p0) );
    CHECK_MPI_OK( mp_init(&P) );
    CHECK_MPI_OK( mp_init(&Q) );
    CHECK_MPI_OK( mp_init(&G) );
    CHECK_MPI_OK( mp_init(&P_) );
    CHECK_MPI_OK( mp_init(&Q_) );
    CHECK_MPI_OK( mp_init(&G_) );
    CHECK_MPI_OK( mp_init(&r) );
    CHECK_MPI_OK( mp_init(&h) );
    *result = SECSuccess;
    SECITEM_TO_MPINT(params->prime,    &P);
    SECITEM_TO_MPINT(params->subPrime, &Q);
    
    if (params->base.len != 0) {
	SECITEM_TO_MPINT(params->base,     &G);
    }
    
    N = mpl_significant_bits(&Q);
    L = mpl_significant_bits(&P);
    if (L < 1024) {
	
	CHECKPARAM( N == DSA1_Q_BITS );
	j = PQG_PBITS_TO_INDEX(L);
	CHECKPARAM( j >= 0 && j <= 8 );
	counter_max = 4096;
    } else {
	
	CHECKPARAM(pqg_validate_dsa2(L, N) == SECSuccess);
	counter_max = 4*L;
    }
    
    if (params->base.len != 0) {
	CHECKPARAM( mp_cmp(&G, &P) < 0 );
    }
    
    CHECK_MPI_OK( mp_mod(&P, &Q, &r) );
    CHECKPARAM( mp_cmp_d(&r, 1) == 0 );
    
    CHECKPARAM( mpp_pprime(&Q, prime_testcount_q(L,N)) == MP_YES );
    
    CHECKPARAM( mpp_pprime(&P, prime_testcount_p(L,N)) == MP_YES );
    
    
    
    CHECKPARAM( (vfy->counter == -1) || (vfy->counter < counter_max) );
    
    g = vfy->seed.len * 8;
    CHECKPARAM( g >= N && g < counter_max/2 );
    
    

    CHECKPARAM( findQfromSeed(L, N, g, &vfy->seed, &Q, &Q_, &qseed_len,
					&hashtype, &type) == SECSuccess );
    CHECKPARAM( mp_cmp(&Q, &Q_) == 0 );
    if (type == FIPS186_3_ST_TYPE) {
	SECItem qseed = { 0, 0, 0 };
	SECItem pseed = { 0, 0, 0 };
	int first_seed_len;
	int pgen_counter = 0;

	


















	first_seed_len = vfy->seed.len/3;
	CHECKPARAM(qseed_len < vfy->seed.len);
	CHECKPARAM(first_seed_len*8 > N-1);
	CHECKPARAM(first_seed_len+qseed_len < vfy->seed.len);
	qseed.len = qseed_len;
	qseed.data = vfy->seed.data + vfy->seed.len - qseed.len;
	pseed.len = vfy->seed.len - (first_seed_len+qseed_len);
	pseed.data = vfy->seed.data + first_seed_len;

	




	


	CHECK_SEC_OK( makePrimefromSeedShaweTaylor(hashtype, (L+1)/2+1,
			&qseed, &p0, &pseed_, &pgen_counter) );
	
	CHECK_SEC_OK( makePrimefromPrimesShaweTaylor(hashtype, L, 
		&p0, &Q_, &P_, &pseed_, &pgen_counter) );
	CHECKPARAM( mp_cmp(&P, &P_) == 0 );
	

	CHECKPARAM( SECITEM_CompareItem(&pseed, &pseed_) == SECEqual );
    } else if (vfy->counter == -1) {
	

	CHECKPARAM(type != FIPS186_1_TYPE); 
    } else {
	

	outlen = HASH_ResultLen(hashtype)*BITS_PER_BYTE;
	n = (L - 1) / outlen;
	offset = vfy->counter * (n + 1) + ((type == FIPS186_1_TYPE) ? 2 : 1);
	CHECK_SEC_OK( makePfromQandSeed(hashtype, L, N, offset, g, &vfy->seed, 
				    	&Q, &P_) );
	CHECKPARAM( mp_cmp(&P, &P_) == 0 );
    }

    
    if (params->base.len == 0) goto cleanup;

    
    
    
    CHECK_MPI_OK( mpl_set_bit(&P, 0, 0) );
    CHECKPARAM( mp_cmp_d(&G, 2) > 0 && mp_cmp(&G, &P) < 0 );
    CHECK_MPI_OK( mpl_set_bit(&P, 0, 1) ); 
    
    CHECK_MPI_OK( mp_exptmod(&G, &Q, &P, &h) );    
    CHECKPARAM(mp_cmp_d(&h, 1) == 0);

    
    if (vfy->h.len == 0) {
	if (type != FIPS186_1_TYPE) {
	    *result = SECWouldBlock;
	}
	goto cleanup;
    }

    





    if ((vfy->h.len == 1) && (type != FIPS186_1_TYPE)) {
	
	CHECK_SEC_OK(makeGfromIndex(hashtype, &P, &Q, &vfy->seed,
				 vfy->h.data[0], &G_) );
	CHECKPARAM( mp_cmp(&G, &G_) == 0 );
    } else {
	int passed;
	
	SECITEM_TO_MPINT(vfy->h, &h);
	
	
	CHECK_MPI_OK( mpl_set_bit(&P, 0, 0) );
	CHECKPARAM( mp_cmp_d(&G, 2) > 0 && mp_cmp(&G, &P) );
	CHECK_MPI_OK( mpl_set_bit(&P, 0, 1) ); 
	
 	CHECK_SEC_OK( makeGfromH(&P, &Q, &h, &G_, &passed) );
	CHECKPARAM( passed && mp_cmp(&G, &G_) == 0 );
    }
cleanup:
    mp_clear(&p0);
    mp_clear(&P);
    mp_clear(&Q);
    mp_clear(&G);
    mp_clear(&P_);
    mp_clear(&Q_);
    mp_clear(&G_);
    mp_clear(&r);
    mp_clear(&h);
    if (pseed_.data) {
	SECITEM_FreeItem(&pseed_,PR_FALSE);
    }
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    return rv;
}




void
PQG_DestroyParams(PQGParams *params)
{
    if (params == NULL) 
    	return;
    if (params->arena != NULL) {
	PORT_FreeArena(params->arena, PR_FALSE);	
    } else {
	SECITEM_FreeItem(&params->prime,    PR_FALSE); 
	SECITEM_FreeItem(&params->subPrime, PR_FALSE); 
	SECITEM_FreeItem(&params->base,     PR_FALSE); 
	PORT_Free(params);
    }
}





void
PQG_DestroyVerify(PQGVerify *vfy)
{
    if (vfy == NULL) 
    	return;
    if (vfy->arena != NULL) {
	PORT_FreeArena(vfy->arena, PR_FALSE);	
    } else {
	SECITEM_FreeItem(&vfy->seed,   PR_FALSE); 
	SECITEM_FreeItem(&vfy->h,      PR_FALSE); 
	PORT_Free(vfy);
    }
}
