






































#ifdef FREEBL_NO_DEPEND
#include "stubs.h"
#endif


#include "blapi.h"
#include "prerr.h"
#include "secerr.h"
#include "secmpi.h"
#include "secitem.h"
#include "mplogic.h"
#include "ec.h"
#include "ecl.h"

#ifdef NSS_ENABLE_ECC




PRBool
ec_point_at_infinity(SECItem *pointP)
{
    unsigned int i;

    for (i = 1; i < pointP->len; i++) {
	if (pointP->data[i] != 0x00) return PR_FALSE;
    }

    return PR_TRUE;
}





SECStatus 
ec_points_mul(const ECParams *params, const mp_int *k1, const mp_int *k2,
             const SECItem *pointP, SECItem *pointQ)
{
    mp_int Px, Py, Qx, Qy;
    mp_int Gx, Gy, order, irreducible, a, b;
#if 0 
    unsigned int irr_arr[5];
#endif
    ECGroup *group = NULL;
    SECStatus rv = SECFailure;
    mp_err err = MP_OKAY;
    int len;

#if EC_DEBUG
    int i;
    char mpstr[256];

    printf("ec_points_mul: params [len=%d]:", params->DEREncoding.len);
    for (i = 0; i < params->DEREncoding.len; i++) 
	    printf("%02x:", params->DEREncoding.data[i]);
    printf("\n");

	if (k1 != NULL) {
		mp_tohex(k1, mpstr);
		printf("ec_points_mul: scalar k1: %s\n", mpstr);
		mp_todecimal(k1, mpstr);
		printf("ec_points_mul: scalar k1: %s (dec)\n", mpstr);
	}

	if (k2 != NULL) {
		mp_tohex(k2, mpstr);
		printf("ec_points_mul: scalar k2: %s\n", mpstr);
		mp_todecimal(k2, mpstr);
		printf("ec_points_mul: scalar k2: %s (dec)\n", mpstr);
	}

	if (pointP != NULL) {
		printf("ec_points_mul: pointP [len=%d]:", pointP->len);
		for (i = 0; i < pointP->len; i++) 
			printf("%02x:", pointP->data[i]);
		printf("\n");
	}
#endif

	
	len = (params->fieldID.size + 7) >> 3;
	if (pointP != NULL) {
		if ((pointP->data[0] != EC_POINT_FORM_UNCOMPRESSED) ||
			(pointP->len != (2 * len + 1))) {
			PORT_SetError(SEC_ERROR_UNSUPPORTED_EC_POINT_FORM);
			return SECFailure;
		};
	}

	MP_DIGITS(&Px) = 0;
	MP_DIGITS(&Py) = 0;
	MP_DIGITS(&Qx) = 0;
	MP_DIGITS(&Qy) = 0;
	MP_DIGITS(&Gx) = 0;
	MP_DIGITS(&Gy) = 0;
	MP_DIGITS(&order) = 0;
	MP_DIGITS(&irreducible) = 0;
	MP_DIGITS(&a) = 0;
	MP_DIGITS(&b) = 0;
	CHECK_MPI_OK( mp_init(&Px) );
	CHECK_MPI_OK( mp_init(&Py) );
	CHECK_MPI_OK( mp_init(&Qx) );
	CHECK_MPI_OK( mp_init(&Qy) );
	CHECK_MPI_OK( mp_init(&Gx) );
	CHECK_MPI_OK( mp_init(&Gy) );
	CHECK_MPI_OK( mp_init(&order) );
	CHECK_MPI_OK( mp_init(&irreducible) );
	CHECK_MPI_OK( mp_init(&a) );
	CHECK_MPI_OK( mp_init(&b) );

	if ((k2 != NULL) && (pointP != NULL)) {
		
		CHECK_MPI_OK( mp_read_unsigned_octets(&Px, pointP->data + 1, (mp_size) len) );
		CHECK_MPI_OK( mp_read_unsigned_octets(&Py, pointP->data + 1 + len, (mp_size) len) );
	}

	
	if (params->name != ECCurve_noName) {
		group = ECGroup_fromName(params->name);
	}

#if 0 
	if (group == NULL) {
		
		CHECK_MPI_OK( mp_read_unsigned_octets(&Gx, params->base.data + 1, 
										  (mp_size) len) );
		CHECK_MPI_OK( mp_read_unsigned_octets(&Gy, params->base.data + 1 + len, 
										  (mp_size) len) );
		SECITEM_TO_MPINT( params->order, &order );
		SECITEM_TO_MPINT( params->curve.a, &a );
		SECITEM_TO_MPINT( params->curve.b, &b );
		if (params->fieldID.type == ec_field_GFp) {
			SECITEM_TO_MPINT( params->fieldID.u.prime, &irreducible );
			group = ECGroup_consGFp(&irreducible, &a, &b, &Gx, &Gy, &order, params->cofactor);
		} else {
			SECITEM_TO_MPINT( params->fieldID.u.poly, &irreducible );
			irr_arr[0] = params->fieldID.size;
			irr_arr[1] = params->fieldID.k1;
			irr_arr[2] = params->fieldID.k2;
			irr_arr[3] = params->fieldID.k3;
			irr_arr[4] = 0;
			group = ECGroup_consGF2m(&irreducible, irr_arr, &a, &b, &Gx, &Gy, &order, params->cofactor);
		}
	}
#endif
	if (group == NULL)
		goto cleanup;

	if ((k2 != NULL) && (pointP != NULL)) {
		CHECK_MPI_OK( ECPoints_mul(group, k1, k2, &Px, &Py, &Qx, &Qy) );
	} else {
		CHECK_MPI_OK( ECPoints_mul(group, k1, NULL, NULL, NULL, &Qx, &Qy) );
    }

    
    pointQ->data[0] = EC_POINT_FORM_UNCOMPRESSED;
    CHECK_MPI_OK( mp_to_fixlen_octets(&Qx, pointQ->data + 1,
	                              (mp_size) len) );
    CHECK_MPI_OK( mp_to_fixlen_octets(&Qy, pointQ->data + 1 + len,
	                              (mp_size) len) );

    rv = SECSuccess;

#if EC_DEBUG
    printf("ec_points_mul: pointQ [len=%d]:", pointQ->len);
    for (i = 0; i < pointQ->len; i++) 
	    printf("%02x:", pointQ->data[i]);
    printf("\n");
#endif

cleanup:
    ECGroup_free(group);
    mp_clear(&Px);
    mp_clear(&Py);
    mp_clear(&Qx);
    mp_clear(&Qy);
    mp_clear(&Gx);
    mp_clear(&Gy);
    mp_clear(&order);
    mp_clear(&irreducible);
    mp_clear(&a);
    mp_clear(&b);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }

    return rv;
}
#endif 





