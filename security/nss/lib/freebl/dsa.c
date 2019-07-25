






































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









static SECStatus
fips186Change_ReduceModQForDSA(const PRUint8 *w, const PRUint8 *q,
                               unsigned int qLen, PRUint8 * xj)
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
    


    CHECK_MPI_OK( mp_read_unsigned_octets(&W, w, 2*qLen) );
    CHECK_MPI_OK( mp_read_unsigned_octets(&Q, q, qLen) );

    




    CHECK_MPI_OK( mp_mod(&W, &Q, &Xj) );
    CHECK_MPI_OK( mp_to_fixlen_octets(&Xj, xj, qLen) );
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
FIPS186Change_ReduceModQForDSA(const unsigned char *w,
                               const unsigned char *q,
                               unsigned char *xj) {
    return fips186Change_ReduceModQForDSA(w, q, DSA_SUBPRIME_LEN, xj);
}









SECStatus
FIPS186Change_GenerateX(PRUint8 *XKEY, const PRUint8 *XSEEDj,
                        PRUint8 *x_j)
{
    PORT_SetError(PR_NOT_IMPLEMENTED_ERROR);
    return SECFailure;
}















static SECStatus 
dsa_GenerateGlobalRandomBytes(const SECItem * qItem, PRUint8 * dest,
                              unsigned int * destLen, unsigned int maxDestLen)
{
    SECStatus rv;
    SECItem w;
    const PRUint8 * q = qItem->data;
    unsigned int qLen = qItem->len;

    if (*q == 0) {
        ++q;
        --qLen;
    }
    if (maxDestLen < qLen) {
        

        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    w.data = NULL; 
    if (!SECITEM_AllocItem(NULL, &w, 2*qLen)) {
        return SECFailure;
    }
    *destLen = qLen;

    rv = RNG_GenerateGlobalRandomBytes(w.data, w.len);
    if (rv == SECSuccess) {
        rv = fips186Change_ReduceModQForDSA(w.data, q, qLen, dest);
    }

    SECITEM_FreeItem(&w, PR_FALSE);
    return rv;
}

static void translate_mpi_error(mp_err err)
{
    MP_TO_SEC_ERROR(err);
}

static SECStatus 
dsa_NewKeyExtended(const PQGParams *params, const SECItem * seed,
                   DSAPrivateKey **privKey)
{
    mp_int p, g;
    mp_int x, y;
    mp_err err;
    PRArenaPool *arena;
    DSAPrivateKey *key;
    
    if (!params || !privKey || !seed || !seed->data) {
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
    OCTETS_TO_MPINT(seed->data, &x, seed->len);
    
    SECITEM_AllocItem(arena, &key->privateValue, seed->len);
    PORT_Memcpy(key->privateValue.data, seed->data, seed->len);
    
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
DSA_NewRandom(PLArenaPool * arena, const SECItem * q, SECItem * seed)
{
    int retries = 10;
    unsigned int i;
    PRBool good;

    if (q == NULL || q->data == NULL || q->len == 0 ||
        (q->data[0] == 0 && q->len == 1)) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    if (!SECITEM_AllocItem(arena, seed, q->len)) {
        return SECFailure;
    }

    do {
	
        if (dsa_GenerateGlobalRandomBytes(q, seed->data, &seed->len,
                                          seed->len)) {
            goto loser;
        }
	
	good = PR_FALSE;
	for (i = 0; i < seed->len-1; i++) {
	    if (seed->data[i] != 0) {
		good = PR_TRUE;
		break;
	    }
	}
	if (!good && seed->data[i] > 1) {
	    good = PR_TRUE;
	}
    } while (!good && --retries > 0);

    if (!good) {
	PORT_SetError(SEC_ERROR_NEED_RANDOM);
loser:	if (arena != NULL) {
            SECITEM_FreeItem(seed, PR_FALSE);
        }
	return SECFailure;
    }

    return SECSuccess;
}







SECStatus 
DSA_NewKey(const PQGParams *params, DSAPrivateKey **privKey)
{
    SECItem seed;
    SECStatus rv;

    seed.data = NULL;

    rv = DSA_NewRandom(NULL, &params->subPrime, &seed);
    if (rv == SECSuccess) {
        if (seed.len != DSA_SUBPRIME_LEN) {
            PORT_SetError(SEC_ERROR_INVALID_ARGS);
            rv = SECFailure;
        } else {
            rv = dsa_NewKeyExtended(params, &seed, privKey);
        }
    }
    SECITEM_FreeItem(&seed, PR_FALSE);
    return rv;
}


SECStatus 
DSA_NewKeyFromSeed(const PQGParams *params, 
                   const unsigned char *seed,
                   DSAPrivateKey **privKey)
{
    
    SECItem seedItem;
    seedItem.data = (unsigned char*) seed;
    seedItem.len = DSA_SUBPRIME_LEN;
    return dsa_NewKeyExtended(params, &seedItem, privKey);
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
    unsigned int kSeedLen = 0;
    unsigned int i;
    PRBool    good;

    PORT_SetError(0);
    do {
	rv = dsa_GenerateGlobalRandomBytes(&key->params.subPrime,
                                           kSeed, &kSeedLen, sizeof kSeed);
	if (rv != SECSuccess) 
	    break;
        if (kSeedLen != DSA_SUBPRIME_LEN) {
            PORT_SetError(SEC_ERROR_INVALID_ARGS);
            rv = SECFailure;
            break;
        }
	
	good = PR_FALSE;
	for (i = 0; i < kSeedLen; i++) {
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
