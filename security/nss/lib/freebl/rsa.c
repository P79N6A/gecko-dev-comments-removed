








































#ifdef FREEBL_NO_DEPEND
#include "stubs.h"
#endif

#include "secerr.h"

#include "prclist.h"
#include "nssilock.h"
#include "prinit.h"
#include "blapi.h"
#include "mpi.h"
#include "mpprime.h"
#include "mplogic.h"
#include "secmpi.h"
#include "secitem.h"
#include "blapii.h"





#define MAX_PRIME_GEN_ATTEMPTS 10




#define MAX_KEY_GEN_ATTEMPTS 10


#define BAD_RSA_KEY_SIZE(modLen, expLen) \
    ((expLen) > (modLen) || (modLen) > RSA_MAX_MODULUS_BITS/8 || \
    (expLen) > RSA_MAX_EXPONENT_BITS/8)









struct RSABlindingParamsStr
{
    
    PRCList   link;                  
    SECItem   modulus;               
    mp_int    f, g;                  
    int       counter;               
};









struct RSABlindingParamsListStr
{
    PZLock  *lock;   
    PRCList  head;   
};




static struct RSABlindingParamsListStr blindingParamsList = { 0 };


#define RSA_BLINDING_PARAMS_MAX_REUSE 50



static PRBool nssRSAUseBlinding = PR_TRUE;

static SECStatus
rsa_keygen_from_primes(mp_int *p, mp_int *q, mp_int *e, RSAPrivateKey *key,
                       unsigned int keySizeInBits)
{
    mp_int n, d, phi;
    mp_int psub1, qsub1, tmp;
    mp_err   err = MP_OKAY;
    SECStatus rv = SECSuccess;
    MP_DIGITS(&n)     = 0;
    MP_DIGITS(&d)     = 0;
    MP_DIGITS(&phi)   = 0;
    MP_DIGITS(&psub1) = 0;
    MP_DIGITS(&qsub1) = 0;
    MP_DIGITS(&tmp)   = 0;
    CHECK_MPI_OK( mp_init(&n)     );
    CHECK_MPI_OK( mp_init(&d)     );
    CHECK_MPI_OK( mp_init(&phi)   );
    CHECK_MPI_OK( mp_init(&psub1) );
    CHECK_MPI_OK( mp_init(&qsub1) );
    CHECK_MPI_OK( mp_init(&tmp)   );
    
    CHECK_MPI_OK( mp_mul(p, q, &n) );
    
    if ((unsigned)mpl_significant_bits(&n) != keySizeInBits) {
	PORT_SetError(SEC_ERROR_NEED_RANDOM);
	rv = SECFailure;
	goto cleanup;
    }
    
    CHECK_MPI_OK( mp_sub_d(p, 1, &psub1) );
    CHECK_MPI_OK( mp_sub_d(q, 1, &qsub1) );
    CHECK_MPI_OK( mp_mul(&psub1, &qsub1, &phi) );
    
    err = mp_invmod(e, &phi, &d);
    
    if (err != MP_OKAY) {
	if (err == MP_UNDEF) {
	    PORT_SetError(SEC_ERROR_NEED_RANDOM);
	    err = MP_OKAY; 
	    rv = SECFailure;
	}
	goto cleanup;
    }
    MPINT_TO_SECITEM(&n, &key->modulus, key->arena);
    MPINT_TO_SECITEM(&d, &key->privateExponent, key->arena);
    
    CHECK_MPI_OK( mp_mod(&d, &psub1, &tmp) );
    MPINT_TO_SECITEM(&tmp, &key->exponent1, key->arena);
    
    CHECK_MPI_OK( mp_mod(&d, &qsub1, &tmp) );
    MPINT_TO_SECITEM(&tmp, &key->exponent2, key->arena);
    
    CHECK_MPI_OK( mp_invmod(q, p, &tmp) );
    MPINT_TO_SECITEM(&tmp, &key->coefficient, key->arena);
cleanup:
    mp_clear(&n);
    mp_clear(&d);
    mp_clear(&phi);
    mp_clear(&psub1);
    mp_clear(&qsub1);
    mp_clear(&tmp);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    return rv;
}
static SECStatus
generate_prime(mp_int *prime, int primeLen)
{
    mp_err   err = MP_OKAY;
    SECStatus rv = SECSuccess;
    unsigned long counter = 0;
    int piter;
    unsigned char *pb = NULL;
    pb = PORT_Alloc(primeLen);
    if (!pb) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto cleanup;
    }
    for (piter = 0; piter < MAX_PRIME_GEN_ATTEMPTS; piter++) {
	CHECK_SEC_OK( RNG_GenerateGlobalRandomBytes(pb, primeLen) );
	pb[0]          |= 0xC0; 
	pb[primeLen-1] |= 0x01; 
	CHECK_MPI_OK( mp_read_unsigned_octets(prime, pb, primeLen) );
	err = mpp_make_prime(prime, primeLen * 8, PR_FALSE, &counter);
	if (err != MP_NO)
	    goto cleanup;
	
    }