SECStatus 
ec_NewKey(ECParams *ecParams, ECPrivateKey **privKey, 
    const unsigned char *privKeyBytes, int privKeyLen)
{
    SECStatus rv = SECFailure;
#ifdef NSS_ENABLE_ECC
    PRArenaPool *arena;
    ECPrivateKey *key;
    mp_int k;
    mp_err err = MP_OKAY;
    int len;

#if EC_DEBUG
    printf("ec_NewKey called\n");
#endif
    MP_DIGITS(&k) = 0;

    if (!ecParams || !privKey || !privKeyBytes || (privKeyLen < 0)) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    
    if (!(arena = PORT_NewArena(NSS_FREEBL_DEFAULT_CHUNKSIZE)))
	return SECFailure;

    key = (ECPrivateKey *)PORT_ArenaZAlloc(arena, sizeof(ECPrivateKey));
    if (!key) {
	PORT_FreeArena(arena, PR_TRUE);
	return SECFailure;
    }

    
    SECITEM_AllocItem(arena, &key->version, 1);
    key->version.data[0] = 1;

    


    key->ecParams.arena = arena;
    key->ecParams.type = ecParams->type;
    key->ecParams.fieldID.size = ecParams->fieldID.size;
    key->ecParams.fieldID.type = ecParams->fieldID.type;
    if (ecParams->fieldID.type == ec_field_GFp) {
	CHECK_SEC_OK(SECITEM_CopyItem(arena, &key->ecParams.fieldID.u.prime,
	    &ecParams->fieldID.u.prime));
    } else {
	CHECK_SEC_OK(SECITEM_CopyItem(arena, &key->ecParams.fieldID.u.poly,
	    &ecParams->fieldID.u.poly));
    }
    key->ecParams.fieldID.k1 = ecParams->fieldID.k1;
    key->ecParams.fieldID.k2 = ecParams->fieldID.k2;
    key->ecParams.fieldID.k3 = ecParams->fieldID.k3;
    CHECK_SEC_OK(SECITEM_CopyItem(arena, &key->ecParams.curve.a,
	&ecParams->curve.a));
    CHECK_SEC_OK(SECITEM_CopyItem(arena, &key->ecParams.curve.b,
	&ecParams->curve.b));
    CHECK_SEC_OK(SECITEM_CopyItem(arena, &key->ecParams.curve.seed,
	&ecParams->curve.seed));
    CHECK_SEC_OK(SECITEM_CopyItem(arena, &key->ecParams.base,
	&ecParams->base));
    CHECK_SEC_OK(SECITEM_CopyItem(arena, &key->ecParams.order,
	&ecParams->order));
    key->ecParams.cofactor = ecParams->cofactor;
    CHECK_SEC_OK(SECITEM_CopyItem(arena, &key->ecParams.DEREncoding,
	&ecParams->DEREncoding));
    key->ecParams.name = ecParams->name;
    CHECK_SEC_OK(SECITEM_CopyItem(arena, &key->ecParams.curveOID,
	&ecParams->curveOID));

    len = (ecParams->fieldID.size + 7) >> 3;
    SECITEM_AllocItem(arena, &key->publicValue, 2*len + 1);
    len = ecParams->order.len;
    SECITEM_AllocItem(arena, &key->privateValue, len);

    
    if (privKeyLen >= len) {
	memcpy(key->privateValue.data, privKeyBytes, len);
    } else {
	memset(key->privateValue.data, 0, (len - privKeyLen));
	memcpy(key->privateValue.data + (len - privKeyLen), privKeyBytes, privKeyLen);
    }

    
    CHECK_MPI_OK( mp_init(&k) );
    CHECK_MPI_OK( mp_read_unsigned_octets(&k, key->privateValue.data, 
	(mp_size) len) );

    rv = ec_points_mul(ecParams, &k, NULL, NULL, &(key->publicValue));
    if (rv != SECSuccess) goto cleanup;
    *privKey = key;

cleanup:
    mp_clear(&k);
    if (rv)
	PORT_FreeArena(arena, PR_TRUE);

#if EC_DEBUG
    printf("ec_NewKey returning %s\n", 
	(rv == SECSuccess) ? "success" : "failure");
#endif
#else
    PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);
