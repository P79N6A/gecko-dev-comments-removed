






#include <stdio.h>
#include "cryptohi.h"
#include "sechash.h"
#include "keyhi.h"
#include "secasn1.h"
#include "secoid.h"
#include "pk11func.h"
#include "pkcs1sig.h"
#include "secdig.h"
#include "secerr.h"
#include "keyi.h"

















static SECStatus
recoverPKCS1DigestInfo(SECOidTag givenDigestAlg,
                        SECOidTag* digestAlgOut,
                        unsigned char** digestInfo,
                        unsigned int* digestInfoLen,
                       SECKEYPublicKey* key,
                       const SECItem* sig, void* wincx)
{
    SGNDigestInfo* di = NULL;
    SECItem it;
    PRBool rv = SECSuccess;

    PORT_Assert(digestAlgOut);
    PORT_Assert(digestInfo);
    PORT_Assert(digestInfoLen);
    PORT_Assert(key);
    PORT_Assert(key->keyType == rsaKey);
    PORT_Assert(sig);

    it.data = NULL;
    it.len  = SECKEY_PublicKeyStrength(key);
    if (it.len != 0) {
        it.data = (unsigned char *)PORT_Alloc(it.len);
    }
    if (it.len == 0 || it.data == NULL ) {
        rv = SECFailure;
    }

    if (rv == SECSuccess) {
        
        rv = PK11_VerifyRecover(key, sig, &it, wincx);
    }
    
    if (rv == SECSuccess) {
        if (givenDigestAlg != SEC_OID_UNKNOWN) {
            




            *digestInfoLen = it.len;
            *digestInfo = (unsigned char*)it.data;
            *digestAlgOut = givenDigestAlg;
            return SECSuccess;
        }
    }

    if (rv == SECSuccess) {
        



        di = SGN_DecodeDigestInfo(&it);
        if (!di) {
            rv = SECFailure;
        }
    }

    if (rv == SECSuccess) {
        *digestAlgOut = SECOID_GetAlgorithmTag(&di->digestAlgorithm);
        if (*digestAlgOut == SEC_OID_UNKNOWN) {
            rv = SECFailure;
        }
    }

    if (di) {
        SGN_DestroyDigestInfo(di);
    }

    if (rv == SECSuccess) {
        *digestInfoLen = it.len;
        *digestInfo = (unsigned char*)it.data;
    } else {
        if (it.data) {
            PORT_Free(it.data);
        }
        *digestInfo = NULL;
        *digestInfoLen = 0;
        PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
    }

    return rv;
}

struct VFYContextStr {
    SECOidTag hashAlg;  
    SECKEYPublicKey *key;
    








    union {
	unsigned char buffer[1];

	
	unsigned char dsasig[DSA_MAX_SIGNATURE_LEN];
	
	unsigned char ecdsasig[2 * MAX_ECKEY_LEN];
    } u;
    unsigned int pkcs1RSADigestInfoLen;
    
    unsigned char *pkcs1RSADigestInfo;
    void * wincx;
    void *hashcx;
    const SECHashObject *hashobj;
    SECOidTag encAlg;  
    PRBool hasSignature;  



};

static SECStatus
verifyPKCS1DigestInfo(const VFYContext* cx, const SECItem* digest)
{
  SECItem pkcs1DigestInfo;
  pkcs1DigestInfo.data = cx->pkcs1RSADigestInfo;
  pkcs1DigestInfo.len = cx->pkcs1RSADigestInfoLen;
  return _SGN_VerifyPKCS1DigestInfo(
           cx->hashAlg, digest, &pkcs1DigestInfo,
           PR_TRUE );
}