cleanup:
    if (pb)
	PORT_ZFree(pb, primeLen);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    return rv;
}











RSAPrivateKey *
RSA_NewKey(int keySizeInBits, SECItem *publicExponent)
{
    unsigned int primeLen;
    mp_int p, q, e;
    int kiter;
    mp_err   err = MP_OKAY;
    SECStatus rv = SECSuccess;
    int prerr = 0;
    RSAPrivateKey *key = NULL;
    PRArenaPool *arena = NULL;
    
    if (!publicExponent || keySizeInBits % 16 != 0 ||
	    BAD_RSA_KEY_SIZE(keySizeInBits/8, publicExponent->len)) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return NULL;
    }
    
    arena = PORT_NewArena(NSS_FREEBL_DEFAULT_CHUNKSIZE);
    if (!arena) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return NULL;
    }
    key = (RSAPrivateKey *)PORT_ArenaZAlloc(arena, sizeof(RSAPrivateKey));
    if (!key) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	PORT_FreeArena(arena, PR_TRUE);
	return NULL;
    }
    key->arena = arena;
    
    primeLen = keySizeInBits / (2 * BITS_PER_BYTE);
    MP_DIGITS(&p) = 0;
    MP_DIGITS(&q) = 0;
    MP_DIGITS(&e) = 0;
    CHECK_MPI_OK( mp_init(&p) );
    CHECK_MPI_OK( mp_init(&q) );
    CHECK_MPI_OK( mp_init(&e) );
    
    SECITEM_AllocItem(arena, &key->version, 1);
    key->version.data[0] = 0;
    
    SECITEM_CopyItem(arena, &key->publicExponent, publicExponent);
    SECITEM_TO_MPINT(*publicExponent, &e);
    kiter = 0;
    do {
	prerr = 0;
	PORT_SetError(0);
	CHECK_SEC_OK( generate_prime(&p, primeLen) );
	CHECK_SEC_OK( generate_prime(&q, primeLen) );
	
	if (mp_cmp(&p, &q) < 0)
	    mp_exch(&p, &q);
	
	rv = rsa_keygen_from_primes(&p, &q, &e, key, keySizeInBits);
	if (rv == SECSuccess)
	    break; 
	prerr = PORT_GetError();
	kiter++;
	
    } while (prerr == SEC_ERROR_NEED_RANDOM && kiter < MAX_KEY_GEN_ATTEMPTS);
    if (prerr)
	goto cleanup;
    MPINT_TO_SECITEM(&p, &key->prime1, arena);
    MPINT_TO_SECITEM(&q, &key->prime2, arena);
cleanup:
    mp_clear(&p);
    mp_clear(&q);
    mp_clear(&e);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    if (rv && arena) {
	PORT_FreeArena(arena, PR_TRUE);
	key = NULL;
    }
    return key;
}

static unsigned int
rsa_modulusLen(SECItem *modulus)
{
    unsigned char byteZero = modulus->data[0];
    unsigned int modLen = modulus->len - !byteZero;
    return modLen;
}