#endif 

    return rv;

}






SECStatus 
EC_NewKeyFromSeed(ECParams *ecParams, ECPrivateKey **privKey, 
    const unsigned char *seed, int seedlen)
{
    SECStatus rv = SECFailure;
#ifdef NSS_ENABLE_ECC
    rv = ec_NewKey(ecParams, privKey, seed, seedlen);
#else
    PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);
#endif 
    return rv;
}

#ifdef NSS_ENABLE_ECC












static unsigned char *
ec_GenerateRandomPrivateKey(const unsigned char *order, int len)
{
    SECStatus rv = SECSuccess;
    mp_err err;
    unsigned char *privKeyBytes = NULL;
    mp_int privKeyVal, order_1, one;

    MP_DIGITS(&privKeyVal) = 0;
    MP_DIGITS(&order_1) = 0;
    MP_DIGITS(&one) = 0;
    CHECK_MPI_OK( mp_init(&privKeyVal) );
    CHECK_MPI_OK( mp_init(&order_1) );
    CHECK_MPI_OK( mp_init(&one) );

    



    if ((privKeyBytes = PORT_Alloc(2*len)) == NULL) goto cleanup;
    CHECK_SEC_OK( RNG_GenerateGlobalRandomBytes(privKeyBytes, 2*len) );
    CHECK_MPI_OK( mp_read_unsigned_octets(&privKeyVal, privKeyBytes, 2*len) );
    CHECK_MPI_OK( mp_read_unsigned_octets(&order_1, order, len) );
    CHECK_MPI_OK( mp_set_int(&one, 1) );
    CHECK_MPI_OK( mp_sub(&order_1, &one, &order_1) );
    CHECK_MPI_OK( mp_mod(&privKeyVal, &order_1, &privKeyVal) );
    CHECK_MPI_OK( mp_add(&privKeyVal, &one, &privKeyVal) );
    CHECK_MPI_OK( mp_to_fixlen_octets(&privKeyVal, privKeyBytes, len) );
    memset(privKeyBytes+len, 0, len);
cleanup:
    mp_clear(&privKeyVal);
    mp_clear(&order_1);
    mp_clear(&one);
    if (err < MP_OKAY) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    if (rv != SECSuccess && privKeyBytes) {
	PORT_Free(privKeyBytes);
	privKeyBytes = NULL;
    }
    return privKeyBytes;
}
#endif 





