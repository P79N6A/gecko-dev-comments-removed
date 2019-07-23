









































#ifdef FREEBL_NO_DEPEND
#include "stubs.h"
#endif

#include "prerr.h"
#include "secerr.h"

#include "blapi.h"
#include "secitem.h"
#include "mpi.h"
#include "mpprime.h"
#include "secmpi.h"

#define DH_SECRET_KEY_LEN      20
#define KEA_DERIVED_SECRET_LEN 128

SECStatus 
DH_GenParam(int primeLen, DHParams **params)
{
    PRArenaPool *arena;
    DHParams *dhparams;
    unsigned char *pb = NULL;
    unsigned char *ab = NULL;
    unsigned long counter = 0;
    mp_int p, q, a, h, psub1, test;
    mp_err err = MP_OKAY;
    SECStatus rv = SECSuccess;
    if (!params || primeLen < 0) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    arena = PORT_NewArena(NSS_FREEBL_DEFAULT_CHUNKSIZE);
    if (!arena) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
    }
    dhparams = (DHParams *)PORT_ArenaZAlloc(arena, sizeof(DHParams));
    if (!dhparams) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	PORT_FreeArena(arena, PR_TRUE);
	return SECFailure;
    }
    dhparams->arena = arena;
    MP_DIGITS(&p) = 0;
    MP_DIGITS(&q) = 0;
    MP_DIGITS(&a) = 0;
    MP_DIGITS(&h) = 0;
    MP_DIGITS(&psub1) = 0;
    MP_DIGITS(&test) = 0;
    CHECK_MPI_OK( mp_init(&p) );
    CHECK_MPI_OK( mp_init(&q) );
    CHECK_MPI_OK( mp_init(&a) );
    CHECK_MPI_OK( mp_init(&h) );
    CHECK_MPI_OK( mp_init(&psub1) );
    CHECK_MPI_OK( mp_init(&test) );
    
    pb = PORT_Alloc(primeLen);
    CHECK_SEC_OK( RNG_GenerateGlobalRandomBytes(pb, primeLen) );
    pb[0]          |= 0x80; 
    pb[primeLen-1] |= 0x01; 
    CHECK_MPI_OK( mp_read_unsigned_octets(&p, pb, primeLen) );
    CHECK_MPI_OK( mpp_make_prime(&p, primeLen * 8, PR_TRUE, &counter) );
    
    CHECK_MPI_OK( mp_sub_d(&p, 1, &psub1) );
    CHECK_MPI_OK( mp_div_2(&psub1, &q)    );
    
    ab = PORT_Alloc(primeLen);
    
    CHECK_SEC_OK( RNG_GenerateGlobalRandomBytes(ab, primeLen) );
    CHECK_MPI_OK( mp_read_unsigned_octets(&a, ab, primeLen) );
    
    if ( mp_cmp(&a, &p) > 0 )
	CHECK_MPI_OK( mp_sub(&a, &p, &a) );
    do {
	
	if ( mp_cmp_d(&a, 2) < 0 || mp_cmp(&a, &psub1) >= 0) {
	    
            mp_set(&a, 3);
	}
	
	CHECK_MPI_OK( mp_exptmod(&a, &q, &p, &test) );
	if ( mp_cmp_d(&test, 1) != 0 )
	    break;
	
	CHECK_MPI_OK( mp_add_d(&a, 1, &a) );
    } while (PR_TRUE);
    MPINT_TO_SECITEM(&p, &dhparams->prime, arena);
    MPINT_TO_SECITEM(&a, &dhparams->base, arena);
    *params = dhparams;
cleanup:
    mp_clear(&p);
    mp_clear(&q);
    mp_clear(&a);
    mp_clear(&h);
    mp_clear(&psub1);
    mp_clear(&test);
    if (pb) PORT_ZFree(pb, primeLen);
    if (ab) PORT_ZFree(ab, primeLen);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    if (rv)
	PORT_FreeArena(arena, PR_TRUE);
    return rv;
}

