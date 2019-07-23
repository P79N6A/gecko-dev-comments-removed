






































#ifdef FREEBL_NO_DEPEND
#include "stubs.h"
#endif

#include "prerror.h"
#include "secerr.h"

#include "prtypes.h"
#include "prinit.h"
#include "blapi.h"
#include "nssilock.h"
#include "secitem.h"
#include "blapi.h"
#include "mpi.h"
#include "secmpi.h"

 
#define NSS_FREEBL_DSA_DEFAULT_CHUNKSIZE 2048

#define FIPS_DSA_Q     160
#define QSIZE      (FIPS_DSA_Q / PR_BITS_PER_BYTE)









SECStatus
FIPS186Change_ReduceModQForDSA(const PRUint8 *w,
                               const PRUint8 *q,
                               PRUint8 *xj)
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
    


    CHECK_MPI_OK( mp_read_unsigned_octets(&W, w, 2*QSIZE) );
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









SECStatus
FIPS186Change_GenerateX(PRUint8 *XKEY, const PRUint8 *XSEEDj,
                        PRUint8 *x_j)
{
    PORT_SetError(PR_NOT_IMPLEMENTED_ERROR);
    return SECFailure;
}















static SECStatus 
dsa_GenerateGlobalRandomBytes(void *dest, size_t len, const PRUint8 *q)
{
    SECStatus rv;
    PRUint8 w[2*QSIZE];

    PORT_Assert(q && len == DSA_SUBPRIME_LEN);
    if (len != DSA_SUBPRIME_LEN) {
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	return SECFailure;
    }
    if (*q == 0) {
        ++q;
    }
    rv = RNG_GenerateGlobalRandomBytes(w, 2*QSIZE);
    if (rv != SECSuccess) {
	return rv;
    }
    FIPS186Change_ReduceModQForDSA(w, q, (PRUint8 *)dest);
    return rv;
}

static void translate_mpi_error(mp_err err)
{
    MP_TO_SEC_ERROR(err);
}

SECStatus 
dsa_NewKey(const PQGParams *params, DSAPrivateKey **privKey, 
           const unsigned char *xb)
{
    mp_int p, g;
    mp_int x, y;
    mp_err err;
    PRArenaPool *arena;
    DSAPrivateKey *key;
    
    if (!params || !privKey) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    
    arena = PORT_NewArena(NSS_FREEBL_DSA_DEFAULT_CHUNKSIZE);
    if (!arena) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
    }
    key = (DSAPrivateKey *)PORT_ArenaZAlloc(arena, sizeof(DSAPrivateKey));
    if (!key) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	PORT_FreeArena(arena, PR_TRUE);
	return SECFailure;
    }
    key->params.arena = arena;
    
    MP_DIGITS(&p) = 0;
    MP_DIGITS(&g) = 0;
    MP_DIGITS(&x) = 0;
    MP_DIGITS(&y) = 0;
    CHECK_MPI_OK( mp_init(&p) );
    CHECK_MPI_OK( mp_init(&g) );
    CHECK_MPI_OK( mp_init(&x) );
    CHECK_MPI_OK( mp_init(&y) );
    
    CHECK_MPI_OK( SECITEM_CopyItem(arena, &key->params.prime,
                                          &params->prime) );
    CHECK_MPI_OK( SECITEM_CopyItem(arena, &key->params.subPrime,
                                          &params->subPrime) );
    CHECK_MPI_OK( SECITEM_CopyItem(arena, &key->params.base, &params->base) );
    
    SECITEM_TO_MPINT(params->prime, &p);
    SECITEM_TO_MPINT(params->base,  &g);
    OCTETS_TO_MPINT(xb, &x, DSA_SUBPRIME_LEN);
    
    SECITEM_AllocItem(arena, &key->privateValue, DSA_SUBPRIME_LEN);
    memcpy(key->privateValue.data, xb, DSA_SUBPRIME_LEN);
    
    CHECK_MPI_OK( mp_exptmod(&g, &x, &p, &y) );
    
    MPINT_TO_SECITEM(&y, &key->publicValue, arena);
    *privKey = key;
    key = NULL;
cleanup:
    mp_clear(&p);
    mp_clear(&g);
    mp_clear(&x);
    mp_clear(&y);
    if (key)
	PORT_FreeArena(key->params.arena, PR_TRUE);
    if (err) {
	translate_mpi_error(err);
	return SECFailure;
    }
    return SECSuccess;
}