SECStatus 
EC_NewKey(ECParams *ecParams, ECPrivateKey **privKey)
{
    SECStatus rv = SECFailure;
#ifdef NSS_ENABLE_ECC
    int len;
    unsigned char *privKeyBytes = NULL;

    if (!ecParams) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    len = ecParams->order.len;
    privKeyBytes = ec_GenerateRandomPrivateKey(ecParams->order.data, len);
    if (privKeyBytes == NULL) goto cleanup;
    
    CHECK_SEC_OK( ec_NewKey(ecParams, privKey, privKeyBytes, len) );

cleanup:
    if (privKeyBytes) {
	PORT_ZFree(privKeyBytes, len);
    }
#if EC_DEBUG
    printf("EC_NewKey returning %s\n", 
	(rv == SECSuccess) ? "success" : "failure");
#endif
#else
    PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);
#endif 
    
    return rv;
}







SECStatus 
EC_ValidatePublicKey(ECParams *ecParams, SECItem *publicValue)
{
#ifdef NSS_ENABLE_ECC
    mp_int Px, Py;
    ECGroup *group = NULL;
    SECStatus rv = SECFailure;
    mp_err err = MP_OKAY;
    int len;

    if (!ecParams || !publicValue) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
	
    
    len = (ecParams->fieldID.size + 7) >> 3;
    if (publicValue->data[0] != EC_POINT_FORM_UNCOMPRESSED) {
	PORT_SetError(SEC_ERROR_UNSUPPORTED_EC_POINT_FORM);
	return SECFailure;
    } else if (publicValue->len != (2 * len + 1)) {
	PORT_SetError(SEC_ERROR_BAD_KEY);
	return SECFailure;
    }

    MP_DIGITS(&Px) = 0;
    MP_DIGITS(&Py) = 0;
    CHECK_MPI_OK( mp_init(&Px) );
    CHECK_MPI_OK( mp_init(&Py) );

    
    CHECK_MPI_OK( mp_read_unsigned_octets(&Px, publicValue->data + 1, (mp_size) len) );
    CHECK_MPI_OK( mp_read_unsigned_octets(&Py, publicValue->data + 1 + len, (mp_size) len) );

    
    group = ECGroup_fromName(ecParams->name);
    if (group == NULL) {
	








	if ((ecParams->name <= ECCurve_noName) ||
	    (ecParams->name >= ECCurve_pastLastCurve)) {
	    err = MP_BADARG;
	} else {
	    err = MP_UNDEF;
	}
	goto cleanup;
    }

    
    if ((err = ECPoint_validate(group, &Px, &Py)) < MP_YES) {
	if (err == MP_NO) {
	    PORT_SetError(SEC_ERROR_BAD_KEY);
	    rv = SECFailure;
	    err = MP_OKAY;  
	}
	goto cleanup;
    }

    rv = SECSuccess;

cleanup:
    ECGroup_free(group);
    mp_clear(&Px);
    mp_clear(&Py);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }
    return rv;