SECStatus 
DH_NewKey(DHParams *params, DHPrivateKey **privKey)
{
    PRArenaPool *arena;
    DHPrivateKey *key;
    mp_int g, xa, p, Ya;
    mp_err   err = MP_OKAY;
    SECStatus rv = SECSuccess;
    if (!params || !privKey) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    arena = PORT_NewArena(NSS_FREEBL_DEFAULT_CHUNKSIZE);
    if (!arena) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
    }
    key = (DHPrivateKey *)PORT_ArenaZAlloc(arena, sizeof(DHPrivateKey));
    if (!key) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	PORT_FreeArena(arena, PR_TRUE);
	return SECFailure;
    }
    key->arena = arena;
    MP_DIGITS(&g)  = 0;
    MP_DIGITS(&xa) = 0;
    MP_DIGITS(&p)  = 0;
    MP_DIGITS(&Ya) = 0;
    CHECK_MPI_OK( mp_init(&g)  );
    CHECK_MPI_OK( mp_init(&xa) );
    CHECK_MPI_OK( mp_init(&p)  );
    CHECK_MPI_OK( mp_init(&Ya) );
    
    CHECK_SEC_OK( SECITEM_CopyItem(arena, &key->prime, &params->prime) );
    SECITEM_TO_MPINT(key->prime, &p);
    
    CHECK_SEC_OK( SECITEM_CopyItem(arena, &key->base, &params->base) );
    SECITEM_TO_MPINT(key->base, &g);
    
    SECITEM_AllocItem(arena, &key->privateValue, DH_SECRET_KEY_LEN);
    RNG_GenerateGlobalRandomBytes(key->privateValue.data, 
                                  key->privateValue.len);
    SECITEM_TO_MPINT( key->privateValue, &xa );
    
    CHECK_MPI_OK( mp_mod(&xa, &p, &xa) );
    
    CHECK_MPI_OK( mp_exptmod(&g, &xa, &p, &Ya) );
    MPINT_TO_SECITEM(&Ya, &key->publicValue, key->arena);
    *privKey = key;
cleanup:
    mp_clear(&g);
    mp_clear(&xa);
    mp_clear(&p);
    mp_clear(&Ya);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    if (rv)
	PORT_FreeArena(arena, PR_TRUE);
    return rv;
}

SECStatus 
DH_Derive(SECItem *publicValue, 
          SECItem *prime, 
          SECItem *privateValue, 
          SECItem *derivedSecret, 
          unsigned int maxOutBytes)
{
    mp_int p, Xa, Yb, ZZ;
    mp_err err = MP_OKAY;
    unsigned int len = 0, nb;
    unsigned char *secret = NULL;
    if (!publicValue || !prime || !privateValue || !derivedSecret) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    memset(derivedSecret, 0, sizeof *derivedSecret);
    MP_DIGITS(&p)  = 0;
    MP_DIGITS(&Xa) = 0;
    MP_DIGITS(&Yb) = 0;
    MP_DIGITS(&ZZ) = 0;
    CHECK_MPI_OK( mp_init(&p)  );
    CHECK_MPI_OK( mp_init(&Xa) );
    CHECK_MPI_OK( mp_init(&Yb) );
    CHECK_MPI_OK( mp_init(&ZZ) );
    SECITEM_TO_MPINT(*publicValue,  &Yb);
    SECITEM_TO_MPINT(*privateValue, &Xa);
    SECITEM_TO_MPINT(*prime,        &p);
    
    CHECK_MPI_OK( mp_exptmod(&Yb, &Xa, &p, &ZZ) );
    
    len = mp_unsigned_octet_size(&ZZ);
    
    secret = PORT_Alloc(len);
    
    err = mp_to_unsigned_octets(&ZZ, secret, len);
    if (err >= 0) err = MP_OKAY;
    


    if (maxOutBytes > 0)
	nb = PR_MIN(len, maxOutBytes);
    else
	nb = len;
    SECITEM_AllocItem(NULL, derivedSecret, nb);
    memcpy(derivedSecret->data, secret, nb);
cleanup:
    mp_clear(&p);
    mp_clear(&Xa);
    mp_clear(&Yb);
    mp_clear(&ZZ);
    if (secret) {
	
	PORT_ZFree(secret, len);
    }
    if (err) {
	MP_TO_SEC_ERROR(err);
	if (derivedSecret->data) 
	    PORT_ZFree(derivedSecret->data, derivedSecret->len);
	return SECFailure;
    }
    return SECSuccess;
}