static SECStatus
decodeECorDSASignature(SECOidTag algid, const SECItem *sig, unsigned char *dsig,
		       unsigned int len) {
    SECItem *dsasig = NULL; 
    SECStatus rv=SECSuccess;

    if ((algid != SEC_OID_ANSIX9_DSA_SIGNATURE) &&
	(algid != SEC_OID_ANSIX962_EC_PUBLIC_KEY) ) {
        if (sig->len != len) {
	    PORT_SetError(SEC_ERROR_BAD_DER);
	    return SECFailure;
	} 

	PORT_Memcpy(dsig, sig->data, sig->len);
	return SECSuccess;
    }

    if (algid ==  SEC_OID_ANSIX962_EC_PUBLIC_KEY) {
	if (len > MAX_ECKEY_LEN * 2) {
	    PORT_SetError(SEC_ERROR_BAD_DER);
	    return SECFailure;
	}
    }
    dsasig = DSAU_DecodeDerSigToLen((SECItem *)sig, len);

    if ((dsasig == NULL) || (dsasig->len != len)) {
	rv = SECFailure;
    } else {
	PORT_Memcpy(dsig, dsasig->data, dsasig->len);
    }

    if (dsasig != NULL) SECITEM_FreeItem(dsasig, PR_TRUE);
    if (rv == SECFailure) PORT_SetError(SEC_ERROR_BAD_DER);
    return rv;
}

const SEC_ASN1Template hashParameterTemplate[] =
{
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SECItem) },
    { SEC_ASN1_OBJECT_ID, 0 },
    { SEC_ASN1_SKIP_REST },
    { 0, }
};