#else
    PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);
    return SECFailure;
#endif 
}











SECStatus 
ECDH_Derive(SECItem  *publicValue, 
            ECParams *ecParams,
            SECItem  *privateValue,
            PRBool    withCofactor,
            SECItem  *derivedSecret)
{
    SECStatus rv = SECFailure;
#ifdef NSS_ENABLE_ECC
    unsigned int len = 0;
    SECItem pointQ = {siBuffer, NULL, 0};
    mp_int k; 
    mp_int cofactor;
    mp_err err = MP_OKAY;
#if EC_DEBUG
    int i;
#endif

    if (!publicValue || !ecParams || !privateValue || 
	!derivedSecret) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    MP_DIGITS(&k) = 0;
    memset(derivedSecret, 0, sizeof *derivedSecret);
    len = (ecParams->fieldID.size + 7) >> 3;  
    pointQ.len = 2*len + 1;
    if ((pointQ.data = PORT_Alloc(2*len + 1)) == NULL) goto cleanup;

    CHECK_MPI_OK( mp_init(&k) );
    CHECK_MPI_OK( mp_read_unsigned_octets(&k, privateValue->data, 
	                                  (mp_size) privateValue->len) );

    if (withCofactor && (ecParams->cofactor != 1)) {
	    
	    MP_DIGITS(&cofactor) = 0;
	    CHECK_MPI_OK( mp_init(&cofactor) );
	    mp_set(&cofactor, ecParams->cofactor);
	    CHECK_MPI_OK( mp_mul(&k, &cofactor, &k) );
    }

    
    if (ec_points_mul(ecParams, NULL, &k, publicValue, &pointQ) != SECSuccess)
	goto cleanup;
    if (ec_point_at_infinity(&pointQ)) {
	PORT_SetError(SEC_ERROR_BAD_KEY);  
	goto cleanup;
    }

    


    SECITEM_AllocItem(NULL, derivedSecret, len);
    memcpy(derivedSecret->data, pointQ.data + 1, len);

    rv = SECSuccess;

#if EC_DEBUG
    printf("derived_secret:\n");
    for (i = 0; i < derivedSecret->len; i++) 
	printf("%02x:", derivedSecret->data[i]);
    printf("\n");
#endif

cleanup:
    mp_clear(&k);

    if (err) {
	MP_TO_SEC_ERROR(err);
    }

    if (pointQ.data) {
	PORT_ZFree(pointQ.data, 2*len + 1);
    }
#else
    PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);
#endif 

    return rv;
}