SECStatus 
KEA_Derive(SECItem *prime, 
           SECItem *public1, 
           SECItem *public2, 
           SECItem *private1, 
           SECItem *private2,
           SECItem *derivedSecret)
{
    mp_int p, Y, R, r, x, t, u, w;
    mp_err err;
    unsigned char *secret = NULL;
    unsigned int len = 0, offset;
    if (!prime || !public1 || !public2 || !private1 || !private2 ||
        !derivedSecret) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    memset(derivedSecret, 0, sizeof *derivedSecret);
    MP_DIGITS(&p) = 0;
    MP_DIGITS(&Y) = 0;
    MP_DIGITS(&R) = 0;
    MP_DIGITS(&r) = 0;
    MP_DIGITS(&x) = 0;
    MP_DIGITS(&t) = 0;
    MP_DIGITS(&u) = 0;
    MP_DIGITS(&w) = 0;
    CHECK_MPI_OK( mp_init(&p) );
    CHECK_MPI_OK( mp_init(&Y) );
    CHECK_MPI_OK( mp_init(&R) );
    CHECK_MPI_OK( mp_init(&r) );
    CHECK_MPI_OK( mp_init(&x) );
    CHECK_MPI_OK( mp_init(&t) );
    CHECK_MPI_OK( mp_init(&u) );
    CHECK_MPI_OK( mp_init(&w) );
    SECITEM_TO_MPINT(*prime,    &p);
    SECITEM_TO_MPINT(*public1,  &Y);
    SECITEM_TO_MPINT(*public2,  &R);
    SECITEM_TO_MPINT(*private1, &r);
    SECITEM_TO_MPINT(*private2, &x);
    
    CHECK_MPI_OK( mp_exptmod(&Y, &r, &p, &t) );
    
    CHECK_MPI_OK( mp_exptmod(&R, &x, &p, &u) );
    
    CHECK_MPI_OK( mp_addmod(&t, &u, &p, &w) );
    
    len = mp_unsigned_octet_size(&w);
    secret = PORT_Alloc(len);
    
    err = mp_to_unsigned_octets(&w, secret, len);
    if (err > 0) err = MP_OKAY;
    
    SECITEM_AllocItem(NULL, derivedSecret, KEA_DERIVED_SECRET_LEN);
    memset(derivedSecret->data, 0, derivedSecret->len);
    
    if (len >= KEA_DERIVED_SECRET_LEN) {
	memcpy(derivedSecret->data, secret + (len - KEA_DERIVED_SECRET_LEN),
	       KEA_DERIVED_SECRET_LEN);
    } else {
	offset = KEA_DERIVED_SECRET_LEN - len;
	memcpy(derivedSecret->data + offset, secret, len);
    }
cleanup:
    mp_clear(&p);
    mp_clear(&Y);
    mp_clear(&R);
    mp_clear(&r);
    mp_clear(&x);
    mp_clear(&t);
    mp_clear(&u);
    mp_clear(&w);
    if (secret)
	PORT_ZFree(secret, len);
    if (err) {
	MP_TO_SEC_ERROR(err);
	return SECFailure;
    }
    return SECSuccess;
}

PRBool 
KEA_Verify(SECItem *Y, SECItem *prime, SECItem *subPrime)
{
    mp_int p, q, y, r;
    mp_err err;
    int cmp = 1;  
    if (!Y || !prime || !subPrime) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    MP_DIGITS(&p) = 0;
    MP_DIGITS(&q) = 0;
    MP_DIGITS(&y) = 0;
    MP_DIGITS(&r) = 0;
    CHECK_MPI_OK( mp_init(&p) );
    CHECK_MPI_OK( mp_init(&q) );
    CHECK_MPI_OK( mp_init(&y) );
    CHECK_MPI_OK( mp_init(&r) );
    SECITEM_TO_MPINT(*prime,    &p);
    SECITEM_TO_MPINT(*subPrime, &q);
    SECITEM_TO_MPINT(*Y,        &y);
    
    CHECK_MPI_OK( mp_exptmod(&y, &q, &p, &r) );
    
    cmp = mp_cmp_d(&r, 1);
cleanup:
    mp_clear(&p);
    mp_clear(&q);
    mp_clear(&y);
    mp_clear(&r);
    if (err) {
	MP_TO_SEC_ERROR(err);
	return PR_FALSE;
    }
    return (cmp == 0) ? PR_TRUE : PR_FALSE;
}