SECStatus 
RSA_PublicKeyOp(RSAPublicKey  *key, 
                unsigned char *output, 
                const unsigned char *input)
{
    unsigned int modLen, expLen, offset;
    mp_int n, e, m, c;
    mp_err err   = MP_OKAY;
    SECStatus rv = SECSuccess;
    if (!key || !output || !input) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    MP_DIGITS(&n) = 0;
    MP_DIGITS(&e) = 0;
    MP_DIGITS(&m) = 0;
    MP_DIGITS(&c) = 0;
    CHECK_MPI_OK( mp_init(&n) );
    CHECK_MPI_OK( mp_init(&e) );
    CHECK_MPI_OK( mp_init(&m) );
    CHECK_MPI_OK( mp_init(&c) );
    modLen = rsa_modulusLen(&key->modulus);
    expLen = rsa_modulusLen(&key->publicExponent);
    
    if (BAD_RSA_KEY_SIZE(modLen, expLen)) {
    	PORT_SetError(SEC_ERROR_INVALID_KEY);
	rv = SECFailure;
	goto cleanup;
    }
    SECITEM_TO_MPINT(key->modulus, &n);
    SECITEM_TO_MPINT(key->publicExponent, &e);
    if (e.used > n.used) {
	
    	PORT_SetError(SEC_ERROR_INVALID_KEY);
	rv = SECFailure;
	goto cleanup;
    }
    
    offset = (key->modulus.data[0] == 0) ? 1 : 0; 
    if (memcmp(input, key->modulus.data + offset, modLen) >= 0) {
        PORT_SetError(SEC_ERROR_INPUT_LEN);
        rv = SECFailure;
        goto cleanup;
    }
    
    CHECK_MPI_OK( mp_read_unsigned_octets(&m, input, modLen) );
    
#ifdef USE_MPI_EXPT_D
    
    if (MP_USED(&e) == 1) {
	CHECK_MPI_OK( mp_exptmod_d(&m, MP_DIGIT(&e, 0), &n, &c) );
    } else
#endif
    CHECK_MPI_OK( mp_exptmod(&m, &e, &n, &c) );
    
    err = mp_to_fixlen_octets(&c, output, modLen);
    if (err >= 0) err = MP_OKAY;
cleanup:
    mp_clear(&n);
    mp_clear(&e);
    mp_clear(&m);
    mp_clear(&c);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    return rv;
}




static SECStatus 
rsa_PrivateKeyOpNoCRT(RSAPrivateKey *key, mp_int *m, mp_int *c, mp_int *n,
                      unsigned int modLen)
{
    mp_int d;
    mp_err   err = MP_OKAY;
    SECStatus rv = SECSuccess;
    MP_DIGITS(&d) = 0;
    CHECK_MPI_OK( mp_init(&d) );
    SECITEM_TO_MPINT(key->privateExponent, &d);
    
    CHECK_MPI_OK( mp_exptmod(c, &d, n, m) );
cleanup:
    mp_clear(&d);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    return rv;
}




static SECStatus 
rsa_PrivateKeyOpCRTNoCheck(RSAPrivateKey *key, mp_int *m, mp_int *c)
{
    mp_int p, q, d_p, d_q, qInv;
    mp_int m1, m2, h, ctmp;
    mp_err   err = MP_OKAY;
    SECStatus rv = SECSuccess;
    MP_DIGITS(&p)    = 0;
    MP_DIGITS(&q)    = 0;
    MP_DIGITS(&d_p)  = 0;
    MP_DIGITS(&d_q)  = 0;
    MP_DIGITS(&qInv) = 0;
    MP_DIGITS(&m1)   = 0;
    MP_DIGITS(&m2)   = 0;
    MP_DIGITS(&h)    = 0;
    MP_DIGITS(&ctmp) = 0;
    CHECK_MPI_OK( mp_init(&p)    );
    CHECK_MPI_OK( mp_init(&q)    );
    CHECK_MPI_OK( mp_init(&d_p)  );
    CHECK_MPI_OK( mp_init(&d_q)  );
    CHECK_MPI_OK( mp_init(&qInv) );
    CHECK_MPI_OK( mp_init(&m1)   );
    CHECK_MPI_OK( mp_init(&m2)   );
    CHECK_MPI_OK( mp_init(&h)    );
    CHECK_MPI_OK( mp_init(&ctmp) );
    
    SECITEM_TO_MPINT(key->prime1,      &p);    
    SECITEM_TO_MPINT(key->prime2,      &q);    
    SECITEM_TO_MPINT(key->exponent1,   &d_p);  
    SECITEM_TO_MPINT(key->exponent2,   &d_q);  
    SECITEM_TO_MPINT(key->coefficient, &qInv); 
    
    CHECK_MPI_OK( mp_mod(c, &p, &ctmp) );
    CHECK_MPI_OK( mp_exptmod(&ctmp, &d_p, &p, &m1) );
    
    CHECK_MPI_OK( mp_mod(c, &q, &ctmp) );
    CHECK_MPI_OK( mp_exptmod(&ctmp, &d_q, &q, &m2) );
    
    CHECK_MPI_OK( mp_submod(&m1, &m2, &p, &h) );
    CHECK_MPI_OK( mp_mulmod(&h, &qInv, &p, &h)  );
    
    CHECK_MPI_OK( mp_mul(&h, &q, m) );
    CHECK_MPI_OK( mp_add(m, &m2, m) );
cleanup:
    mp_clear(&p);
    mp_clear(&q);
    mp_clear(&d_p);
    mp_clear(&d_q);
    mp_clear(&qInv);
    mp_clear(&m1);
    mp_clear(&m2);
    mp_clear(&h);
    mp_clear(&ctmp);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    return rv;
}










