








































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


#define RSA_BLINDING_PARAMS_MAX_CACHE_SIZE 20


#define BAD_RSA_KEY_SIZE(modLen, expLen) \
    ((expLen) > (modLen) || (modLen) > RSA_MAX_MODULUS_BITS/8 || \
    (expLen) > RSA_MAX_EXPONENT_BITS/8)

struct blindingParamsStr;
typedef struct blindingParamsStr blindingParams;

struct blindingParamsStr {
    blindingParams *next;
    mp_int         f, g;             
    int            counter;          
};









struct RSABlindingParamsStr
{
    
    PRCList   link;                  
    SECItem   modulus;               
    blindingParams *free, *bp;       
    blindingParams array[RSA_BLINDING_PARAMS_MAX_CACHE_SIZE];
};
typedef struct RSABlindingParamsStr RSABlindingParams;









struct RSABlindingParamsListStr
{
    PZLock  *lock;   
    PRCondVar *cVar; 
    int  waitCount;  
    PRCList  head;   
};




static struct RSABlindingParamsListStr blindingParamsList = { 0 };


#define RSA_BLINDING_PARAMS_MAX_REUSE 50



static PRBool nssRSAUseBlinding = PR_TRUE;

static SECStatus
rsa_build_from_primes(mp_int *p, mp_int *q, 
		mp_int *e, PRBool needPublicExponent, 
		mp_int *d, PRBool needPrivateExponent,
		RSAPrivateKey *key, unsigned int keySizeInBits)
{
    mp_int n, phi;
    mp_int psub1, qsub1, tmp;
    mp_err   err = MP_OKAY;
    SECStatus rv = SECSuccess;
    MP_DIGITS(&n)     = 0;
    MP_DIGITS(&phi)   = 0;
    MP_DIGITS(&psub1) = 0;
    MP_DIGITS(&qsub1) = 0;
    MP_DIGITS(&tmp)   = 0;
    CHECK_MPI_OK( mp_init(&n)     );
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

    
    PORT_Assert(!(needPublicExponent && needPrivateExponent));

    
    CHECK_MPI_OK( mp_sub_d(p, 1, &psub1) );
    CHECK_MPI_OK( mp_sub_d(q, 1, &qsub1) );
    if (needPublicExponent || needPrivateExponent) {
	CHECK_MPI_OK( mp_mul(&psub1, &qsub1, &phi) );
	
	
	if (needPublicExponent) {
	    err = mp_invmod(d, &phi, e);
	} else {
	    err = mp_invmod(e, &phi, d);
	}
    } else {
	err = MP_OKAY;
    }
    
    if (err != MP_OKAY) {
	if (err == MP_UNDEF) {
	    PORT_SetError(SEC_ERROR_NEED_RANDOM);
	    err = MP_OKAY; 
	    rv = SECFailure;
	}
	goto cleanup;
    }

    
    CHECK_MPI_OK( mp_mod(d, &psub1, &tmp) );
    MPINT_TO_SECITEM(&tmp, &key->exponent1, key->arena);
    
    CHECK_MPI_OK( mp_mod(d, &qsub1, &tmp) );
    MPINT_TO_SECITEM(&tmp, &key->exponent2, key->arena);
    
    CHECK_MPI_OK( mp_invmod(q, p, &tmp) );
    MPINT_TO_SECITEM(&tmp, &key->coefficient, key->arena);

    
    key->modulus.data = NULL;
    MPINT_TO_SECITEM(&n, &key->modulus, key->arena);
    key->privateExponent.data = NULL;
    MPINT_TO_SECITEM(d, &key->privateExponent, key->arena);
    key->publicExponent.data = NULL;
    MPINT_TO_SECITEM(e, &key->publicExponent, key->arena);
    key->prime1.data = NULL;
    MPINT_TO_SECITEM(p, &key->prime1, key->arena);
    key->prime2.data = NULL;
    MPINT_TO_SECITEM(q, &key->prime2, key->arena);
cleanup:
    mp_clear(&n);
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
    mp_int p, q, e, d;
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
    key = PORT_ArenaZNew(arena, RSAPrivateKey);
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
    MP_DIGITS(&d) = 0;
    CHECK_MPI_OK( mp_init(&p) );
    CHECK_MPI_OK( mp_init(&q) );
    CHECK_MPI_OK( mp_init(&e) );
    CHECK_MPI_OK( mp_init(&d) );
    
    SECITEM_AllocItem(arena, &key->version, 1);
    key->version.data[0] = 0;
    
    SECITEM_TO_MPINT(*publicExponent, &e);
    kiter = 0;
    do {
	prerr = 0;
	PORT_SetError(0);
	CHECK_SEC_OK( generate_prime(&p, primeLen) );
	CHECK_SEC_OK( generate_prime(&q, primeLen) );
	
	if (mp_cmp(&p, &q) < 0)
	    mp_exch(&p, &q);
	
	rv = rsa_build_from_primes(&p, &q, 
			&e, PR_FALSE,  
			&d, PR_TRUE,   
			key, keySizeInBits);
	if (rv == SECSuccess)
	    break; 
	prerr = PORT_GetError();
	kiter++;
	
    } while (prerr == SEC_ERROR_NEED_RANDOM && kiter < MAX_KEY_GEN_ATTEMPTS);
    if (prerr)
	goto cleanup;
cleanup:
    mp_clear(&p);
    mp_clear(&q);
    mp_clear(&e);
    mp_clear(&d);
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

mp_err
rsa_is_prime(mp_int *p) {
    int res;

    
    res = mpp_fermat(p, 2);
    if (res != MP_OKAY) {
	return res;
    }

    
    res = mpp_pprime(p, 2);
    return res;
}
















































static mp_err
rsa_get_primes_from_exponents(mp_int *e, mp_int *d, mp_int *p, mp_int *q,
			      mp_int *n, PRBool hasModulus, 
			      unsigned int keySizeInBits)
{
    mp_int kphi; 
    mp_int k;    
    mp_int phi;  
    mp_int s;    
    mp_int r;    
    mp_int tmp; 
    mp_int sqrt; 
    mp_err err = MP_OKAY;
    unsigned int order_k;

    MP_DIGITS(&kphi) = 0;
    MP_DIGITS(&phi) = 0;
    MP_DIGITS(&s) = 0;
    MP_DIGITS(&k) = 0;
    MP_DIGITS(&r) = 0;
    MP_DIGITS(&tmp) = 0;
    MP_DIGITS(&sqrt) = 0;
    CHECK_MPI_OK( mp_init(&kphi) );
    CHECK_MPI_OK( mp_init(&phi) );
    CHECK_MPI_OK( mp_init(&s) );
    CHECK_MPI_OK( mp_init(&k) );
    CHECK_MPI_OK( mp_init(&r) );
    CHECK_MPI_OK( mp_init(&tmp) );
    CHECK_MPI_OK( mp_init(&sqrt) );

    

















    if (mpl_significant_bits(e) > 23) {
	err=MP_RANGE;
	goto cleanup;
    }

    
    CHECK_MPI_OK( mp_mul(e, d, &kphi) );
    CHECK_MPI_OK( mp_sub_d(&kphi, 1, &kphi) );


    





    order_k = (unsigned)mpl_significant_bits(&kphi) - keySizeInBits;

    
    
    CHECK_MPI_OK( mp_2expt(&k,keySizeInBits-1) );
    CHECK_MPI_OK( mp_div(&kphi, &k, &k, NULL));
    if (mp_cmp(&k,e) >= 0) {
	
        CHECK_MPI_OK( mp_sub_d(e, 1, &k) );
    }

    
    

    
    
    if (hasModulus) {
	CHECK_MPI_OK( mp_add_d(n, 1, &tmp) );
    } else {
	CHECK_MPI_OK( mp_sub_d(p, 1, &tmp) );
	CHECK_MPI_OK(mp_div(&kphi,&tmp,&kphi,&r));
	if (mp_cmp_z(&r) != 0) {
	    
	    err=MP_RANGE;
	    goto cleanup;
	}
	mp_zero(q);
	
    }

    
    for (; (err == MP_OKAY) && (mpl_significant_bits(&k) >= order_k); 
						err = mp_sub_d(&k, 1, &k)) {
	
	CHECK_MPI_OK(mp_div(&kphi,&k,&phi,&r));
	if (mp_cmp_z(&r) != 0) {
	    
	    continue;
	}
	
	if (!hasModulus) {
	    if ((unsigned)mpl_significant_bits(&phi) != keySizeInBits/2) {
		
		continue;
	    }
	    

	    if (mpp_divis_d(&phi,2) == MP_NO) {
		
		continue;
	    }
	    
	    CHECK_MPI_OK(mp_add_d(&phi, 1, &tmp));
	    
	    
	    err = rsa_is_prime(&tmp);
	    if (err != MP_OKAY) {
		if (err == MP_NO) {
		    
		    err = MP_OKAY;
        	    continue;
		}
		goto cleanup;
	    }
	    


















	    if (mp_cmp_z(q) != 0) {
		

		err = MP_RANGE;
		break;
	    }
	    

	    CHECK_MPI_OK(mp_copy(&tmp, q));
	    continue;
	}
	
	
	if ((unsigned)mpl_significant_bits(&phi) != keySizeInBits) {
	    
	    continue;
	}
	

	if (mpp_divis_d(&phi,4) == MP_NO) {
	    
	    continue;
	}
	
	CHECK_MPI_OK( mp_sub(&tmp, &phi, &s) );
	CHECK_MPI_OK( mp_div_2(&s, &s) );

	
	CHECK_MPI_OK(mp_sqr(&s,&sqrt));
	CHECK_MPI_OK(mp_sub(&sqrt,n,&r));  
	CHECK_MPI_OK(mp_sqrt(&r,&sqrt));
	
	
	
	CHECK_MPI_OK(mp_sqr(&sqrt,q)); 
	if (mp_cmp(&r,q) != 0) {
	    
	   CHECK_MPI_OK(mp_add_d(&sqrt,1,&sqrt));
	   CHECK_MPI_OK(mp_sqr(&sqrt,q));
	   if (mp_cmp(&r,q) != 0) {
		
		continue;
	    }
	}

	










	
	
	CHECK_MPI_OK(mp_add(&s,&sqrt,p));
	CHECK_MPI_OK(mp_sub(&s,&sqrt,q));
	break;
    }
    if ((unsigned)mpl_significant_bits(&k) < order_k) {
	if (hasModulus || (mp_cmp_z(q) == 0)) {
	    

	    err = MP_RANGE; 
	}
    }
cleanup:
    mp_clear(&kphi);
    mp_clear(&phi);
    mp_clear(&s);
    mp_clear(&k);
    mp_clear(&r);
    mp_clear(&tmp);
    mp_clear(&sqrt);
    return err;
}
     

















































SECStatus
RSA_PopulatePrivateKey(RSAPrivateKey *key)
{
    PRArenaPool *arena = NULL;
    PRBool needPublicExponent = PR_TRUE;
    PRBool needPrivateExponent = PR_TRUE;
    PRBool hasModulus = PR_FALSE;
    unsigned int keySizeInBits = 0;
    int prime_count = 0;
    
    mp_int p, q, e, d, n;
    
    mp_int r;
    mp_err err = 0;
    SECStatus rv = SECFailure;

    MP_DIGITS(&p) = 0;
    MP_DIGITS(&q) = 0;
    MP_DIGITS(&e) = 0;
    MP_DIGITS(&d) = 0;
    MP_DIGITS(&n) = 0;
    MP_DIGITS(&r) = 0;
    CHECK_MPI_OK( mp_init(&p) );
    CHECK_MPI_OK( mp_init(&q) );
    CHECK_MPI_OK( mp_init(&e) );
    CHECK_MPI_OK( mp_init(&d) );
    CHECK_MPI_OK( mp_init(&n) );
    CHECK_MPI_OK( mp_init(&r) );
 
    
    if (key->arena == NULL) {
	arena = PORT_NewArena(NSS_FREEBL_DEFAULT_CHUNKSIZE);
	if (!arena) {
	    goto cleanup;
	}
	key->arena = arena;
    }

    
    if (key->publicExponent.data) {
        SECITEM_TO_MPINT(key->publicExponent, &e);
	needPublicExponent = PR_FALSE;
    } 
    if (key->privateExponent.data) {
        SECITEM_TO_MPINT(key->privateExponent, &d);
	needPrivateExponent = PR_FALSE;
    }
    if (needPrivateExponent && needPublicExponent) {
	
	err = MP_BADARG;
	goto cleanup;
    }

    



    if (key->prime1.data) {
	int primeLen = key->prime1.len;
	if (key->prime1.data[0] == 0) {
	   primeLen--;
	}
	keySizeInBits = primeLen * 2 * BITS_PER_BYTE;
        SECITEM_TO_MPINT(key->prime1, &p);
	prime_count++;
    }
    if (key->prime2.data) {
	int primeLen = key->prime2.len;
	if (key->prime2.data[0] == 0) {
	   primeLen--;
	}
	keySizeInBits = primeLen * 2 * BITS_PER_BYTE;
        SECITEM_TO_MPINT(key->prime2, prime_count ? &q : &p);
	prime_count++;
    }
    
    if (key->modulus.data) {
	int modLen = key->modulus.len;
	if (key->modulus.data[0] == 0) {
	   modLen--;
	}
	keySizeInBits = modLen * BITS_PER_BYTE;
	SECITEM_TO_MPINT(key->modulus, &n);
	hasModulus = PR_TRUE;
    }
    
    if ((prime_count == 1) && (hasModulus)) {
	mp_div(&n,&p,&q,&r);
	if (mp_cmp_z(&r) != 0) {
	   
	   err = MP_BADARG;
	   goto cleanup;
	}
	prime_count++;
    }

    

    if (prime_count < 2) {
	

	if (!needPublicExponent && !needPrivateExponent &&
		((prime_count > 0) || hasModulus)) {
	    CHECK_MPI_OK(rsa_get_primes_from_exponents(&e,&d,&p,&q,
			&n,hasModulus,keySizeInBits));
	} else {
	    
	    err = MP_BADARG;
	    goto cleanup;
	}
     }

     
     if (mp_cmp(&p, &q) < 0)
	mp_exch(&p, &q);

     

     rv = rsa_build_from_primes(&p, &q, 
			&e, needPublicExponent,
			&d, needPrivateExponent,
			key, keySizeInBits);
cleanup:
    mp_clear(&p);
    mp_clear(&q);
    mp_clear(&e);
    mp_clear(&d);
    mp_clear(&n);
    mp_clear(&r);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    if (rv && arena) {
	PORT_FreeArena(arena, PR_TRUE);
	key->arena = NULL;
    }
    return rv;
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
    blindingParamsList.cVar = PR_NewCondVar( blindingParamsList.lock );
    if (!blindingParamsList.cVar) {
	PORT_SetError(SEC_ERROR_NO_MEMORY);
	return PR_FAILURE;
    }
    blindingParamsList.waitCount = 0;
    PR_INIT_CLIST(&blindingParamsList.head);
    return PR_SUCCESS;
}

static SECStatus
generate_blinding_params(RSAPrivateKey *key, mp_int* f, mp_int* g, mp_int *n, 
                         unsigned int modLen)
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
    
    CHECK_MPI_OK( mp_exptmod(&k, &e, n, f) );
    
    CHECK_MPI_OK( mp_invmod(&k, n, g) );
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
init_blinding_params(RSABlindingParams *rsabp, RSAPrivateKey *key,
                     mp_int *n, unsigned int modLen)
{
    blindingParams * bp = rsabp->array;
    SECStatus rv = SECSuccess;
    mp_err err = MP_OKAY;
    int i = 0;

    
    PR_INIT_CLIST(&rsabp->link);
    for (i = 0; i < RSA_BLINDING_PARAMS_MAX_CACHE_SIZE; ++i, ++bp) {
    	bp->next = bp + 1;
	MP_DIGITS(&bp->f) = 0;
	MP_DIGITS(&bp->g) = 0;
	bp->counter = 0;
    }
    

 
    rsabp->array[RSA_BLINDING_PARAMS_MAX_CACHE_SIZE - 1].next = NULL;
    
    bp          = rsabp->array;
    rsabp->bp   = NULL;
    rsabp->free = bp;

    
    SECITEM_CopyItem(NULL, &rsabp->modulus, &key->modulus);

    return SECSuccess;
}

static SECStatus
get_blinding_params(RSAPrivateKey *key, mp_int *n, unsigned int modLen,
                    mp_int *f, mp_int *g)
{
    RSABlindingParams *rsabp           = NULL;
    blindingParams    *bpUnlinked      = NULL;
    blindingParams    *bp, *prevbp     = NULL;
    PRCList           *el;
    SECStatus          rv              = SECSuccess;
    mp_err             err             = MP_OKAY;
    int                cmp             = -1;
    PRBool             holdingLock     = PR_FALSE;

    do {
	if (blindingParamsList.lock == NULL) {
	    PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	    return SECFailure;
	}
	
	PZ_Lock(blindingParamsList.lock);
	holdingLock = PR_TRUE;

	
	for (el = PR_NEXT_LINK(&blindingParamsList.head);
	     el != &blindingParamsList.head;
	     el = PR_NEXT_LINK(el)) {
	    rsabp = (RSABlindingParams *)el;
	    cmp = SECITEM_CompareItem(&rsabp->modulus, &key->modulus);
	    if (cmp >= 0) {
		
		break;
	    }
	}

	if (cmp) {
	    


	    rsabp = PORT_ZNew(RSABlindingParams);
	    if (!rsabp) {
		PORT_SetError(SEC_ERROR_NO_MEMORY);
		goto cleanup;
	    }

	    rv = init_blinding_params(rsabp, key, n, modLen);
	    if (rv != SECSuccess) {
		PORT_ZFree(rsabp, sizeof(RSABlindingParams));
		goto cleanup;
	    }

	    





	    PR_INSERT_BEFORE(&rsabp->link, el);
	}

	


	while (0 != (bp = rsabp->bp)) {
	    if (--(bp->counter) > 0) {
		
		
		CHECK_MPI_OK( mp_copy(&bp->f, f) );
		CHECK_MPI_OK( mp_copy(&bp->g, g) );

		PZ_Unlock(blindingParamsList.lock); 
		return SECSuccess;
	    }
	    


	    mp_exch(&bp->f, f);
	    mp_exch(&bp->g, g);
	    mp_clear( &bp->f );
	    mp_clear( &bp->g );
	    bp->counter = 0;
	    
	    rsabp->bp   = bp->next;
	    bp->next    = rsabp->free;
	    rsabp->free = bp;
	    


	    if (blindingParamsList.waitCount > 0) {
		PR_NotifyCondVar( blindingParamsList.cVar );
		blindingParamsList.waitCount--;
	    }
	    PZ_Unlock(blindingParamsList.lock); 
	    return SECSuccess;
	}
	

	prevbp = NULL;
	if ((bp = rsabp->free) != NULL) {
	    
	    rsabp->free  = bp->next;
	    bp->next     = NULL;
	    bpUnlinked   = bp;  

	    PZ_Unlock(blindingParamsList.lock); 
	    holdingLock = PR_FALSE;
	    
	    CHECK_SEC_OK( generate_blinding_params(key, f, g, n, modLen ) );

	    
	    CHECK_MPI_OK( mp_init( &bp->f) );
	    CHECK_MPI_OK( mp_init( &bp->g) );
	    CHECK_MPI_OK( mp_copy( f, &bp->f) );
	    CHECK_MPI_OK( mp_copy( g, &bp->g) );

	    
	    PZ_Lock(blindingParamsList.lock);
	    holdingLock = PR_TRUE;
	    
	    bp->counter = RSA_BLINDING_PARAMS_MAX_REUSE;
	    bp->next    = rsabp->bp;
	    rsabp->bp   = bp;
	    bpUnlinked  = NULL;
	    


	    if (blindingParamsList.waitCount > 0) {
		PR_NotifyAllCondVar( blindingParamsList.cVar );
		blindingParamsList.waitCount = 0;
	    }
	    PZ_Unlock(blindingParamsList.lock);
	    return SECSuccess;
	}
	





	blindingParamsList.waitCount++;
	PR_WaitCondVar( blindingParamsList.cVar, PR_INTERVAL_NO_TIMEOUT );
	PZ_Unlock(blindingParamsList.lock); 
	holdingLock = PR_FALSE;
    } while (1);

cleanup:
    
    if (bpUnlinked) {
	if (!holdingLock) {
	    PZ_Lock(blindingParamsList.lock);
	    holdingLock = PR_TRUE;
	}
	bp = bpUnlinked;
	mp_clear( &bp->f );
	mp_clear( &bp->g );
	bp->counter = 0;
    	
	bp->next    = rsabp->free;
	rsabp->free = bp;
    }
    if (holdingLock) {
	PZ_Unlock(blindingParamsList.lock);
	holdingLock = PR_FALSE;
    }
    if (err) {
	MP_TO_SEC_ERROR(err);
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
    blindingParams * bp = NULL;
    if (!coBPInit.initialized)
	return;

    while (!PR_CLIST_IS_EMPTY(&blindingParamsList.head)) {
	RSABlindingParams *rsabp = 
	    (RSABlindingParams *)PR_LIST_HEAD(&blindingParamsList.head);
	PR_REMOVE_LINK(&rsabp->link);
	
	while (rsabp->bp != NULL) {
	    bp = rsabp->bp;
	    rsabp->bp = rsabp->bp->next;
	    mp_clear( &bp->f );
	    mp_clear( &bp->g );
	}
	SECITEM_FreeItem(&rsabp->modulus,PR_FALSE);
	PORT_Free(rsabp);
    }

    if (blindingParamsList.cVar) {
	PR_DestroyCondVar(blindingParamsList.cVar);
	blindingParamsList.cVar = NULL;
    }

    if (blindingParamsList.lock) {
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