SECStatus 
DSA_NewKey(const PQGParams *params, DSAPrivateKey **privKey)
{
    SECStatus rv;
    unsigned char seed[DSA_SUBPRIME_LEN];
    int retries = 10;
    int i;
    PRBool good;

    do {
	
	if (dsa_GenerateGlobalRandomBytes(seed, DSA_SUBPRIME_LEN,
					  params->subPrime.data))
	    return SECFailure;
	
	good = PR_FALSE;
	for (i = 0; i < DSA_SUBPRIME_LEN-1; i++) {
	    if (seed[i] != 0) {
		good = PR_TRUE;
		break;
	    }
	}
	if (!good && seed[i] > 1) {
	    good = PR_TRUE;
	}
    } while (!good && --retries > 0);

    if (!good) {
	PORT_SetError(SEC_ERROR_NEED_RANDOM);
	return SECFailure;
    }

    
    rv = dsa_NewKey(params, privKey, seed);
    return rv;
}


SECStatus 
DSA_NewKeyFromSeed(const PQGParams *params, 
                   const unsigned char *seed,
                   DSAPrivateKey **privKey)
{
    SECStatus rv;
    rv = dsa_NewKey(params, privKey, seed);
    return rv;
}

static SECStatus 
dsa_SignDigest(DSAPrivateKey *key, SECItem *signature, const SECItem *digest,
               const unsigned char *kb)
{
    mp_int p, q, g;  
    mp_int x, k;     
    mp_int r, s;     
    mp_err err   = MP_OKAY;
    SECStatus rv = SECSuccess;

    
    
    if (!key || !signature || !digest ||
        (signature->len < DSA_SIGNATURE_LEN) ||
	(digest->len != SHA1_LENGTH)) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    
    MP_DIGITS(&p) = 0;
    MP_DIGITS(&q) = 0;
    MP_DIGITS(&g) = 0;
    MP_DIGITS(&x) = 0;
    MP_DIGITS(&k) = 0;
    MP_DIGITS(&r) = 0;
    MP_DIGITS(&s) = 0;
    CHECK_MPI_OK( mp_init(&p) );
    CHECK_MPI_OK( mp_init(&q) );
    CHECK_MPI_OK( mp_init(&g) );
    CHECK_MPI_OK( mp_init(&x) );
    CHECK_MPI_OK( mp_init(&k) );
    CHECK_MPI_OK( mp_init(&r) );
    CHECK_MPI_OK( mp_init(&s) );
    


    SECITEM_TO_MPINT(key->params.prime,    &p);
    SECITEM_TO_MPINT(key->params.subPrime, &q);
    SECITEM_TO_MPINT(key->params.base,     &g);
    SECITEM_TO_MPINT(key->privateValue,    &x);
    OCTETS_TO_MPINT(kb, &k, DSA_SUBPRIME_LEN);
    




    CHECK_MPI_OK( mp_exptmod(&g, &k, &p, &r) ); 
    CHECK_MPI_OK(     mp_mod(&r, &q, &r) );     
    




    SECITEM_TO_MPINT(*digest, &s);         
    CHECK_MPI_OK( mp_invmod(&k, &q, &k) );      
    CHECK_MPI_OK( mp_mulmod(&x, &r, &q, &x) );  
    CHECK_MPI_OK( mp_addmod(&s, &x, &q, &s) );  
    CHECK_MPI_OK( mp_mulmod(&s, &k, &q, &s) );  
    



    if (mp_cmp_z(&r) == 0 || mp_cmp_z(&s) == 0) {
	PORT_SetError(SEC_ERROR_NEED_RANDOM);
	rv = SECFailure;
	goto cleanup;
    }
    




    err = mp_to_fixlen_octets(&r, signature->data, DSA_SUBPRIME_LEN);
    if (err < 0) goto cleanup; 
    err = mp_to_fixlen_octets(&s, signature->data + DSA_SUBPRIME_LEN, 
                                  DSA_SUBPRIME_LEN);
    if (err < 0) goto cleanup; 
    err = MP_OKAY;
    signature->len = DSA_SIGNATURE_LEN;
cleanup:
    mp_clear(&p);
    mp_clear(&q);
    mp_clear(&g);
    mp_clear(&x);
    mp_clear(&k);
    mp_clear(&r);
    mp_clear(&s);
    if (err) {
	translate_mpi_error(err);
	rv = SECFailure;
    }
    return rv;
}