static SECStatus 
rsa_PrivateKeyOpCRTCheckedPubKey(RSAPrivateKey *key, mp_int *m, mp_int *c)
{
    mp_int n, e, v;
    mp_err   err = MP_OKAY;
    SECStatus rv = SECSuccess;
    MP_DIGITS(&n) = 0;
    MP_DIGITS(&e) = 0;
    MP_DIGITS(&v) = 0;
    CHECK_MPI_OK( mp_init(&n) );
    CHECK_MPI_OK( mp_init(&e) );
    CHECK_MPI_OK( mp_init(&v) );
    CHECK_SEC_OK( rsa_PrivateKeyOpCRTNoCheck(key, m, c) );
    SECITEM_TO_MPINT(key->modulus,        &n);
    SECITEM_TO_MPINT(key->publicExponent, &e);
    
    CHECK_MPI_OK( mp_exptmod(m, &e, &n, &v) );
    if (mp_cmp(&v, c) != 0) {
	rv = SECFailure;
    }
cleanup:
    mp_clear(&n);
    mp_clear(&e);
    mp_clear(&v);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    return rv;
}

static PRCallOnceType coBPInit = { 0, 0, 0 };
static PRStatus 
init_blinding_params_list(void)
{
    blindingParamsList.lock = PZ_NewLock(nssILockOther);
    if (!blindingParamsList.lock) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return PR_FAILURE;
    }
    PR_INIT_CLIST(&blindingParamsList.head);
    return PR_SUCCESS;
}

static SECStatus
generate_blinding_params(struct RSABlindingParamsStr *rsabp, 
                         RSAPrivateKey *key, mp_int *n, unsigned int modLen)
{
    SECStatus rv = SECSuccess;
    mp_int e, k;
    mp_err err = MP_OKAY;
    unsigned char *kb = NULL;
    MP_DIGITS(&e) = 0;
    MP_DIGITS(&k) = 0;
    CHECK_MPI_OK( mp_init(&e) );
    CHECK_MPI_OK( mp_init(&k) );
    SECITEM_TO_MPINT(key->publicExponent, &e);
    
    kb = PORT_Alloc(modLen);
    if (!kb) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto cleanup;
    }
    CHECK_SEC_OK( RNG_GenerateGlobalRandomBytes(kb, modLen) );
    CHECK_MPI_OK( mp_read_unsigned_octets(&k, kb, modLen) );
    
    CHECK_MPI_OK( mp_mod(&k, n, &k) );
    
    CHECK_MPI_OK( mp_exptmod(&k, &e, n, &rsabp->f) );
    
    CHECK_MPI_OK( mp_invmod(&k, n, &rsabp->g) );
    
    rsabp->counter = RSA_BLINDING_PARAMS_MAX_REUSE;
cleanup:
    if (kb)
	PORT_ZFree(kb, modLen);
    mp_clear(&k);
    mp_clear(&e);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    return rv;
}

static SECStatus
init_blinding_params(struct RSABlindingParamsStr *rsabp, RSAPrivateKey *key,
                     mp_int *n, unsigned int modLen)
{
    SECStatus rv = SECSuccess;
    mp_err err = MP_OKAY;
    MP_DIGITS(&rsabp->f) = 0;
    MP_DIGITS(&rsabp->g) = 0;
    
    CHECK_MPI_OK( mp_init(&rsabp->f) );
    CHECK_MPI_OK( mp_init(&rsabp->g) );
    
    SECITEM_CopyItem(NULL, &rsabp->modulus, &key->modulus);
    CHECK_SEC_OK( generate_blinding_params(rsabp, key, n, modLen) );
    return SECSuccess;
cleanup:
    mp_clear(&rsabp->f);
    mp_clear(&rsabp->g);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    return rv;
}