SECStatus 
ECDSA_SignDigestWithSeed(ECPrivateKey *key, SECItem *signature, 
    const SECItem *digest, const unsigned char *kb, const int kblen)
{
    SECStatus rv = SECFailure;
#ifdef NSS_ENABLE_ECC
    mp_int x1;
    mp_int d, k;     
    mp_int r, s;     
    mp_int n;
    mp_err err = MP_OKAY;
    ECParams *ecParams = NULL;
    SECItem kGpoint = { siBuffer, NULL, 0};
    int flen = 0;    
    unsigned olen;   
    unsigned obits;  

#if EC_DEBUG
    char mpstr[256];
#endif

    
    
    MP_DIGITS(&x1) = 0;
    MP_DIGITS(&d) = 0;
    MP_DIGITS(&k) = 0;
    MP_DIGITS(&r) = 0;
    MP_DIGITS(&s) = 0;
    MP_DIGITS(&n) = 0;

    
    if (!key || !signature || !digest || !kb || (kblen < 0)) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	goto cleanup;
    }

    ecParams = &(key->ecParams);
    flen = (ecParams->fieldID.size + 7) >> 3;
    olen = ecParams->order.len;  
    if (signature->data == NULL) {
	
	goto finish;
    }
    if (signature->len < 2*olen) {
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	goto cleanup;
    }


    CHECK_MPI_OK( mp_init(&x1) );
    CHECK_MPI_OK( mp_init(&d) );
    CHECK_MPI_OK( mp_init(&k) );
    CHECK_MPI_OK( mp_init(&r) );
    CHECK_MPI_OK( mp_init(&s) );
    CHECK_MPI_OK( mp_init(&n) );

    SECITEM_TO_MPINT( ecParams->order, &n );
    SECITEM_TO_MPINT( key->privateValue, &d );

    CHECK_MPI_OK( mp_read_unsigned_octets(&k, kb, kblen) );
    
    if ((mp_cmp_z(&k) <= 0) || (mp_cmp(&k, &n) >= 0)) {
#if EC_DEBUG
        printf("k is outside [1, n-1]\n");
        mp_tohex(&k, mpstr);
	printf("k : %s \n", mpstr);
        mp_tohex(&n, mpstr);
	printf("n : %s \n", mpstr);
#endif
	PORT_SetError(SEC_ERROR_NEED_RANDOM);
	goto cleanup;
    }

    















    CHECK_MPI_OK( mp_add(&k, &n, &k) );
    if (mpl_significant_bits(&k) <= mpl_significant_bits(&n)) {
	CHECK_MPI_OK( mp_add(&k, &n, &k) );
    }

    




    kGpoint.len = 2*flen + 1;
    kGpoint.data = PORT_Alloc(2*flen + 1);
    if ((kGpoint.data == NULL) ||
	(ec_points_mul(ecParams, &k, NULL, NULL, &kGpoint)
	    != SECSuccess))
	goto cleanup;

    




    CHECK_MPI_OK( mp_read_unsigned_octets(&x1, kGpoint.data + 1, 
	                                  (mp_size) flen) );

    




    CHECK_MPI_OK( mp_mod(&x1, &n, &r) );

    




    if (mp_cmp_z(&r) == 0) {
	PORT_SetError(SEC_ERROR_NEED_RANDOM);
	goto cleanup;
    }

    




    SECITEM_TO_MPINT(*digest, &s);        

    


    CHECK_MPI_OK( (obits = mpl_significant_bits(&n)) );
    if (digest->len*8 > obits) {
	mpl_rsh(&s,&s,digest->len*8 - obits);
    }

#if EC_DEBUG
    mp_todecimal(&n, mpstr);
    printf("n : %s (dec)\n", mpstr);
    mp_todecimal(&d, mpstr);
    printf("d : %s (dec)\n", mpstr);
    mp_tohex(&x1, mpstr);
    printf("x1: %s\n", mpstr);
    mp_todecimal(&s, mpstr);
    printf("digest: %s (decimal)\n", mpstr);
    mp_todecimal(&r, mpstr);
    printf("r : %s (dec)\n", mpstr);
    mp_tohex(&r, mpstr);
    printf("r : %s\n", mpstr);
#endif

    CHECK_MPI_OK( mp_invmod(&k, &n, &k) );      
    CHECK_MPI_OK( mp_mulmod(&d, &r, &n, &d) );  
    CHECK_MPI_OK( mp_addmod(&s, &d, &n, &s) );  
    CHECK_MPI_OK( mp_mulmod(&s, &k, &n, &s) );  