SECStatus 
DSA_SignDigest(DSAPrivateKey *key, SECItem *signature, const SECItem *digest)
{
    SECStatus rv;
    int       retries = 10;
    unsigned char kSeed[DSA_SUBPRIME_LEN];
    int       i;
    PRBool    good;

    PORT_SetError(0);
    do {
	rv = dsa_GenerateGlobalRandomBytes(kSeed, DSA_SUBPRIME_LEN, 
					   key->params.subPrime.data);
	if (rv != SECSuccess) 
	    break;
	
	good = PR_FALSE;
	for (i = 0; i < DSA_SUBPRIME_LEN; i++) {
	    if (kSeed[i] != 0) {
		good = PR_TRUE;
		break;
	    }
	}
	if (!good) {
	    PORT_SetError(SEC_ERROR_NEED_RANDOM);
	    rv = SECFailure;
	    continue;
	}
	rv = dsa_SignDigest(key, signature, digest, kSeed);
    } while (rv != SECSuccess && PORT_GetError() == SEC_ERROR_NEED_RANDOM &&
	     --retries > 0);
    return rv;
}


SECStatus 
DSA_SignDigestWithSeed(DSAPrivateKey * key,
                       SECItem *       signature,
                       const SECItem * digest,
                       const unsigned char * seed)
{
    SECStatus rv;
    rv = dsa_SignDigest(key, signature, digest, seed);
    return rv;
}





SECStatus 
DSA_VerifyDigest(DSAPublicKey *key, const SECItem *signature, 
                 const SECItem *digest)
{
    
    mp_int p, q, g;      
    mp_int r_, s_;       
    mp_int u1, u2, v, w; 
    mp_int y;            
    mp_err err;
    SECStatus verified = SECFailure;

    
    if (!key || !signature || !digest ||
        (signature->len != DSA_SIGNATURE_LEN) ||
	(digest->len != SHA1_LENGTH)) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    
    MP_DIGITS(&p)  = 0;
    MP_DIGITS(&q)  = 0;
    MP_DIGITS(&g)  = 0;
    MP_DIGITS(&y)  = 0;
    MP_DIGITS(&r_) = 0;
    MP_DIGITS(&s_) = 0;
    MP_DIGITS(&u1) = 0;
    MP_DIGITS(&u2) = 0;
    MP_DIGITS(&v)  = 0;
    MP_DIGITS(&w)  = 0;
    CHECK_MPI_OK( mp_init(&p)  );
    CHECK_MPI_OK( mp_init(&q)  );
    CHECK_MPI_OK( mp_init(&g)  );
    CHECK_MPI_OK( mp_init(&y)  );
    CHECK_MPI_OK( mp_init(&r_) );
    CHECK_MPI_OK( mp_init(&s_) );
    CHECK_MPI_OK( mp_init(&u1) );
    CHECK_MPI_OK( mp_init(&u2) );
    CHECK_MPI_OK( mp_init(&v)  );
    CHECK_MPI_OK( mp_init(&w)  );
    


    SECITEM_TO_MPINT(key->params.prime,    &p);
    SECITEM_TO_MPINT(key->params.subPrime, &q);
    SECITEM_TO_MPINT(key->params.base,     &g);
    SECITEM_TO_MPINT(key->publicValue,     &y);
    


    OCTETS_TO_MPINT(signature->data, &r_, DSA_SUBPRIME_LEN);
    OCTETS_TO_MPINT(signature->data + DSA_SUBPRIME_LEN, &s_, DSA_SUBPRIME_LEN);
    


    if (mp_cmp_z(&r_) <= 0 || mp_cmp_z(&s_) <= 0 ||
        mp_cmp(&r_, &q) >= 0 || mp_cmp(&s_, &q) >= 0) {
	
	PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
	goto cleanup; 
    }
    




    CHECK_MPI_OK( mp_invmod(&s_, &q, &w) );      
    




    SECITEM_TO_MPINT(*digest, &u1);              
    CHECK_MPI_OK( mp_mulmod(&u1, &w, &q, &u1) ); 
    




    CHECK_MPI_OK( mp_mulmod(&r_, &w, &q, &u2) );
    




    CHECK_MPI_OK( mp_exptmod(&g, &u1, &p, &g) ); 
    CHECK_MPI_OK( mp_exptmod(&y, &u2, &p, &y) ); 
    CHECK_MPI_OK(  mp_mulmod(&g, &y, &p, &v)  ); 
    CHECK_MPI_OK(     mp_mod(&v, &q, &v)      ); 
    


    if (mp_cmp(&v, &r_)) {
	PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
	verified = SECFailure; 
    } else {
	verified = SECSuccess; 
    }
cleanup:
    mp_clear(&p);
    mp_clear(&q);
    mp_clear(&g);
    mp_clear(&y);
    mp_clear(&r_);
    mp_clear(&s_);
    mp_clear(&u1);
    mp_clear(&u2);
    mp_clear(&v);
    mp_clear(&w);
    if (err) {
	translate_mpi_error(err);
    }
    return verified;
}