static SECStatus
get_blinding_params(RSAPrivateKey *key, mp_int *n, unsigned int modLen,
                    mp_int *f, mp_int *g)
{
    SECStatus rv = SECSuccess;
    mp_err err = MP_OKAY;
    int cmp;
    PRCList *el;
    struct RSABlindingParamsStr *rsabp = NULL;
    
    if (blindingParamsList.lock == NULL) {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    
    PZ_Lock(blindingParamsList.lock);
    
    for (el = PR_NEXT_LINK(&blindingParamsList.head);
         el != &blindingParamsList.head;
         el = PR_NEXT_LINK(el)) {
	rsabp = (struct RSABlindingParamsStr *)el;
	cmp = SECITEM_CompareItem(&rsabp->modulus, &key->modulus);
	if (cmp == 0) {
	    
	    if (--rsabp->counter <= 0) {
		
		CHECK_SEC_OK( generate_blinding_params(rsabp, key, n, modLen) );
	    }
	    
	    CHECK_MPI_OK( mp_copy(&rsabp->f, f) );
	    CHECK_MPI_OK( mp_copy(&rsabp->g, g) );
	    
	    PZ_Unlock(blindingParamsList.lock); 
	    return SECSuccess;
	} else if (cmp > 0) {
	    
	    break;
	}
    }
    



    rsabp = (struct RSABlindingParamsStr *)
              PORT_ZAlloc(sizeof(struct RSABlindingParamsStr));
    if (!rsabp) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	goto cleanup;
    }
    
    PR_INIT_CLIST(&rsabp->link);
    






    rv = init_blinding_params(rsabp, key, n, modLen);
    if (rv != SECSuccess) {
	PORT_ZFree(rsabp, sizeof(struct RSABlindingParamsStr));
	goto cleanup;
    }
    





    PR_INSERT_BEFORE(&rsabp->link, el);
    
    CHECK_MPI_OK( mp_copy(&rsabp->f, f) );
    CHECK_MPI_OK( mp_copy(&rsabp->g, g) );
    
    PZ_Unlock(blindingParamsList.lock); 
    return SECSuccess;
cleanup:
    


    PZ_Unlock(blindingParamsList.lock);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    return SECFailure;
}





static SECStatus 
rsa_PrivateKeyOp(RSAPrivateKey *key, 
                 unsigned char *output, 
                 const unsigned char *input,
                 PRBool check)
{
    unsigned int modLen;
    unsigned int offset;
    SECStatus rv = SECSuccess;
    mp_err err;
    mp_int n, c, m;
    mp_int f, g;
    if (!key || !output || !input) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    
    modLen = rsa_modulusLen(&key->modulus);
    offset = (key->modulus.data[0] == 0) ? 1 : 0; 
    if (memcmp(input, key->modulus.data + offset, modLen) >= 0) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    MP_DIGITS(&n) = 0;
    MP_DIGITS(&c) = 0;
    MP_DIGITS(&m) = 0;
    MP_DIGITS(&f) = 0;
    MP_DIGITS(&g) = 0;
    CHECK_MPI_OK( mp_init(&n) );
    CHECK_MPI_OK( mp_init(&c) );
    CHECK_MPI_OK( mp_init(&m) );
    CHECK_MPI_OK( mp_init(&f) );
    CHECK_MPI_OK( mp_init(&g) );
    SECITEM_TO_MPINT(key->modulus, &n);
    OCTETS_TO_MPINT(input, &c, modLen);
    


    if (nssRSAUseBlinding) {
	CHECK_SEC_OK( get_blinding_params(key, &n, modLen, &f, &g) );
	
	CHECK_MPI_OK( mp_mulmod(&c, &f, &n, &c) );
    }
    
    if ( key->prime1.len      == 0 ||
         key->prime2.len      == 0 ||
         key->exponent1.len   == 0 ||
         key->exponent2.len   == 0 ||
         key->coefficient.len == 0) {
	CHECK_SEC_OK( rsa_PrivateKeyOpNoCRT(key, &m, &c, &n, modLen) );
    } else if (check) {
	CHECK_SEC_OK( rsa_PrivateKeyOpCRTCheckedPubKey(key, &m, &c) );
    } else {
	CHECK_SEC_OK( rsa_PrivateKeyOpCRTNoCheck(key, &m, &c) );
    }
    


    if (nssRSAUseBlinding) {
	
	CHECK_MPI_OK( mp_mulmod(&m, &g, &n, &m) );
    }
    err = mp_to_fixlen_octets(&m, output, modLen);
    if (err >= 0) err = MP_OKAY;
cleanup:
    mp_clear(&n);
    mp_clear(&c);
    mp_clear(&m);
    mp_clear(&f);
    mp_clear(&g);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    return rv;
}