#if EC_DEBUG
    mp_todecimal(&s, mpstr);
    printf("s : %s (dec)\n", mpstr);
    mp_tohex(&s, mpstr);
    printf("s : %s\n", mpstr);
#endif

    




    if (mp_cmp_z(&s) == 0) {
	PORT_SetError(SEC_ERROR_NEED_RANDOM);
	goto cleanup;
    }

   



    CHECK_MPI_OK( mp_to_fixlen_octets(&r, signature->data, olen) );
    CHECK_MPI_OK( mp_to_fixlen_octets(&s, signature->data + olen, olen) );
finish:
    signature->len = 2*olen;

    rv = SECSuccess;
    err = MP_OKAY;
cleanup:
    mp_clear(&x1);
    mp_clear(&d);
    mp_clear(&k);
    mp_clear(&r);
    mp_clear(&s);
    mp_clear(&n);

    if (kGpoint.data) {
	PORT_ZFree(kGpoint.data, 2*flen + 1);
    }

    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }

#if EC_DEBUG
    printf("ECDSA signing with seed %s\n",
	(rv == SECSuccess) ? "succeeded" : "failed");
#endif
#else
    PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);
#endif 

   return rv;
}





SECStatus 
ECDSA_SignDigest(ECPrivateKey *key, SECItem *signature, const SECItem *digest)
{
    SECStatus rv = SECFailure;
#ifdef NSS_ENABLE_ECC
    int len;
    unsigned char *kBytes= NULL;

    if (!key) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    
    len = key->ecParams.order.len;
    kBytes = ec_GenerateRandomPrivateKey(key->ecParams.order.data, len);
    if (kBytes == NULL) goto cleanup;

    
    rv = ECDSA_SignDigestWithSeed(key, signature, digest, kBytes, len);

cleanup:    
    if (kBytes) {
	PORT_ZFree(kBytes, len);
    }

#if EC_DEBUG
    printf("ECDSA signing %s\n",
	(rv == SECSuccess) ? "succeeded" : "failed");
#endif
#else
    PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);
#endif 

    return rv;
}




SECStatus 
ECDSA_VerifyDigest(ECPublicKey *key, const SECItem *signature, 
                 const SECItem *digest)
{
    SECStatus rv = SECFailure;
#ifdef NSS_ENABLE_ECC
    mp_int r_, s_;           
    mp_int c, u1, u2, v;     
    mp_int x1;
    mp_int n;
    mp_err err = MP_OKAY;
    ECParams *ecParams = NULL;
    SECItem pointC = { siBuffer, NULL, 0 };
    int slen;       
    int flen;       
    unsigned olen;  
    unsigned obits; 

#if EC_DEBUG
    char mpstr[256];
    printf("ECDSA verification called\n");
#endif

    
    
    MP_DIGITS(&r_) = 0;
    MP_DIGITS(&s_) = 0;
    MP_DIGITS(&c) = 0;
    MP_DIGITS(&u1) = 0;
    MP_DIGITS(&u2) = 0;
    MP_DIGITS(&x1) = 0;
    MP_DIGITS(&v)  = 0;
    MP_DIGITS(&n)  = 0;

    
    if (!key || !signature || !digest) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	goto cleanup;
    }

    ecParams = &(key->ecParams);
    flen = (ecParams->fieldID.size + 7) >> 3;  
    olen = ecParams->order.len;  
    if (signature->len == 0 || signature->len%2 != 0 ||
	signature->len > 2*olen) {
	PORT_SetError(SEC_ERROR_INPUT_LEN);
	goto cleanup;
    }
    slen = signature->len/2;

    SECITEM_AllocItem(NULL, &pointC, 2*flen + 1);
    if (pointC.data == NULL)
	goto cleanup;

    CHECK_MPI_OK( mp_init(&r_) );
    CHECK_MPI_OK( mp_init(&s_) );
    CHECK_MPI_OK( mp_init(&c)  );
    CHECK_MPI_OK( mp_init(&u1) );
    CHECK_MPI_OK( mp_init(&u2) );
    CHECK_MPI_OK( mp_init(&x1)  );
    CHECK_MPI_OK( mp_init(&v)  );
    CHECK_MPI_OK( mp_init(&n)  );

    


    CHECK_MPI_OK( mp_read_unsigned_octets(&r_, signature->data, slen) );
    CHECK_MPI_OK( mp_read_unsigned_octets(&s_, signature->data + slen, slen) );
                                          
    




    SECITEM_TO_MPINT(ecParams->order, &n);
    if (mp_cmp_z(&r_) <= 0 || mp_cmp_z(&s_) <= 0 ||
        mp_cmp(&r_, &n) >= 0 || mp_cmp(&s_, &n) >= 0) {
	PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
	goto cleanup; 
    }

    




    CHECK_MPI_OK( mp_invmod(&s_, &n, &c) );      

    




    SECITEM_TO_MPINT(*digest, &u1);                  

    


    CHECK_MPI_OK( (obits = mpl_significant_bits(&n)) );
    if (digest->len*8 > obits) {  
	mpl_rsh(&u1,&u1,digest->len*8 - obits);
    }