SECStatus
sec_DecodeSigAlg(const SECKEYPublicKey *key, SECOidTag sigAlg, 
             const SECItem *param, SECOidTag *encalg, SECOidTag *hashalg)
{
    int len;
    PLArenaPool *arena;
    SECStatus rv;
    SECItem oid;

    PR_ASSERT(hashalg!=NULL);
    PR_ASSERT(encalg!=NULL);

    switch (sigAlg) {
      
      case SEC_OID_PKCS1_MD2_WITH_RSA_ENCRYPTION:
        *hashalg = SEC_OID_MD2;
	break;
      case SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION:
        *hashalg = SEC_OID_MD5;
	break;
      case SEC_OID_PKCS1_SHA1_WITH_RSA_ENCRYPTION:
      case SEC_OID_ISO_SHA_WITH_RSA_SIGNATURE:
      case SEC_OID_ISO_SHA1_WITH_RSA_SIGNATURE:
        *hashalg = SEC_OID_SHA1;
	break;
      case SEC_OID_PKCS1_RSA_ENCRYPTION:
      case SEC_OID_PKCS1_RSA_PSS_SIGNATURE:
        *hashalg = SEC_OID_UNKNOWN; 
	break;

      case SEC_OID_ANSIX962_ECDSA_SHA224_SIGNATURE:
      case SEC_OID_PKCS1_SHA224_WITH_RSA_ENCRYPTION:
      case SEC_OID_NIST_DSA_SIGNATURE_WITH_SHA224_DIGEST:
	*hashalg = SEC_OID_SHA224;
	break;
      case SEC_OID_ANSIX962_ECDSA_SHA256_SIGNATURE:
      case SEC_OID_PKCS1_SHA256_WITH_RSA_ENCRYPTION:
      case SEC_OID_NIST_DSA_SIGNATURE_WITH_SHA256_DIGEST:
	*hashalg = SEC_OID_SHA256;
	break;
      case SEC_OID_ANSIX962_ECDSA_SHA384_SIGNATURE:
      case SEC_OID_PKCS1_SHA384_WITH_RSA_ENCRYPTION:
	*hashalg = SEC_OID_SHA384;
	break;
      case SEC_OID_ANSIX962_ECDSA_SHA512_SIGNATURE:
      case SEC_OID_PKCS1_SHA512_WITH_RSA_ENCRYPTION:
	*hashalg = SEC_OID_SHA512;
	break;

      
      case SEC_OID_ANSIX9_DSA_SIGNATURE_WITH_SHA1_DIGEST:
      case SEC_OID_BOGUS_DSA_SIGNATURE_WITH_SHA1_DIGEST:
      case SEC_OID_ANSIX962_ECDSA_SHA1_SIGNATURE:
        *hashalg = SEC_OID_SHA1;
	break;
      case SEC_OID_MISSI_DSS:
      case SEC_OID_MISSI_KEA_DSS:
      case SEC_OID_MISSI_KEA_DSS_OLD:
      case SEC_OID_MISSI_DSS_OLD:
        *hashalg = SEC_OID_SHA1;
	break;
      case SEC_OID_ANSIX962_ECDSA_SIGNATURE_RECOMMENDED_DIGEST:
	




	len = SECKEY_PublicKeyStrength(key);
	if (len < 28) { 
	    *hashalg = SEC_OID_SHA1;
	} else if (len < 32) { 
	    *hashalg = SEC_OID_SHA224;
	} else if (len < 48) { 
	    *hashalg = SEC_OID_SHA256;
	} else if (len < 64) { 
	    *hashalg = SEC_OID_SHA384;
	} else {
	    
	    *hashalg = SEC_OID_SHA512;
	}
	break;
      case SEC_OID_ANSIX962_ECDSA_SIGNATURE_SPECIFIED_DIGEST:
	if (param == NULL) {
	    PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	    return SECFailure;
	}
	arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
	if (arena == NULL) {
	    return SECFailure;
	}
	rv = SEC_QuickDERDecodeItem(arena, &oid, hashParameterTemplate, param);
	if (rv == SECSuccess) {
            *hashalg = SECOID_FindOIDTag(&oid);
        }
        PORT_FreeArena(arena, PR_FALSE);
        if (rv != SECSuccess) {
	    return rv;
	}
	
	if (HASH_GetHashTypeByOidTag(*hashalg) == HASH_AlgNULL) {
	    
	    return SECFailure;
	}
	break;
      
      case SEC_OID_PKCS1_MD4_WITH_RSA_ENCRYPTION:
      default:
	PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	return SECFailure;
    }
     
    switch (sigAlg) {
      case SEC_OID_PKCS1_RSA_ENCRYPTION:
      case SEC_OID_PKCS1_MD2_WITH_RSA_ENCRYPTION:
      case SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION:
      case SEC_OID_PKCS1_SHA1_WITH_RSA_ENCRYPTION:
      case SEC_OID_ISO_SHA_WITH_RSA_SIGNATURE:
      case SEC_OID_ISO_SHA1_WITH_RSA_SIGNATURE:
      case SEC_OID_PKCS1_SHA224_WITH_RSA_ENCRYPTION:
      case SEC_OID_PKCS1_SHA256_WITH_RSA_ENCRYPTION:
      case SEC_OID_PKCS1_SHA384_WITH_RSA_ENCRYPTION:
      case SEC_OID_PKCS1_SHA512_WITH_RSA_ENCRYPTION:
	*encalg = SEC_OID_PKCS1_RSA_ENCRYPTION;
	break;
      case SEC_OID_PKCS1_RSA_PSS_SIGNATURE:
	*encalg = SEC_OID_PKCS1_RSA_PSS_SIGNATURE;
	break;

      
      case SEC_OID_ANSIX9_DSA_SIGNATURE_WITH_SHA1_DIGEST:
      case SEC_OID_BOGUS_DSA_SIGNATURE_WITH_SHA1_DIGEST:
      case SEC_OID_NIST_DSA_SIGNATURE_WITH_SHA224_DIGEST:
      case SEC_OID_NIST_DSA_SIGNATURE_WITH_SHA256_DIGEST:
	*encalg = SEC_OID_ANSIX9_DSA_SIGNATURE;
	break;
      case SEC_OID_MISSI_DSS:
      case SEC_OID_MISSI_KEA_DSS:
      case SEC_OID_MISSI_KEA_DSS_OLD:
      case SEC_OID_MISSI_DSS_OLD:
	*encalg = SEC_OID_MISSI_DSS;
	break;
      case SEC_OID_ANSIX962_ECDSA_SHA1_SIGNATURE:
      case SEC_OID_ANSIX962_ECDSA_SHA224_SIGNATURE:
      case SEC_OID_ANSIX962_ECDSA_SHA256_SIGNATURE:
      case SEC_OID_ANSIX962_ECDSA_SHA384_SIGNATURE:
      case SEC_OID_ANSIX962_ECDSA_SHA512_SIGNATURE:
      case SEC_OID_ANSIX962_ECDSA_SIGNATURE_RECOMMENDED_DIGEST:
      case SEC_OID_ANSIX962_ECDSA_SIGNATURE_SPECIFIED_DIGEST:
	*encalg = SEC_OID_ANSIX962_EC_PUBLIC_KEY;
	break;
      
      case SEC_OID_PKCS1_MD4_WITH_RSA_ENCRYPTION:
      default:
	PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	return SECFailure;
    }
    return SECSuccess;
}