SECStatus 
RSA_PrivateKeyOp(RSAPrivateKey *key, 
                 unsigned char *output, 
                 const unsigned char *input)
{
    return rsa_PrivateKeyOp(key, output, input, PR_FALSE);
}

SECStatus 
RSA_PrivateKeyOpDoubleChecked(RSAPrivateKey *key, 
                              unsigned char *output, 
                              const unsigned char *input)
{
    return rsa_PrivateKeyOp(key, output, input, PR_TRUE);
}

static SECStatus
swap_in_key_value(PRArenaPool *arena, mp_int *mpval, SECItem *buffer)
{
    int len;
    mp_err err = MP_OKAY;
    memset(buffer->data, 0, buffer->len);
    len = mp_unsigned_octet_size(mpval);
    if (len <= 0) return SECFailure;
    if ((unsigned int)len <= buffer->len) {
	
	err = mp_to_unsigned_octets(mpval, buffer->data, len);
	if (err >= 0) err = MP_OKAY;
	buffer->len = len;
    } else if (arena) {
	
	(void)SECITEM_AllocItem(arena, buffer, len);
	err = mp_to_unsigned_octets(mpval, buffer->data, len);
	if (err >= 0) err = MP_OKAY;
    } else {
	
	return SECFailure;
    }
    return (err == MP_OKAY) ? SECSuccess : SECFailure;
}