#if EC_DEBUG
    mp_todecimal(&r_, mpstr);
    printf("r_: %s (dec)\n", mpstr);
    mp_todecimal(&s_, mpstr);
    printf("s_: %s (dec)\n", mpstr);
    mp_todecimal(&c, mpstr);
    printf("c : %s (dec)\n", mpstr);
    mp_todecimal(&u1, mpstr);
    printf("digest: %s (dec)\n", mpstr);
#endif

    CHECK_MPI_OK( mp_mulmod(&u1, &c, &n, &u1) );  

    




    CHECK_MPI_OK( mp_mulmod(&r_, &c, &n, &u2) );

    






    if (ec_points_mul(ecParams, &u1, &u2, &key->publicValue, &pointC)
	!= SECSuccess) {
	rv = SECFailure;
	goto cleanup;
    }
    if (ec_point_at_infinity(&pointC)) {
	PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
	rv = SECFailure;
	goto cleanup;
    }

    CHECK_MPI_OK( mp_read_unsigned_octets(&x1, pointC.data + 1, flen) );

    




    CHECK_MPI_OK( mp_mod(&x1, &n, &v) );

#if EC_DEBUG
    mp_todecimal(&r_, mpstr);
    printf("r_: %s (dec)\n", mpstr);
    mp_todecimal(&v, mpstr);
    printf("v : %s (dec)\n", mpstr);
#endif

    




    if (mp_cmp(&v, &r_)) {
	PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
	rv = SECFailure; 
    } else {
	rv = SECSuccess; 
    }

#if EC_DEBUG
    mp_todecimal(&u1, mpstr);
    printf("u1: %s (dec)\n", mpstr);
    mp_todecimal(&u2, mpstr);
    printf("u2: %s (dec)\n", mpstr);
    mp_tohex(&x1, mpstr);
    printf("x1: %s\n", mpstr);
    mp_todecimal(&v, mpstr);
    printf("v : %s (dec)\n", mpstr);
#endif

cleanup:
    mp_clear(&r_);
    mp_clear(&s_);
    mp_clear(&c);
    mp_clear(&u1);
    mp_clear(&u2);
    mp_clear(&x1);
    mp_clear(&v);
    mp_clear(&n);

    if (pointC.data) SECITEM_FreeItem(&pointC, PR_FALSE);
    if (err) {
	MP_TO_SEC_ERROR(err);
	rv = SECFailure;
    }

#if EC_DEBUG
    printf("ECDSA verification %s\n",
	(rv == SECSuccess) ? "succeeded" : "failed");
#endif
#else
    PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);
#endif 

    return rv;
}