static VFYContext *
vfy_CreateContext(const SECKEYPublicKey *key, const SECItem *sig, 
	SECOidTag encAlg, SECOidTag hashAlg, SECOidTag *hash, void *wincx)
{
    VFYContext *cx;
    SECStatus rv;
    unsigned int sigLen;
    KeyType type;

    
    
    type = seckey_GetKeyType(encAlg);
    if ((key->keyType != type) &&
	((key->keyType != rsaKey) || (type != rsaPssKey))) {
	PORT_SetError(SEC_ERROR_PKCS7_KEYALG_MISMATCH);
	return NULL;
    }

    cx = (VFYContext*) PORT_ZAlloc(sizeof(VFYContext));
    if (cx == NULL) {
	goto loser;
    }

    cx->wincx = wincx;
    cx->hasSignature = (sig != NULL);
    cx->encAlg = encAlg;
    cx->hashAlg = hashAlg;
    cx->key = SECKEY_CopyPublicKey(key);
    cx->pkcs1RSADigestInfo = NULL;
    rv = SECSuccess;
    if (sig) {
	switch (type) {
	case rsaKey:
	    rv = recoverPKCS1DigestInfo(hashAlg, &cx->hashAlg,
					&cx->pkcs1RSADigestInfo,
					&cx->pkcs1RSADigestInfoLen,
					cx->key,
					sig, wincx);
	    break;
	case dsaKey:
	case ecKey:
	    sigLen = SECKEY_SignatureLen(key);
	    if (sigLen == 0) {
		
		rv = SECFailure;	
		break;
	    }
	    rv = decodeECorDSASignature(encAlg, sig, cx->u.buffer, sigLen);
 	    break;
	default:
	    rv = SECFailure;
	    PORT_SetError(SEC_ERROR_UNSUPPORTED_KEYALG);
	    break;
	}
    }

    if (rv) goto loser;

    
    if (HASH_GetHashTypeByOidTag(cx->hashAlg) == HASH_AlgNULL) {
	
	goto loser;
    }

    if (hash) {
	*hash = cx->hashAlg;
    }
    return cx;

  loser:
    if (cx) {
	VFY_DestroyContext(cx, PR_TRUE);
    }
    return 0;
}

VFYContext *
VFY_CreateContext(SECKEYPublicKey *key, SECItem *sig, SECOidTag sigAlg,
		  void *wincx)
{
    SECOidTag encAlg, hashAlg;
    SECStatus rv = sec_DecodeSigAlg(key, sigAlg, NULL, &encAlg, &hashAlg);
    if (rv != SECSuccess) {
	return NULL;
    }
    return vfy_CreateContext(key, sig, encAlg, hashAlg, NULL, wincx);
}

VFYContext *
VFY_CreateContextDirect(const SECKEYPublicKey *key, const SECItem *sig, 
			SECOidTag encAlg, SECOidTag hashAlg, 
			SECOidTag *hash, void *wincx)
{
  return vfy_CreateContext(key, sig, encAlg, hashAlg, hash, wincx);
}

VFYContext *
VFY_CreateContextWithAlgorithmID(const SECKEYPublicKey *key, const SECItem *sig,
	      const SECAlgorithmID *sigAlgorithm, SECOidTag *hash, void *wincx)
{
    SECOidTag encAlg, hashAlg;
    SECStatus rv = sec_DecodeSigAlg(key, 
		    SECOID_GetAlgorithmTag((SECAlgorithmID *)sigAlgorithm), 
		    &sigAlgorithm->parameters, &encAlg, &hashAlg);
    if (rv != SECSuccess) {
	return NULL;
    }
    return vfy_CreateContext(key, sig, encAlg, hashAlg, hash, wincx);
}

void
VFY_DestroyContext(VFYContext *cx, PRBool freeit)
{
    if (cx) {
	if (cx->hashcx != NULL) {
	    (*cx->hashobj->destroy)(cx->hashcx, PR_TRUE);
	    cx->hashcx = NULL;
	}
	if (cx->key) {
	    SECKEY_DestroyPublicKey(cx->key);
	}
    if (cx->pkcs1RSADigestInfo) {
        PORT_Free(cx->pkcs1RSADigestInfo);
    }
	if (freeit) {
	    PORT_ZFree(cx, sizeof(VFYContext));
	}
    }
}