SECStatus
RSA_PrivateKeyCheck(RSAPrivateKey *key)
{
    mp_int p, q, n, psub1, qsub1, e, d, d_p, d_q, qInv, res;
    mp_err   err = MP_OKAY;
    SECStatus rv = SECSuccess;
    MP_DIGITS(&n)    = 0;
    MP_DIGITS(&psub1)= 0;
    MP_DIGITS(&qsub1)= 0;
    MP_DIGITS(&e)    = 0;
    MP_DIGITS(&d)    = 0;
    MP_DIGITS(&d_p)  = 0;
    MP_DIGITS(&d_q)  = 0;
    MP_DIGITS(&qInv) = 0;
    MP_DIGITS(&res)  = 0;
    CHECK_MPI_OK( mp_init(&n)    );
    CHECK_MPI_OK( mp_init(&p)    );
    CHECK_MPI_OK( mp_init(&q)    );
    CHECK_MPI_OK( mp_init(&psub1));
    CHECK_MPI_OK( mp_init(&qsub1));
    CHECK_MPI_OK( mp_init(&e)    );
    CHECK_MPI_OK( mp_init(&d)    );
    CHECK_MPI_OK( mp_init(&d_p)  );
    CHECK_MPI_OK( mp_init(&d_q)  );
    CHECK_MPI_OK( mp_init(&qInv) );
    CHECK_MPI_OK( mp_init(&res)  );
    SECITEM_TO_MPINT(key->modulus,         &n);
    SECITEM_TO_MPINT(key->prime1,          &p);
    SECITEM_TO_MPINT(key->prime2,          &q);
    SECITEM_TO_MPINT(key->publicExponent,  &e);
    SECITEM_TO_MPINT(key->privateExponent, &d);
    SECITEM_TO_MPINT(key->exponent1,       &d_p);
    SECITEM_TO_MPINT(key->exponent2,       &d_q);
    SECITEM_TO_MPINT(key->coefficient,     &qInv);
    
    if (mp_cmp(&p, &q) <= 0) {
	
	SECItem tmp;
	mp_exch(&p, &q);
	mp_exch(&d_p,&d_q);
	tmp = key->prime1;
	key->prime1 = key->prime2;
	key->prime2 = tmp;
	tmp = key->exponent1;
	key->exponent1 = key->exponent2;
	key->exponent2 = tmp;
    }
#define VERIFY_MPI_EQUAL(m1, m2) \
    if (mp_cmp(m1, m2) != 0) {   \
	rv = SECFailure;         \
	goto cleanup;            \
    }
#define VERIFY_MPI_EQUAL_1(m)    \
    if (mp_cmp_d(m, 1) != 0) {   \
	rv = SECFailure;         \
	goto cleanup;            \
    }
    


    
    CHECK_MPI_OK( mp_mul(&p, &q, &res) );
    VERIFY_MPI_EQUAL(&res, &n);
    
    CHECK_MPI_OK( mp_sub_d(&p, 1, &psub1) );
    CHECK_MPI_OK( mp_gcd(&e, &psub1, &res) );
    VERIFY_MPI_EQUAL_1(&res);
    
    CHECK_MPI_OK( mp_sub_d(&q, 1, &qsub1) );
    CHECK_MPI_OK( mp_gcd(&e, &qsub1, &res) );
    VERIFY_MPI_EQUAL_1(&res);
    
    CHECK_MPI_OK( mp_mulmod(&d, &e, &psub1, &res) );
    VERIFY_MPI_EQUAL_1(&res);
    
    CHECK_MPI_OK( mp_mulmod(&d, &e, &qsub1, &res) );
    VERIFY_MPI_EQUAL_1(&res);
    


    
    CHECK_MPI_OK( mp_mod(&d, &psub1, &res) );
    if (mp_cmp(&d_p, &res) != 0) {
	
	CHECK_SEC_OK( swap_in_key_value(key->arena, &res, &key->exponent1) );
    }
    
    CHECK_MPI_OK( mp_mod(&d, &qsub1, &res) );
    if (mp_cmp(&d_q, &res) != 0) {
	
	CHECK_SEC_OK( swap_in_key_value(key->arena, &res, &key->exponent2) );
    }
    
    CHECK_MPI_OK( mp_mulmod(&q, &qInv, &p, &res) );
    if (mp_cmp_d(&res, 1) != 0) {
	
	CHECK_MPI_OK( mp_invmod(&q, &p, &qInv) );
	CHECK_SEC_OK( swap_in_key_value(key->arena, &qInv, &key->coefficient) );
    }
cleanup:
    mp_clear(&n);
    mp_clear(&p);
    mp_clear(&q);
    mp_clear(&psub1);
    mp_clear(&qsub1);
    mp_clear(&e);
    mp_clear(&d);
    mp_clear(&d_p);
    mp_clear(&d_q);
    mp_clear(&qInv);
    mp_clear(&res);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    return rv;
}

static SECStatus RSA_Init(void)
{
    if (PR_CallOnce(&coBPInit, init_blinding_params_list) != PR_SUCCESS) {
        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
        return SECFailure;
    }
    return SECSuccess;
}

SECStatus BL_Init(void)
{
    return RSA_Init();
}


void RSA_Cleanup(void)
{
    if (!coBPInit.initialized)
	return;

    while (!PR_CLIST_IS_EMPTY(&blindingParamsList.head))
    {
	struct RSABlindingParamsStr * rsabp = (struct RSABlindingParamsStr *)
	    PR_LIST_HEAD(&blindingParamsList.head);
	PR_REMOVE_LINK(&rsabp->link);
	mp_clear(&rsabp->f);
	mp_clear(&rsabp->g);
	SECITEM_FreeItem(&rsabp->modulus,PR_FALSE);
	PORT_Free(rsabp);
    }

    if (blindingParamsList.lock)
    {
	SKIP_AFTER_FORK(PZ_DestroyLock(blindingParamsList.lock));
	blindingParamsList.lock = NULL;
    }

    coBPInit.initialized = 0;
    coBPInit.inProgress = 0;
    coBPInit.status = 0;
}






void BL_Cleanup(void)
{
    RSA_Cleanup();
}

PRBool parentForkedAfterC_Initialize;




void BL_SetForkState(PRBool forked)
{
    parentForkedAfterC_Initialize = forked;
}