SECStatus
VFY_Begin(VFYContext *cx)
{
    if (cx->hashcx != NULL) {
	(*cx->hashobj->destroy)(cx->hashcx, PR_TRUE);
	cx->hashcx = NULL;
    }

    cx->hashobj = HASH_GetHashObjectByOidTag(cx->hashAlg);
    if (!cx->hashobj) 
	return SECFailure;	

    cx->hashcx = (*cx->hashobj->create)();
    if (cx->hashcx == NULL)
	return SECFailure;

    (*cx->hashobj->begin)(cx->hashcx);
    return SECSuccess;
}

SECStatus
VFY_Update(VFYContext *cx, const unsigned char *input, unsigned inputLen)
{
    if (cx->hashcx == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    (*cx->hashobj->update)(cx->hashcx, input, inputLen);
    return SECSuccess;
}

SECStatus
VFY_EndWithSignature(VFYContext *cx, SECItem *sig)
{
    unsigned char final[HASH_LENGTH_MAX];
    unsigned part;
    SECItem hash,dsasig; 
    SECStatus rv;

    if ((cx->hasSignature == PR_FALSE) && (sig == NULL)) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    if (cx->hashcx == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    (*cx->hashobj->end)(cx->hashcx, final, &part, sizeof(final));
    switch (cx->key->keyType) {
      case ecKey:
      case dsaKey:
	dsasig.data = cx->u.buffer;
	dsasig.len = SECKEY_SignatureLen(cx->key);
	if (dsasig.len == 0) {
	    return SECFailure;
	} 
	if (sig) {
	    rv = decodeECorDSASignature(cx->encAlg, sig, dsasig.data,
					dsasig.len);
	    if (rv != SECSuccess) {
		PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
		return SECFailure;
	    }
	} 
	hash.data = final;
	hash.len = part;
	if (PK11_Verify(cx->key,&dsasig,&hash,cx->wincx) != SECSuccess) {
		PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
		return SECFailure;
	}
	break;
      case rsaKey:
      {
        SECItem digest;
        digest.data = final;
        digest.len = part;
	if (sig) {
	    SECOidTag hashid;
	    PORT_Assert(cx->hashAlg != SEC_OID_UNKNOWN);
	    rv = recoverPKCS1DigestInfo(cx->hashAlg, &hashid,
					&cx->pkcs1RSADigestInfo,
					&cx->pkcs1RSADigestInfoLen,
					cx->key,
					sig, cx->wincx);
	    PORT_Assert(cx->hashAlg == hashid);
	    if (rv != SECSuccess) {
		return SECFailure;
	    }
	}
	return verifyPKCS1DigestInfo(cx, &digest);
      }
      default:
	PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
	return SECFailure; 
    }
    return SECSuccess;
}

SECStatus
VFY_End(VFYContext *cx)
{
    return VFY_EndWithSignature(cx,NULL);
}





static SECStatus
vfy_VerifyDigest(const SECItem *digest, const SECKEYPublicKey *key, 
		 const SECItem *sig, SECOidTag encAlg, SECOidTag hashAlg,
		 void *wincx)
{
    SECStatus rv;
    VFYContext *cx;
    SECItem dsasig; 

    rv = SECFailure;

    cx = vfy_CreateContext(key, sig, encAlg, hashAlg, NULL, wincx);
    if (cx != NULL) {
	switch (key->keyType) {
	case rsaKey:
	    rv = verifyPKCS1DigestInfo(cx, digest);
	    break;
	case dsaKey:
	case ecKey:
	    dsasig.data = cx->u.buffer;
	    dsasig.len = SECKEY_SignatureLen(cx->key);
	    if (dsasig.len == 0) {
		break;
	    }
	    if (PK11_Verify(cx->key, &dsasig, (SECItem *)digest, cx->wincx)
		!= SECSuccess) {
		PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
	    } else {
		rv = SECSuccess;
	    }
	    break;
	default:
	    break;
	}
	VFY_DestroyContext(cx, PR_TRUE);
    }
    return rv;
}

SECStatus
VFY_VerifyDigestDirect(const SECItem *digest, const SECKEYPublicKey *key, 
		       const SECItem *sig, SECOidTag encAlg, 
		       SECOidTag hashAlg, void *wincx)
{
    return vfy_VerifyDigest(digest, key, sig, encAlg, hashAlg, wincx);
}

SECStatus
VFY_VerifyDigest(SECItem *digest, SECKEYPublicKey *key, SECItem *sig,
		 SECOidTag algid, void *wincx)
{
    SECOidTag encAlg, hashAlg;
    SECStatus rv = sec_DecodeSigAlg(key, algid, NULL, &encAlg, &hashAlg);
    if (rv != SECSuccess) {
	return SECFailure;
    }
    return vfy_VerifyDigest(digest, key, sig, encAlg, hashAlg, wincx);
}





SECStatus
VFY_VerifyDigestWithAlgorithmID(const SECItem *digest, 
		const SECKEYPublicKey *key, const SECItem *sig, 
		const SECAlgorithmID *sigAlgorithm, 
		SECOidTag hashCmp, void *wincx)
{
    SECOidTag encAlg, hashAlg;
    SECStatus rv = sec_DecodeSigAlg(key, 
		    SECOID_GetAlgorithmTag((SECAlgorithmID *)sigAlgorithm), 
		    &sigAlgorithm->parameters, &encAlg, &hashAlg);
    if (rv != SECSuccess) {
	return rv;
    }
    if ( hashCmp != SEC_OID_UNKNOWN && 
	 hashAlg != SEC_OID_UNKNOWN &&
	 hashCmp != hashAlg) {
	PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
	return SECFailure;
    }
    return vfy_VerifyDigest(digest, key, sig, encAlg, hashAlg, wincx);
}

static SECStatus
vfy_VerifyData(const unsigned char *buf, int len, const SECKEYPublicKey *key,
	       const SECItem *sig, SECOidTag encAlg, SECOidTag hashAlg,
	       SECOidTag *hash, void *wincx)
{
    SECStatus rv;
    VFYContext *cx;

    cx = vfy_CreateContext(key, sig, encAlg, hashAlg, hash, wincx);
    if (cx == NULL)
	return SECFailure;

    rv = VFY_Begin(cx);
    if (rv == SECSuccess) {
	rv = VFY_Update(cx, (unsigned char *)buf, len);
	if (rv == SECSuccess)
	    rv = VFY_End(cx);
    }

    VFY_DestroyContext(cx, PR_TRUE);
    return rv;
}

SECStatus
VFY_VerifyDataDirect(const unsigned char *buf, int len, 
		     const SECKEYPublicKey *key, const SECItem *sig,
		     SECOidTag encAlg, SECOidTag hashAlg,
		     SECOidTag *hash, void *wincx)
{
    return vfy_VerifyData(buf, len, key, sig, encAlg, hashAlg, hash, wincx);
}

SECStatus
VFY_VerifyData(const unsigned char *buf, int len, const SECKEYPublicKey *key,
	       const SECItem *sig, SECOidTag algid, void *wincx)
{
    SECOidTag encAlg, hashAlg;
    SECStatus rv = sec_DecodeSigAlg(key, algid, NULL, &encAlg, &hashAlg);
    if (rv != SECSuccess) {
	return rv;
    }
    return vfy_VerifyData(buf, len, key, sig, encAlg, hashAlg, NULL, wincx);
}

SECStatus
VFY_VerifyDataWithAlgorithmID(const unsigned char *buf, int len, 
			      const SECKEYPublicKey *key,
			      const SECItem *sig, 
			      const SECAlgorithmID *sigAlgorithm, 
			      SECOidTag *hash, void *wincx)
{
    SECOidTag encAlg, hashAlg;
    SECOidTag sigAlg = SECOID_GetAlgorithmTag((SECAlgorithmID *)sigAlgorithm);
    SECStatus rv     = sec_DecodeSigAlg(key, sigAlg, 
		                &sigAlgorithm->parameters, &encAlg, &hashAlg);
    if (rv != SECSuccess) {
	return rv;
    }
    return vfy_VerifyData(buf, len, key, sig, encAlg, hashAlg, hash, wincx);
}
