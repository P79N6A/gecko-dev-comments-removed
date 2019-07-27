


















#include "seccomon.h"
#include "secitem.h"
#include "secport.h"
#include "blapi.h"
#include "pkcs11.h"
#include "pkcs11i.h"
#include "pkcs1sig.h"
#include "lowkeyi.h"
#include "secder.h"
#include "secdig.h"
#include "lowpbe.h"	
#include "pkcs11t.h"
#include "secoid.h"
#include "alghmac.h"
#include "softoken.h"
#include "secasn1.h"
#include "secerr.h"

#include "prprf.h"

#define __PASTE(x,y)    x##y




 
#undef CK_PKCS11_FUNCTION_INFO
#undef CK_NEED_ARG_LIST

#define CK_EXTERN extern
#define CK_PKCS11_FUNCTION_INFO(func) \
		CK_RV __PASTE(NS,func)
#define CK_NEED_ARG_LIST	1
 
#include "pkcs11f.h"

typedef struct {
    PRUint8 client_version[2];
    PRUint8 random[46];
} SSL3RSAPreMasterSecret;

static void sftk_Null(void *data, PRBool freeit)
{
    return;
} 

#ifndef NSS_DISABLE_ECC
#ifdef EC_DEBUG
#define SEC_PRINT(str1, str2, num, sitem) \
    printf("pkcs11c.c:%s:%s (keytype=%d) [len=%d]\n", \
            str1, str2, num, sitem->len); \
    for (i = 0; i < sitem->len; i++) { \
	    printf("%02x:", sitem->data[i]); \
    } \
    printf("\n") 
#else
#define SEC_PRINT(a, b, c, d) 
#endif
#endif 





static void
sftk_FreePrivKey(NSSLOWKEYPrivateKey *key, PRBool freeit)
{
    nsslowkey_DestroyPrivateKey(key);
}

static void
sftk_Space(void *data, PRBool freeit)
{
    PORT_Free(data);
} 






static CK_RV
sftk_MapCryptError(int error)
{
    switch (error) {
	case SEC_ERROR_INVALID_ARGS:
	case SEC_ERROR_BAD_DATA:  
	    return CKR_ARGUMENTS_BAD;
	case SEC_ERROR_INPUT_LEN:
	    return CKR_DATA_LEN_RANGE;
	case SEC_ERROR_OUTPUT_LEN:
	    return CKR_BUFFER_TOO_SMALL;
	case SEC_ERROR_LIBRARY_FAILURE:
	    return CKR_GENERAL_ERROR;
	case SEC_ERROR_NO_MEMORY:
	    return CKR_HOST_MEMORY;
	case SEC_ERROR_BAD_SIGNATURE:
	    return CKR_SIGNATURE_INVALID;
	case SEC_ERROR_INVALID_KEY:
	    return CKR_KEY_SIZE_RANGE;
	case SEC_ERROR_BAD_KEY:  
	    return CKR_KEY_SIZE_RANGE;  
	case SEC_ERROR_UNSUPPORTED_EC_POINT_FORM:
	    return CKR_TEMPLATE_INCONSISTENT;
	
	case SEC_ERROR_UNSUPPORTED_KEYALG:
	    return CKR_MECHANISM_INVALID;
	case SEC_ERROR_UNSUPPORTED_ELLIPTIC_CURVE:
	    return CKR_DOMAIN_PARAMS_INVALID;
	
	case SEC_ERROR_NEED_RANDOM:
	    return CKR_FUNCTION_FAILED;
    }
    return CKR_DEVICE_ERROR;
}


static CK_RV
sftk_MapDecryptError(int error)
{
    switch (error) {
	case SEC_ERROR_BAD_DATA:
	    return CKR_ENCRYPTED_DATA_INVALID;
	default:
	    return sftk_MapCryptError(error);
    }
}





static CK_RV
sftk_MapVerifyError(int error)
{
    CK_RV crv = sftk_MapCryptError(error);
    if (crv == CKR_DEVICE_ERROR)
	crv = CKR_SIGNATURE_INVALID;
    return crv;
}






static CK_RV
sftk_cdmf2des(unsigned char *cdmfkey, unsigned char *deskey)
{
    unsigned char key1[8] = { 0xc4, 0x08, 0xb0, 0x54, 0x0b, 0xa1, 0xe0, 0xae };
    unsigned char key2[8] = { 0xef, 0x2c, 0x04, 0x1c, 0xe6, 0x38, 0x2f, 0xe6 };
    unsigned char enc_src[8];
    unsigned char enc_dest[8];
    unsigned int leng,i;
    DESContext *descx;
    SECStatus rv;
    
    
    
    for (i=0; i < 8; i++) {
	enc_src[i] = cdmfkey[i] & 0xfe;
    }

    
    descx = DES_CreateContext(key1, NULL, NSS_DES, PR_TRUE);
    if (descx == NULL) return CKR_HOST_MEMORY;
    rv = DES_Encrypt(descx, enc_dest, &leng, 8, enc_src, 8);
    DES_DestroyContext(descx,PR_TRUE);
    if (rv != SECSuccess) return sftk_MapCryptError(PORT_GetError());

    
    for (i=0; i < 8; i++) {
	if (i & 1) {
	    enc_src[i] = (enc_src[i] ^ enc_dest[i]) & 0xfe;
	} else {
	    enc_src[i] = (enc_src[i] ^ enc_dest[i]) & 0x0e;
	}
    }

    
    descx = DES_CreateContext(key2, NULL, NSS_DES, PR_TRUE);
    if (descx == NULL) return CKR_HOST_MEMORY;
    rv = DES_Encrypt(descx, deskey, &leng, 8, enc_src, 8);
    DES_DestroyContext(descx,PR_TRUE);
    if (rv != SECSuccess) return sftk_MapCryptError(PORT_GetError());

    	
    sftk_FormatDESKey(deskey, 8);
    return CKR_OK;
}



CK_RV
NSC_DestroyObject(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hObject)
{
    SFTKSlot *slot = sftk_SlotFromSessionHandle(hSession);
    SFTKSession *session;
    SFTKObject *object;
    SFTKFreeStatus status;

    CHECK_FORK();

    if (slot == NULL) {
	return CKR_SESSION_HANDLE_INVALID;
    }
    



    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) {
        return CKR_SESSION_HANDLE_INVALID;
    }

    object = sftk_ObjectFromHandle(hObject,session);
    if (object == NULL) {
	sftk_FreeSession(session);
	return CKR_OBJECT_HANDLE_INVALID;
    }

    
    if ((!slot->isLoggedIn) && (slot->needLogin) &&
				(sftk_isTrue(object,CKA_PRIVATE))) {
	sftk_FreeSession(session);
	sftk_FreeObject(object);
	return CKR_USER_NOT_LOGGED_IN;
    }

    

    if (((session->info.flags & CKF_RW_SESSION) == 0) &&
				(sftk_isTrue(object,CKA_TOKEN))) {
	sftk_FreeSession(session);
	sftk_FreeObject(object);
	return CKR_SESSION_READ_ONLY;
    }

    sftk_DeleteObject(session,object);

    sftk_FreeSession(session);

    






    status = sftk_FreeObject(object);

    return (status != SFTK_DestroyFailure) ? CKR_OK : CKR_DEVICE_ERROR;
}









static HASH_HashType
GetHashTypeFromMechanism(CK_MECHANISM_TYPE mech)
{
    switch (mech) {
        case CKM_SHA_1:
        case CKG_MGF1_SHA1:
            return HASH_AlgSHA1;
        case CKM_SHA224:
        case CKG_MGF1_SHA224:
            return HASH_AlgSHA224;
        case CKM_SHA256:
        case CKG_MGF1_SHA256:
            return HASH_AlgSHA256;
        case CKM_SHA384:
        case CKG_MGF1_SHA384:
            return HASH_AlgSHA384;
        case CKM_SHA512:
        case CKG_MGF1_SHA512:
            return HASH_AlgSHA512;
        default:
            return HASH_AlgNULL;
    }
}




static PRBool
sftk_ValidatePssParams(const CK_RSA_PKCS_PSS_PARAMS *params)
{
    if (!params) {
        return PR_FALSE;
    }
    if (GetHashTypeFromMechanism(params->hashAlg) == HASH_AlgNULL ||
        GetHashTypeFromMechanism(params->mgf) == HASH_AlgNULL) {
        return PR_FALSE;
    }
    return PR_TRUE;
}




static PRBool
sftk_ValidateOaepParams(const CK_RSA_PKCS_OAEP_PARAMS *params)
{
    if (!params) {
        return PR_FALSE;
    }
    




    if (params->source != CKZ_DATA_SPECIFIED ||
        (GetHashTypeFromMechanism(params->hashAlg) == HASH_AlgNULL) ||
        (GetHashTypeFromMechanism(params->mgf) == HASH_AlgNULL) ||
        (params->ulSourceDataLen == 0 && params->pSourceData != NULL) ||
        (params->ulSourceDataLen != 0 && params->pSourceData == NULL)) {
        return PR_FALSE;
    }
    return PR_TRUE;
}




SFTKSessionContext *
sftk_ReturnContextByType(SFTKSession *session, SFTKContextType type)
{
    switch (type) {
	case SFTK_ENCRYPT:
	case SFTK_DECRYPT:
	    return session->enc_context;
	case SFTK_HASH:
	    return session->hash_context;
	case SFTK_SIGN:
	case SFTK_SIGN_RECOVER:
	case SFTK_VERIFY:
	case SFTK_VERIFY_RECOVER:
	    return session->hash_context;
    }
    return NULL;
}




void
sftk_SetContextByType(SFTKSession *session, SFTKContextType type, 
						SFTKSessionContext *context)
{
    switch (type) {
	case SFTK_ENCRYPT:
	case SFTK_DECRYPT:
	    session->enc_context = context;
	    break;
	case SFTK_HASH:
	    session->hash_context = context;
	    break;
	case SFTK_SIGN:
	case SFTK_SIGN_RECOVER:
	case SFTK_VERIFY:
	case SFTK_VERIFY_RECOVER:
	    session->hash_context = context;
	    break;
    }
    return;
}








static CK_RV
sftk_GetContext(CK_SESSION_HANDLE handle,SFTKSessionContext **contextPtr,
	SFTKContextType type, PRBool needMulti, SFTKSession **sessionPtr)
{
    SFTKSession *session;
    SFTKSessionContext *context;

    session = sftk_SessionFromHandle(handle);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;
    context = sftk_ReturnContextByType(session,type);
    
    if((context==NULL)||(context->type!=type)||(needMulti&&!(context->multi))){
        sftk_FreeSession(session);
	return CKR_OPERATION_NOT_INITIALIZED;
    }
    *contextPtr = context;
    if (sessionPtr != NULL) {
	*sessionPtr = session;
    } else {
	sftk_FreeSession(session);
    }
    return CKR_OK;
}




static void
sftk_TerminateOp( SFTKSession *session, SFTKContextType ctype,
        SFTKSessionContext *context )
{
    sftk_FreeContext( context );
    sftk_SetContextByType( session, ctype, NULL );
}









static CK_RV
sftk_InitGeneric(SFTKSession *session,SFTKSessionContext **contextPtr,
		 SFTKContextType ctype,SFTKObject **keyPtr,
		 CK_OBJECT_HANDLE hKey, CK_KEY_TYPE *keyTypePtr,
		 CK_OBJECT_CLASS pubKeyType, CK_ATTRIBUTE_TYPE operation)
{
    SFTKObject *key = NULL;
    SFTKAttribute *att;
    SFTKSessionContext *context;

    
    if (sftk_ReturnContextByType(session,ctype) != NULL) {
	return CKR_OPERATION_ACTIVE;
    }

    
    if (keyPtr) {
        key = sftk_ObjectFromHandle(hKey,session);
        if (key == NULL) {
	    return CKR_KEY_HANDLE_INVALID;
    	}

	
	if (((key->objclass != CKO_SECRET_KEY) && (key->objclass != pubKeyType))
					|| !sftk_isTrue(key,operation)) {
	    sftk_FreeObject(key);
	    return CKR_KEY_TYPE_INCONSISTENT;
	}
	
	att = sftk_FindAttribute(key,CKA_KEY_TYPE);
	if (att == NULL) {
	    sftk_FreeObject(key);
	    return CKR_KEY_TYPE_INCONSISTENT;
	}
	PORT_Assert(att->attrib.ulValueLen == sizeof(CK_KEY_TYPE));
	if (att->attrib.ulValueLen != sizeof(CK_KEY_TYPE)) {
	    sftk_FreeAttribute(att);
	    sftk_FreeObject(key);
	    return CKR_ATTRIBUTE_VALUE_INVALID;
	}
	PORT_Memcpy(keyTypePtr, att->attrib.pValue, sizeof(CK_KEY_TYPE));
	sftk_FreeAttribute(att);
	*keyPtr = key;
    }

    
    context = (SFTKSessionContext *)PORT_Alloc(sizeof(SFTKSessionContext));
    if (context == NULL) {
	if (key) sftk_FreeObject(key);
	return CKR_HOST_MEMORY;
    }
    context->type = ctype;
    context->multi = PR_TRUE;
    context->rsa = PR_FALSE;
    context->cipherInfo = NULL;
    context->hashInfo = NULL;
    context->doPad = PR_FALSE;
    context->padDataLength = 0;
    context->key = key;
    context->blockSize = 0;
    context->maxLen = 0;

    *contextPtr = context;
    return CKR_OK;
}

static int
sftk_aes_mode(CK_MECHANISM_TYPE mechanism)
{
    switch (mechanism) {
    case CKM_AES_CBC_PAD:
    case CKM_AES_CBC:
	return NSS_AES_CBC;
    case CKM_AES_ECB:
	return NSS_AES;
    case CKM_AES_CTS:
	return NSS_AES_CTS;
    case CKM_AES_CTR:
	return NSS_AES_CTR;
    case CKM_AES_GCM:
	return NSS_AES_GCM;
    }
    return -1;
}

static SECStatus
sftk_RSAEncryptRaw(NSSLOWKEYPublicKey *key, unsigned char *output,
                   unsigned int *outputLen, unsigned int maxLen,
                   const unsigned char *input, unsigned int inputLen)
{
    SECStatus rv = SECFailure;

    PORT_Assert(key->keyType == NSSLOWKEYRSAKey);
    if (key->keyType != NSSLOWKEYRSAKey) {
        PORT_SetError(SEC_ERROR_INVALID_KEY);
        return SECFailure;
    }

    rv = RSA_EncryptRaw(&key->u.rsa, output, outputLen, maxLen, input,
                        inputLen);
    if (rv != SECSuccess && PORT_GetError() == SEC_ERROR_LIBRARY_FAILURE) {
        sftk_fatalError = PR_TRUE;
    }

    return rv;
}

static SECStatus
sftk_RSADecryptRaw(NSSLOWKEYPrivateKey *key, unsigned char *output,
                   unsigned int *outputLen, unsigned int maxLen,
                   const unsigned char *input, unsigned int inputLen)
{
    SECStatus rv = SECFailure;

    PORT_Assert(key->keyType == NSSLOWKEYRSAKey);
    if (key->keyType != NSSLOWKEYRSAKey) {
        PORT_SetError(SEC_ERROR_INVALID_KEY);
        return SECFailure;
    }

    rv = RSA_DecryptRaw(&key->u.rsa, output, outputLen, maxLen, input,
                        inputLen);
    if (rv != SECSuccess && PORT_GetError() == SEC_ERROR_LIBRARY_FAILURE) {
        sftk_fatalError = PR_TRUE;
    }

    return rv;
}

static SECStatus
sftk_RSAEncrypt(NSSLOWKEYPublicKey *key, unsigned char *output,
                unsigned int *outputLen, unsigned int maxLen,
                const unsigned char *input, unsigned int inputLen)
{
    SECStatus rv = SECFailure;

    PORT_Assert(key->keyType == NSSLOWKEYRSAKey);
    if (key->keyType != NSSLOWKEYRSAKey) {
        PORT_SetError(SEC_ERROR_INVALID_KEY);
        return SECFailure;
    }

    rv = RSA_EncryptBlock(&key->u.rsa, output, outputLen, maxLen, input,
                          inputLen);
    if (rv != SECSuccess && PORT_GetError() == SEC_ERROR_LIBRARY_FAILURE) {
        sftk_fatalError = PR_TRUE;
    }

    return rv;
}

static SECStatus
sftk_RSADecrypt(NSSLOWKEYPrivateKey *key, unsigned char *output,
                unsigned int *outputLen, unsigned int maxLen,
                const unsigned char *input, unsigned int inputLen)
{
    SECStatus rv = SECFailure;

    PORT_Assert(key->keyType == NSSLOWKEYRSAKey);
    if (key->keyType != NSSLOWKEYRSAKey) {
        PORT_SetError(SEC_ERROR_INVALID_KEY);
        return SECFailure;
    }

    rv = RSA_DecryptBlock(&key->u.rsa, output, outputLen, maxLen, input,
                          inputLen);
    if (rv != SECSuccess && PORT_GetError() == SEC_ERROR_LIBRARY_FAILURE) {
        sftk_fatalError = PR_TRUE;
    }

    return rv;
}

static SECStatus
sftk_RSAEncryptOAEP(SFTKOAEPEncryptInfo *info, unsigned char *output,
                    unsigned int *outputLen, unsigned int maxLen,
                    const unsigned char *input, unsigned int inputLen)
{
    HASH_HashType hashAlg;
    HASH_HashType maskHashAlg;

    PORT_Assert(info->key->keyType == NSSLOWKEYRSAKey);
    if (info->key->keyType != NSSLOWKEYRSAKey) {
        PORT_SetError(SEC_ERROR_INVALID_KEY);
        return SECFailure;
    }

    hashAlg = GetHashTypeFromMechanism(info->params->hashAlg);
    maskHashAlg = GetHashTypeFromMechanism(info->params->mgf);

    return RSA_EncryptOAEP(&info->key->u.rsa, hashAlg, maskHashAlg,
                           (const unsigned char*)info->params->pSourceData,
                           info->params->ulSourceDataLen, NULL, 0,
                           output, outputLen, maxLen, input, inputLen);
}

static SECStatus
sftk_RSADecryptOAEP(SFTKOAEPDecryptInfo *info, unsigned char *output,
                    unsigned int *outputLen, unsigned int maxLen,
                    const unsigned char *input, unsigned int inputLen)
{
    SECStatus rv = SECFailure;
    HASH_HashType hashAlg;
    HASH_HashType maskHashAlg;

    PORT_Assert(info->key->keyType == NSSLOWKEYRSAKey);
    if (info->key->keyType != NSSLOWKEYRSAKey) {
        PORT_SetError(SEC_ERROR_INVALID_KEY);
        return SECFailure;
    }

    hashAlg = GetHashTypeFromMechanism(info->params->hashAlg);
    maskHashAlg = GetHashTypeFromMechanism(info->params->mgf);

    rv = RSA_DecryptOAEP(&info->key->u.rsa, hashAlg, maskHashAlg,
                         (const unsigned char*)info->params->pSourceData,
                         info->params->ulSourceDataLen,
                         output, outputLen, maxLen, input, inputLen);
     if (rv != SECSuccess && PORT_GetError() == SEC_ERROR_LIBRARY_FAILURE) {
        sftk_fatalError = PR_TRUE;
    }
    return rv;
}







static CK_RV
sftk_CryptInit(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism,
     CK_OBJECT_HANDLE hKey,
     CK_ATTRIBUTE_TYPE mechUsage, CK_ATTRIBUTE_TYPE keyUsage,
		 SFTKContextType contextType, PRBool isEncrypt)
{
    SFTKSession *session;
    SFTKObject *key;
    SFTKSessionContext *context;
    SFTKAttribute *att;
    CK_RC2_CBC_PARAMS *rc2_param;
#if NSS_SOFTOKEN_DOES_RC5
    CK_RC5_CBC_PARAMS *rc5_param;
    SECItem rc5Key;
#endif
    CK_KEY_TYPE key_type;
    CK_RV crv = CKR_OK;
    unsigned effectiveKeyLength;
    unsigned char newdeskey[24];
    PRBool useNewKey=PR_FALSE;
    int t;

    crv = sftk_MechAllowsOperation(pMechanism->mechanism, mechUsage );
    if (crv != CKR_OK) 
    	return crv;

    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;

    crv = sftk_InitGeneric(session,&context,contextType,&key,hKey,&key_type,
    			isEncrypt ?CKO_PUBLIC_KEY:CKO_PRIVATE_KEY, keyUsage);
						
    if (crv != CKR_OK) {
	sftk_FreeSession(session);
	return crv;
    }

    context->doPad = PR_FALSE;
    switch(pMechanism->mechanism) {
    case CKM_RSA_PKCS:
    case CKM_RSA_X_509:
	if (key_type != CKK_RSA) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	context->multi = PR_FALSE;
	context->rsa = PR_TRUE;
	if (isEncrypt) {
	    NSSLOWKEYPublicKey *pubKey = sftk_GetPubKey(key,CKK_RSA,&crv);
	    if (pubKey == NULL) {
	        crv = CKR_KEY_HANDLE_INVALID;
		break;
	    }
	    context->maxLen = nsslowkey_PublicModulusLen(pubKey);
	    context->cipherInfo =  (void *)pubKey;
	    context->update = (SFTKCipher) 
		(pMechanism->mechanism == CKM_RSA_X_509
					? sftk_RSAEncryptRaw : sftk_RSAEncrypt);
	} else {
	    NSSLOWKEYPrivateKey *privKey = sftk_GetPrivKey(key,CKK_RSA,&crv);
	    if (privKey == NULL) {
	        crv = CKR_KEY_HANDLE_INVALID;
		break;
	    }
	    context->maxLen = nsslowkey_PrivateModulusLen(privKey);
	    context->cipherInfo =  (void *)privKey;
	    context->update = (SFTKCipher) 
		(pMechanism->mechanism == CKM_RSA_X_509
					? sftk_RSADecryptRaw : sftk_RSADecrypt);
	}
	context->destroy = sftk_Null;
	break;
    case CKM_RSA_PKCS_OAEP:
	if (key_type != CKK_RSA) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	if (pMechanism->ulParameterLen != sizeof(CK_RSA_PKCS_OAEP_PARAMS) ||
	    !sftk_ValidateOaepParams((CK_RSA_PKCS_OAEP_PARAMS*)pMechanism->pParameter)) {
	    crv = CKR_MECHANISM_PARAM_INVALID;
	    break;
	}
	context->multi = PR_FALSE;
	context->rsa = PR_TRUE;
	if (isEncrypt) {
	    SFTKOAEPEncryptInfo *info = PORT_New(SFTKOAEPEncryptInfo);
	    if (info == NULL) {
	        crv = CKR_HOST_MEMORY;
	        break;
	    }
	    info->params = pMechanism->pParameter;
	    info->key = sftk_GetPubKey(key, CKK_RSA, &crv);
	    if (info->key == NULL) {
	        PORT_Free(info);
	        crv = CKR_KEY_HANDLE_INVALID;
	        break;
	    }
	    context->update = (SFTKCipher) sftk_RSAEncryptOAEP;
	    context->maxLen = nsslowkey_PublicModulusLen(info->key);
	    context->cipherInfo = info;
	} else {
	    SFTKOAEPDecryptInfo *info = PORT_New(SFTKOAEPDecryptInfo);
	    if (info == NULL) {
	        crv = CKR_HOST_MEMORY;
	        break;
	    }
	    info->params = pMechanism->pParameter;
	    info->key = sftk_GetPrivKey(key, CKK_RSA, &crv);
	    if (info->key == NULL) {
	        PORT_Free(info);
	        crv = CKR_KEY_HANDLE_INVALID;
	        break;
	    }
	    context->update = (SFTKCipher) sftk_RSADecryptOAEP;
	    context->maxLen = nsslowkey_PrivateModulusLen(info->key);
	    context->cipherInfo = info;
	}
	context->destroy = (SFTKDestroy) sftk_Space;
	break;
    case CKM_RC2_CBC_PAD:
	context->doPad = PR_TRUE;
	
    case CKM_RC2_ECB:
    case CKM_RC2_CBC:
	context->blockSize = 8;
	if (key_type != CKK_RC2) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	att = sftk_FindAttribute(key,CKA_VALUE);
	if (att == NULL) {
	    crv = CKR_KEY_HANDLE_INVALID;
	    break;
	}
	rc2_param = (CK_RC2_CBC_PARAMS *)pMechanism->pParameter;
	effectiveKeyLength = (rc2_param->ulEffectiveBits+7)/8;
	context->cipherInfo = 
	    RC2_CreateContext((unsigned char*)att->attrib.pValue,
			      att->attrib.ulValueLen, rc2_param->iv,
			      pMechanism->mechanism == CKM_RC2_ECB ? NSS_RC2 :
			      NSS_RC2_CBC,effectiveKeyLength);
	sftk_FreeAttribute(att);
	if (context->cipherInfo == NULL) {
	    crv = CKR_HOST_MEMORY;
	    break;
	}
	context->update = (SFTKCipher) (isEncrypt ? RC2_Encrypt : RC2_Decrypt);
	context->destroy = (SFTKDestroy) RC2_DestroyContext;
	break;
#if NSS_SOFTOKEN_DOES_RC5
    case CKM_RC5_CBC_PAD:
	context->doPad = PR_TRUE;
	
    case CKM_RC5_ECB:
    case CKM_RC5_CBC:
	if (key_type != CKK_RC5) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	att = sftk_FindAttribute(key,CKA_VALUE);
	if (att == NULL) {
	    crv = CKR_KEY_HANDLE_INVALID;
	    break;
	}
	rc5_param = (CK_RC5_CBC_PARAMS *)pMechanism->pParameter;
	context->blockSize = rc5_param->ulWordsize*2;
	rc5Key.data = (unsigned char*)att->attrib.pValue;
	rc5Key.len = att->attrib.ulValueLen;
	context->cipherInfo = RC5_CreateContext(&rc5Key,rc5_param->ulRounds,
	   rc5_param->ulWordsize,rc5_param->pIv,
		 pMechanism->mechanism == CKM_RC5_ECB ? NSS_RC5 : NSS_RC5_CBC);
	sftk_FreeAttribute(att);
	if (context->cipherInfo == NULL) {
	    crv = CKR_HOST_MEMORY;
	    break;
	}
	context->update = (SFTKCipher) (isEncrypt ? RC5_Encrypt : RC5_Decrypt);
	context->destroy = (SFTKDestroy) RC5_DestroyContext;
	break;
#endif
    case CKM_RC4:
	if (key_type != CKK_RC4) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	att = sftk_FindAttribute(key,CKA_VALUE);
	if (att == NULL) {
	    crv = CKR_KEY_HANDLE_INVALID;
	    break;
	}
	context->cipherInfo = 
	    RC4_CreateContext((unsigned char*)att->attrib.pValue,
			      att->attrib.ulValueLen);
	sftk_FreeAttribute(att);
	if (context->cipherInfo == NULL) {
	    crv = CKR_HOST_MEMORY;  
	    break;
	}
	context->update = (SFTKCipher) (isEncrypt ? RC4_Encrypt : RC4_Decrypt);
	context->destroy = (SFTKDestroy) RC4_DestroyContext;
	break;
    case CKM_CDMF_CBC_PAD:
	context->doPad = PR_TRUE;
	
    case CKM_CDMF_ECB:
    case CKM_CDMF_CBC:
	if (key_type != CKK_CDMF) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	t = (pMechanism->mechanism == CKM_CDMF_ECB) ? NSS_DES : NSS_DES_CBC;
	if (crv != CKR_OK) break;
	goto finish_des;
    case CKM_DES_ECB:
	if (key_type != CKK_DES) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	t = NSS_DES;
	goto finish_des;
    case CKM_DES_CBC_PAD:
	context->doPad = PR_TRUE;
	
    case CKM_DES_CBC:
	if (key_type != CKK_DES) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	t = NSS_DES_CBC;
	goto finish_des;
    case CKM_DES3_ECB:
	if ((key_type != CKK_DES2) && (key_type != CKK_DES3)) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	t = NSS_DES_EDE3;
	goto finish_des;
    case CKM_DES3_CBC_PAD:
	context->doPad = PR_TRUE;
	
    case CKM_DES3_CBC:
	if ((key_type != CKK_DES2) && (key_type != CKK_DES3)) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	t = NSS_DES_EDE3_CBC;
finish_des:
	context->blockSize = 8;
	att = sftk_FindAttribute(key,CKA_VALUE);
	if (att == NULL) {
	    crv = CKR_KEY_HANDLE_INVALID;
	    break;
	}
	if (key_type == CKK_DES2 && 
            (t == NSS_DES_EDE3_CBC || t == NSS_DES_EDE3)) {
	    
	    memcpy(newdeskey, att->attrib.pValue, 16);
	    memcpy(newdeskey + 16, newdeskey, 8);
	    useNewKey=PR_TRUE;
	} else if (key_type == CKK_CDMF) {
	    crv = sftk_cdmf2des((unsigned char*)att->attrib.pValue,newdeskey);
	    if (crv != CKR_OK) {
		sftk_FreeAttribute(att);
		break;
	    }
	    useNewKey=PR_TRUE;
	}
	context->cipherInfo = DES_CreateContext(
		useNewKey ? newdeskey : (unsigned char*)att->attrib.pValue,
		(unsigned char*)pMechanism->pParameter,t, isEncrypt);
	if (useNewKey) 
	    memset(newdeskey, 0, sizeof newdeskey);
	sftk_FreeAttribute(att);
	if (context->cipherInfo == NULL) {
	    crv = CKR_HOST_MEMORY;
	    break;
	}
	context->update = (SFTKCipher) (isEncrypt ? DES_Encrypt : DES_Decrypt);
	context->destroy = (SFTKDestroy) DES_DestroyContext;
	break;
    case CKM_SEED_CBC_PAD:
	context->doPad = PR_TRUE;
	
    case CKM_SEED_CBC:
        if (!pMechanism->pParameter ||
	     pMechanism->ulParameterLen != 16) {
            crv = CKR_MECHANISM_PARAM_INVALID;
            break;
        }
        
    case CKM_SEED_ECB:
	context->blockSize = 16;
	if (key_type != CKK_SEED) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	att = sftk_FindAttribute(key,CKA_VALUE);
	if (att == NULL) {
	    crv = CKR_KEY_HANDLE_INVALID;
	    break;
	}
	context->cipherInfo = SEED_CreateContext(
	    (unsigned char*)att->attrib.pValue,
	    (unsigned char*)pMechanism->pParameter,
	    pMechanism->mechanism == CKM_SEED_ECB ? NSS_SEED : NSS_SEED_CBC,
	    isEncrypt);
	sftk_FreeAttribute(att);
	if (context->cipherInfo == NULL) {
	    crv = CKR_HOST_MEMORY;
	    break;
	}
	context->update = (SFTKCipher)(isEncrypt ? SEED_Encrypt : SEED_Decrypt);
	context->destroy = (SFTKDestroy) SEED_DestroyContext;
	break;

    case CKM_CAMELLIA_CBC_PAD:
	context->doPad = PR_TRUE;
	
    case CKM_CAMELLIA_CBC:
	if (!pMechanism->pParameter ||
		 pMechanism->ulParameterLen != 16) {
	    crv = CKR_MECHANISM_PARAM_INVALID;
	    break;
	}
	
    case CKM_CAMELLIA_ECB:
	context->blockSize = 16;
	if (key_type != CKK_CAMELLIA) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	att = sftk_FindAttribute(key,CKA_VALUE);
	if (att == NULL) {
	    crv = CKR_KEY_HANDLE_INVALID;
	    break;
	}
	context->cipherInfo = Camellia_CreateContext(
	    (unsigned char*)att->attrib.pValue,
	    (unsigned char*)pMechanism->pParameter,
	    pMechanism->mechanism ==
	    CKM_CAMELLIA_ECB ? NSS_CAMELLIA : NSS_CAMELLIA_CBC,
	    isEncrypt, att->attrib.ulValueLen);
	sftk_FreeAttribute(att);
	if (context->cipherInfo == NULL) {
	    crv = CKR_HOST_MEMORY;
	    break;
	}
	context->update = (SFTKCipher) (isEncrypt ?
					Camellia_Encrypt : Camellia_Decrypt);
	context->destroy = (SFTKDestroy) Camellia_DestroyContext;
	break;

    case CKM_AES_CBC_PAD:
	context->doPad = PR_TRUE;
	
    case CKM_AES_ECB:
    case CKM_AES_CBC:
	context->blockSize = 16;
    case CKM_AES_CTS:
    case CKM_AES_CTR:
    case CKM_AES_GCM:
	if (pMechanism->mechanism == CKM_AES_GCM) {
	    context->multi = PR_FALSE;
	}
	if (key_type != CKK_AES) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	att = sftk_FindAttribute(key,CKA_VALUE);
	if (att == NULL) {
	    crv = CKR_KEY_HANDLE_INVALID;
	    break;
	}
	context->cipherInfo = AES_CreateContext(
	    (unsigned char*)att->attrib.pValue,
	    (unsigned char*)pMechanism->pParameter,
	    sftk_aes_mode(pMechanism->mechanism),
	    isEncrypt, att->attrib.ulValueLen, 16);
	sftk_FreeAttribute(att);
	if (context->cipherInfo == NULL) {
	    crv = CKR_HOST_MEMORY;
	    break;
	}
	context->update = (SFTKCipher) (isEncrypt ? AES_Encrypt : AES_Decrypt);
	context->destroy = (SFTKDestroy) AES_DestroyContext;
	break;

    case CKM_NETSCAPE_AES_KEY_WRAP_PAD:
    	context->doPad = PR_TRUE;
	
    case CKM_NETSCAPE_AES_KEY_WRAP:
	context->multi = PR_FALSE;
	context->blockSize = 8;
	if (key_type != CKK_AES) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	att = sftk_FindAttribute(key,CKA_VALUE);
	if (att == NULL) {
	    crv = CKR_KEY_HANDLE_INVALID;
	    break;
	}
	context->cipherInfo = AESKeyWrap_CreateContext(
	    (unsigned char*)att->attrib.pValue,
	    (unsigned char*)pMechanism->pParameter,
	    isEncrypt, att->attrib.ulValueLen);
	sftk_FreeAttribute(att);
	if (context->cipherInfo == NULL) {
	    crv = CKR_HOST_MEMORY;
	    break;
	}
	context->update = (SFTKCipher) (isEncrypt ? AESKeyWrap_Encrypt 
	                                          : AESKeyWrap_Decrypt);
	context->destroy = (SFTKDestroy) AESKeyWrap_DestroyContext;
	break;

    default:
	crv = CKR_MECHANISM_INVALID;
	break;
    }

    if (crv != CKR_OK) {
        sftk_FreeContext(context);
	sftk_FreeSession(session);
	return crv;
    }
    sftk_SetContextByType(session, contextType, context);
    sftk_FreeSession(session);
    return CKR_OK;
}


CK_RV NSC_EncryptInit(CK_SESSION_HANDLE hSession,
		 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey)
{
    CHECK_FORK();
    return sftk_CryptInit(hSession, pMechanism, hKey, CKA_ENCRYPT, CKA_ENCRYPT,
						SFTK_ENCRYPT, PR_TRUE);
}


CK_RV NSC_EncryptUpdate(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pPart, CK_ULONG ulPartLen, CK_BYTE_PTR pEncryptedPart,	
					CK_ULONG_PTR pulEncryptedPartLen)
{
    SFTKSessionContext *context;
    unsigned int outlen,i;
    unsigned int padoutlen = 0;
    unsigned int maxout = *pulEncryptedPartLen;
    CK_RV crv;
    SECStatus rv;

    CHECK_FORK();

    
    crv = sftk_GetContext(hSession,&context,SFTK_ENCRYPT,PR_TRUE,NULL);
    if (crv != CKR_OK) return crv;

    if (!pEncryptedPart) {
	if (context->doPad) {
	    CK_ULONG totalDataAvailable = ulPartLen + context->padDataLength;
	    CK_ULONG blocksToSend = totalDataAvailable/context->blockSize;

	    *pulEncryptedPartLen = blocksToSend * context->blockSize;
	    return CKR_OK;
	}
	*pulEncryptedPartLen = ulPartLen;
	return CKR_OK;
    }

    
    if (context->doPad) {
	
	if (context->padDataLength != 0) {
	    
	    for (i=context->padDataLength; 
			(ulPartLen != 0) && i < context->blockSize; i++) {
		context->padBuf[i] = *pPart++;
		ulPartLen--;
		context->padDataLength++;
	    }

	    
	    if (context->padDataLength != context->blockSize) {
		*pulEncryptedPartLen = 0;
		return CKR_OK;
	    }
	    
    	    rv = (*context->update)(context->cipherInfo, pEncryptedPart, 
		&padoutlen, context->blockSize, context->padBuf,
							context->blockSize);
	    if (rv != SECSuccess) {
		return sftk_MapCryptError(PORT_GetError());
	    }
	    pEncryptedPart += padoutlen;
	    maxout -= padoutlen;
	}
	
	context->padDataLength = ulPartLen % context->blockSize;
	if (context->padDataLength) {
	    PORT_Memcpy(context->padBuf,
			&pPart[ulPartLen-context->padDataLength],
							context->padDataLength);
	    ulPartLen -= context->padDataLength;
	}
	
	if (ulPartLen == 0) {
	    *pulEncryptedPartLen = padoutlen;
	    return CKR_OK;
	}
    }


    
    rv = (*context->update)(context->cipherInfo,pEncryptedPart, 
					&outlen, maxout, pPart, ulPartLen);
    *pulEncryptedPartLen = (CK_ULONG) (outlen + padoutlen);
    return (rv == SECSuccess) ? CKR_OK : sftk_MapCryptError(PORT_GetError());
}



CK_RV NSC_EncryptFinal(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pLastEncryptedPart, CK_ULONG_PTR pulLastEncryptedPartLen)
{
    SFTKSession *session;
    SFTKSessionContext *context;
    unsigned int outlen,i;
    unsigned int maxout = *pulLastEncryptedPartLen;
    CK_RV crv;
    SECStatus rv = SECSuccess;
    PRBool contextFinished = PR_TRUE;

    CHECK_FORK();

    
    crv = sftk_GetContext(hSession,&context,SFTK_ENCRYPT,PR_TRUE,&session);
    if (crv != CKR_OK) return crv;

    *pulLastEncryptedPartLen = 0;
    if (!pLastEncryptedPart) {
	
	if (context->blockSize > 0 && context->doPad) {
	    *pulLastEncryptedPartLen = context->blockSize;
	    contextFinished = PR_FALSE; 
	}
	goto finish;
    }

    
    if (context->doPad) {
	unsigned char  padbyte = (unsigned char) 
				(context->blockSize - context->padDataLength); 
	
	for (i=context->padDataLength; i < context->blockSize; i++) {
	    context->padBuf[i] = padbyte;
	}
	rv = (*context->update)(context->cipherInfo,pLastEncryptedPart, 
			&outlen, maxout, context->padBuf, context->blockSize);
	if (rv == SECSuccess) *pulLastEncryptedPartLen = (CK_ULONG) outlen;
    }

finish:
    if (contextFinished)
        sftk_TerminateOp( session, SFTK_ENCRYPT, context );
    sftk_FreeSession(session);
    return (rv == SECSuccess) ? CKR_OK : sftk_MapCryptError(PORT_GetError());
}


CK_RV NSC_Encrypt (CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData,
    		   CK_ULONG ulDataLen, CK_BYTE_PTR pEncryptedData,
		   CK_ULONG_PTR pulEncryptedDataLen)
{
    SFTKSession *session;
    SFTKSessionContext *context;
    unsigned int outlen;
    unsigned int maxoutlen = *pulEncryptedDataLen;
    CK_RV crv;
    CK_RV crv2;
    SECStatus rv = SECSuccess;
    SECItem   pText;

    pText.type = siBuffer;
    pText.data = pData;
    pText.len  = ulDataLen;

    CHECK_FORK();

    
    crv = sftk_GetContext(hSession,&context,SFTK_ENCRYPT,PR_FALSE,&session);
    if (crv != CKR_OK) return crv;

    if (!pEncryptedData) {
	*pulEncryptedDataLen = context->rsa ? context->maxLen :
		ulDataLen + 2 * context->blockSize;
	goto finish;
    }

    if (context->doPad) {
	if (context->multi) {
	    CK_ULONG finalLen;
	    

	    sftk_FreeSession(session);
	    crv = NSC_EncryptUpdate(hSession, pData, ulDataLen, pEncryptedData, 
	                            pulEncryptedDataLen);
	    if (crv != CKR_OK) 
	    	*pulEncryptedDataLen = 0;
	    maxoutlen      -= *pulEncryptedDataLen;
	    pEncryptedData += *pulEncryptedDataLen;
	    finalLen = maxoutlen;
	    crv2 = NSC_EncryptFinal(hSession, pEncryptedData, &finalLen);
	    if (crv2 == CKR_OK) 
	    	*pulEncryptedDataLen += finalLen;
	    return crv == CKR_OK ? crv2 : crv;
	}
	


	PORT_Assert(context->blockSize > 1);
	if (context->blockSize > 1) {
	    CK_ULONG remainder = ulDataLen % context->blockSize;
	    CK_ULONG padding   = context->blockSize - remainder;
	    pText.len += padding;
	    pText.data = PORT_ZAlloc(pText.len);
	    if (pText.data) {
		memcpy(pText.data, pData, ulDataLen);
		memset(pText.data + ulDataLen, padding, padding);
	    } else {
		crv = CKR_HOST_MEMORY;
		goto fail;
	    }
	}
    }

    
    rv = (*context->update)(context->cipherInfo, pEncryptedData, 
			    &outlen, maxoutlen, pText.data, pText.len);
    crv = (rv == SECSuccess) ? CKR_OK : sftk_MapCryptError(PORT_GetError());
    *pulEncryptedDataLen = (CK_ULONG) outlen;
    if (pText.data != pData)
    	PORT_ZFree(pText.data, pText.len);
fail:
    sftk_TerminateOp( session, SFTK_ENCRYPT, context );
finish:
    sftk_FreeSession(session);

    return crv;
}







CK_RV NSC_DecryptInit( CK_SESSION_HANDLE hSession,
			 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey)
{
    CHECK_FORK();
    return sftk_CryptInit(hSession, pMechanism, hKey, CKA_DECRYPT, CKA_DECRYPT,
						SFTK_DECRYPT, PR_FALSE);
}


CK_RV NSC_DecryptUpdate(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pEncryptedPart, CK_ULONG ulEncryptedPartLen,
    				CK_BYTE_PTR pPart, CK_ULONG_PTR pulPartLen)
{
    SFTKSessionContext *context;
    unsigned int padoutlen = 0;
    unsigned int outlen;
    unsigned int maxout = *pulPartLen;
    CK_RV crv;
    SECStatus rv;

    CHECK_FORK();

    
    crv = sftk_GetContext(hSession,&context,SFTK_DECRYPT,PR_TRUE,NULL);
    if (crv != CKR_OK) return crv;

    
    PORT_Assert((context->padDataLength == 0) 
		|| context->padDataLength == context->blockSize);


    if (context->doPad) {
	






	if ((ulEncryptedPartLen == 0) ||
	    (ulEncryptedPartLen % context->blockSize) != 0) {
	    return CKR_ENCRYPTED_DATA_LEN_RANGE;
	}
    }

    if (!pPart) {
	if (context->doPad) {
	    *pulPartLen = 
		ulEncryptedPartLen + context->padDataLength - context->blockSize;
	    return CKR_OK;
	}
	



	*pulPartLen = ulEncryptedPartLen;
	return CKR_OK;
    }

    if (context->doPad) {
	
	if (context->padDataLength != 0) {
    	    rv = (*context->update)(context->cipherInfo, pPart, &padoutlen,
		 maxout, context->padBuf, context->blockSize);
    	    if (rv != SECSuccess) return sftk_MapDecryptError(PORT_GetError());
	    pPart += padoutlen;
	    maxout -= padoutlen;
	}
	
	PORT_Memcpy(context->padBuf,&pEncryptedPart[ulEncryptedPartLen -
				context->blockSize], context->blockSize);
	context->padDataLength = context->blockSize;
	ulEncryptedPartLen -= context->padDataLength;
    }

    
    rv = (*context->update)(context->cipherInfo,pPart, &outlen,
		 maxout, pEncryptedPart, ulEncryptedPartLen);
    *pulPartLen = (CK_ULONG) (outlen + padoutlen);
    return (rv == SECSuccess)  ? CKR_OK : sftk_MapDecryptError(PORT_GetError());
}



CK_RV NSC_DecryptFinal(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pLastPart, CK_ULONG_PTR pulLastPartLen)
{
    SFTKSession *session;
    SFTKSessionContext *context;
    unsigned int outlen;
    unsigned int maxout = *pulLastPartLen;
    CK_RV crv;
    SECStatus rv = SECSuccess;

    CHECK_FORK();

    
    crv = sftk_GetContext(hSession,&context,SFTK_DECRYPT,PR_TRUE,&session);
    if (crv != CKR_OK) return crv;

    *pulLastPartLen = 0;
    if (!pLastPart) {
	
	if (context->padDataLength > 0) {
	    *pulLastPartLen = context->padDataLength;
	}
	goto finish;
    }

    if (context->doPad) {
	
	if (context->padDataLength != 0) {
	    

    	    rv = (*context->update)(context->cipherInfo, pLastPart, &outlen,
		 maxout, context->padBuf, context->blockSize);
	    if (rv != SECSuccess) {
		crv = sftk_MapDecryptError(PORT_GetError());
	    } else {
		unsigned int padSize = 
			    (unsigned int) pLastPart[context->blockSize-1];
		if ((padSize > context->blockSize) || (padSize == 0)) {
		    crv = CKR_ENCRYPTED_DATA_INVALID;
		} else {
		    unsigned int i;
		    unsigned int badPadding = 0;  
		    for (i = 0; i < padSize; i++) {
			badPadding |=
			    (unsigned int) pLastPart[context->blockSize-1-i] ^
			    padSize;
		    }
		    if (badPadding) {
			crv = CKR_ENCRYPTED_DATA_INVALID;
		    } else {
			*pulLastPartLen = outlen - padSize;
		    }
		}
	    }
	}
    }

    sftk_TerminateOp( session, SFTK_DECRYPT, context );
finish:
    sftk_FreeSession(session);
    return crv;
}


CK_RV NSC_Decrypt(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pEncryptedData,CK_ULONG ulEncryptedDataLen,CK_BYTE_PTR pData,
    						CK_ULONG_PTR pulDataLen)
{
    SFTKSession *session;
    SFTKSessionContext *context;
    unsigned int outlen;
    unsigned int maxoutlen = *pulDataLen;
    CK_RV crv;
    CK_RV crv2;
    SECStatus rv = SECSuccess;

    CHECK_FORK();

    
    crv = sftk_GetContext(hSession,&context,SFTK_DECRYPT,PR_FALSE,&session);
    if (crv != CKR_OK) return crv;

    if (!pData) {
	*pulDataLen = ulEncryptedDataLen + context->blockSize;
	goto finish;
    }

    if (context->doPad && context->multi) {
	CK_ULONG finalLen;
	

	sftk_FreeSession(session);
	crv = NSC_DecryptUpdate(hSession,pEncryptedData,ulEncryptedDataLen,
							pData, pulDataLen);
	if (crv != CKR_OK) 
	    *pulDataLen = 0;
	maxoutlen -= *pulDataLen;
	pData     += *pulDataLen;
	finalLen = maxoutlen;
	crv2 = NSC_DecryptFinal(hSession, pData, &finalLen);
	if (crv2 == CKR_OK) 
	    *pulDataLen += finalLen;
	return crv == CKR_OK ? crv2 : crv;
    }

    rv = (*context->update)(context->cipherInfo, pData, &outlen, maxoutlen, 
					pEncryptedData, ulEncryptedDataLen);
    
    crv = (rv == SECSuccess) ? CKR_OK : sftk_MapDecryptError(PORT_GetError());
    if (rv == SECSuccess && context->doPad) {
	unsigned int padding = pData[outlen - 1];
	if (padding > context->blockSize || !padding) {
	    crv = CKR_ENCRYPTED_DATA_INVALID;
	} else {
	    unsigned int i;
	    unsigned int badPadding = 0;  
	    for (i = 0; i < padding; i++) {
		badPadding |= (unsigned int) pData[outlen - 1 - i] ^ padding;
	    }
	    if (badPadding) {
		crv = CKR_ENCRYPTED_DATA_INVALID;
	    } else {
		outlen -= padding;
	    }
	}
    }
    *pulDataLen = (CK_ULONG) outlen;
    sftk_TerminateOp( session, SFTK_DECRYPT, context );
finish:
    sftk_FreeSession(session);
    return crv;
}








CK_RV NSC_DigestInit(CK_SESSION_HANDLE hSession,
    					CK_MECHANISM_PTR pMechanism)
{
    SFTKSession *session;
    SFTKSessionContext *context;
    CK_RV crv = CKR_OK;

    CHECK_FORK();

    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) 
    	return CKR_SESSION_HANDLE_INVALID;
    crv = sftk_InitGeneric(session,&context,SFTK_HASH,NULL,0,NULL, 0, 0);
    if (crv != CKR_OK) {
	sftk_FreeSession(session);
	return crv;
    }


#define INIT_MECH(mech,mmm) \
    case mech: { \
	mmm ## Context * mmm ## _ctx = mmm ## _NewContext(); \
	context->cipherInfo    = (void *)mmm ## _ctx; \
	context->cipherInfoLen = mmm ## _FlattenSize(mmm ## _ctx); \
	context->currentMech   = mech; \
	context->hashUpdate    = (SFTKHash)    mmm ## _Update; \
	context->end           = (SFTKEnd)     mmm ## _End; \
	context->destroy       = (SFTKDestroy) mmm ## _DestroyContext; \
	context->maxLen        = mmm ## _LENGTH; \
        if (mmm ## _ctx) \
	    mmm ## _Begin(mmm ## _ctx); \
	else  \
	    crv = CKR_HOST_MEMORY; \
	break; \
    }

    switch(pMechanism->mechanism) {
    INIT_MECH(CKM_MD2,    MD2)
    INIT_MECH(CKM_MD5,    MD5)
    INIT_MECH(CKM_SHA_1,  SHA1)
    INIT_MECH(CKM_SHA224, SHA224)
    INIT_MECH(CKM_SHA256, SHA256)
    INIT_MECH(CKM_SHA384, SHA384)
    INIT_MECH(CKM_SHA512, SHA512)

    default:
	crv = CKR_MECHANISM_INVALID;
	break;
    }

    if (crv != CKR_OK) {
        sftk_FreeContext(context);
	sftk_FreeSession(session);
	return crv;
    }
    sftk_SetContextByType(session, SFTK_HASH, context);
    sftk_FreeSession(session);
    return CKR_OK;
}



CK_RV NSC_Digest(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pData, CK_ULONG ulDataLen, CK_BYTE_PTR pDigest,
    						CK_ULONG_PTR pulDigestLen)
{
    SFTKSession *session;
    SFTKSessionContext *context;
    unsigned int digestLen;
    unsigned int maxout = *pulDigestLen;
    CK_RV crv;

    CHECK_FORK();

    
    crv = sftk_GetContext(hSession,&context,SFTK_HASH,PR_FALSE,&session);
    if (crv != CKR_OK) return crv;

    if (pDigest == NULL) {
	*pulDigestLen = context->maxLen;
	goto finish;
    }

    
    (*context->hashUpdate)(context->cipherInfo, pData, ulDataLen);
    
    (*context->end)(context->cipherInfo, pDigest, &digestLen,maxout);
    *pulDigestLen = digestLen;

    sftk_TerminateOp( session, SFTK_HASH, context );
finish:
    sftk_FreeSession(session);
    return CKR_OK;
}



CK_RV NSC_DigestUpdate(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pPart,
					    CK_ULONG ulPartLen)
{
    SFTKSessionContext *context;
    CK_RV crv;

    CHECK_FORK();

    
    crv = sftk_GetContext(hSession,&context,SFTK_HASH,PR_TRUE,NULL);
    if (crv != CKR_OK) return crv;
    
    (*context->hashUpdate)(context->cipherInfo, pPart, ulPartLen);
    return CKR_OK;
}



CK_RV NSC_DigestFinal(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pDigest,
    						CK_ULONG_PTR pulDigestLen)
{
    SFTKSession *session;
    SFTKSessionContext *context;
    unsigned int maxout = *pulDigestLen;
    unsigned int digestLen;
    CK_RV crv;

    CHECK_FORK();

    
    crv = sftk_GetContext(hSession, &context, SFTK_HASH, PR_TRUE, &session);
    if (crv != CKR_OK) return crv;

    if (pDigest != NULL) {
        (*context->end)(context->cipherInfo, pDigest, &digestLen, maxout);
        *pulDigestLen = digestLen;
        sftk_TerminateOp( session, SFTK_HASH, context );
    } else {
	*pulDigestLen = context->maxLen;
    }

    sftk_FreeSession(session);
    return CKR_OK;
}





#define DOSUB(mmm) \
static CK_RV \
sftk_doSub ## mmm(SFTKSessionContext *context) { \
    mmm ## Context * mmm ## _ctx = mmm ## _NewContext(); \
    context->hashInfo    = (void *)      mmm ## _ctx; \
    context->hashUpdate  = (SFTKHash)    mmm ## _Update; \
    context->end         = (SFTKEnd)     mmm ## _End; \
    context->hashdestroy = (SFTKDestroy) mmm ## _DestroyContext; \
    if (!context->hashInfo) { \
	return CKR_HOST_MEMORY; \
    } \
    mmm ## _Begin( mmm ## _ctx ); \
    return CKR_OK; \
}

DOSUB(MD2)
DOSUB(MD5)
DOSUB(SHA1)
DOSUB(SHA224)
DOSUB(SHA256)
DOSUB(SHA384)
DOSUB(SHA512)

static SECStatus
sftk_SignCopy(
	CK_ULONG *copyLen,
	void *out, unsigned int *outLength,
	unsigned int maxLength,
	const unsigned char *hashResult,
	unsigned int hashResultLength)
{
    unsigned int toCopy = *copyLen;
    if (toCopy > maxLength) {
	toCopy = maxLength;
    }
    if (toCopy > hashResultLength) {
	toCopy = hashResultLength;
    }
    memcpy(out, hashResult, toCopy);
    if (outLength) {
	*outLength = toCopy;
    }
    return SECSuccess;
}


static SECStatus
sftk_HMACCmp(CK_ULONG *copyLen,unsigned char *sig,unsigned int sigLen,
				unsigned char *hash, unsigned int hashLen)
{
    return (PORT_Memcmp(sig,hash,*copyLen) == 0) ? SECSuccess : SECFailure ; 
}




static CK_RV
sftk_doHMACInit(SFTKSessionContext *context,HASH_HashType hash,
					SFTKObject *key, CK_ULONG mac_size)
{
    SFTKAttribute *keyval;
    HMACContext *HMACcontext;
    CK_ULONG *intpointer;
    const SECHashObject *hashObj = HASH_GetRawHashObject(hash);
    PRBool isFIPS = (key->slot->slotID == FIPS_SLOT_ID);

    
    if (isFIPS && (mac_size < 4 || mac_size < hashObj->length/2)) {
	return CKR_BUFFER_TOO_SMALL;
    }

    keyval = sftk_FindAttribute(key,CKA_VALUE);
    if (keyval == NULL) return CKR_KEY_SIZE_RANGE;

    HMACcontext = HMAC_Create(hashObj, 
		(const unsigned char*)keyval->attrib.pValue,
		keyval->attrib.ulValueLen, isFIPS);
    context->hashInfo = HMACcontext;
    context->multi = PR_TRUE;
    sftk_FreeAttribute(keyval);
    if (context->hashInfo == NULL) {
	if (PORT_GetError() == SEC_ERROR_INVALID_ARGS) {
	    return CKR_KEY_SIZE_RANGE;
	}
	return CKR_HOST_MEMORY;
    }
    context->hashUpdate = (SFTKHash) HMAC_Update;
    context->end = (SFTKEnd) HMAC_Finish;

    context->hashdestroy = (SFTKDestroy) HMAC_Destroy;
    intpointer = PORT_New(CK_ULONG);
    if (intpointer == NULL) {
	return CKR_HOST_MEMORY;
    }
    *intpointer = mac_size;
    context->cipherInfo = intpointer;
    context->destroy = (SFTKDestroy) sftk_Space;
    context->update = (SFTKCipher) sftk_SignCopy;
    context->verify = (SFTKVerify) sftk_HMACCmp;
    context->maxLen = hashObj->length;
    HMAC_Begin(HMACcontext);
    return CKR_OK;
}












static unsigned char ssl_pad_1 [60] = {
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36, 0x36,
    0x36, 0x36, 0x36, 0x36
};
static unsigned char ssl_pad_2 [60] = {
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c, 0x5c,
    0x5c, 0x5c, 0x5c, 0x5c
};

static SECStatus
sftk_SSLMACSign(SFTKSSLMACInfo *info,unsigned char *sig,unsigned int *sigLen,
		unsigned int maxLen,unsigned char *hash, unsigned int hashLen)
{
    unsigned char tmpBuf[SFTK_MAX_MAC_LENGTH];
    unsigned int out;

    info->begin(info->hashContext);
    info->update(info->hashContext,info->key,info->keySize);
    info->update(info->hashContext,ssl_pad_2,info->padSize);
    info->update(info->hashContext,hash,hashLen);
    info->end(info->hashContext,tmpBuf,&out,SFTK_MAX_MAC_LENGTH);
    PORT_Memcpy(sig,tmpBuf,info->macSize);
    *sigLen = info->macSize;
    return SECSuccess;
}

static SECStatus
sftk_SSLMACVerify(SFTKSSLMACInfo *info,unsigned char *sig,unsigned int sigLen,
		unsigned char *hash, unsigned int hashLen)
{
    unsigned char tmpBuf[SFTK_MAX_MAC_LENGTH];
    unsigned int out;

    info->begin(info->hashContext);
    info->update(info->hashContext,info->key,info->keySize);
    info->update(info->hashContext,ssl_pad_2,info->padSize);
    info->update(info->hashContext,hash,hashLen);
    info->end(info->hashContext,tmpBuf,&out,SFTK_MAX_MAC_LENGTH);
    return (PORT_Memcmp(sig,tmpBuf,info->macSize) == 0) ? 
						SECSuccess : SECFailure;
}




static CK_RV
sftk_doSSLMACInit(SFTKSessionContext *context,SECOidTag oid,
					SFTKObject *key, CK_ULONG mac_size)
{
    SFTKAttribute *keyval;
    SFTKBegin begin;
    int padSize;
    SFTKSSLMACInfo *sslmacinfo;
    CK_RV crv = CKR_MECHANISM_INVALID;

    if (oid == SEC_OID_SHA1) {
	crv = sftk_doSubSHA1(context);
	if (crv != CKR_OK) return crv;
	begin = (SFTKBegin) SHA1_Begin;
	padSize = 40;
    } else {
	crv = sftk_doSubMD5(context);
	if (crv != CKR_OK) return crv;
	begin = (SFTKBegin) MD5_Begin;
	padSize = 48;
    }
    context->multi = PR_TRUE;

    keyval = sftk_FindAttribute(key,CKA_VALUE);
    if (keyval == NULL) return CKR_KEY_SIZE_RANGE;

    context->hashUpdate(context->hashInfo,keyval->attrib.pValue,
						 keyval->attrib.ulValueLen);
    context->hashUpdate(context->hashInfo,ssl_pad_1,padSize);
    sslmacinfo = (SFTKSSLMACInfo *) PORT_Alloc(sizeof(SFTKSSLMACInfo));
    if (sslmacinfo == NULL) {
        sftk_FreeAttribute(keyval);
	return CKR_HOST_MEMORY;
    }
    sslmacinfo->macSize = mac_size;
    sslmacinfo->hashContext = context->hashInfo;
    PORT_Memcpy(sslmacinfo->key,keyval->attrib.pValue,
					keyval->attrib.ulValueLen);
    sslmacinfo->keySize = keyval->attrib.ulValueLen;
    sslmacinfo->begin = begin;
    sslmacinfo->end = context->end;
    sslmacinfo->update = context->hashUpdate;
    sslmacinfo->padSize = padSize;
    sftk_FreeAttribute(keyval);
    context->cipherInfo = (void *) sslmacinfo;
    context->destroy = (SFTKDestroy) sftk_Space;
    context->update = (SFTKCipher) sftk_SSLMACSign;
    context->verify = (SFTKVerify) sftk_SSLMACVerify;
    context->maxLen = mac_size;
    return CKR_OK;
}










static CK_RV
sftk_InitCBCMac(CK_SESSION_HANDLE hSession, CK_MECHANISM_PTR pMechanism,
 	CK_OBJECT_HANDLE hKey, CK_ATTRIBUTE_TYPE keyUsage,
						 SFTKContextType contextType)
	
{
    CK_MECHANISM cbc_mechanism;
    CK_ULONG mac_bytes = SFTK_INVALID_MAC_SIZE;
    CK_RC2_CBC_PARAMS rc2_params;
#if NSS_SOFTOKEN_DOES_RC5
    CK_RC5_CBC_PARAMS rc5_params;
    CK_RC5_MAC_GENERAL_PARAMS *rc5_mac;
#endif
    unsigned char ivBlock[SFTK_MAX_BLOCK_SIZE];
    SFTKSessionContext *context;
    CK_RV crv;
    unsigned int blockSize;

    switch (pMechanism->mechanism) {
    case CKM_RC2_MAC_GENERAL:
	mac_bytes = 
	    ((CK_RC2_MAC_GENERAL_PARAMS *)pMechanism->pParameter)->ulMacLength;
	
    case CKM_RC2_MAC:
	

	rc2_params.ulEffectiveBits = ((CK_RC2_MAC_GENERAL_PARAMS *)
				pMechanism->pParameter)->ulEffectiveBits;
	PORT_Memset(rc2_params.iv,0,sizeof(rc2_params.iv));
	cbc_mechanism.mechanism = CKM_RC2_CBC;
	cbc_mechanism.pParameter = &rc2_params;
	cbc_mechanism.ulParameterLen = sizeof(rc2_params);
	blockSize = 8;
	break;
#if NSS_SOFTOKEN_DOES_RC5
    case CKM_RC5_MAC_GENERAL:
	mac_bytes = 
	    ((CK_RC5_MAC_GENERAL_PARAMS *)pMechanism->pParameter)->ulMacLength;
	
    case CKM_RC5_MAC:
	

	rc5_mac = (CK_RC5_MAC_GENERAL_PARAMS *)pMechanism->pParameter;
	rc5_params.ulWordsize = rc5_mac->ulWordsize;
	rc5_params.ulRounds = rc5_mac->ulRounds;
	rc5_params.pIv = ivBlock;
	if( (blockSize = rc5_mac->ulWordsize*2) > SFTK_MAX_BLOCK_SIZE )
            return CKR_MECHANISM_PARAM_INVALID;
	rc5_params.ulIvLen = blockSize;
	PORT_Memset(ivBlock,0,blockSize);
	cbc_mechanism.mechanism = CKM_RC5_CBC;
	cbc_mechanism.pParameter = &rc5_params;
	cbc_mechanism.ulParameterLen = sizeof(rc5_params);
	break;
#endif
    
    case CKM_DES_MAC_GENERAL:
	mac_bytes = *(CK_ULONG *)pMechanism->pParameter;
	
    case CKM_DES_MAC:
	blockSize = 8;
	PORT_Memset(ivBlock,0,blockSize);
	cbc_mechanism.mechanism = CKM_DES_CBC;
	cbc_mechanism.pParameter = &ivBlock;
	cbc_mechanism.ulParameterLen = blockSize;
	break;
    case CKM_DES3_MAC_GENERAL:
	mac_bytes = *(CK_ULONG *)pMechanism->pParameter;
	
    case CKM_DES3_MAC:
	blockSize = 8;
	PORT_Memset(ivBlock,0,blockSize);
	cbc_mechanism.mechanism = CKM_DES3_CBC;
	cbc_mechanism.pParameter = &ivBlock;
	cbc_mechanism.ulParameterLen = blockSize;
	break;
    case CKM_CDMF_MAC_GENERAL:
	mac_bytes = *(CK_ULONG *)pMechanism->pParameter;
	
    case CKM_CDMF_MAC:
	blockSize = 8;
	PORT_Memset(ivBlock,0,blockSize);
	cbc_mechanism.mechanism = CKM_CDMF_CBC;
	cbc_mechanism.pParameter = &ivBlock;
	cbc_mechanism.ulParameterLen = blockSize;
	break;
    case CKM_SEED_MAC_GENERAL:
	mac_bytes = *(CK_ULONG *)pMechanism->pParameter;
	
    case CKM_SEED_MAC:
	blockSize = 16;
	PORT_Memset(ivBlock,0,blockSize);
	cbc_mechanism.mechanism = CKM_SEED_CBC;
	cbc_mechanism.pParameter = &ivBlock;
	cbc_mechanism.ulParameterLen = blockSize;
	break;
    case CKM_CAMELLIA_MAC_GENERAL:
	mac_bytes = *(CK_ULONG *)pMechanism->pParameter;
	
    case CKM_CAMELLIA_MAC:
	blockSize = 16;
	PORT_Memset(ivBlock,0,blockSize);
	cbc_mechanism.mechanism = CKM_CAMELLIA_CBC;
	cbc_mechanism.pParameter = &ivBlock;
	cbc_mechanism.ulParameterLen = blockSize;
	break;
    case CKM_AES_MAC_GENERAL:
	mac_bytes = *(CK_ULONG *)pMechanism->pParameter;
	
    case CKM_AES_MAC:
	blockSize = 16;
	PORT_Memset(ivBlock,0,blockSize);
	cbc_mechanism.mechanism = CKM_AES_CBC;
	cbc_mechanism.pParameter = &ivBlock;
	cbc_mechanism.ulParameterLen = blockSize;
	break;
    default:
	return CKR_FUNCTION_NOT_SUPPORTED;
    }

    

    if (mac_bytes == SFTK_INVALID_MAC_SIZE)
        mac_bytes = blockSize >> 1;
    else {
        if( mac_bytes > blockSize )
            return CKR_MECHANISM_PARAM_INVALID;
    }

    crv = sftk_CryptInit(hSession, &cbc_mechanism, hKey,
            CKA_ENCRYPT, 
            keyUsage, contextType, PR_TRUE );
    if (crv != CKR_OK) return crv;
    crv = sftk_GetContext(hSession,&context,contextType,PR_TRUE,NULL);

    
    PORT_Assert(crv == CKR_OK);
    if (crv != CKR_OK) return crv;
    context->blockSize = blockSize;
    context->macSize = mac_bytes;
    return CKR_OK;
}




static SECStatus
sftk_RSAHashSign(SFTKHashSignInfo *info, unsigned char *sig,
                 unsigned int *sigLen, unsigned int maxLen,
                 const unsigned char *hash, unsigned int hashLen)
{
    PORT_Assert(info->key->keyType == NSSLOWKEYRSAKey);
    if (info->key->keyType != NSSLOWKEYRSAKey) {
        PORT_SetError(SEC_ERROR_INVALID_KEY);
        return SECFailure;
    }

    return RSA_HashSign(info->hashOid, info->key, sig, sigLen, maxLen,
                        hash, hashLen);
}


static DERTemplate SECAlgorithmIDTemplate[] = {
    { DER_SEQUENCE,
	  0, NULL, sizeof(SECAlgorithmID) },
    { DER_OBJECT_ID,
	  offsetof(SECAlgorithmID,algorithm), },
    { DER_OPTIONAL | DER_ANY,
	  offsetof(SECAlgorithmID,parameters), },
    { 0, }
};





static DERTemplate SGNDigestInfoTemplate[] = {
    { DER_SEQUENCE,
	  0, NULL, sizeof(SGNDigestInfo) },
    { DER_INLINE,
	  offsetof(SGNDigestInfo,digestAlgorithm),
	  SECAlgorithmIDTemplate, },
    { DER_OCTET_STRING,
	  offsetof(SGNDigestInfo,digest), },
    { 0, }
};




SECStatus
RSA_HashSign(SECOidTag hashOid, NSSLOWKEYPrivateKey *key,
             unsigned char *sig, unsigned int *sigLen, unsigned int maxLen,
             const unsigned char *hash, unsigned int hashLen)
{
    SECStatus rv = SECFailure;
    SECItem digder;
    PLArenaPool *arena = NULL;
    SGNDigestInfo *di = NULL;

    digder.data = NULL;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!arena) {
        goto loser;
    }

    
    di = SGN_CreateDigestInfo(hashOid, hash, hashLen);
    if (!di) {
        goto loser;
    }

    
    rv = DER_Encode(arena, &digder, SGNDigestInfoTemplate, di);
    if (rv != SECSuccess) {
        goto loser;
    }

    



    rv = RSA_Sign(&key->u.rsa, sig, sigLen, maxLen, digder.data,
                  digder.len);
    if (rv != SECSuccess && PORT_GetError() == SEC_ERROR_LIBRARY_FAILURE) {
        sftk_fatalError = PR_TRUE;
    }

  loser:
    SGN_DestroyDigestInfo(di);
    if (arena != NULL) {
        PORT_FreeArena(arena, PR_FALSE);
    }
    return rv;
}

static SECStatus
sftk_RSASign(NSSLOWKEYPrivateKey *key, unsigned char *output,
             unsigned int *outputLen, unsigned int maxOutputLen,
             const unsigned char *input, unsigned int inputLen)
{
    SECStatus rv = SECFailure;

    PORT_Assert(key->keyType == NSSLOWKEYRSAKey);
    if (key->keyType != NSSLOWKEYRSAKey) {
        PORT_SetError(SEC_ERROR_INVALID_KEY);
        return SECFailure;
    }

    rv = RSA_Sign(&key->u.rsa, output, outputLen, maxOutputLen, input,
                  inputLen);
    if (rv != SECSuccess && PORT_GetError() == SEC_ERROR_LIBRARY_FAILURE) {
        sftk_fatalError = PR_TRUE;
    }
    return rv;
}

static SECStatus
sftk_RSASignRaw(NSSLOWKEYPrivateKey *key, unsigned char *output,
                unsigned int *outputLen, unsigned int maxOutputLen,
                const unsigned char *input, unsigned int inputLen)
{
    SECStatus rv = SECFailure;

    PORT_Assert(key->keyType == NSSLOWKEYRSAKey);
    if (key->keyType != NSSLOWKEYRSAKey) {
        PORT_SetError(SEC_ERROR_INVALID_KEY);
        return SECFailure;
    }

    rv = RSA_SignRaw(&key->u.rsa, output, outputLen, maxOutputLen, input,
                     inputLen);
    if (rv != SECSuccess && PORT_GetError() == SEC_ERROR_LIBRARY_FAILURE) {
        sftk_fatalError = PR_TRUE;
    }
    return rv;
    
}

static SECStatus
sftk_RSASignPSS(SFTKHashSignInfo *info, unsigned char *sig,
                unsigned int *sigLen, unsigned int maxLen,
                const unsigned char *hash, unsigned int hashLen)
{
    SECStatus rv = SECFailure;
    HASH_HashType hashAlg;
    HASH_HashType maskHashAlg;
    CK_RSA_PKCS_PSS_PARAMS *params = (CK_RSA_PKCS_PSS_PARAMS *)info->params;

    PORT_Assert(info->key->keyType == NSSLOWKEYRSAKey);
    if (info->key->keyType != NSSLOWKEYRSAKey) {
        PORT_SetError(SEC_ERROR_INVALID_KEY);
        return SECFailure;
    }

    hashAlg = GetHashTypeFromMechanism(params->hashAlg);
    maskHashAlg = GetHashTypeFromMechanism(params->mgf);

    rv = RSA_SignPSS(&info->key->u.rsa, hashAlg, maskHashAlg, NULL,
                     params->sLen, sig, sigLen, maxLen, hash, hashLen);
    if (rv != SECSuccess && PORT_GetError() == SEC_ERROR_LIBRARY_FAILURE) {
        sftk_fatalError = PR_TRUE;
    }
    return rv;
}

static SECStatus
nsc_DSA_Verify_Stub(void *ctx, void *sigBuf, unsigned int sigLen,
                               void *dataBuf, unsigned int dataLen)
{
    SECItem signature, digest;
    NSSLOWKEYPublicKey *key = (NSSLOWKEYPublicKey *)ctx;

    signature.data = (unsigned char *)sigBuf;
    signature.len = sigLen;
    digest.data = (unsigned char *)dataBuf;
    digest.len = dataLen;
    return DSA_VerifyDigest(&(key->u.dsa), &signature, &digest);
}

static SECStatus
nsc_DSA_Sign_Stub(void *ctx, void *sigBuf,
                  unsigned int *sigLen, unsigned int maxSigLen,
                  void *dataBuf, unsigned int dataLen)
{
    SECItem signature, digest;
    SECStatus rv;
    NSSLOWKEYPrivateKey *key = (NSSLOWKEYPrivateKey *)ctx;

    signature.data = (unsigned char *)sigBuf;
    signature.len = maxSigLen;
    digest.data = (unsigned char *)dataBuf;
    digest.len = dataLen;
    rv = DSA_SignDigest(&(key->u.dsa), &signature, &digest);
    if (rv != SECSuccess && PORT_GetError() == SEC_ERROR_LIBRARY_FAILURE) {
	sftk_fatalError = PR_TRUE;
    }
    *sigLen = signature.len;
    return rv;
}

#ifndef NSS_DISABLE_ECC
static SECStatus
nsc_ECDSAVerifyStub(void *ctx, void *sigBuf, unsigned int sigLen,
                    void *dataBuf, unsigned int dataLen)
{
    SECItem signature, digest;
    NSSLOWKEYPublicKey *key = (NSSLOWKEYPublicKey *)ctx;

    signature.data = (unsigned char *)sigBuf;
    signature.len = sigLen;
    digest.data = (unsigned char *)dataBuf;
    digest.len = dataLen;
    return ECDSA_VerifyDigest(&(key->u.ec), &signature, &digest);
}

static SECStatus
nsc_ECDSASignStub(void *ctx, void *sigBuf,
                  unsigned int *sigLen, unsigned int maxSigLen,
                  void *dataBuf, unsigned int dataLen)
{
    SECItem signature, digest;
    SECStatus rv;
    NSSLOWKEYPrivateKey *key = (NSSLOWKEYPrivateKey *)ctx;

    signature.data = (unsigned char *)sigBuf;
    signature.len = maxSigLen;
    digest.data = (unsigned char *)dataBuf;
    digest.len = dataLen;
    rv = ECDSA_SignDigest(&(key->u.ec), &signature, &digest);
    if (rv != SECSuccess && PORT_GetError() == SEC_ERROR_LIBRARY_FAILURE) {
	sftk_fatalError = PR_TRUE;
    }
    *sigLen = signature.len;
    return rv;
}
#endif 























CK_RV NSC_SignInit(CK_SESSION_HANDLE hSession,
		 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hKey)
{
    SFTKSession *session;
    SFTKObject *key;
    SFTKSessionContext *context;
    CK_KEY_TYPE key_type;
    CK_RV crv = CKR_OK;
    NSSLOWKEYPrivateKey *privKey;
    SFTKHashSignInfo *info = NULL;

    CHECK_FORK();

    
    crv = sftk_InitCBCMac(hSession, pMechanism, hKey, CKA_SIGN, SFTK_SIGN);
    if (crv != CKR_FUNCTION_NOT_SUPPORTED) return crv;

    
    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;
    crv = sftk_InitGeneric(session,&context,SFTK_SIGN,&key,hKey,&key_type,
						CKO_PRIVATE_KEY,CKA_SIGN);
    if (crv != CKR_OK) {
	sftk_FreeSession(session);
	return crv;
    }

    context->multi = PR_FALSE;

#define INIT_RSA_SIGN_MECH(mmm) \
    case CKM_ ## mmm ## _RSA_PKCS: \
        context->multi = PR_TRUE; \
	crv = sftk_doSub ## mmm (context); \
	if (crv != CKR_OK) break; \
	context->update = (SFTKCipher) sftk_RSAHashSign; \
	info = PORT_New(SFTKHashSignInfo); \
	if (info == NULL) { crv = CKR_HOST_MEMORY; break; } \
	info->hashOid = SEC_OID_ ## mmm ; \
	goto finish_rsa; 

    switch(pMechanism->mechanism) {
    INIT_RSA_SIGN_MECH(MD5)
    INIT_RSA_SIGN_MECH(MD2)
    INIT_RSA_SIGN_MECH(SHA1)
    INIT_RSA_SIGN_MECH(SHA224)
    INIT_RSA_SIGN_MECH(SHA256)
    INIT_RSA_SIGN_MECH(SHA384)
    INIT_RSA_SIGN_MECH(SHA512)

    case CKM_RSA_PKCS:
	context->update = (SFTKCipher) sftk_RSASign;
	goto finish_rsa;
    case CKM_RSA_X_509:
	context->update = (SFTKCipher) sftk_RSASignRaw;
finish_rsa:
	if (key_type != CKK_RSA) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	context->rsa = PR_TRUE;
	privKey = sftk_GetPrivKey(key,CKK_RSA,&crv);
	if (privKey == NULL) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	



	if (info) {
	    info->key = privKey;
	    context->cipherInfo = info;
	    context->destroy = (SFTKDestroy)sftk_Space;
	} else {
	    context->cipherInfo = privKey;
	    context->destroy = (SFTKDestroy)sftk_Null;
	}
	context->maxLen = nsslowkey_PrivateModulusLen(privKey);
	break;
    case CKM_RSA_PKCS_PSS:
	if (key_type != CKK_RSA) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	} 
	context->rsa = PR_TRUE;
	if (pMechanism->ulParameterLen != sizeof(CK_RSA_PKCS_PSS_PARAMS) ||
	    !sftk_ValidatePssParams((const CK_RSA_PKCS_PSS_PARAMS*)pMechanism->pParameter)) {
	    crv = CKR_MECHANISM_PARAM_INVALID;
	    break;
	}
	info = PORT_New(SFTKHashSignInfo);
	if (info == NULL) {
	    crv = CKR_HOST_MEMORY;
	    break;
	}
	info->params = pMechanism->pParameter;
	info->key = sftk_GetPrivKey(key,CKK_RSA,&crv);
	if (info->key == NULL) {
	    PORT_Free(info);
	    break;
	}
	context->cipherInfo = info;
	context->destroy = (SFTKDestroy) sftk_Space;
	context->update = (SFTKCipher) sftk_RSASignPSS;
	context->maxLen = nsslowkey_PrivateModulusLen(info->key);
	break;	

    case CKM_DSA_SHA1:
        context->multi = PR_TRUE;
	crv = sftk_doSubSHA1(context);
	if (crv != CKR_OK) break;
	
    case CKM_DSA:
	if (key_type != CKK_DSA) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	privKey = sftk_GetPrivKey(key,CKK_DSA,&crv);
	if (privKey == NULL) {
	    break;
	}
	context->cipherInfo = privKey;
	context->update     = (SFTKCipher) nsc_DSA_Sign_Stub;
	context->destroy    = (privKey == key->objectInfo) ?
		(SFTKDestroy) sftk_Null:(SFTKDestroy)sftk_FreePrivKey;
	context->maxLen     = DSA_MAX_SIGNATURE_LEN;

	break;

#ifndef NSS_DISABLE_ECC
    case CKM_ECDSA_SHA1:
	context->multi = PR_TRUE;
	crv = sftk_doSubSHA1(context);
	if (crv != CKR_OK) break;
	
    case CKM_ECDSA:
	if (key_type != CKK_EC) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	privKey = sftk_GetPrivKey(key,CKK_EC,&crv);
	if (privKey == NULL) {
	    crv = CKR_HOST_MEMORY;
	    break;
	}
	context->cipherInfo = privKey;
	context->update     = (SFTKCipher) nsc_ECDSASignStub;
	context->destroy    = (privKey == key->objectInfo) ?
		(SFTKDestroy) sftk_Null:(SFTKDestroy)sftk_FreePrivKey;
	context->maxLen     = MAX_ECKEY_LEN * 2;

	break;
#endif 

#define INIT_HMAC_MECH(mmm) \
    case CKM_ ## mmm ## _HMAC_GENERAL: \
	crv = sftk_doHMACInit(context, HASH_Alg ## mmm ,key, \
				*(CK_ULONG *)pMechanism->pParameter); \
	break; \
    case CKM_ ## mmm ## _HMAC: \
	crv = sftk_doHMACInit(context, HASH_Alg ## mmm ,key, mmm ## _LENGTH); \
	break; 

    INIT_HMAC_MECH(MD2)
    INIT_HMAC_MECH(MD5)
    INIT_HMAC_MECH(SHA224)
    INIT_HMAC_MECH(SHA256)
    INIT_HMAC_MECH(SHA384)
    INIT_HMAC_MECH(SHA512)

    case CKM_SHA_1_HMAC_GENERAL:
	crv = sftk_doHMACInit(context,HASH_AlgSHA1,key,
				*(CK_ULONG *)pMechanism->pParameter);
	break;
    case CKM_SHA_1_HMAC:
	crv = sftk_doHMACInit(context,HASH_AlgSHA1,key,SHA1_LENGTH);
	break;

    case CKM_SSL3_MD5_MAC:
	crv = sftk_doSSLMACInit(context,SEC_OID_MD5,key,
					*(CK_ULONG *)pMechanism->pParameter);
	break;
    case CKM_SSL3_SHA1_MAC:
	crv = sftk_doSSLMACInit(context,SEC_OID_SHA1,key,
					*(CK_ULONG *)pMechanism->pParameter);
	break;
    case CKM_TLS_PRF_GENERAL:
	crv = sftk_TLSPRFInit(context, key, key_type, HASH_AlgNULL);
	break;
    case CKM_NSS_TLS_PRF_GENERAL_SHA256:
	crv = sftk_TLSPRFInit(context, key, key_type, HASH_AlgSHA256);
	break;

    case CKM_NSS_HMAC_CONSTANT_TIME: {
	sftk_MACConstantTimeCtx *ctx =
	    sftk_HMACConstantTime_New(pMechanism,key);
	CK_ULONG *intpointer;

	if (ctx == NULL) {
	    crv = CKR_ARGUMENTS_BAD;
	    break;
	}
	intpointer = PORT_New(CK_ULONG);
	if (intpointer == NULL) {
	    crv = CKR_HOST_MEMORY;
	    break;
	}
	*intpointer = ctx->hash->length;

	context->cipherInfo    = intpointer;
	context->hashInfo      = ctx;
	context->currentMech   = pMechanism->mechanism;
	context->hashUpdate    = sftk_HMACConstantTime_Update;
	context->hashdestroy   = sftk_MACConstantTime_DestroyContext;
	context->end           = sftk_MACConstantTime_EndHash;
	context->update        = (SFTKCipher) sftk_SignCopy;
	context->destroy       = sftk_Space;
	context->maxLen        = 64;
	context->multi         = PR_TRUE;
	break;
    }

    case CKM_NSS_SSL3_MAC_CONSTANT_TIME: {
	sftk_MACConstantTimeCtx *ctx =
	    sftk_SSLv3MACConstantTime_New(pMechanism,key);
	CK_ULONG *intpointer;

	if (ctx == NULL) {
	    crv = CKR_ARGUMENTS_BAD;
	    break;
	}
	intpointer = PORT_New(CK_ULONG);
	if (intpointer == NULL) {
	    crv = CKR_HOST_MEMORY;
	    break;
	}
	*intpointer = ctx->hash->length;

	context->cipherInfo    = intpointer;
	context->hashInfo      = ctx;
	context->currentMech   = pMechanism->mechanism;
	context->hashUpdate    = sftk_SSLv3MACConstantTime_Update;
	context->hashdestroy   = sftk_MACConstantTime_DestroyContext;
	context->end           = sftk_MACConstantTime_EndHash;
	context->update        = (SFTKCipher) sftk_SignCopy;
	context->destroy       = sftk_Space;
	context->maxLen        = 64;
	context->multi         = PR_TRUE;
	break;
    }

    default:
	crv = CKR_MECHANISM_INVALID;
	break;
    }

    if (crv != CKR_OK) {
	if (info) PORT_Free(info);
        sftk_FreeContext(context);
        sftk_FreeSession(session);
        return crv;
    }
    sftk_SetContextByType(session, SFTK_SIGN, context);
    sftk_FreeSession(session);
    return CKR_OK;
}



static CK_RV
sftk_MACBlock( SFTKSessionContext *ctx, void *blk )
{
    unsigned int outlen;
    return ( SECSuccess == (ctx->update)( ctx->cipherInfo, ctx->macBuf, &outlen,
                SFTK_MAX_BLOCK_SIZE, blk, ctx->blockSize ))
            ? CKR_OK : sftk_MapCryptError(PORT_GetError());
}





static CK_RV
sftk_MACFinal( SFTKSessionContext *ctx )
{
    unsigned int padLen = ctx->padDataLength;
    
    if( padLen ) {
        
        PORT_Memset( ctx->padBuf + padLen, 0, ctx->blockSize - padLen );
        return sftk_MACBlock( ctx, ctx->padBuf );
    } else
        return CKR_OK;
}






static CK_RV
sftk_MACUpdate(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pPart,
    					CK_ULONG ulPartLen,SFTKContextType type)
{
    SFTKSession *session;
    SFTKSessionContext *context;
    CK_RV crv;

    
    crv = sftk_GetContext(hSession,&context,type, PR_TRUE, &session );
    if (crv != CKR_OK) return crv;

    if (context->hashInfo) {
	(*context->hashUpdate)(context->hashInfo, pPart, ulPartLen);
    } else {   
    	

        unsigned int  blkSize   = context->blockSize;
        unsigned char *residual = 
                                context->padBuf + context->padDataLength;
        unsigned int  minInput  = 
                                blkSize - context->padDataLength;

        
        if( ulPartLen < minInput ) {
            PORT_Memcpy( residual, pPart, ulPartLen );
            context->padDataLength += ulPartLen;
            goto cleanup;
        }
        
        if( context->padDataLength ) {
            PORT_Memcpy( residual, pPart, minInput );
            ulPartLen -= minInput;
            pPart     += minInput;
            if( CKR_OK != (crv = sftk_MACBlock( context, context->padBuf )) )
                goto terminate;
        }
        
        while( ulPartLen >= blkSize )
        {
            if( CKR_OK != (crv = sftk_MACBlock( context, pPart )) )
                goto terminate;
            ulPartLen -= blkSize;
            pPart     += blkSize;
        }
        
        if( (context->padDataLength = ulPartLen) )
            PORT_Memcpy( context->padBuf, pPart, ulPartLen );
    } 

    goto  cleanup;

terminate:
    sftk_TerminateOp( session, type, context );
cleanup:
    sftk_FreeSession(session);
    return crv;
}







CK_RV NSC_SignUpdate(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pPart,
    							CK_ULONG ulPartLen)
{
    CHECK_FORK();
    return sftk_MACUpdate(hSession, pPart, ulPartLen, SFTK_SIGN);
}




CK_RV NSC_SignFinal(CK_SESSION_HANDLE hSession,CK_BYTE_PTR pSignature,
					    CK_ULONG_PTR pulSignatureLen)
{
    SFTKSession *session;
    SFTKSessionContext *context;
    unsigned int outlen;
    unsigned int maxoutlen = *pulSignatureLen;
    CK_RV crv;

    CHECK_FORK();

    
    crv = sftk_GetContext(hSession,&context,SFTK_SIGN,PR_TRUE,&session);
    if (crv != CKR_OK) return crv;

    if (context->hashInfo) {
        unsigned int digestLen;
        unsigned char tmpbuf[SFTK_MAX_MAC_LENGTH];

        if( !pSignature ) {
            outlen = context->maxLen; goto finish;
        }
        (*context->end)(context->hashInfo, tmpbuf, &digestLen, sizeof(tmpbuf));
        if( SECSuccess != (context->update)(context->cipherInfo, pSignature,
                    &outlen, maxoutlen, tmpbuf, digestLen))
            crv = sftk_MapCryptError(PORT_GetError());
        


    } else {
        
        outlen = context->macSize;
        
        if( !pSignature  ||  maxoutlen < outlen ) {
            if( pSignature ) crv = CKR_BUFFER_TOO_SMALL;
            goto finish;
        }
        if( CKR_OK == (crv = sftk_MACFinal( context )) )
	    PORT_Memcpy(pSignature, context->macBuf, outlen );
    }

    sftk_TerminateOp( session, SFTK_SIGN, context );
finish:
    *pulSignatureLen = outlen;
    sftk_FreeSession(session);
    return crv;
}




CK_RV NSC_Sign(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR pData,CK_ULONG ulDataLen,CK_BYTE_PTR pSignature,
    					CK_ULONG_PTR pulSignatureLen)
{
    SFTKSession *session;
    SFTKSessionContext *context;
    CK_RV crv;

    CHECK_FORK();

    
    crv = sftk_GetContext(hSession,&context,SFTK_SIGN,PR_FALSE,&session);
    if (crv != CKR_OK) return crv;

    if (!pSignature) {
        
	*pulSignatureLen = (!context->multi || context->hashInfo)
            ? context->maxLen
            : context->macSize; 
	goto finish;
    }

    

    if (context->multi) {
        
	if( CKR_OK == (crv = NSC_SignUpdate(hSession,pData,ulDataLen) ))
            crv = NSC_SignFinal(hSession, pSignature, pulSignatureLen);
    } else {   
    	
        unsigned int outlen;
        unsigned int maxoutlen = *pulSignatureLen;
        if( SECSuccess != (*context->update)(context->cipherInfo, pSignature,
                    &outlen, maxoutlen, pData, ulDataLen))
            crv = sftk_MapCryptError(PORT_GetError());
        *pulSignatureLen = (CK_ULONG) outlen;
        
        if( crv != CKR_BUFFER_TOO_SMALL )
            sftk_TerminateOp(session, SFTK_SIGN, context);
    } 

finish:
    sftk_FreeSession(session);
    return crv;
}








CK_RV NSC_SignRecoverInit(CK_SESSION_HANDLE hSession,
			 CK_MECHANISM_PTR pMechanism,CK_OBJECT_HANDLE hKey)
{
    CHECK_FORK();

    switch (pMechanism->mechanism) {
    case CKM_RSA_PKCS:
    case CKM_RSA_X_509:
	return NSC_SignInit(hSession,pMechanism,hKey);
    default:
	break;
    }
    return CKR_MECHANISM_INVALID;
}





CK_RV NSC_SignRecover(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData,
  CK_ULONG ulDataLen, CK_BYTE_PTR pSignature, CK_ULONG_PTR pulSignatureLen)
{
    CHECK_FORK();

    return NSC_Sign(hSession,pData,ulDataLen,pSignature,pulSignatureLen);
}






static SECStatus
sftk_hashCheckSign(SFTKHashVerifyInfo *info, const unsigned char *sig, 
                   unsigned int sigLen, const unsigned char *digest,
                   unsigned int digestLen)
{
    PORT_Assert(info->key->keyType == NSSLOWKEYRSAKey);
    if (info->key->keyType != NSSLOWKEYRSAKey) {
        PORT_SetError(SEC_ERROR_INVALID_KEY);
        return SECFailure;
    }

    return RSA_HashCheckSign(info->hashOid, info->key, sig, sigLen, digest,
                             digestLen);
}

SECStatus
RSA_HashCheckSign(SECOidTag digestOid, NSSLOWKEYPublicKey *key,
                  const unsigned char *sig, unsigned int sigLen,
                  const unsigned char *digestData, unsigned int digestLen)
{
    unsigned char *pkcs1DigestInfoData;
    SECItem pkcs1DigestInfo;
    SECItem digest;
    unsigned int bufferSize;
    SECStatus rv;

    
    bufferSize = key->u.rsa.modulus.len;
    pkcs1DigestInfoData = PORT_ZAlloc(bufferSize);
    if (!pkcs1DigestInfoData) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }

    pkcs1DigestInfo.data = pkcs1DigestInfoData;
    pkcs1DigestInfo.len = bufferSize;
    
    
    rv = RSA_CheckSignRecover(&key->u.rsa, pkcs1DigestInfo.data,
                             &pkcs1DigestInfo.len, pkcs1DigestInfo.len,
                             sig, sigLen);
    if (rv != SECSuccess) {
        PORT_SetError(SEC_ERROR_BAD_SIGNATURE);
    } else {
        digest.data = (PRUint8*) digestData;
        digest.len = digestLen;
        rv = _SGN_VerifyPKCS1DigestInfo(
                digestOid, &digest, &pkcs1DigestInfo,
                PR_TRUE );
    }

    PORT_Free(pkcs1DigestInfoData);
    return rv;
}

static SECStatus
sftk_RSACheckSign(NSSLOWKEYPublicKey *key, const unsigned char *sig,
                  unsigned int sigLen, const unsigned char *digest,
                  unsigned int digestLen)
{
    PORT_Assert(key->keyType == NSSLOWKEYRSAKey);
    if (key->keyType != NSSLOWKEYRSAKey) {
        PORT_SetError(SEC_ERROR_INVALID_KEY);
        return SECFailure;
    }

    return RSA_CheckSign(&key->u.rsa, sig, sigLen, digest, digestLen);
}

static SECStatus
sftk_RSACheckSignRaw(NSSLOWKEYPublicKey *key, const unsigned char *sig,
                     unsigned int sigLen, const unsigned char *digest,
                     unsigned int digestLen)
{
    PORT_Assert(key->keyType == NSSLOWKEYRSAKey);
    if (key->keyType != NSSLOWKEYRSAKey) {
        PORT_SetError(SEC_ERROR_INVALID_KEY);
        return SECFailure;
    }

    return RSA_CheckSignRaw(&key->u.rsa, sig, sigLen, digest, digestLen);
}

static SECStatus
sftk_RSACheckSignPSS(SFTKHashVerifyInfo *info, const unsigned char *sig,
                     unsigned int sigLen, const unsigned char *digest,
                     unsigned int digestLen)
{
    HASH_HashType hashAlg;
    HASH_HashType maskHashAlg;
    CK_RSA_PKCS_PSS_PARAMS *params = (CK_RSA_PKCS_PSS_PARAMS *)info->params;

    PORT_Assert(info->key->keyType == NSSLOWKEYRSAKey);
    if (info->key->keyType != NSSLOWKEYRSAKey) {
        PORT_SetError(SEC_ERROR_INVALID_KEY);
        return SECFailure;
    }

    hashAlg = GetHashTypeFromMechanism(params->hashAlg);
    maskHashAlg = GetHashTypeFromMechanism(params->mgf);

    return RSA_CheckSignPSS(&info->key->u.rsa, hashAlg, maskHashAlg,
                            params->sLen, sig, sigLen, digest, digestLen);
}




CK_RV NSC_VerifyInit(CK_SESSION_HANDLE hSession,
			   CK_MECHANISM_PTR pMechanism,CK_OBJECT_HANDLE hKey) 
{
    SFTKSession *session;
    SFTKObject *key;
    SFTKSessionContext *context;
    CK_KEY_TYPE key_type;
    CK_RV crv = CKR_OK;
    NSSLOWKEYPublicKey *pubKey;
    SFTKHashVerifyInfo *info = NULL;

    CHECK_FORK();

    
    crv = sftk_InitCBCMac(hSession, pMechanism, hKey, CKA_VERIFY, SFTK_VERIFY);
    if (crv != CKR_FUNCTION_NOT_SUPPORTED) return crv;

    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;
    crv = sftk_InitGeneric(session,&context,SFTK_VERIFY,&key,hKey,&key_type,
						CKO_PUBLIC_KEY,CKA_VERIFY);
    if (crv != CKR_OK) {
	sftk_FreeSession(session);
	return crv;
    }

    context->multi = PR_FALSE;

#define INIT_RSA_VFY_MECH(mmm) \
    case CKM_ ## mmm ## _RSA_PKCS: \
        context->multi = PR_TRUE; \
	crv = sftk_doSub ## mmm (context); \
	if (crv != CKR_OK) break; \
	context->verify = (SFTKVerify) sftk_hashCheckSign; \
	info = PORT_New(SFTKHashVerifyInfo); \
	if (info == NULL) { crv = CKR_HOST_MEMORY; break; } \
	info->hashOid = SEC_OID_ ## mmm ; \
	goto finish_rsa; 

    switch(pMechanism->mechanism) {
    INIT_RSA_VFY_MECH(MD5) 
    INIT_RSA_VFY_MECH(MD2) 
    INIT_RSA_VFY_MECH(SHA1) 
    INIT_RSA_VFY_MECH(SHA224)
    INIT_RSA_VFY_MECH(SHA256) 
    INIT_RSA_VFY_MECH(SHA384) 
    INIT_RSA_VFY_MECH(SHA512) 

    case CKM_RSA_PKCS:
	context->verify = (SFTKVerify) sftk_RSACheckSign;
	goto finish_rsa;
    case CKM_RSA_X_509:
	context->verify = (SFTKVerify) sftk_RSACheckSignRaw;
finish_rsa:
	if (key_type != CKK_RSA) {
	    if (info) PORT_Free(info);
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	context->rsa = PR_TRUE;
	pubKey = sftk_GetPubKey(key,CKK_RSA,&crv);
	if (pubKey == NULL) {
	    if (info) PORT_Free(info);
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	if (info) {
	    info->key = pubKey;
	    context->cipherInfo = info;
	    context->destroy = sftk_Space;
	} else {
	    context->cipherInfo = pubKey;
	    context->destroy = sftk_Null;
	}
	break;
    case CKM_RSA_PKCS_PSS:
	if (key_type != CKK_RSA) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	} 
	context->rsa = PR_TRUE;
	if (pMechanism->ulParameterLen != sizeof(CK_RSA_PKCS_PSS_PARAMS) ||
	    !sftk_ValidatePssParams((const CK_RSA_PKCS_PSS_PARAMS*)pMechanism->pParameter)) {
	    crv = CKR_MECHANISM_PARAM_INVALID;
	    break;
	}
	info = PORT_New(SFTKHashVerifyInfo);
	if (info == NULL) {
	    crv = CKR_HOST_MEMORY;
	    break;
	}
	info->params = pMechanism->pParameter;
	info->key = sftk_GetPubKey(key,CKK_RSA,&crv);
	if (info->key == NULL) {
	    PORT_Free(info);
	    break;
	}
	context->cipherInfo = info;
	context->destroy = (SFTKDestroy) sftk_Space;
	context->verify = (SFTKVerify) sftk_RSACheckSignPSS;
	break;
    case CKM_DSA_SHA1:
        context->multi = PR_TRUE;
	crv = sftk_doSubSHA1(context);
	if (crv != CKR_OK) break;
	
    case CKM_DSA:
	if (key_type != CKK_DSA) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	pubKey = sftk_GetPubKey(key,CKK_DSA,&crv);
	if (pubKey == NULL) {
	    break;
	}
	context->cipherInfo = pubKey;
	context->verify     = (SFTKVerify) nsc_DSA_Verify_Stub;
	context->destroy    = sftk_Null;
	break;
#ifndef NSS_DISABLE_ECC
    case CKM_ECDSA_SHA1:
	context->multi = PR_TRUE;
	crv = sftk_doSubSHA1(context);
	if (crv != CKR_OK) break;
	
    case CKM_ECDSA:
	if (key_type != CKK_EC) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	pubKey = sftk_GetPubKey(key,CKK_EC,&crv);
	if (pubKey == NULL) {
	    crv = CKR_HOST_MEMORY;
	    break;
	}
	context->cipherInfo = pubKey;
	context->verify     = (SFTKVerify) nsc_ECDSAVerifyStub;
	context->destroy    = sftk_Null;
	break;
#endif 

    INIT_HMAC_MECH(MD2)
    INIT_HMAC_MECH(MD5)
    INIT_HMAC_MECH(SHA224)
    INIT_HMAC_MECH(SHA256)
    INIT_HMAC_MECH(SHA384)
    INIT_HMAC_MECH(SHA512)

    case CKM_SHA_1_HMAC_GENERAL:
	crv = sftk_doHMACInit(context,HASH_AlgSHA1,key,
				*(CK_ULONG *)pMechanism->pParameter);
	break;
    case CKM_SHA_1_HMAC:
	crv = sftk_doHMACInit(context,HASH_AlgSHA1,key,SHA1_LENGTH);
	break;

    case CKM_SSL3_MD5_MAC:
	crv = sftk_doSSLMACInit(context,SEC_OID_MD5,key,
					*(CK_ULONG *)pMechanism->pParameter);
	break;
    case CKM_SSL3_SHA1_MAC:
	crv = sftk_doSSLMACInit(context,SEC_OID_SHA1,key,
					*(CK_ULONG *)pMechanism->pParameter);
	break;
    case CKM_TLS_PRF_GENERAL:
	crv = sftk_TLSPRFInit(context, key, key_type, HASH_AlgNULL);
	break;
    case CKM_NSS_TLS_PRF_GENERAL_SHA256:
	crv = sftk_TLSPRFInit(context, key, key_type, HASH_AlgSHA256);
	break;

    default:
	crv = CKR_MECHANISM_INVALID;
	break;
    }

    if (crv != CKR_OK) {
	if (info) PORT_Free(info);
        sftk_FreeContext(context);
	sftk_FreeSession(session);
	return crv;
    }
    sftk_SetContextByType(session, SFTK_VERIFY, context);
    sftk_FreeSession(session);
    return CKR_OK;
}




CK_RV NSC_Verify(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pData,
    CK_ULONG ulDataLen, CK_BYTE_PTR pSignature, CK_ULONG ulSignatureLen)
{
    SFTKSession *session;
    SFTKSessionContext *context;
    CK_RV crv;

    CHECK_FORK();

    
    crv = sftk_GetContext(hSession,&context,SFTK_VERIFY,PR_FALSE,&session);
    if (crv != CKR_OK) return crv;

    

    if (context->multi) {
        
	if( CKR_OK == (crv = NSC_VerifyUpdate(hSession, pData, ulDataLen)))
            crv = NSC_VerifyFinal(hSession, pSignature, ulSignatureLen);
    } else {
        if (SECSuccess != (*context->verify)(context->cipherInfo,pSignature,
                                             ulSignatureLen, pData, ulDataLen))
            crv = sftk_MapCryptError(PORT_GetError());

        sftk_TerminateOp( session, SFTK_VERIFY, context );
    }
    sftk_FreeSession(session);
    return crv;
}








CK_RV NSC_VerifyUpdate( CK_SESSION_HANDLE hSession, CK_BYTE_PTR pPart,
						CK_ULONG ulPartLen)
{
    CHECK_FORK();
    return sftk_MACUpdate(hSession, pPart, ulPartLen, SFTK_VERIFY);
}




CK_RV NSC_VerifyFinal(CK_SESSION_HANDLE hSession,
			CK_BYTE_PTR pSignature,CK_ULONG ulSignatureLen)
{
    SFTKSession *session;
    SFTKSessionContext *context;
    CK_RV crv;

    CHECK_FORK();

    if (!pSignature) 
    	return CKR_ARGUMENTS_BAD;

    
    crv = sftk_GetContext(hSession,&context,SFTK_VERIFY,PR_TRUE,&session);
    if (crv != CKR_OK) 
    	return crv;
    
    if (context->hashInfo) {
        unsigned int digestLen;
        unsigned char tmpbuf[SFTK_MAX_MAC_LENGTH];
        
        (*context->end)(context->hashInfo, tmpbuf, &digestLen, sizeof(tmpbuf));
        if( SECSuccess != (context->verify)(context->cipherInfo, pSignature,
                                            ulSignatureLen, tmpbuf, digestLen))
            crv = sftk_MapCryptError(PORT_GetError());
    } else if (ulSignatureLen != context->macSize) {
	
	crv = CKR_SIGNATURE_LEN_RANGE;
    } else if (CKR_OK == (crv = sftk_MACFinal(context))) {
	if (PORT_Memcmp(pSignature, context->macBuf, ulSignatureLen))
	    crv = CKR_SIGNATURE_INVALID;
    }

    sftk_TerminateOp( session, SFTK_VERIFY, context );
    sftk_FreeSession(session);
    return crv;

}




static SECStatus
sftk_RSACheckSignRecover(NSSLOWKEYPublicKey *key, unsigned char *data,
                         unsigned int *dataLen, unsigned int maxDataLen,
                         const unsigned char *sig, unsigned int sigLen)
{
    PORT_Assert(key->keyType == NSSLOWKEYRSAKey);
    if (key->keyType != NSSLOWKEYRSAKey) {
        PORT_SetError(SEC_ERROR_INVALID_KEY);
        return SECFailure;
    }

    return RSA_CheckSignRecover(&key->u.rsa, data, dataLen, maxDataLen,
                                sig, sigLen);
}

static SECStatus
sftk_RSACheckSignRecoverRaw(NSSLOWKEYPublicKey *key, unsigned char *data,
                            unsigned int *dataLen, unsigned int maxDataLen,
                            const unsigned char *sig, unsigned int sigLen)
{
    PORT_Assert(key->keyType == NSSLOWKEYRSAKey);
    if (key->keyType != NSSLOWKEYRSAKey) {
        PORT_SetError(SEC_ERROR_INVALID_KEY);
        return SECFailure;
    }

    return RSA_CheckSignRecoverRaw(&key->u.rsa, data, dataLen, maxDataLen,
                                   sig, sigLen);
}




CK_RV NSC_VerifyRecoverInit(CK_SESSION_HANDLE hSession,
			CK_MECHANISM_PTR pMechanism,CK_OBJECT_HANDLE hKey)
{
    SFTKSession *session;
    SFTKObject *key;
    SFTKSessionContext *context;
    CK_KEY_TYPE key_type;
    CK_RV crv = CKR_OK;
    NSSLOWKEYPublicKey *pubKey;

    CHECK_FORK();

    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;
    crv = sftk_InitGeneric(session,&context,SFTK_VERIFY_RECOVER,
			&key,hKey,&key_type,CKO_PUBLIC_KEY,CKA_VERIFY_RECOVER);
    if (crv != CKR_OK) {
	sftk_FreeSession(session);
	return crv;
    }

    context->multi = PR_TRUE;

    switch(pMechanism->mechanism) {
    case CKM_RSA_PKCS:
    case CKM_RSA_X_509:
	if (key_type != CKK_RSA) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	context->multi = PR_FALSE;
	context->rsa = PR_TRUE;
	pubKey = sftk_GetPubKey(key,CKK_RSA,&crv);
	if (pubKey == NULL) {
	    break;
	}
	context->cipherInfo = pubKey;
	context->update = (SFTKCipher) (pMechanism->mechanism == CKM_RSA_X_509
			? sftk_RSACheckSignRecoverRaw : sftk_RSACheckSignRecover);
	context->destroy = sftk_Null;
	break;
    default:
	crv = CKR_MECHANISM_INVALID;
	break;
    }

    if (crv != CKR_OK) {
        PORT_Free(context);
	sftk_FreeSession(session);
	return crv;
    }
    sftk_SetContextByType(session, SFTK_VERIFY_RECOVER, context);
    sftk_FreeSession(session);
    return CKR_OK;
}





CK_RV NSC_VerifyRecover(CK_SESSION_HANDLE hSession,
		 CK_BYTE_PTR pSignature,CK_ULONG ulSignatureLen,
    				CK_BYTE_PTR pData,CK_ULONG_PTR pulDataLen)
{
    SFTKSession *session;
    SFTKSessionContext *context;
    unsigned int outlen;
    unsigned int maxoutlen = *pulDataLen;
    CK_RV crv;
    SECStatus rv;

    CHECK_FORK();

    
    crv = sftk_GetContext(hSession,&context,SFTK_VERIFY_RECOVER,
							PR_FALSE,&session);
    if (crv != CKR_OK) return crv;
    if (pData == NULL) {
	

	*pulDataLen = ulSignatureLen;
	rv = SECSuccess;
	goto finish;
    }

    rv = (*context->update)(context->cipherInfo, pData, &outlen, maxoutlen, 
						pSignature, ulSignatureLen);
    *pulDataLen = (CK_ULONG) outlen;

    sftk_TerminateOp(session, SFTK_VERIFY_RECOVER, context);
finish:
    sftk_FreeSession(session);
    return (rv == SECSuccess)  ? CKR_OK : sftk_MapVerifyError(PORT_GetError());
}







CK_RV NSC_SeedRandom(CK_SESSION_HANDLE hSession, CK_BYTE_PTR pSeed,
    CK_ULONG ulSeedLen) 
{
    SECStatus rv;

    CHECK_FORK();

    rv = RNG_RandomUpdate(pSeed, ulSeedLen);
    return (rv == SECSuccess) ? CKR_OK : sftk_MapCryptError(PORT_GetError());
}


CK_RV NSC_GenerateRandom(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR	pRandomData, CK_ULONG ulRandomLen)
{
    SECStatus rv;

    CHECK_FORK();

    rv = RNG_GenerateGlobalRandomBytes(pRandomData, ulRandomLen);
    



    return (rv == SECSuccess) ? CKR_OK : sftk_MapCryptError(PORT_GetError());
}










static CK_RV
nsc_pbe_key_gen(NSSPKCS5PBEParameter *pkcs5_pbe, CK_MECHANISM_PTR pMechanism,
			void *buf, CK_ULONG *key_length, PRBool faulty3DES)
{
    SECItem *pbe_key = NULL, iv, pwitem;
    CK_PBE_PARAMS *pbe_params = NULL;
    CK_PKCS5_PBKD2_PARAMS *pbkd2_params = NULL;

    *key_length = 0;
    iv.data = NULL; iv.len = 0;

    if (pMechanism->mechanism == CKM_PKCS5_PBKD2) {
	pbkd2_params = (CK_PKCS5_PBKD2_PARAMS *)pMechanism->pParameter;
	pwitem.data = (unsigned char *)pbkd2_params->pPassword;
	
	pwitem.len = *pbkd2_params->ulPasswordLen;
    } else {
	pbe_params = (CK_PBE_PARAMS *)pMechanism->pParameter;
	pwitem.data = (unsigned char *)pbe_params->pPassword;
	pwitem.len = pbe_params->ulPasswordLen;
    }
    pbe_key = nsspkcs5_ComputeKeyAndIV(pkcs5_pbe, &pwitem, &iv, faulty3DES);
    if (pbe_key == NULL) {
	return CKR_HOST_MEMORY;
    }

    PORT_Memcpy(buf, pbe_key->data, pbe_key->len);
    *key_length = pbe_key->len;
    SECITEM_ZfreeItem(pbe_key, PR_TRUE);
    pbe_key = NULL;

    if (iv.data) {
        if (pbe_params && pbe_params->pInitVector != NULL) {
	    PORT_Memcpy(pbe_params->pInitVector, iv.data, iv.len);
        }
        PORT_Free(iv.data);
    }

    return CKR_OK;
}





static unsigned int
sftk_GetSubPrimeFromPrime(unsigned int primeBits)
{
   if (primeBits <= 1024) {
	return 160;
   } else if (primeBits <= 2048) {
	return 224;
   } else if (primeBits <= 3072) {
	return 256;
   } else if (primeBits <= 7680) {
	return 384;
   } else {
	return 512;
   }
}

static CK_RV
nsc_parameter_gen(CK_KEY_TYPE key_type, SFTKObject *key)
{
    SFTKAttribute *attribute;
    CK_ULONG counter;
    unsigned int seedBits = 0;
    unsigned int subprimeBits = 0;
    unsigned int primeBits;
    unsigned int j = 8; 
    CK_RV crv = CKR_OK;
    PQGParams *params = NULL;
    PQGVerify *vfy = NULL;
    SECStatus rv;

    attribute = sftk_FindAttribute(key, CKA_PRIME_BITS);
    if (attribute == NULL) {
	return CKR_TEMPLATE_INCOMPLETE;
    }
    primeBits = (unsigned int) *(CK_ULONG *)attribute->attrib.pValue;
    sftk_FreeAttribute(attribute);
    if (primeBits < 1024) {
	j = PQG_PBITS_TO_INDEX(primeBits);
	if (j == (unsigned int)-1) {
	    return CKR_ATTRIBUTE_VALUE_INVALID;
	}
    }

    attribute = sftk_FindAttribute(key, CKA_NETSCAPE_PQG_SEED_BITS);
    if (attribute != NULL) {
	seedBits = (unsigned int) *(CK_ULONG *)attribute->attrib.pValue;
	sftk_FreeAttribute(attribute);
    }

    attribute = sftk_FindAttribute(key, CKA_SUBPRIME_BITS);
    if (attribute != NULL) {
	subprimeBits = (unsigned int) *(CK_ULONG *)attribute->attrib.pValue;
	sftk_FreeAttribute(attribute);
    }

    sftk_DeleteAttributeType(key,CKA_PRIME_BITS);
    sftk_DeleteAttributeType(key,CKA_SUBPRIME_BITS);
    sftk_DeleteAttributeType(key,CKA_NETSCAPE_PQG_SEED_BITS);

    
    if ((primeBits < 1024) || ((primeBits == 1024) && (subprimeBits == 0))) {
	if (seedBits == 0) {
	    rv = PQG_ParamGen(j, &params, &vfy);
	} else {
	    rv = PQG_ParamGenSeedLen(j,seedBits/8, &params, &vfy);
	}
    } else {
	if (subprimeBits == 0) {
	    subprimeBits = sftk_GetSubPrimeFromPrime(primeBits);
        }
	if (seedBits == 0) {
	    seedBits = primeBits;
	}
	rv = PQG_ParamGenV2(primeBits, subprimeBits, seedBits/8, &params, &vfy);
    }
	


    if (rv != SECSuccess) {
	if (PORT_GetError() == SEC_ERROR_LIBRARY_FAILURE) {
	    sftk_fatalError = PR_TRUE;
	}
	return sftk_MapCryptError(PORT_GetError());
    }
    crv = sftk_AddAttributeType(key,CKA_PRIME,
				 params->prime.data, params->prime.len);
    if (crv != CKR_OK) goto loser;
    crv = sftk_AddAttributeType(key,CKA_SUBPRIME,
				 params->subPrime.data, params->subPrime.len);
    if (crv != CKR_OK) goto loser;
    crv = sftk_AddAttributeType(key,CKA_BASE,
				 params->base.data, params->base.len);
    if (crv != CKR_OK) goto loser;
    counter = vfy->counter;
    crv = sftk_AddAttributeType(key,CKA_NETSCAPE_PQG_COUNTER,
				 &counter, sizeof(counter));
    crv = sftk_AddAttributeType(key,CKA_NETSCAPE_PQG_SEED,
				 vfy->seed.data, vfy->seed.len);
    if (crv != CKR_OK) goto loser;
    crv = sftk_AddAttributeType(key,CKA_NETSCAPE_PQG_H,
				 vfy->h.data, vfy->h.len);
    if (crv != CKR_OK) goto loser;

loser:
    PQG_DestroyParams(params);

    if (vfy) {
	PQG_DestroyVerify(vfy);
    }
    return crv;
}


static CK_RV
nsc_SetupBulkKeyGen(CK_MECHANISM_TYPE mechanism, CK_KEY_TYPE *key_type,
							CK_ULONG *key_length)
{
    CK_RV crv = CKR_OK;

    switch (mechanism) {
    case CKM_RC2_KEY_GEN:
	*key_type = CKK_RC2;
	if (*key_length == 0) crv = CKR_TEMPLATE_INCOMPLETE;
	break;
#if NSS_SOFTOKEN_DOES_RC5
    case CKM_RC5_KEY_GEN:
	*key_type = CKK_RC5;
	if (*key_length == 0) crv = CKR_TEMPLATE_INCOMPLETE;
	break;
#endif
    case CKM_RC4_KEY_GEN:
	*key_type = CKK_RC4;
	if (*key_length == 0) crv = CKR_TEMPLATE_INCOMPLETE;
	break;
    case CKM_GENERIC_SECRET_KEY_GEN:
	*key_type = CKK_GENERIC_SECRET;
	if (*key_length == 0) crv = CKR_TEMPLATE_INCOMPLETE;
	break;
    case CKM_CDMF_KEY_GEN:
	*key_type = CKK_CDMF;
	*key_length = 8;
	break;
    case CKM_DES_KEY_GEN:
	*key_type = CKK_DES;
	*key_length = 8;
	break;
    case CKM_DES2_KEY_GEN:
	*key_type = CKK_DES2;
	*key_length = 16;
	break;
    case CKM_DES3_KEY_GEN:
	*key_type = CKK_DES3;
	*key_length = 24;
	break;
    case CKM_SEED_KEY_GEN:
	*key_type = CKK_SEED;
	*key_length = 16;
	break;
    case CKM_CAMELLIA_KEY_GEN:
	*key_type = CKK_CAMELLIA;
	if (*key_length == 0) crv = CKR_TEMPLATE_INCOMPLETE;
	break;
    case CKM_AES_KEY_GEN:
	*key_type = CKK_AES;
	if (*key_length == 0) crv = CKR_TEMPLATE_INCOMPLETE;
	break;
    default:
	PORT_Assert(0);
	crv = CKR_MECHANISM_INVALID;
	break;
    }

    return crv;
}

CK_RV
nsc_SetupHMACKeyGen(CK_MECHANISM_PTR pMechanism, NSSPKCS5PBEParameter **pbe)
{
    SECItem  salt;
    CK_PBE_PARAMS *pbe_params = NULL;
    NSSPKCS5PBEParameter *params;
    PLArenaPool *arena = NULL;
    SECStatus rv;

    *pbe = NULL;

    arena = PORT_NewArena(SEC_ASN1_DEFAULT_ARENA_SIZE);
    if (arena == NULL) {
	return CKR_HOST_MEMORY;
    }

    params = (NSSPKCS5PBEParameter *) PORT_ArenaZAlloc(arena,
				sizeof(NSSPKCS5PBEParameter));
    if (params == NULL) {
	PORT_FreeArena(arena,PR_TRUE);
	return CKR_HOST_MEMORY;
    }

    params->poolp = arena;
    params->ivLen = 0;
    params->pbeType = NSSPKCS5_PKCS12_V2;
    params->hashType = HASH_AlgSHA1;
    params->encAlg = SEC_OID_SHA1; 
    params->is2KeyDES = PR_FALSE;
    params->keyID = pbeBitGenIntegrityKey;
    pbe_params = (CK_PBE_PARAMS *)pMechanism->pParameter;
    params->iter = pbe_params->ulIteration;

    salt.data = (unsigned char *)pbe_params->pSalt;
    salt.len = (unsigned int)pbe_params->ulSaltLen;
    rv = SECITEM_CopyItem(arena,&params->salt,&salt);
    if (rv != SECSuccess) {
	PORT_FreeArena(arena,PR_TRUE);
	return CKR_HOST_MEMORY;
    }
    switch (pMechanism->mechanism) {
    case CKM_NETSCAPE_PBE_SHA1_HMAC_KEY_GEN:
    case CKM_PBA_SHA1_WITH_SHA1_HMAC:
	params->hashType = HASH_AlgSHA1; 
	params->keyLen = 20;
	break;
    case CKM_NETSCAPE_PBE_MD5_HMAC_KEY_GEN:
	params->hashType = HASH_AlgMD5; 
	params->keyLen = 16;
	break;
    case CKM_NETSCAPE_PBE_MD2_HMAC_KEY_GEN:
	params->hashType = HASH_AlgMD2; 
	params->keyLen = 16;
	break;
    default:
	PORT_FreeArena(arena,PR_TRUE);
	return CKR_MECHANISM_INVALID;
    }
    *pbe = params;
    return CKR_OK;
}


static CK_RV
nsc_SetupPBEKeyGen(CK_MECHANISM_PTR pMechanism, NSSPKCS5PBEParameter  **pbe,
				CK_KEY_TYPE *key_type, CK_ULONG *key_length)
{
    CK_RV crv = CKR_OK;
    SECOidData *oid;
    CK_PBE_PARAMS *pbe_params = NULL;
    NSSPKCS5PBEParameter *params = NULL;
    CK_PKCS5_PBKD2_PARAMS *pbkd2_params = NULL;
    SECItem salt;
    CK_ULONG iteration = 0;

    *pbe = NULL;

    oid = SECOID_FindOIDByMechanism(pMechanism->mechanism);
    if (oid == NULL) {
	return CKR_MECHANISM_INVALID;
    }

    if (pMechanism->mechanism == CKM_PKCS5_PBKD2) {
	pbkd2_params = (CK_PKCS5_PBKD2_PARAMS *)pMechanism->pParameter;
	if (pbkd2_params->saltSource != CKZ_SALT_SPECIFIED) {
	    return CKR_MECHANISM_PARAM_INVALID;
	}
	salt.data = (unsigned char *)pbkd2_params->pSaltSourceData;
	salt.len = (unsigned int)pbkd2_params->ulSaltSourceDataLen;
	iteration = pbkd2_params->iterations;
    } else {
	pbe_params = (CK_PBE_PARAMS *)pMechanism->pParameter;
	salt.data = (unsigned char *)pbe_params->pSalt;
	salt.len = (unsigned int)pbe_params->ulSaltLen;
	iteration = pbe_params->ulIteration;
    }
    params=nsspkcs5_NewParam(oid->offset, &salt, iteration);
    if (params == NULL) {
	return CKR_MECHANISM_INVALID;
    }

    switch (params->encAlg) {
    case SEC_OID_DES_CBC:
	*key_type = CKK_DES;
	*key_length = params->keyLen;
	break;
    case SEC_OID_DES_EDE3_CBC:
	*key_type = params->is2KeyDES ? CKK_DES2 : CKK_DES3;
	*key_length = params->keyLen;
	break;
    case SEC_OID_RC2_CBC:
	*key_type = CKK_RC2;
	*key_length = params->keyLen;
	break;
    case SEC_OID_RC4:
	*key_type = CKK_RC4;
	*key_length = params->keyLen;
	break;
    case SEC_OID_PKCS5_PBKDF2:
	


	if (pbkd2_params == NULL || 
		pbkd2_params->prf != CKP_PKCS5_PBKD2_HMAC_SHA1) {
	    crv = CKR_MECHANISM_PARAM_INVALID;
	    break;
	}
	
	if (*key_type == CKK_INVALID_KEY_TYPE) {
	    crv = CKR_TEMPLATE_INCOMPLETE;
	    break;
	}
	

	if (*key_length == 0) {
	    *key_length = sftk_MapKeySize(*key_type);
	}
	if (*key_length == 0) {
	    crv = CKR_TEMPLATE_INCOMPLETE;
	    break;
	}
	params->keyLen = *key_length;
	break;
    default:
	crv = CKR_MECHANISM_INVALID;
	nsspkcs5_DestroyPBEParameter(params);
	break;
    }
    if (crv == CKR_OK) {
    	*pbe = params;
    }
    return crv;
}


CK_RV NSC_GenerateKey(CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism,CK_ATTRIBUTE_PTR pTemplate,CK_ULONG ulCount,
    						CK_OBJECT_HANDLE_PTR phKey)
{
    SFTKObject *key;
    SFTKSession *session;
    PRBool checkWeak = PR_FALSE;
    CK_ULONG key_length = 0;
    CK_KEY_TYPE key_type = CKK_INVALID_KEY_TYPE;
    CK_OBJECT_CLASS objclass = CKO_SECRET_KEY;
    CK_RV crv = CKR_OK;
    CK_BBOOL cktrue = CK_TRUE;
    int i;
    SFTKSlot *slot = sftk_SlotFromSessionHandle(hSession);
    unsigned char buf[MAX_KEY_LEN];
    enum {nsc_pbe, nsc_ssl, nsc_bulk, nsc_param, nsc_jpake} key_gen_type;
    NSSPKCS5PBEParameter *pbe_param;
    SSL3RSAPreMasterSecret *rsa_pms;
    CK_VERSION *version;
    



    PRBool faultyPBE3DES = PR_FALSE;
    HASH_HashType hashType;

    CHECK_FORK();

    if (!slot) {
        return CKR_SESSION_HANDLE_INVALID;
    }
    


    key = sftk_NewObject(slot); 
    if (key == NULL) {
	return CKR_HOST_MEMORY;
    }

    


    for (i=0; i < (int) ulCount; i++) {
	if (pTemplate[i].type == CKA_VALUE_LEN) {
	    key_length = *(CK_ULONG *)pTemplate[i].pValue;
	    continue;
	}
	
	if (pTemplate[i].type == CKA_KEY_TYPE) {
	    key_type = *(CK_ULONG *)pTemplate[i].pValue;
	    continue;
	}

	crv = sftk_AddAttributeType(key,sftk_attr_expand(&pTemplate[i]));
	if (crv != CKR_OK) break;
    }
    if (crv != CKR_OK) {
	sftk_FreeObject(key);
	return crv;
    }

    
    sftk_DeleteAttributeType(key,CKA_CLASS);
    sftk_DeleteAttributeType(key,CKA_KEY_TYPE);
    sftk_DeleteAttributeType(key,CKA_VALUE);

    
    key_gen_type = nsc_bulk; 
    switch (pMechanism->mechanism) {
    case CKM_CDMF_KEY_GEN:
    case CKM_DES_KEY_GEN:
    case CKM_DES2_KEY_GEN:
    case CKM_DES3_KEY_GEN:
	checkWeak = PR_TRUE;
        
    case CKM_RC2_KEY_GEN:
    case CKM_RC4_KEY_GEN:
    case CKM_GENERIC_SECRET_KEY_GEN:
    case CKM_SEED_KEY_GEN:
    case CKM_CAMELLIA_KEY_GEN:
    case CKM_AES_KEY_GEN:
#if NSS_SOFTOKEN_DOES_RC5
    case CKM_RC5_KEY_GEN:
#endif
	crv = nsc_SetupBulkKeyGen(pMechanism->mechanism,&key_type,&key_length);
	break;
    case CKM_SSL3_PRE_MASTER_KEY_GEN:
	key_type = CKK_GENERIC_SECRET;
	key_length = 48;
	key_gen_type = nsc_ssl;
	break;
    case CKM_PBA_SHA1_WITH_SHA1_HMAC:
    case CKM_NETSCAPE_PBE_SHA1_HMAC_KEY_GEN:
    case CKM_NETSCAPE_PBE_MD5_HMAC_KEY_GEN:
    case CKM_NETSCAPE_PBE_MD2_HMAC_KEY_GEN:
	key_gen_type = nsc_pbe;
	key_type = CKK_GENERIC_SECRET;
	crv = nsc_SetupHMACKeyGen(pMechanism, &pbe_param);
	break;
    case CKM_NETSCAPE_PBE_SHA1_FAULTY_3DES_CBC:
	faultyPBE3DES = PR_TRUE;
        
    case CKM_NETSCAPE_PBE_SHA1_TRIPLE_DES_CBC:
    case CKM_NETSCAPE_PBE_SHA1_40_BIT_RC2_CBC:
    case CKM_NETSCAPE_PBE_SHA1_DES_CBC:
    case CKM_NETSCAPE_PBE_SHA1_128_BIT_RC2_CBC:
    case CKM_NETSCAPE_PBE_SHA1_40_BIT_RC4:
    case CKM_NETSCAPE_PBE_SHA1_128_BIT_RC4:
    case CKM_PBE_SHA1_DES3_EDE_CBC:
    case CKM_PBE_SHA1_DES2_EDE_CBC:
    case CKM_PBE_SHA1_RC2_128_CBC:
    case CKM_PBE_SHA1_RC2_40_CBC:
    case CKM_PBE_SHA1_RC4_128:
    case CKM_PBE_SHA1_RC4_40:
    case CKM_PBE_MD5_DES_CBC:
    case CKM_PBE_MD2_DES_CBC:
    case CKM_PKCS5_PBKD2:
	key_gen_type = nsc_pbe;
	crv = nsc_SetupPBEKeyGen(pMechanism,&pbe_param, &key_type, &key_length);
	break;
    case CKM_DSA_PARAMETER_GEN:
	key_gen_type = nsc_param;
	key_type = CKK_DSA;
	objclass = CKO_KG_PARAMETERS;
	crv = CKR_OK;
	break;
    case CKM_NSS_JPAKE_ROUND1_SHA1:   hashType = HASH_AlgSHA1;   goto jpake1;
    case CKM_NSS_JPAKE_ROUND1_SHA256: hashType = HASH_AlgSHA256; goto jpake1;
    case CKM_NSS_JPAKE_ROUND1_SHA384: hashType = HASH_AlgSHA384; goto jpake1;
    case CKM_NSS_JPAKE_ROUND1_SHA512: hashType = HASH_AlgSHA512; goto jpake1;
jpake1:
	key_gen_type = nsc_jpake;
	key_type = CKK_NSS_JPAKE_ROUND1;
        objclass = CKO_PRIVATE_KEY;
        if (pMechanism->pParameter == NULL ||
            pMechanism->ulParameterLen != sizeof(CK_NSS_JPAKERound1Params)) {
            crv = CKR_MECHANISM_PARAM_INVALID;
            break;
        }
        if (sftk_isTrue(key, CKA_TOKEN)) {
            crv = CKR_TEMPLATE_INCONSISTENT;
            break;
        }
        crv = CKR_OK;
        break;
    default:
	crv = CKR_MECHANISM_INVALID;
	break;
    }

    
    if (sizeof(buf) < key_length) {
	

        crv = CKR_TEMPLATE_INCONSISTENT;
    }

    if (crv != CKR_OK) { sftk_FreeObject(key); return crv; }

    

    PORT_Assert( key_type != CKK_INVALID_KEY_TYPE );

    


    switch (key_gen_type) {
    case nsc_pbe:
	crv = nsc_pbe_key_gen(pbe_param, pMechanism, buf, &key_length,
			       faultyPBE3DES);
	nsspkcs5_DestroyPBEParameter(pbe_param);
	break;
    case nsc_ssl:
	rsa_pms = (SSL3RSAPreMasterSecret *)buf;
	version = (CK_VERSION *)pMechanism->pParameter;
	rsa_pms->client_version[0] = version->major;
        rsa_pms->client_version[1] = version->minor;
        crv = 
	    NSC_GenerateRandom(0,&rsa_pms->random[0], sizeof(rsa_pms->random));
	break;
    case nsc_bulk:
	
	do {
            crv = NSC_GenerateRandom(0, buf, key_length);
	} while (crv == CKR_OK && checkWeak && sftk_IsWeakKey(buf,key_type));
	break;
    case nsc_param:
	
	*buf = 0;
	crv = nsc_parameter_gen(key_type,key);
	break;
    case nsc_jpake:
        crv = jpake_Round1(hashType,
                           (CK_NSS_JPAKERound1Params *) pMechanism->pParameter,
                           key);
        break;
    }

    if (crv != CKR_OK) { sftk_FreeObject(key); return crv; }

    
    crv = sftk_AddAttributeType(key,CKA_CLASS,&objclass,sizeof(CK_OBJECT_CLASS));
    if (crv != CKR_OK) { sftk_FreeObject(key); return crv; }
    crv = sftk_AddAttributeType(key,CKA_KEY_TYPE,&key_type,sizeof(CK_KEY_TYPE));
    if (crv != CKR_OK) { sftk_FreeObject(key); return crv; }
    if (key_length != 0) {
	crv = sftk_AddAttributeType(key,CKA_VALUE,buf,key_length);
	if (crv != CKR_OK) { sftk_FreeObject(key); return crv; }
    }

    
    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) {
	sftk_FreeObject(key);
        return CKR_SESSION_HANDLE_INVALID;
    }

    


    crv = sftk_handleObject(key,session);
    sftk_FreeSession(session);
    if (sftk_isTrue(key,CKA_SENSITIVE)) {
	sftk_forceAttribute(key,CKA_ALWAYS_SENSITIVE,&cktrue,sizeof(CK_BBOOL));
    }
    if (!sftk_isTrue(key,CKA_EXTRACTABLE)) {
	sftk_forceAttribute(key,CKA_NEVER_EXTRACTABLE,&cktrue,sizeof(CK_BBOOL));
    }

    *phKey = key->handle;
    sftk_FreeObject(key);
    return crv;
}

#define PAIRWISE_DIGEST_LENGTH			SHA1_LENGTH /* 160-bits */
#define PAIRWISE_MESSAGE_LENGTH			20          /* 160-bits */










static CK_RV
sftk_PairwiseConsistencyCheck(CK_SESSION_HANDLE hSession,
    SFTKObject *publicKey, SFTKObject *privateKey, CK_KEY_TYPE keyType)
{
    












    CK_MECHANISM mech = {0, NULL, 0};

    CK_ULONG modulusLen;
    CK_ULONG subPrimeLen;
    PRBool isEncryptable = PR_FALSE;
    PRBool canSignVerify = PR_FALSE;
    PRBool isDerivable = PR_FALSE;
    CK_RV crv;

    
    unsigned char *known_message = (unsigned char *)"Known Crypto Message";
    unsigned char plaintext[PAIRWISE_MESSAGE_LENGTH];
    CK_ULONG bytes_decrypted;
    unsigned char *ciphertext;
    unsigned char *text_compared;
    CK_ULONG bytes_encrypted;
    CK_ULONG bytes_compared;
    CK_ULONG pairwise_digest_length = PAIRWISE_DIGEST_LENGTH;

    
    
    unsigned char *known_digest = (unsigned char *)
				"Mozilla Rules the World through NSS!";
    unsigned char *signature;
    CK_ULONG signature_length;

    if (keyType == CKK_RSA) {
	SFTKAttribute *attribute;

	
	attribute = sftk_FindAttribute(privateKey, CKA_MODULUS);
	if (attribute == NULL) {
	    return CKR_DEVICE_ERROR;
	}
	modulusLen = attribute->attrib.ulValueLen;
	if (*(unsigned char *)attribute->attrib.pValue == 0) {
	    modulusLen--;
	}
	sftk_FreeAttribute(attribute);
    } else if (keyType == CKK_DSA) {
	SFTKAttribute *attribute;

	
	attribute = sftk_FindAttribute(privateKey, CKA_SUBPRIME);
	if (attribute == NULL) {
	    return CKR_DEVICE_ERROR;
	}
	subPrimeLen = attribute->attrib.ulValueLen;
	if (subPrimeLen > 1 && *(unsigned char *)attribute->attrib.pValue == 0) {
	    subPrimeLen--;
	}
	sftk_FreeAttribute(attribute);
    }

    
    
    

    isEncryptable = sftk_isTrue(privateKey, CKA_DECRYPT); 

    



    if (isEncryptable) {
	if (keyType != CKK_RSA) {
	    return CKR_DEVICE_ERROR;
	}
	bytes_encrypted = modulusLen;
	mech.mechanism = CKM_RSA_PKCS;

	
	ciphertext = (unsigned char *) PORT_ZAlloc(bytes_encrypted);
	if (ciphertext == NULL) {
	    return CKR_HOST_MEMORY;
	}

	
	crv = NSC_EncryptInit(hSession, &mech, publicKey->handle);
	if (crv != CKR_OK) {
	    PORT_Free(ciphertext);
	    return crv;
	}

	
	crv = NSC_Encrypt(hSession,
			  known_message,
			  PAIRWISE_MESSAGE_LENGTH,
			  ciphertext,
			  &bytes_encrypted);
	if (crv != CKR_OK) {
	    PORT_Free(ciphertext);
	    return crv;
	}

	
	bytes_compared = PR_MIN(bytes_encrypted, PAIRWISE_MESSAGE_LENGTH);

	



	text_compared = ciphertext + bytes_encrypted - bytes_compared;

	




	if (PORT_Memcmp(text_compared, known_message,
			bytes_compared) == 0) {
	    
	    PORT_SetError(SEC_ERROR_INVALID_KEY);
	    PORT_Free(ciphertext);
	    return CKR_GENERAL_ERROR;
	}

	
	crv = NSC_DecryptInit(hSession, &mech, privateKey->handle);
	if (crv != CKR_OK) {
	    PORT_Free(ciphertext);
	    return crv;
	}

	memset(plaintext, 0, PAIRWISE_MESSAGE_LENGTH);

	



	bytes_decrypted = PAIRWISE_MESSAGE_LENGTH;

	




	crv = NSC_Decrypt(hSession,
			  ciphertext,
			  bytes_encrypted,
			  plaintext,
			  &bytes_decrypted);

	
	PORT_Free(ciphertext);

	if (crv != CKR_OK) {
	    return crv;
	}

	



	if ((bytes_decrypted != PAIRWISE_MESSAGE_LENGTH) ||
	    (PORT_Memcmp(plaintext, known_message,
			 PAIRWISE_MESSAGE_LENGTH) != 0)) {
	    
	    PORT_SetError(SEC_ERROR_BAD_KEY);
	    return CKR_GENERAL_ERROR;
	}
    }

    
    
    

    canSignVerify = sftk_isTrue(privateKey, CKA_SIGN);
    
    if (canSignVerify) {
	
	switch (keyType) {
	case CKK_RSA:
	    signature_length = modulusLen;
	    mech.mechanism = CKM_RSA_PKCS;
	    break;
	case CKK_DSA:
	    signature_length = DSA_MAX_SIGNATURE_LEN;
	    pairwise_digest_length = subPrimeLen;
	    mech.mechanism = CKM_DSA;
	    break;
#ifndef NSS_DISABLE_ECC
	case CKK_EC:
	    signature_length = MAX_ECKEY_LEN * 2;
	    mech.mechanism = CKM_ECDSA;
	    break;
#endif
	default:
	    return CKR_DEVICE_ERROR;
	}
	
	
	signature = (unsigned char *) PORT_ZAlloc(signature_length);
	if (signature == NULL) {
	    return CKR_HOST_MEMORY;
	}
	
	
	crv = NSC_SignInit(hSession, &mech, privateKey->handle);
	if (crv != CKR_OK) {
	    PORT_Free(signature);
	    return crv;
	}

	crv = NSC_Sign(hSession,
		       known_digest,
		       pairwise_digest_length,
		       signature,
		       &signature_length);
	if (crv != CKR_OK) {
	    PORT_Free(signature);
	    return crv;
	}
	
	
	crv = NSC_VerifyInit(hSession, &mech, publicKey->handle);
	if (crv != CKR_OK) {
	    PORT_Free(signature);
	    return crv;
	}

	crv = NSC_Verify(hSession,
			 known_digest,
			 pairwise_digest_length,
			 signature,
			 signature_length);

	
	PORT_Free(signature);

	if ((crv == CKR_SIGNATURE_LEN_RANGE) ||
		(crv == CKR_SIGNATURE_INVALID)) {
	    return CKR_GENERAL_ERROR;
	}
	if (crv != CKR_OK) {
	    return crv;
	}
    }

    
    
    

    isDerivable = sftk_isTrue(privateKey, CKA_DERIVE);
    
    if (isDerivable) {
	






	











    }

    return CKR_OK;
}



CK_RV NSC_GenerateKeyPair (CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_ATTRIBUTE_PTR pPublicKeyTemplate,
    CK_ULONG ulPublicKeyAttributeCount, CK_ATTRIBUTE_PTR pPrivateKeyTemplate,
    CK_ULONG ulPrivateKeyAttributeCount, CK_OBJECT_HANDLE_PTR phPublicKey,
    					CK_OBJECT_HANDLE_PTR phPrivateKey)
{
    SFTKObject *	publicKey,*privateKey;
    SFTKSession *	session;
    CK_KEY_TYPE 	key_type;
    CK_RV 		crv 	= CKR_OK;
    CK_BBOOL 		cktrue 	= CK_TRUE;
    SECStatus 		rv;
    CK_OBJECT_CLASS 	pubClass = CKO_PUBLIC_KEY;
    CK_OBJECT_CLASS 	privClass = CKO_PRIVATE_KEY;
    int 		i;
    SFTKSlot *		slot 	= sftk_SlotFromSessionHandle(hSession);
    unsigned int bitSize;

    
    int 		public_modulus_bits = 0;
    SECItem 		pubExp;
    RSAPrivateKey *	rsaPriv;

    
    PQGParams 		pqgParam;
    DHParams  		dhParam;
    DSAPrivateKey *	dsaPriv;

    
    int 		private_value_bits = 0;
    DHPrivateKey *	dhPriv;

#ifndef NSS_DISABLE_ECC
    
    SECItem  		ecEncodedParams;  
    ECPrivateKey *	ecPriv;
    ECParams *          ecParams;
#endif 

    CHECK_FORK();

    if (!slot) {
        return CKR_SESSION_HANDLE_INVALID;
    }
    


    publicKey = sftk_NewObject(slot); 
    if (publicKey == NULL) {
	return CKR_HOST_MEMORY;
    }

    


    for (i=0; i < (int) ulPublicKeyAttributeCount; i++) {
	if (pPublicKeyTemplate[i].type == CKA_MODULUS_BITS) {
	    public_modulus_bits = *(CK_ULONG *)pPublicKeyTemplate[i].pValue;
	    continue;
	}

	crv = sftk_AddAttributeType(publicKey,
				    sftk_attr_expand(&pPublicKeyTemplate[i]));
	if (crv != CKR_OK) break;
    }

    if (crv != CKR_OK) {
	sftk_FreeObject(publicKey);
	return CKR_HOST_MEMORY;
    }

    privateKey = sftk_NewObject(slot); 
    if (privateKey == NULL) {
	sftk_FreeObject(publicKey);
	return CKR_HOST_MEMORY;
    }
    


    for (i=0; i < (int) ulPrivateKeyAttributeCount; i++) {
	if (pPrivateKeyTemplate[i].type == CKA_VALUE_BITS) {
	    private_value_bits = *(CK_ULONG *)pPrivateKeyTemplate[i].pValue;
	    continue;
	}

	crv = sftk_AddAttributeType(privateKey,
				    sftk_attr_expand(&pPrivateKeyTemplate[i]));
	if (crv != CKR_OK) break;
    }

    if (crv != CKR_OK) {
	sftk_FreeObject(publicKey);
	sftk_FreeObject(privateKey);
	return CKR_HOST_MEMORY;
    }
    sftk_DeleteAttributeType(privateKey,CKA_CLASS);
    sftk_DeleteAttributeType(privateKey,CKA_KEY_TYPE);
    sftk_DeleteAttributeType(privateKey,CKA_VALUE);
    sftk_DeleteAttributeType(publicKey,CKA_CLASS);
    sftk_DeleteAttributeType(publicKey,CKA_KEY_TYPE);
    sftk_DeleteAttributeType(publicKey,CKA_VALUE);

    
    switch (pMechanism->mechanism) {
    case CKM_RSA_PKCS_KEY_PAIR_GEN:
	
    	sftk_DeleteAttributeType(publicKey,CKA_MODULUS);
    	sftk_DeleteAttributeType(privateKey,CKA_NETSCAPE_DB);
    	sftk_DeleteAttributeType(privateKey,CKA_MODULUS);
    	sftk_DeleteAttributeType(privateKey,CKA_PRIVATE_EXPONENT);
    	sftk_DeleteAttributeType(privateKey,CKA_PUBLIC_EXPONENT);
    	sftk_DeleteAttributeType(privateKey,CKA_PRIME_1);
    	sftk_DeleteAttributeType(privateKey,CKA_PRIME_2);
    	sftk_DeleteAttributeType(privateKey,CKA_EXPONENT_1);
    	sftk_DeleteAttributeType(privateKey,CKA_EXPONENT_2);
    	sftk_DeleteAttributeType(privateKey,CKA_COEFFICIENT);
	key_type = CKK_RSA;
	if (public_modulus_bits == 0) {
	    crv = CKR_TEMPLATE_INCOMPLETE;
	    break;
	}
	if (public_modulus_bits < RSA_MIN_MODULUS_BITS) {
	    crv = CKR_ATTRIBUTE_VALUE_INVALID;
	    break;
	}
	if (public_modulus_bits % 2 != 0) {
	    crv = CKR_ATTRIBUTE_VALUE_INVALID;
	    break;
	}

	
	crv=sftk_Attribute2SSecItem(NULL,&pubExp,publicKey,CKA_PUBLIC_EXPONENT);
	if (crv != CKR_OK) break;
        bitSize = sftk_GetLengthInBits(pubExp.data, pubExp.len);
	if (bitSize < 2) {
	    crv = CKR_ATTRIBUTE_VALUE_INVALID;
	    break;
	}
        crv = sftk_AddAttributeType(privateKey,CKA_PUBLIC_EXPONENT,
				    		    sftk_item_expand(&pubExp));
	if (crv != CKR_OK) {
	    PORT_Free(pubExp.data);
	    break;
	}

	rsaPriv = RSA_NewKey(public_modulus_bits, &pubExp);
	PORT_Free(pubExp.data);
	if (rsaPriv == NULL) {
	    if (PORT_GetError() == SEC_ERROR_LIBRARY_FAILURE) {
		sftk_fatalError = PR_TRUE;
	    }
	    crv = sftk_MapCryptError(PORT_GetError());
	    break;
	}
        
        crv = sftk_AddAttributeType(publicKey,CKA_MODULUS,
			   sftk_item_expand(&rsaPriv->modulus));
	if (crv != CKR_OK) goto kpg_done;
        
        crv = sftk_AddAttributeType(privateKey,CKA_NETSCAPE_DB,
			   sftk_item_expand(&rsaPriv->modulus));
	if (crv != CKR_OK) goto kpg_done;
        crv = sftk_AddAttributeType(privateKey,CKA_MODULUS,
			   sftk_item_expand(&rsaPriv->modulus));
	if (crv != CKR_OK) goto kpg_done;
        crv = sftk_AddAttributeType(privateKey,CKA_PRIVATE_EXPONENT,
			   sftk_item_expand(&rsaPriv->privateExponent));
	if (crv != CKR_OK) goto kpg_done;
        crv = sftk_AddAttributeType(privateKey,CKA_PRIME_1,
			   sftk_item_expand(&rsaPriv->prime1));
	if (crv != CKR_OK) goto kpg_done;
        crv = sftk_AddAttributeType(privateKey,CKA_PRIME_2,
			   sftk_item_expand(&rsaPriv->prime2));
	if (crv != CKR_OK) goto kpg_done;
        crv = sftk_AddAttributeType(privateKey,CKA_EXPONENT_1,
			   sftk_item_expand(&rsaPriv->exponent1));
	if (crv != CKR_OK) goto kpg_done;
        crv = sftk_AddAttributeType(privateKey,CKA_EXPONENT_2,
			   sftk_item_expand(&rsaPriv->exponent2));
	if (crv != CKR_OK) goto kpg_done;
        crv = sftk_AddAttributeType(privateKey,CKA_COEFFICIENT,
			   sftk_item_expand(&rsaPriv->coefficient));
kpg_done:
	
	PORT_FreeArena(rsaPriv->arena, PR_TRUE);
	break;
    case CKM_DSA_KEY_PAIR_GEN:
    	sftk_DeleteAttributeType(publicKey,CKA_VALUE);
    	sftk_DeleteAttributeType(privateKey,CKA_NETSCAPE_DB);
	sftk_DeleteAttributeType(privateKey,CKA_PRIME);
	sftk_DeleteAttributeType(privateKey,CKA_SUBPRIME);
	sftk_DeleteAttributeType(privateKey,CKA_BASE);
	key_type = CKK_DSA;

	
	crv=sftk_Attribute2SSecItem(NULL,&pqgParam.prime,publicKey,CKA_PRIME);
	if (crv != CKR_OK) break;
	crv=sftk_Attribute2SSecItem(NULL,&pqgParam.subPrime,publicKey,
	                            CKA_SUBPRIME);
	if (crv != CKR_OK) {
	    PORT_Free(pqgParam.prime.data);
	    break;
	}
	crv=sftk_Attribute2SSecItem(NULL,&pqgParam.base,publicKey,CKA_BASE);
	if (crv != CKR_OK) {
	    PORT_Free(pqgParam.prime.data);
	    PORT_Free(pqgParam.subPrime.data);
	    break;
	}
        crv = sftk_AddAttributeType(privateKey,CKA_PRIME,
				    sftk_item_expand(&pqgParam.prime));
	if (crv != CKR_OK) {
	    PORT_Free(pqgParam.prime.data);
	    PORT_Free(pqgParam.subPrime.data);
	    PORT_Free(pqgParam.base.data);
	    break;
	}
        crv = sftk_AddAttributeType(privateKey,CKA_SUBPRIME,
			    	    sftk_item_expand(&pqgParam.subPrime));
	if (crv != CKR_OK) {
	    PORT_Free(pqgParam.prime.data);
	    PORT_Free(pqgParam.subPrime.data);
	    PORT_Free(pqgParam.base.data);
	    break;
	}
        crv = sftk_AddAttributeType(privateKey,CKA_BASE,
			    	    sftk_item_expand(&pqgParam.base));
	if (crv != CKR_OK) {
	    PORT_Free(pqgParam.prime.data);
	    PORT_Free(pqgParam.subPrime.data);
	    PORT_Free(pqgParam.base.data);
	    break;
	}

	


        bitSize = sftk_GetLengthInBits(pqgParam.subPrime.data, 
							pqgParam.subPrime.len);
        if ((bitSize < DSA_MIN_Q_BITS) || (bitSize > DSA_MAX_Q_BITS))  {
	    crv = CKR_TEMPLATE_INCOMPLETE;
	    PORT_Free(pqgParam.prime.data);
	    PORT_Free(pqgParam.subPrime.data);
	    PORT_Free(pqgParam.base.data);
	    break;
	}
        bitSize = sftk_GetLengthInBits(pqgParam.prime.data,pqgParam.prime.len);
        if ((bitSize <  DSA_MIN_P_BITS) || (bitSize > DSA_MAX_P_BITS)) {
	    crv = CKR_TEMPLATE_INCOMPLETE;
	    PORT_Free(pqgParam.prime.data);
	    PORT_Free(pqgParam.subPrime.data);
	    PORT_Free(pqgParam.base.data);
	    break;
	}
        bitSize = sftk_GetLengthInBits(pqgParam.base.data,pqgParam.base.len);
        if ((bitSize <  2) || (bitSize > DSA_MAX_P_BITS)) {
	    crv = CKR_TEMPLATE_INCOMPLETE;
	    PORT_Free(pqgParam.prime.data);
	    PORT_Free(pqgParam.subPrime.data);
	    PORT_Free(pqgParam.base.data);
	    break;
	}
	    
	
	rv = DSA_NewKey(&pqgParam, &dsaPriv);

	PORT_Free(pqgParam.prime.data);
	PORT_Free(pqgParam.subPrime.data);
	PORT_Free(pqgParam.base.data);

	if (rv != SECSuccess) {
	    if (PORT_GetError() == SEC_ERROR_LIBRARY_FAILURE) {
		sftk_fatalError = PR_TRUE;
	    }
	    crv = sftk_MapCryptError(PORT_GetError());
	    break;
	}

	
        crv = sftk_AddAttributeType(publicKey,CKA_VALUE,
			   sftk_item_expand(&dsaPriv->publicValue));
	if (crv != CKR_OK) goto dsagn_done;

        
        crv = sftk_AddAttributeType(privateKey,CKA_NETSCAPE_DB,
			   sftk_item_expand(&dsaPriv->publicValue));
	if (crv != CKR_OK) goto dsagn_done;
        crv = sftk_AddAttributeType(privateKey,CKA_VALUE,
			   sftk_item_expand(&dsaPriv->privateValue));

dsagn_done:
	
	PORT_FreeArena(dsaPriv->params.arena, PR_TRUE);
	break;

    case CKM_DH_PKCS_KEY_PAIR_GEN:
	sftk_DeleteAttributeType(privateKey,CKA_PRIME);
	sftk_DeleteAttributeType(privateKey,CKA_BASE);
	sftk_DeleteAttributeType(privateKey,CKA_VALUE);
    	sftk_DeleteAttributeType(privateKey,CKA_NETSCAPE_DB);
	key_type = CKK_DH;

	
        crv = sftk_Attribute2SSecItem(NULL, &dhParam.prime, publicKey, 
				      CKA_PRIME);
	if (crv != CKR_OK) break;
	crv = sftk_Attribute2SSecItem(NULL, &dhParam.base, publicKey, CKA_BASE);
	if (crv != CKR_OK) {
	    PORT_Free(dhParam.prime.data);
	    break;
	}
	crv = sftk_AddAttributeType(privateKey, CKA_PRIME, 
				    sftk_item_expand(&dhParam.prime));
	if (crv != CKR_OK) {
	    PORT_Free(dhParam.prime.data);
	    PORT_Free(dhParam.base.data);
	    break;
	}
	crv = sftk_AddAttributeType(privateKey, CKA_BASE, 
				    sftk_item_expand(&dhParam.base));
	if (crv != CKR_OK) {
	    PORT_Free(dhParam.prime.data);
	    PORT_Free(dhParam.base.data);
	    break;
	}
        bitSize = sftk_GetLengthInBits(dhParam.prime.data,dhParam.prime.len);
        if ((bitSize <  DH_MIN_P_BITS) || (bitSize > DH_MAX_P_BITS)) {
	    crv = CKR_TEMPLATE_INCOMPLETE;
	    PORT_Free(dhParam.prime.data);
	    PORT_Free(dhParam.base.data);
	    break;
	}
        bitSize = sftk_GetLengthInBits(dhParam.base.data,dhParam.base.len);
        if ((bitSize <  1) || (bitSize > DH_MAX_P_BITS)) {
	    crv = CKR_TEMPLATE_INCOMPLETE;
	    PORT_Free(dhParam.prime.data);
	    PORT_Free(dhParam.base.data);
	    break;
	}

	rv = DH_NewKey(&dhParam, &dhPriv);
	PORT_Free(dhParam.prime.data);
	PORT_Free(dhParam.base.data);
	if (rv != SECSuccess) { 
	    if (PORT_GetError() == SEC_ERROR_LIBRARY_FAILURE) {
		sftk_fatalError = PR_TRUE;
	    }
	    crv = sftk_MapCryptError(PORT_GetError());
	    break;
	}

	crv=sftk_AddAttributeType(publicKey, CKA_VALUE, 
				sftk_item_expand(&dhPriv->publicValue));
	if (crv != CKR_OK) goto dhgn_done;

        crv = sftk_AddAttributeType(privateKey,CKA_NETSCAPE_DB,
			   sftk_item_expand(&dhPriv->publicValue));
	if (crv != CKR_OK) goto dhgn_done;

	crv=sftk_AddAttributeType(privateKey, CKA_VALUE, 
			      sftk_item_expand(&dhPriv->privateValue));

dhgn_done:
	
	PORT_FreeArena(dhPriv->arena, PR_TRUE);
	break;

#ifndef NSS_DISABLE_ECC
    case CKM_EC_KEY_PAIR_GEN:
	sftk_DeleteAttributeType(privateKey,CKA_EC_PARAMS);
	sftk_DeleteAttributeType(privateKey,CKA_VALUE);
    	sftk_DeleteAttributeType(privateKey,CKA_NETSCAPE_DB);
	key_type = CKK_EC;

	
	crv = sftk_Attribute2SSecItem(NULL, &ecEncodedParams, publicKey, 
				      CKA_EC_PARAMS);
	if (crv != CKR_OK) break;

	crv = sftk_AddAttributeType(privateKey, CKA_EC_PARAMS, 
				    sftk_item_expand(&ecEncodedParams));
	if (crv != CKR_OK) {
	  PORT_Free(ecEncodedParams.data);
	  break;
	}

	
	rv = EC_DecodeParams(&ecEncodedParams, &ecParams);
	PORT_Free(ecEncodedParams.data);
	if (rv != SECSuccess) {
	    crv = sftk_MapCryptError(PORT_GetError());
	    break;
	}
	rv = EC_NewKey(ecParams, &ecPriv);
	PORT_FreeArena(ecParams->arena, PR_TRUE);
	if (rv != SECSuccess) { 
	    if (PORT_GetError() == SEC_ERROR_LIBRARY_FAILURE) {
		sftk_fatalError = PR_TRUE;
	    }
	    crv = sftk_MapCryptError(PORT_GetError());
	    break;
	}

	if (getenv("NSS_USE_DECODED_CKA_EC_POINT")) {
	    crv = sftk_AddAttributeType(publicKey, CKA_EC_POINT, 
				sftk_item_expand(&ecPriv->publicValue));
	} else {
	    SECItem *pubValue = SEC_ASN1EncodeItem(NULL, NULL, 
					&ecPriv->publicValue, 
					SEC_ASN1_GET(SEC_OctetStringTemplate));
	    if (!pubValue) {
		crv = CKR_ARGUMENTS_BAD;
		goto ecgn_done;
	    }
	    crv = sftk_AddAttributeType(publicKey, CKA_EC_POINT, 
				sftk_item_expand(pubValue));
	    SECITEM_FreeItem(pubValue, PR_TRUE);
	}
	if (crv != CKR_OK) goto ecgn_done;

	crv = sftk_AddAttributeType(privateKey, CKA_VALUE, 
			      sftk_item_expand(&ecPriv->privateValue));
	if (crv != CKR_OK) goto ecgn_done;

        crv = sftk_AddAttributeType(privateKey,CKA_NETSCAPE_DB,
			   sftk_item_expand(&ecPriv->publicValue));
ecgn_done:
	
	PORT_FreeArena(ecPriv->ecParams.arena, PR_TRUE);
	break;
#endif 

    default:
	crv = CKR_MECHANISM_INVALID;
    }

    if (crv != CKR_OK) {
	sftk_FreeObject(privateKey);
	sftk_FreeObject(publicKey);
	return crv;
    }


    

    session = NULL; 
		    
    do {
	crv = sftk_AddAttributeType(privateKey,CKA_CLASS,&privClass,
						sizeof(CK_OBJECT_CLASS));
        if (crv != CKR_OK) break;
	crv = sftk_AddAttributeType(publicKey,CKA_CLASS,&pubClass,
						sizeof(CK_OBJECT_CLASS));
        if (crv != CKR_OK) break;
	crv = sftk_AddAttributeType(privateKey,CKA_KEY_TYPE,&key_type,
						sizeof(CK_KEY_TYPE));
        if (crv != CKR_OK) break;
	crv = sftk_AddAttributeType(publicKey,CKA_KEY_TYPE,&key_type,
						sizeof(CK_KEY_TYPE));
        if (crv != CKR_OK) break;
        session = sftk_SessionFromHandle(hSession);
        if (session == NULL) crv = CKR_SESSION_HANDLE_INVALID;
    } while (0);

    if (crv != CKR_OK) {
	 sftk_FreeObject(privateKey);
	 sftk_FreeObject(publicKey);
	 return crv;
    }

    


    crv = sftk_handleObject(privateKey,session);
    if (crv != CKR_OK) {
        sftk_FreeSession(session);
	sftk_FreeObject(privateKey);
	sftk_FreeObject(publicKey);
	return crv;
    }

    




    crv = sftk_handleObject(publicKey,session);
    sftk_FreeSession(session);
    if (crv != CKR_OK) {
	sftk_FreeObject(publicKey);
	NSC_DestroyObject(hSession,privateKey->handle);
	sftk_FreeObject(privateKey);
	return crv;
    }
    if (sftk_isTrue(privateKey,CKA_SENSITIVE)) {
	sftk_forceAttribute(privateKey,CKA_ALWAYS_SENSITIVE,
						&cktrue,sizeof(CK_BBOOL));
    }
    if (sftk_isTrue(publicKey,CKA_SENSITIVE)) {
	sftk_forceAttribute(publicKey,CKA_ALWAYS_SENSITIVE,
						&cktrue,sizeof(CK_BBOOL));
    }
    if (!sftk_isTrue(privateKey,CKA_EXTRACTABLE)) {
	sftk_forceAttribute(privateKey,CKA_NEVER_EXTRACTABLE,
						&cktrue,sizeof(CK_BBOOL));
    }
    if (!sftk_isTrue(publicKey,CKA_EXTRACTABLE)) {
	sftk_forceAttribute(publicKey,CKA_NEVER_EXTRACTABLE,
						&cktrue,sizeof(CK_BBOOL));
    }

    
    crv = sftk_PairwiseConsistencyCheck(hSession,
					publicKey, privateKey, key_type);
    if (crv != CKR_OK) {
	NSC_DestroyObject(hSession,publicKey->handle);
	sftk_FreeObject(publicKey);
	NSC_DestroyObject(hSession,privateKey->handle);
	sftk_FreeObject(privateKey);
	if (sftk_audit_enabled) {
	    char msg[128];
	    PR_snprintf(msg,sizeof msg,
			"C_GenerateKeyPair(hSession=0x%08lX, "
			"pMechanism->mechanism=0x%08lX)=0x%08lX "
			"self-test: pair-wise consistency test failed",
			(PRUint32)hSession,(PRUint32)pMechanism->mechanism,
			(PRUint32)crv);
	    sftk_LogAuditMessage(NSS_AUDIT_ERROR, NSS_AUDIT_SELF_TEST, msg);
	}
	return crv;
    }

    *phPrivateKey = privateKey->handle;
    *phPublicKey = publicKey->handle;
    sftk_FreeObject(publicKey);
    sftk_FreeObject(privateKey);

    return CKR_OK;
}

static SECItem *sftk_PackagePrivateKey(SFTKObject *key, CK_RV *crvp)
{
    NSSLOWKEYPrivateKey *lk = NULL;
    NSSLOWKEYPrivateKeyInfo *pki = NULL;
    SFTKAttribute *attribute = NULL;
    PLArenaPool *arena = NULL;
    SECOidTag algorithm = SEC_OID_UNKNOWN;
    void *dummy, *param = NULL;
    SECStatus rv = SECSuccess;
    SECItem *encodedKey = NULL;
#ifndef NSS_DISABLE_ECC
    SECItem *fordebug;
    int savelen;
#endif

    if(!key) {
	*crvp = CKR_KEY_HANDLE_INVALID; 
	return NULL;
    }

    attribute = sftk_FindAttribute(key, CKA_KEY_TYPE);
    if(!attribute) {
	*crvp = CKR_KEY_TYPE_INCONSISTENT;
	return NULL;
    }

    lk = sftk_GetPrivKey(key, *(CK_KEY_TYPE *)attribute->attrib.pValue, crvp);
    sftk_FreeAttribute(attribute);
    if(!lk) {
	return NULL;
    }

    arena = PORT_NewArena(2048); 	
    if(!arena) {
	*crvp = CKR_HOST_MEMORY;
	rv = SECFailure;
	goto loser;
    }

    pki = (NSSLOWKEYPrivateKeyInfo*)PORT_ArenaZAlloc(arena, 
					sizeof(NSSLOWKEYPrivateKeyInfo));
    if(!pki) {
	*crvp = CKR_HOST_MEMORY;
	rv = SECFailure;
	goto loser;
    }
    pki->arena = arena;

    param = NULL;
    switch(lk->keyType) {
	case NSSLOWKEYRSAKey:
	    prepare_low_rsa_priv_key_for_asn1(lk);
	    dummy = SEC_ASN1EncodeItem(arena, &pki->privateKey, lk,
				       nsslowkey_RSAPrivateKeyTemplate);
	    algorithm = SEC_OID_PKCS1_RSA_ENCRYPTION;
	    break;
	case NSSLOWKEYDSAKey:
            prepare_low_dsa_priv_key_export_for_asn1(lk);
	    dummy = SEC_ASN1EncodeItem(arena, &pki->privateKey, lk,
				       nsslowkey_DSAPrivateKeyExportTemplate);
	    prepare_low_pqg_params_for_asn1(&lk->u.dsa.params);
	    param = SEC_ASN1EncodeItem(NULL, NULL, &(lk->u.dsa.params),
				       nsslowkey_PQGParamsTemplate);
	    algorithm = SEC_OID_ANSIX9_DSA_SIGNATURE;
	    break;
#ifndef NSS_DISABLE_ECC
        case NSSLOWKEYECKey:
            prepare_low_ec_priv_key_for_asn1(lk);
	    







	    lk->u.ec.publicValue.len <<= 3;
	    savelen = lk->u.ec.ecParams.curveOID.len;
	    lk->u.ec.ecParams.curveOID.len = 0;
	    dummy = SEC_ASN1EncodeItem(arena, &pki->privateKey, lk,
				       nsslowkey_ECPrivateKeyTemplate);
	    lk->u.ec.ecParams.curveOID.len = savelen;
	    lk->u.ec.publicValue.len >>= 3;

	    fordebug = &pki->privateKey;
	    SEC_PRINT("sftk_PackagePrivateKey()", "PrivateKey", lk->keyType,
		      fordebug);

	    param = SECITEM_DupItem(&lk->u.ec.ecParams.DEREncoding);

	    algorithm = SEC_OID_ANSIX962_EC_PUBLIC_KEY;
	    break;
#endif 
	case NSSLOWKEYDHKey:
	default:
	    dummy = NULL;
	    break;
    }
 
    if(!dummy || ((lk->keyType == NSSLOWKEYDSAKey) && !param)) {
	*crvp = CKR_DEVICE_ERROR; 
	rv = SECFailure;
	goto loser;
    }
    
    rv = SECOID_SetAlgorithmID(arena, &pki->algorithm, algorithm, 
			       (SECItem*)param);
    if(rv != SECSuccess) {
	*crvp = CKR_DEVICE_ERROR; 
	rv = SECFailure;
	goto loser;
    }

    dummy = SEC_ASN1EncodeInteger(arena, &pki->version,
				  NSSLOWKEY_PRIVATE_KEY_INFO_VERSION);
    if(!dummy) {
	*crvp = CKR_DEVICE_ERROR; 
	rv = SECFailure;
	goto loser;
    }

    encodedKey = SEC_ASN1EncodeItem(NULL, NULL, pki, 
				    nsslowkey_PrivateKeyInfoTemplate);
    *crvp = encodedKey ? CKR_OK : CKR_DEVICE_ERROR;

#ifndef NSS_DISABLE_ECC
    fordebug = encodedKey;
    SEC_PRINT("sftk_PackagePrivateKey()", "PrivateKeyInfo", lk->keyType,
	      fordebug);
#endif
loser:
    if(arena) {
	PORT_FreeArena(arena, PR_TRUE);
    }

    if(lk && (lk != key->objectInfo)) {
	nsslowkey_DestroyPrivateKey(lk);
    }
 
    if(param) {
	SECITEM_ZfreeItem((SECItem*)param, PR_TRUE);
    }

    if(rv != SECSuccess) {
	return NULL;
    }

    return encodedKey;
}
    


static CK_RV 
sftk_mapWrap(CK_RV crv) 
{ 
    switch (crv) {
    case CKR_ENCRYPTED_DATA_INVALID:  crv = CKR_WRAPPED_KEY_INVALID; break;
    }
    return crv; 
}


CK_RV NSC_WrapKey(CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hWrappingKey,
    CK_OBJECT_HANDLE hKey, CK_BYTE_PTR pWrappedKey,
					 CK_ULONG_PTR pulWrappedKeyLen)
{
    SFTKSession *session;
    SFTKAttribute *attribute;
    SFTKObject *key;
    CK_RV crv;

    CHECK_FORK();

    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) {
    	return CKR_SESSION_HANDLE_INVALID;
    }

    key = sftk_ObjectFromHandle(hKey,session);
    sftk_FreeSession(session);
    if (key == NULL) {
	return CKR_KEY_HANDLE_INVALID;
    }

    switch(key->objclass) {
	case CKO_SECRET_KEY:
	  {
	    SFTKSessionContext *context = NULL;
	    SECItem pText;

	    attribute = sftk_FindAttribute(key,CKA_VALUE);

	    if (attribute == NULL) {
		crv = CKR_KEY_TYPE_INCONSISTENT;
		break;
	    }
	    crv = sftk_CryptInit(hSession, pMechanism, hWrappingKey, 
				CKA_WRAP, CKA_WRAP, SFTK_ENCRYPT, PR_TRUE);
	    if (crv != CKR_OK) {
		sftk_FreeAttribute(attribute);
		break;
	    }

	    pText.type = siBuffer;
	    pText.data = (unsigned char *)attribute->attrib.pValue;
	    pText.len  = attribute->attrib.ulValueLen;

	    
	    crv = sftk_GetContext(hSession,&context,SFTK_ENCRYPT,PR_FALSE,NULL);
	    if (crv != CKR_OK || !context) 
	        break;
	    if (context->blockSize > 1) {
		unsigned int remainder = pText.len % context->blockSize;
	        if (!context->doPad && remainder) {
		    



		    pText.len += context->blockSize - remainder;
		    pText.data = PORT_ZAlloc(pText.len);
		    if (pText.data)
			memcpy(pText.data, attribute->attrib.pValue,
			                   attribute->attrib.ulValueLen);
		    else {
			crv = CKR_HOST_MEMORY;
			break;
		    }
		}
	    }

	    crv = NSC_Encrypt(hSession, (CK_BYTE_PTR)pText.data, 
		              pText.len, pWrappedKey, pulWrappedKeyLen);
	    

	    if (crv != CKR_OK || pWrappedKey == NULL) {
	    	CK_RV lcrv ;
		lcrv = sftk_GetContext(hSession,&context,
				       SFTK_ENCRYPT,PR_FALSE,NULL);
		sftk_SetContextByType(session, SFTK_ENCRYPT, NULL);
	    	if (lcrv == CKR_OK && context) {
		    sftk_FreeContext(context);
		}
    	    }

	    if (pText.data != (unsigned char *)attribute->attrib.pValue) 
	    	PORT_ZFree(pText.data, pText.len);
	    sftk_FreeAttribute(attribute);
	    break;
	  }

	case CKO_PRIVATE_KEY:
	    {
		SECItem *bpki = sftk_PackagePrivateKey(key, &crv);
		SFTKSessionContext *context = NULL;

		if(!bpki) {
		    break;
		}

		crv = sftk_CryptInit(hSession, pMechanism, hWrappingKey,
				CKA_WRAP, CKA_WRAP, SFTK_ENCRYPT, PR_TRUE);
		if(crv != CKR_OK) {
		    SECITEM_ZfreeItem(bpki, PR_TRUE);
		    crv = CKR_KEY_TYPE_INCONSISTENT;
		    break;
		}

		crv = NSC_Encrypt(hSession, bpki->data, bpki->len,
					pWrappedKey, pulWrappedKeyLen);
		
		if (crv != CKR_OK || pWrappedKey == NULL) {
	    	    CK_RV lcrv ;
		    lcrv = sftk_GetContext(hSession,&context,
					   SFTK_ENCRYPT,PR_FALSE,NULL);
		    sftk_SetContextByType(session, SFTK_ENCRYPT, NULL);
	    	    if (lcrv == CKR_OK && context)  {
			sftk_FreeContext(context);
		    }
		}
		SECITEM_ZfreeItem(bpki, PR_TRUE);
		break;
	    }

	default:
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
    }
    sftk_FreeObject(key);

    return sftk_mapWrap(crv);
}




static SECStatus
sftk_unwrapPrivateKey(SFTKObject *key, SECItem *bpki)
{
    CK_BBOOL cktrue = CK_TRUE; 
    CK_KEY_TYPE keyType = CKK_RSA;
    SECStatus rv = SECFailure;
    const SEC_ASN1Template *keyTemplate, *paramTemplate;
    void *paramDest = NULL;
    PLArenaPool *arena;
    NSSLOWKEYPrivateKey *lpk = NULL;
    NSSLOWKEYPrivateKeyInfo *pki = NULL;
    CK_RV crv = CKR_KEY_TYPE_INCONSISTENT;

    arena = PORT_NewArena(2048);
    if(!arena) {
	return SECFailure;
    }

    pki = (NSSLOWKEYPrivateKeyInfo*)PORT_ArenaZAlloc(arena, 
					sizeof(NSSLOWKEYPrivateKeyInfo));
    if(!pki) {
	PORT_FreeArena(arena, PR_FALSE);
	return SECFailure;
    }

    if(SEC_ASN1DecodeItem(arena, pki, nsslowkey_PrivateKeyInfoTemplate, bpki) 
				!= SECSuccess) {
	PORT_FreeArena(arena, PR_TRUE);
	return SECFailure;
    }

    lpk = (NSSLOWKEYPrivateKey *)PORT_ArenaZAlloc(arena,
						  sizeof(NSSLOWKEYPrivateKey));
    if(lpk == NULL) {
	goto loser;
    }
    lpk->arena = arena;

    switch(SECOID_GetAlgorithmTag(&pki->algorithm)) {
	case SEC_OID_PKCS1_RSA_ENCRYPTION:
	    keyTemplate = nsslowkey_RSAPrivateKeyTemplate;
	    paramTemplate = NULL;
	    paramDest = NULL;
	    lpk->keyType = NSSLOWKEYRSAKey;
	    prepare_low_rsa_priv_key_for_asn1(lpk);
	    break;
	case SEC_OID_ANSIX9_DSA_SIGNATURE:
	    keyTemplate = nsslowkey_DSAPrivateKeyExportTemplate;
	    paramTemplate = nsslowkey_PQGParamsTemplate;
	    paramDest = &(lpk->u.dsa.params);
	    lpk->keyType = NSSLOWKEYDSAKey;
	    prepare_low_dsa_priv_key_export_for_asn1(lpk);
	    prepare_low_pqg_params_for_asn1(&lpk->u.dsa.params);
	    break;
	
#ifndef NSS_DISABLE_ECC
        case SEC_OID_ANSIX962_EC_PUBLIC_KEY:
	    keyTemplate = nsslowkey_ECPrivateKeyTemplate;
	    paramTemplate = NULL;
	    paramDest = &(lpk->u.ec.ecParams.DEREncoding);
	    lpk->keyType = NSSLOWKEYECKey;
	    prepare_low_ec_priv_key_for_asn1(lpk);
	    prepare_low_ecparams_for_asn1(&lpk->u.ec.ecParams);
	    break;
#endif 
	default:
	    keyTemplate = NULL;
	    paramTemplate = NULL;
	    paramDest = NULL;
	    break;
    }

    if(!keyTemplate) {
	goto loser;
    }

    
    rv = SEC_QuickDERDecodeItem(arena, lpk, keyTemplate, &pki->privateKey);

#ifndef NSS_DISABLE_ECC
    if (lpk->keyType == NSSLOWKEYECKey) {
        
	lpk->u.ec.publicValue.len >>= 3;
        rv = SECITEM_CopyItem(arena, 
			      &(lpk->u.ec.ecParams.DEREncoding),
	                      &(pki->algorithm.parameters));
	if(rv != SECSuccess) {
	    goto loser;
	}
    }
#endif 

    if(rv != SECSuccess) {
	goto loser;
    }
    if(paramDest && paramTemplate) {
	rv = SEC_QuickDERDecodeItem(arena, paramDest, paramTemplate, 
				 &(pki->algorithm.parameters));
	if(rv != SECSuccess) {
	    goto loser;
	}
    }

    rv = SECFailure;

    switch (lpk->keyType) {
        case NSSLOWKEYRSAKey:
	    keyType = CKK_RSA;
	    if(sftk_hasAttribute(key, CKA_NETSCAPE_DB)) {
		sftk_DeleteAttributeType(key, CKA_NETSCAPE_DB);
	    }
	    crv = sftk_AddAttributeType(key, CKA_KEY_TYPE, &keyType, 
					sizeof(keyType));
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_UNWRAP, &cktrue, 
					sizeof(CK_BBOOL));
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_DECRYPT, &cktrue, 
					sizeof(CK_BBOOL));
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_SIGN, &cktrue, 
					sizeof(CK_BBOOL));
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_SIGN_RECOVER, &cktrue, 
				    sizeof(CK_BBOOL));
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_MODULUS, 
				sftk_item_expand(&lpk->u.rsa.modulus));
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_PUBLIC_EXPONENT, 
	     			sftk_item_expand(&lpk->u.rsa.publicExponent));
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_PRIVATE_EXPONENT, 
	     			sftk_item_expand(&lpk->u.rsa.privateExponent));
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_PRIME_1, 
				sftk_item_expand(&lpk->u.rsa.prime1));
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_PRIME_2, 
	     			sftk_item_expand(&lpk->u.rsa.prime2));
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_EXPONENT_1, 
	     			sftk_item_expand(&lpk->u.rsa.exponent1));
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_EXPONENT_2, 
	     			sftk_item_expand(&lpk->u.rsa.exponent2));
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_COEFFICIENT, 
	     			sftk_item_expand(&lpk->u.rsa.coefficient));
	    break;
        case NSSLOWKEYDSAKey:
	    keyType = CKK_DSA;
	    crv = (sftk_hasAttribute(key, CKA_NETSCAPE_DB)) ? CKR_OK :
						CKR_KEY_TYPE_INCONSISTENT;
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_KEY_TYPE, &keyType, 
						sizeof(keyType));
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_SIGN, &cktrue, 
						sizeof(CK_BBOOL));
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_SIGN_RECOVER, &cktrue, 
						sizeof(CK_BBOOL)); 
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_PRIME,    
				    sftk_item_expand(&lpk->u.dsa.params.prime));
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_SUBPRIME,
				 sftk_item_expand(&lpk->u.dsa.params.subPrime));
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_BASE,  
				    sftk_item_expand(&lpk->u.dsa.params.base));
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_VALUE, 
			sftk_item_expand(&lpk->u.dsa.privateValue));
	    if(crv != CKR_OK) break;
	    break;
#ifdef notdef
        case NSSLOWKEYDHKey:
	    template = dhTemplate;
	    templateCount = sizeof(dhTemplate)/sizeof(CK_ATTRIBUTE);
	    keyType = CKK_DH;
	    break;
#endif
	
#ifndef NSS_DISABLE_ECC
        case NSSLOWKEYECKey:
	    keyType = CKK_EC;
	    crv = (sftk_hasAttribute(key, CKA_NETSCAPE_DB)) ? CKR_OK :
						CKR_KEY_TYPE_INCONSISTENT;
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_KEY_TYPE, &keyType, 
						sizeof(keyType));
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_SIGN, &cktrue, 
						sizeof(CK_BBOOL));
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_SIGN_RECOVER, &cktrue, 
						sizeof(CK_BBOOL)); 
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_DERIVE, &cktrue, 
						sizeof(CK_BBOOL)); 
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_EC_PARAMS,
				 sftk_item_expand(&lpk->u.ec.ecParams.DEREncoding));
	    if(crv != CKR_OK) break;
	    crv = sftk_AddAttributeType(key, CKA_VALUE, 
			sftk_item_expand(&lpk->u.ec.privateValue));
	    if(crv != CKR_OK) break;
	    
	    break;
#endif 
	default:
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
    }

loser:
    if(lpk) {
	nsslowkey_DestroyPrivateKey(lpk);
    }

    if(crv != CKR_OK) {
	return SECFailure;
    }

    return SECSuccess;
}



CK_RV NSC_UnwrapKey(CK_SESSION_HANDLE hSession,
    CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hUnwrappingKey,
    CK_BYTE_PTR pWrappedKey, CK_ULONG ulWrappedKeyLen,
    CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulAttributeCount,
						 CK_OBJECT_HANDLE_PTR phKey)
{
    SFTKObject *key = NULL;
    SFTKSession *session;
    CK_ULONG key_length = 0;
    unsigned char * buf = NULL;
    CK_RV crv = CKR_OK;
    int i;
    CK_ULONG bsize = ulWrappedKeyLen;
    SFTKSlot *slot = sftk_SlotFromSessionHandle(hSession);
    SECItem bpki;
    CK_OBJECT_CLASS target_type = CKO_SECRET_KEY;

    CHECK_FORK();

    if (!slot) {
        return CKR_SESSION_HANDLE_INVALID;
    }
    


    key = sftk_NewObject(slot); 
    if (key == NULL) {
	return CKR_HOST_MEMORY;
    }

    


    for (i=0; i < (int) ulAttributeCount; i++) {
	if (pTemplate[i].type == CKA_VALUE_LEN) {
	    key_length = *(CK_ULONG *)pTemplate[i].pValue;
	    continue;
	}
        if (pTemplate[i].type == CKA_CLASS) {
	    target_type = *(CK_OBJECT_CLASS *)pTemplate[i].pValue;
	}
	crv = sftk_AddAttributeType(key,sftk_attr_expand(&pTemplate[i]));
	if (crv != CKR_OK) break;
    }
    if (crv != CKR_OK) {
	sftk_FreeObject(key);
	return crv;
    }

    crv = sftk_CryptInit(hSession,pMechanism,hUnwrappingKey,CKA_UNWRAP,
					CKA_UNWRAP, SFTK_DECRYPT, PR_FALSE);
    if (crv != CKR_OK) {
	sftk_FreeObject(key);
	return sftk_mapWrap(crv);
    }

    


    buf = (unsigned char *)PORT_Alloc( ulWrappedKeyLen);
    bsize = ulWrappedKeyLen;

    crv = NSC_Decrypt(hSession, pWrappedKey, ulWrappedKeyLen, buf, &bsize);
    if (crv != CKR_OK) {
	sftk_FreeObject(key);
	PORT_Free(buf);
	return sftk_mapWrap(crv);
    }

    switch(target_type) {
	case CKO_SECRET_KEY:
	    if (!sftk_hasAttribute(key,CKA_KEY_TYPE)) {
		crv = CKR_TEMPLATE_INCOMPLETE;
		break;
	    }

	    if (key_length == 0 || key_length > bsize) {
		key_length = bsize;
	    }
	    if (key_length > MAX_KEY_LEN) {
		crv = CKR_TEMPLATE_INCONSISTENT;
		break;
	    }
    
	    
	    crv = sftk_AddAttributeType(key,CKA_VALUE,buf,key_length);
	    break;
	case CKO_PRIVATE_KEY:
	    bpki.data = (unsigned char *)buf;
	    bpki.len = bsize;
	    crv = CKR_OK;
	    if(sftk_unwrapPrivateKey(key, &bpki) != SECSuccess) {
		crv = CKR_TEMPLATE_INCOMPLETE;
	    }
	    break;
	default:
	    crv = CKR_TEMPLATE_INCONSISTENT;
	    break;
    }

    PORT_ZFree(buf, bsize);
    if (crv != CKR_OK) { sftk_FreeObject(key); return crv; }

    
    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) {
	sftk_FreeObject(key);
        return CKR_SESSION_HANDLE_INVALID;
    }

    


    crv = sftk_handleObject(key,session);
    *phKey = key->handle;
    sftk_FreeSession(session);
    sftk_FreeObject(key);

    return crv;

}





static CK_RV
sftk_buildSSLKey(CK_SESSION_HANDLE hSession, SFTKObject *baseKey, 
    PRBool isMacKey, unsigned char *keyBlock, unsigned int keySize,
						 CK_OBJECT_HANDLE *keyHandle)
{
    SFTKObject *key;
    SFTKSession *session;
    CK_KEY_TYPE keyType = CKK_GENERIC_SECRET;
    CK_BBOOL cktrue = CK_TRUE;
    CK_BBOOL ckfalse = CK_FALSE;
    CK_RV crv = CKR_HOST_MEMORY;

    


    *keyHandle = CK_INVALID_HANDLE;
    key = sftk_NewObject(baseKey->slot); 
    if (key == NULL) return CKR_HOST_MEMORY;
    sftk_narrowToSessionObject(key)->wasDerived = PR_TRUE;

    crv = sftk_CopyObject(key,baseKey);
    if (crv != CKR_OK) goto loser;
    if (isMacKey) {
	crv = sftk_forceAttribute(key,CKA_KEY_TYPE,&keyType,sizeof(keyType));
	if (crv != CKR_OK) goto loser;
	crv = sftk_forceAttribute(key,CKA_DERIVE,&cktrue,sizeof(CK_BBOOL));
	if (crv != CKR_OK) goto loser;
	crv = sftk_forceAttribute(key,CKA_ENCRYPT,&ckfalse,sizeof(CK_BBOOL));
	if (crv != CKR_OK) goto loser;
	crv = sftk_forceAttribute(key,CKA_DECRYPT,&ckfalse,sizeof(CK_BBOOL));
	if (crv != CKR_OK) goto loser;
	crv = sftk_forceAttribute(key,CKA_SIGN,&cktrue,sizeof(CK_BBOOL));
	if (crv != CKR_OK) goto loser;
	crv = sftk_forceAttribute(key,CKA_VERIFY,&cktrue,sizeof(CK_BBOOL));
	if (crv != CKR_OK) goto loser;
	crv = sftk_forceAttribute(key,CKA_WRAP,&ckfalse,sizeof(CK_BBOOL));
	if (crv != CKR_OK) goto loser;
	crv = sftk_forceAttribute(key,CKA_UNWRAP,&ckfalse,sizeof(CK_BBOOL));
	if (crv != CKR_OK) goto loser;
    }
    crv = sftk_forceAttribute(key,CKA_VALUE,keyBlock,keySize);
    if (crv != CKR_OK) goto loser;

    
    crv = CKR_HOST_MEMORY;
    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) { goto loser; }

    crv = sftk_handleObject(key,session);
    sftk_FreeSession(session);
    *keyHandle = key->handle;
loser:
    if (key) sftk_FreeObject(key);
    return crv;
}





static void
sftk_freeSSLKeys(CK_SESSION_HANDLE session,
				CK_SSL3_KEY_MAT_OUT *returnedMaterial ) 
{
	if (returnedMaterial->hClientMacSecret != CK_INVALID_HANDLE) {
	   NSC_DestroyObject(session,returnedMaterial->hClientMacSecret);
	}
	if (returnedMaterial->hServerMacSecret != CK_INVALID_HANDLE) {
	   NSC_DestroyObject(session, returnedMaterial->hServerMacSecret);
	}
	if (returnedMaterial->hClientKey != CK_INVALID_HANDLE) {
	   NSC_DestroyObject(session, returnedMaterial->hClientKey);
	}
	if (returnedMaterial->hServerKey != CK_INVALID_HANDLE) {
	   NSC_DestroyObject(session, returnedMaterial->hServerKey);
	}
}






static CK_RV
sftk_DeriveSensitiveCheck(SFTKObject *baseKey,SFTKObject *destKey) 
{
    PRBool hasSensitive;
    PRBool sensitive = PR_FALSE;
    PRBool hasExtractable;
    PRBool extractable = PR_TRUE;
    CK_RV crv = CKR_OK;
    SFTKAttribute *att;

    hasSensitive = PR_FALSE;
    att = sftk_FindAttribute(destKey,CKA_SENSITIVE);
    if (att) {
        hasSensitive = PR_TRUE;
	sensitive = (PRBool) *(CK_BBOOL *)att->attrib.pValue;
	sftk_FreeAttribute(att);
    }

    hasExtractable = PR_FALSE;
    att = sftk_FindAttribute(destKey,CKA_EXTRACTABLE);
    if (att) {
        hasExtractable = PR_TRUE;
	extractable = (PRBool) *(CK_BBOOL *)att->attrib.pValue;
	sftk_FreeAttribute(att);
    }


    
    if (sftk_isTrue(baseKey,CKA_SENSITIVE) && hasSensitive && 
						(sensitive == PR_FALSE)) {
	return CKR_KEY_FUNCTION_NOT_PERMITTED;
    }
    if (!sftk_isTrue(baseKey,CKA_EXTRACTABLE) && hasExtractable && 
						(extractable == PR_TRUE)) {
	return CKR_KEY_FUNCTION_NOT_PERMITTED;
    }

    
    if (!hasSensitive) {
        att = sftk_FindAttribute(baseKey,CKA_SENSITIVE);
	if (att == NULL) return CKR_KEY_TYPE_INCONSISTENT;
	crv = sftk_defaultAttribute(destKey,sftk_attr_expand(&att->attrib));
	sftk_FreeAttribute(att);
	if (crv != CKR_OK) return crv;
    }
    if (!hasExtractable) {
        att = sftk_FindAttribute(baseKey,CKA_EXTRACTABLE);
	if (att == NULL) return CKR_KEY_TYPE_INCONSISTENT;
	crv = sftk_defaultAttribute(destKey,sftk_attr_expand(&att->attrib));
	sftk_FreeAttribute(att);
	if (crv != CKR_OK) return crv;
    }

    


    return CKR_OK;
}



	
unsigned long
sftk_MapKeySize(CK_KEY_TYPE keyType) 
{
    switch (keyType) {
    case CKK_CDMF:
	return 8;
    case CKK_DES:
	return 8;
    case CKK_DES2:
	return 16;
    case CKK_DES3:
	return 24;
    
    default:
	break;
    }
    return 0;
}

#ifndef NSS_DISABLE_ECC










static CK_RV sftk_compute_ANSI_X9_63_kdf(CK_BYTE **key, CK_ULONG key_len, SECItem *SharedSecret,
		CK_BYTE_PTR SharedInfo, CK_ULONG SharedInfoLen,
		SECStatus Hash(unsigned char *, const unsigned char *, PRUint32),
		CK_ULONG HashLen)
{
    unsigned char *buffer = NULL, *output_buffer = NULL;
    PRUint32 buffer_len, max_counter, i;
    SECStatus rv;
    CK_RV crv;

    


    if (key_len > 254 * HashLen)
	return CKR_ARGUMENTS_BAD;

    if (SharedInfo == NULL)
	SharedInfoLen = 0;

    buffer_len = SharedSecret->len + 4 + SharedInfoLen;
    buffer = (CK_BYTE *)PORT_Alloc(buffer_len);
    if (buffer == NULL) {
	crv = CKR_HOST_MEMORY;
	goto loser;
    }

    max_counter = key_len/HashLen;
    if (key_len > max_counter * HashLen)
	max_counter++;

    output_buffer = (CK_BYTE *)PORT_Alloc(max_counter * HashLen);
    if (output_buffer == NULL) {
	crv = CKR_HOST_MEMORY;
	goto loser;
    }

    

    PORT_Memcpy(buffer, SharedSecret->data, SharedSecret->len);
    buffer[SharedSecret->len] = 0;
    buffer[SharedSecret->len + 1] = 0;
    buffer[SharedSecret->len + 2] = 0;
    buffer[SharedSecret->len + 3] = 1;
    if (SharedInfo) {
	PORT_Memcpy(&buffer[SharedSecret->len + 4], SharedInfo, SharedInfoLen);
    }

    for(i=0; i < max_counter; i++) {
	rv = Hash(&output_buffer[i * HashLen], buffer, buffer_len);
	if (rv != SECSuccess) {
	    
	    crv = CKR_FUNCTION_FAILED;
	    goto loser;
	}

	
	buffer[SharedSecret->len + 3]++;
    }

    PORT_ZFree(buffer, buffer_len);
    if (key_len < max_counter * HashLen) {
	PORT_Memset(output_buffer + key_len, 0, max_counter * HashLen - key_len);
    }
    *key = output_buffer;

    return CKR_OK;

    loser:
	if (buffer) {
	    PORT_ZFree(buffer, buffer_len);
	}
	if (output_buffer) {
	    PORT_ZFree(output_buffer, max_counter * HashLen);
	}
	return crv;
}

static CK_RV sftk_ANSI_X9_63_kdf(CK_BYTE **key, CK_ULONG key_len,
		SECItem *SharedSecret,
		CK_BYTE_PTR SharedInfo, CK_ULONG SharedInfoLen,
		CK_EC_KDF_TYPE kdf)
{
    if (kdf == CKD_SHA1_KDF)
	return sftk_compute_ANSI_X9_63_kdf(key, key_len, SharedSecret, SharedInfo,
		   		 SharedInfoLen, SHA1_HashBuf, SHA1_LENGTH);
    else if (kdf == CKD_SHA224_KDF)
	return sftk_compute_ANSI_X9_63_kdf(key, key_len, SharedSecret, SharedInfo,
		   		 SharedInfoLen, SHA224_HashBuf, SHA224_LENGTH);
    else if (kdf == CKD_SHA256_KDF)
	return sftk_compute_ANSI_X9_63_kdf(key, key_len, SharedSecret, SharedInfo,
		   		 SharedInfoLen, SHA256_HashBuf, SHA256_LENGTH);
    else if (kdf == CKD_SHA384_KDF)
	return sftk_compute_ANSI_X9_63_kdf(key, key_len, SharedSecret, SharedInfo,
		   		 SharedInfoLen, SHA384_HashBuf, SHA384_LENGTH);
    else if (kdf == CKD_SHA512_KDF)
	return sftk_compute_ANSI_X9_63_kdf(key, key_len, SharedSecret, SharedInfo,
		   		 SharedInfoLen, SHA512_HashBuf, SHA512_LENGTH);
    else
	return CKR_MECHANISM_INVALID;
}
#endif 




#define NUM_MIXERS 9
static const char * const mixers[NUM_MIXERS] = { 
    "A", 
    "BB", 
    "CCC", 
    "DDDD", 
    "EEEEE", 
    "FFFFFF", 
    "GGGGGGG",
    "HHHHHHHH",
    "IIIIIIIII" };
#define SSL3_PMS_LENGTH 48
#define SSL3_MASTER_SECRET_LENGTH 48
#define SSL3_RANDOM_LENGTH 32



CK_RV NSC_DeriveKey( CK_SESSION_HANDLE hSession,
	 CK_MECHANISM_PTR pMechanism, CK_OBJECT_HANDLE hBaseKey,
	 CK_ATTRIBUTE_PTR pTemplate, CK_ULONG ulAttributeCount, 
						CK_OBJECT_HANDLE_PTR phKey)
{
    SFTKSession *   session;
    SFTKSlot    *   slot	= sftk_SlotFromSessionHandle(hSession);
    SFTKObject  *   key;
    SFTKObject  *   sourceKey;
    SFTKAttribute * att = NULL;
    SFTKAttribute * att2 = NULL;
    unsigned char * buf;
    SHA1Context *   sha;
    MD5Context *    md5;
    MD2Context *    md2;
    CK_ULONG        macSize;
    CK_ULONG        tmpKeySize;
    CK_ULONG        IVSize;
    CK_ULONG        keySize	= 0;
    CK_RV           crv 	= CKR_OK;
    CK_BBOOL        cktrue	= CK_TRUE;
    CK_KEY_TYPE     keyType	= CKK_GENERIC_SECRET;
    CK_OBJECT_CLASS classType	= CKO_SECRET_KEY;
    CK_KEY_DERIVATION_STRING_DATA *stringPtr;
    PRBool          isTLS = PR_FALSE;
    PRBool          isSHA256 = PR_FALSE;
    PRBool          isDH = PR_FALSE;
    SECStatus       rv;
    int             i;
    unsigned int    outLen;
    unsigned char   sha_out[SHA1_LENGTH];
    unsigned char   key_block[NUM_MIXERS * MD5_LENGTH];
    unsigned char   key_block2[MD5_LENGTH];
    PRBool          isFIPS;		
    HASH_HashType   hashType;
    PRBool          extractValue = PR_TRUE;

    CHECK_FORK();

    if (!slot) {
        return CKR_SESSION_HANDLE_INVALID;
    }
    


    if (phKey) *phKey = CK_INVALID_HANDLE;

    key = sftk_NewObject(slot); 
    if (key == NULL) {
	return CKR_HOST_MEMORY;
    }
    isFIPS = (slot->slotID == FIPS_SLOT_ID);

    


    for (i=0; i < (int) ulAttributeCount; i++) {
	crv = sftk_AddAttributeType(key,sftk_attr_expand(&pTemplate[i]));
	if (crv != CKR_OK) break;

	if (pTemplate[i].type == CKA_KEY_TYPE) {
	    keyType = *(CK_KEY_TYPE *)pTemplate[i].pValue;
	}
	if (pTemplate[i].type == CKA_VALUE_LEN) {
	    keySize = *(CK_ULONG *)pTemplate[i].pValue;
	}
    }
    if (crv != CKR_OK) { sftk_FreeObject(key); return crv; }

    if (keySize == 0) {
	keySize = sftk_MapKeySize(keyType);
    }

    switch (pMechanism->mechanism) {
      case CKM_NSS_JPAKE_ROUND2_SHA1:   
      case CKM_NSS_JPAKE_ROUND2_SHA256: 
      case CKM_NSS_JPAKE_ROUND2_SHA384: 
      case CKM_NSS_JPAKE_ROUND2_SHA512:
          extractValue = PR_FALSE;
          classType = CKO_PRIVATE_KEY;
          break;
      case CKM_NSS_JPAKE_FINAL_SHA1:   
      case CKM_NSS_JPAKE_FINAL_SHA256: 
      case CKM_NSS_JPAKE_FINAL_SHA384: 
      case CKM_NSS_JPAKE_FINAL_SHA512:
          extractValue = PR_FALSE;
          
      default:
          classType = CKO_SECRET_KEY;
    }
    
    crv = sftk_forceAttribute (key,CKA_CLASS,&classType,sizeof(classType));
    if (crv != CKR_OK) {
	sftk_FreeObject(key);
	return crv;
    }

     
    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) {
	sftk_FreeObject(key);
        return CKR_SESSION_HANDLE_INVALID;
    }

    sourceKey = sftk_ObjectFromHandle(hBaseKey,session);
    sftk_FreeSession(session);
    if (sourceKey == NULL) {
	sftk_FreeObject(key);
        return CKR_KEY_HANDLE_INVALID;
    }

    if (extractValue) {
        
        att = sftk_FindAttribute(sourceKey,CKA_VALUE);
        if (att == NULL) {
            sftk_FreeObject(key);
            sftk_FreeObject(sourceKey);
            return CKR_KEY_HANDLE_INVALID;
        }
    }

    switch (pMechanism->mechanism) {
    


    case CKM_NSS_TLS_MASTER_KEY_DERIVE_SHA256:
    case CKM_NSS_TLS_MASTER_KEY_DERIVE_DH_SHA256:
	isSHA256 = PR_TRUE;
	
    case CKM_TLS_MASTER_KEY_DERIVE:
    case CKM_TLS_MASTER_KEY_DERIVE_DH:
	isTLS = PR_TRUE;
	
    case CKM_SSL3_MASTER_KEY_DERIVE:
    case CKM_SSL3_MASTER_KEY_DERIVE_DH:
      {
	CK_SSL3_MASTER_KEY_DERIVE_PARAMS *ssl3_master;
	SSL3RSAPreMasterSecret *          rsa_pms;
	unsigned char                     crsrdata[SSL3_RANDOM_LENGTH * 2];

        if ((pMechanism->mechanism == CKM_SSL3_MASTER_KEY_DERIVE_DH) ||
            (pMechanism->mechanism == CKM_TLS_MASTER_KEY_DERIVE_DH) ||
            (pMechanism->mechanism == CKM_NSS_TLS_MASTER_KEY_DERIVE_DH_SHA256))
		isDH = PR_TRUE;

	
	if (!isDH && (att->attrib.ulValueLen != SSL3_PMS_LENGTH)) {
	    crv = CKR_KEY_TYPE_INCONSISTENT;
	    break;
	}
	att2 = sftk_FindAttribute(sourceKey,CKA_KEY_TYPE);
	if ((att2 == NULL) || (*(CK_KEY_TYPE *)att2->attrib.pValue !=
					CKK_GENERIC_SECRET)) {
	    if (att2) sftk_FreeAttribute(att2);
	    crv = CKR_KEY_FUNCTION_NOT_PERMITTED;
	    break;
	}
	sftk_FreeAttribute(att2);
	if (keyType != CKK_GENERIC_SECRET) {
	    crv = CKR_KEY_FUNCTION_NOT_PERMITTED;
	    break;
	}
	if ((keySize != 0) && (keySize != SSL3_MASTER_SECRET_LENGTH)) {
	    crv = CKR_KEY_FUNCTION_NOT_PERMITTED;
	    break;
	}

	
	ssl3_master = (CK_SSL3_MASTER_KEY_DERIVE_PARAMS *)
					pMechanism->pParameter;

	PORT_Memcpy(crsrdata, 
	            ssl3_master->RandomInfo.pClientRandom, SSL3_RANDOM_LENGTH);
	PORT_Memcpy(crsrdata + SSL3_RANDOM_LENGTH, 
	            ssl3_master->RandomInfo.pServerRandom, SSL3_RANDOM_LENGTH);

	if (ssl3_master->pVersion) {
	    SFTKSessionObject *sessKey = sftk_narrowToSessionObject(key);
	    rsa_pms = (SSL3RSAPreMasterSecret *) att->attrib.pValue;
	    
	    if ((sessKey == NULL) || sessKey->wasDerived) {
		ssl3_master->pVersion->major = 0xff;
		ssl3_master->pVersion->minor = 0xff;
	    } else {
		ssl3_master->pVersion->major = rsa_pms->client_version[0];
		ssl3_master->pVersion->minor = rsa_pms->client_version[1];
	    }
	}
	if (ssl3_master->RandomInfo.ulClientRandomLen != SSL3_RANDOM_LENGTH) {
	   crv = CKR_MECHANISM_PARAM_INVALID;
	   break;
	}
	if (ssl3_master->RandomInfo.ulServerRandomLen != SSL3_RANDOM_LENGTH) {
	   crv = CKR_MECHANISM_PARAM_INVALID;
	   break;
	}

        if (isTLS) {
	    SECStatus status;
 	    SECItem crsr   = { siBuffer, NULL, 0 };
 	    SECItem master = { siBuffer, NULL, 0 };
 	    SECItem pms    = { siBuffer, NULL, 0 };

 	    crsr.data   = crsrdata;
	    crsr.len    = sizeof crsrdata;
 	    master.data = key_block;
	    master.len  = SSL3_MASTER_SECRET_LENGTH;
 	    pms.data    = (unsigned char*)att->attrib.pValue;
	    pms.len     =                 att->attrib.ulValueLen;

	    if (isSHA256) {
		status = TLS_P_hash(HASH_AlgSHA256, &pms, "master secret",
				    &crsr, &master, isFIPS);
	    } else {
		status = TLS_PRF(&pms, "master secret", &crsr, &master, isFIPS);
	    }
	    if (status != SECSuccess) {
	    	crv = CKR_FUNCTION_FAILED;
		break;
	    }
	} else {
	    
	    md5 = MD5_NewContext();
	    if (md5 == NULL) { 
		crv = CKR_HOST_MEMORY;
		break;
	    }
	    sha = SHA1_NewContext();
	    if (sha == NULL) { 
		PORT_Free(md5);
		crv = CKR_HOST_MEMORY;
		break;
	    }
            for (i = 0; i < 3; i++) {
              SHA1_Begin(sha);
              SHA1_Update(sha, (unsigned char*) mixers[i], strlen(mixers[i]));
              SHA1_Update(sha, (const unsigned char*)att->attrib.pValue, 
			  att->attrib.ulValueLen);
              SHA1_Update(sha, crsrdata, sizeof crsrdata);
              SHA1_End(sha, sha_out, &outLen, SHA1_LENGTH);
              PORT_Assert(outLen == SHA1_LENGTH);

              MD5_Begin(md5);
              MD5_Update(md5, (const unsigned char*)att->attrib.pValue, 
			 att->attrib.ulValueLen);
              MD5_Update(md5, sha_out, outLen);
              MD5_End(md5, &key_block[i*MD5_LENGTH], &outLen, MD5_LENGTH);
              PORT_Assert(outLen == MD5_LENGTH);
            }
	    PORT_Free(md5);
	    PORT_Free(sha);
	}

	
	crv = sftk_forceAttribute
			(key,CKA_VALUE,key_block,SSL3_MASTER_SECRET_LENGTH);
	if (crv != CKR_OK) break;
	keyType = CKK_GENERIC_SECRET;
	crv = sftk_forceAttribute (key,CKA_KEY_TYPE,&keyType,sizeof(keyType));
	if (isTLS) {
	    
	    

	    crv = sftk_forceAttribute(key,CKA_SIGN,  &cktrue,sizeof(CK_BBOOL));
	    if (crv != CKR_OK) break;
	    crv = sftk_forceAttribute(key,CKA_VERIFY,&cktrue,sizeof(CK_BBOOL));
	    if (crv != CKR_OK) break;
	    
	    crv = sftk_forceAttribute(key,CKA_DERIVE,&cktrue,sizeof(CK_BBOOL));
	    if (crv != CKR_OK) break;
	}
	break;
      }

    case CKM_NSS_TLS_KEY_AND_MAC_DERIVE_SHA256:
	isSHA256 = PR_TRUE;
	
    case CKM_TLS_KEY_AND_MAC_DERIVE:
	isTLS = PR_TRUE;
	
    case CKM_SSL3_KEY_AND_MAC_DERIVE:
      {
	CK_SSL3_KEY_MAT_PARAMS *ssl3_keys;
	CK_SSL3_KEY_MAT_OUT *   ssl3_keys_out;
	CK_ULONG                effKeySize;
	unsigned int            block_needed;
	unsigned char           srcrdata[SSL3_RANDOM_LENGTH * 2];
	unsigned char           crsrdata[SSL3_RANDOM_LENGTH * 2];

	crv = sftk_DeriveSensitiveCheck(sourceKey,key);
	if (crv != CKR_OK) break;

	if (att->attrib.ulValueLen != SSL3_MASTER_SECRET_LENGTH) {
	    crv = CKR_KEY_FUNCTION_NOT_PERMITTED;
	    break;
	}
	att2 = sftk_FindAttribute(sourceKey,CKA_KEY_TYPE);
	if ((att2 == NULL) || (*(CK_KEY_TYPE *)att2->attrib.pValue !=
					CKK_GENERIC_SECRET)) {
	    if (att2) sftk_FreeAttribute(att2);
	    crv = CKR_KEY_FUNCTION_NOT_PERMITTED;
	    break;
	}
	sftk_FreeAttribute(att2);
	md5 = MD5_NewContext();
	if (md5 == NULL) { 
	    crv = CKR_HOST_MEMORY;
	    break;
	}
	sha = SHA1_NewContext();
	if (sha == NULL) { 
	    PORT_Free(md5);
	    crv = CKR_HOST_MEMORY;
	    break;
	}
	ssl3_keys = (CK_SSL3_KEY_MAT_PARAMS *) pMechanism->pParameter;

	PORT_Memcpy(srcrdata, 
	            ssl3_keys->RandomInfo.pServerRandom, SSL3_RANDOM_LENGTH);
	PORT_Memcpy(srcrdata + SSL3_RANDOM_LENGTH, 
		    ssl3_keys->RandomInfo.pClientRandom, SSL3_RANDOM_LENGTH);

	PORT_Memcpy(crsrdata, 
		    ssl3_keys->RandomInfo.pClientRandom, SSL3_RANDOM_LENGTH);
	PORT_Memcpy(crsrdata + SSL3_RANDOM_LENGTH, 
		    ssl3_keys->RandomInfo.pServerRandom, SSL3_RANDOM_LENGTH);

	


	ssl3_keys_out = ssl3_keys->pReturnedKeyMaterial;
	ssl3_keys_out->hClientMacSecret = CK_INVALID_HANDLE;
	ssl3_keys_out->hServerMacSecret = CK_INVALID_HANDLE;
	ssl3_keys_out->hClientKey       = CK_INVALID_HANDLE;
	ssl3_keys_out->hServerKey       = CK_INVALID_HANDLE;

	


	macSize    = ssl3_keys->ulMacSizeInBits/8;
	effKeySize = ssl3_keys->ulKeySizeInBits/8;
	IVSize     = ssl3_keys->ulIVSizeInBits/8;
	if (keySize == 0) {
	    effKeySize = keySize;
	}
	block_needed = 2 * (macSize + effKeySize + 
	                    ((!ssl3_keys->bIsExport) * IVSize));
	PORT_Assert(block_needed <= sizeof key_block);
	if (block_needed > sizeof key_block)
	    block_needed = sizeof key_block;

	



	if (isTLS) {
	    SECStatus     status;
	    SECItem       srcr   = { siBuffer, NULL, 0 };
	    SECItem       keyblk = { siBuffer, NULL, 0 };
	    SECItem       master = { siBuffer, NULL, 0 }; 

	    srcr.data   = srcrdata;
	    srcr.len    = sizeof srcrdata;
	    keyblk.data = key_block;
	    keyblk.len  = block_needed;
	    master.data = (unsigned char*)att->attrib.pValue;
	    master.len  =                 att->attrib.ulValueLen;

	    if (isSHA256) {
		status = TLS_P_hash(HASH_AlgSHA256, &master, "key expansion",
				    &srcr, &keyblk, isFIPS);
	    } else {
		status = TLS_PRF(&master, "key expansion", &srcr, &keyblk,
				 isFIPS);
	    }
	    if (status != SECSuccess) {
		goto key_and_mac_derive_fail;
	    }
	} else {
	    unsigned int block_bytes = 0;
	    








	    for (i = 0; i < NUM_MIXERS && block_bytes < block_needed; i++) {
	      SHA1_Begin(sha);
	      SHA1_Update(sha, (unsigned char*) mixers[i], strlen(mixers[i]));
	      SHA1_Update(sha, (const unsigned char*)att->attrib.pValue, 
			  att->attrib.ulValueLen);
	      SHA1_Update(sha, srcrdata, sizeof srcrdata);
	      SHA1_End(sha, sha_out, &outLen, SHA1_LENGTH);
	      PORT_Assert(outLen == SHA1_LENGTH);
	      MD5_Begin(md5);
	      MD5_Update(md5, (const unsigned char*)att->attrib.pValue,
			 att->attrib.ulValueLen);
	      MD5_Update(md5, sha_out, outLen);
	      MD5_End(md5, &key_block[i*MD5_LENGTH], &outLen, MD5_LENGTH);
	      PORT_Assert(outLen == MD5_LENGTH);
	      block_bytes += outLen;
	    }
	}

	


	i = 0;			

	



	crv = sftk_buildSSLKey(hSession,key,PR_TRUE,&key_block[i],macSize,
					 &ssl3_keys_out->hClientMacSecret);
	if (crv != CKR_OK)
	    goto key_and_mac_derive_fail;

	i += macSize;

	


	crv = sftk_buildSSLKey(hSession,key,PR_TRUE,&key_block[i],macSize,
					    &ssl3_keys_out->hServerMacSecret);
	if (crv != CKR_OK) {
	    goto key_and_mac_derive_fail;
	}
	i += macSize;

	if (keySize) {
	    if (!ssl3_keys->bIsExport) {
		



		crv = sftk_buildSSLKey(hSession,key,PR_FALSE,&key_block[i],
					keySize, &ssl3_keys_out->hClientKey);
		if (crv != CKR_OK) {
		    goto key_and_mac_derive_fail;
		}
		i += keySize;

		


		crv = sftk_buildSSLKey(hSession,key,PR_FALSE,&key_block[i],
					keySize, &ssl3_keys_out->hServerKey);
		if (crv != CKR_OK) {
		    goto key_and_mac_derive_fail;
		}
		i += keySize;

		


		if (IVSize > 0) {
		    PORT_Memcpy(ssl3_keys_out->pIVClient, 
		                &key_block[i], IVSize);
		    i += IVSize;
		}

		


		if (IVSize > 0) {
		    PORT_Memcpy(ssl3_keys_out->pIVServer, 
		                &key_block[i], IVSize);
		    i += IVSize;
		}
		PORT_Assert(i <= sizeof key_block);

	    } else if (!isTLS) {

		





		MD5_Begin(md5);
		MD5_Update(md5, &key_block[i], effKeySize);
            	MD5_Update(md5, crsrdata, sizeof crsrdata);
		MD5_End(md5, key_block2, &outLen, MD5_LENGTH);
		i += effKeySize;
		crv = sftk_buildSSLKey(hSession,key,PR_FALSE,key_block2,
				 	keySize,&ssl3_keys_out->hClientKey);
		if (crv != CKR_OK) {
		    goto key_and_mac_derive_fail;
		}

		




		MD5_Begin(md5);
		MD5_Update(md5, &key_block[i], effKeySize);
            	MD5_Update(md5, srcrdata, sizeof srcrdata);
		MD5_End(md5, key_block2, &outLen, MD5_LENGTH);
		i += effKeySize;
		crv = sftk_buildSSLKey(hSession,key,PR_FALSE,key_block2,
					 keySize,&ssl3_keys_out->hServerKey);
		if (crv != CKR_OK) {
		    goto key_and_mac_derive_fail;
		}

		



		MD5_Begin(md5);
            	MD5_Update(md5, crsrdata, sizeof crsrdata);
		MD5_End(md5, key_block2, &outLen, MD5_LENGTH);
		PORT_Memcpy(ssl3_keys_out->pIVClient, key_block2, IVSize);

		



		MD5_Begin(md5);
            	MD5_Update(md5, srcrdata, sizeof srcrdata);
		MD5_End(md5, key_block2, &outLen, MD5_LENGTH);
		PORT_Memcpy(ssl3_keys_out->pIVServer, key_block2, IVSize);

	    } else {

		


		SECStatus     status;
		SECItem       secret = { siBuffer, NULL, 0 };
		SECItem       crsr   = { siBuffer, NULL, 0 };
		SECItem       keyblk = { siBuffer, NULL, 0 };

		





		secret.data = &key_block[i];
		secret.len  = effKeySize;
		i          += effKeySize;
		crsr.data   = crsrdata;
		crsr.len    = sizeof crsrdata;
		keyblk.data = key_block2;
		keyblk.len  = sizeof key_block2;
		status = TLS_PRF(&secret, "client write key", &crsr, &keyblk,
				  isFIPS);
		if (status != SECSuccess) {
		    goto key_and_mac_derive_fail;
		}
		crv = sftk_buildSSLKey(hSession, key, PR_FALSE, key_block2, 
				       keySize, &ssl3_keys_out->hClientKey);
		if (crv != CKR_OK) {
		    goto key_and_mac_derive_fail;
		}

		





		secret.data = &key_block[i];
		secret.len  = effKeySize;
		i          += effKeySize;
		keyblk.data = key_block2;
		keyblk.len  = sizeof key_block2;
		status = TLS_PRF(&secret, "server write key", &crsr, &keyblk,
				  isFIPS);
		if (status != SECSuccess) {
		    goto key_and_mac_derive_fail;
		}
		crv = sftk_buildSSLKey(hSession, key, PR_FALSE, key_block2, 
				       keySize, &ssl3_keys_out->hServerKey);
		if (crv != CKR_OK) {
		    goto key_and_mac_derive_fail;
		}

		





		if (IVSize) {
		    secret.data = NULL;
		    secret.len  = 0;
		    keyblk.data = &key_block[i];
		    keyblk.len  = 2 * IVSize;
		    status = TLS_PRF(&secret, "IV block", &crsr, &keyblk,
				      isFIPS);
		    if (status != SECSuccess) {
			goto key_and_mac_derive_fail;
		    }
		    PORT_Memcpy(ssl3_keys_out->pIVClient, keyblk.data, IVSize);
		    PORT_Memcpy(ssl3_keys_out->pIVServer, keyblk.data + IVSize,
                                IVSize);
		}
	    }
	}

	crv = CKR_OK;

	if (0) {
key_and_mac_derive_fail:
	    if (crv == CKR_OK)
	    	crv = CKR_FUNCTION_FAILED;
	    sftk_freeSSLKeys(hSession, ssl3_keys_out);
	}
	MD5_DestroyContext(md5, PR_TRUE);
	SHA1_DestroyContext(sha, PR_TRUE);
	sftk_FreeObject(key);
	key = NULL;
	break;
      }

    case CKM_CONCATENATE_BASE_AND_KEY:
      {
	SFTKObject *newKey;

	crv = sftk_DeriveSensitiveCheck(sourceKey,key);
	if (crv != CKR_OK) break;

	session = sftk_SessionFromHandle(hSession);
	if (session == NULL) {
            crv = CKR_SESSION_HANDLE_INVALID;
	    break;
    	}

	newKey = sftk_ObjectFromHandle(*(CK_OBJECT_HANDLE *)
					pMechanism->pParameter,session);
	sftk_FreeSession(session);
	if ( newKey == NULL) {
            crv = CKR_KEY_HANDLE_INVALID;
	    break;
	}

	if (sftk_isTrue(newKey,CKA_SENSITIVE)) {
	    crv = sftk_forceAttribute(newKey,CKA_SENSITIVE,&cktrue,
							sizeof(CK_BBOOL));
	    if (crv != CKR_OK) {
		sftk_FreeObject(newKey);
		break;
	    }
	}

	att2 = sftk_FindAttribute(newKey,CKA_VALUE);
	if (att2 == NULL) {
	    sftk_FreeObject(newKey);
            crv = CKR_KEY_HANDLE_INVALID;
	    break;
	}
	tmpKeySize = att->attrib.ulValueLen+att2->attrib.ulValueLen;
	if (keySize == 0) keySize = tmpKeySize;
	if (keySize > tmpKeySize) {
	    sftk_FreeObject(newKey);
	    sftk_FreeAttribute(att2);
	    crv = CKR_TEMPLATE_INCONSISTENT;
	    break;
	}
	buf = (unsigned char*)PORT_Alloc(tmpKeySize);
	if (buf == NULL) {
	    sftk_FreeAttribute(att2);
	    sftk_FreeObject(newKey);
	    crv = CKR_HOST_MEMORY;	
	    break;
	}

	PORT_Memcpy(buf,att->attrib.pValue,att->attrib.ulValueLen);
	PORT_Memcpy(buf+att->attrib.ulValueLen,
				att2->attrib.pValue,att2->attrib.ulValueLen);

	crv = sftk_forceAttribute (key,CKA_VALUE,buf,keySize);
	PORT_ZFree(buf,tmpKeySize);
	sftk_FreeAttribute(att2);
	sftk_FreeObject(newKey);
	break;
      }

    case CKM_CONCATENATE_BASE_AND_DATA:
	crv = sftk_DeriveSensitiveCheck(sourceKey,key);
	if (crv != CKR_OK) break;

	stringPtr = (CK_KEY_DERIVATION_STRING_DATA *) pMechanism->pParameter;
	tmpKeySize = att->attrib.ulValueLen+stringPtr->ulLen;
	if (keySize == 0) keySize = tmpKeySize;
	if (keySize > tmpKeySize) {
	    crv = CKR_TEMPLATE_INCONSISTENT;
	    break;
	}
	buf = (unsigned char*)PORT_Alloc(tmpKeySize);
	if (buf == NULL) {
	    crv = CKR_HOST_MEMORY;
	    break;
	}

	PORT_Memcpy(buf,att->attrib.pValue,att->attrib.ulValueLen);
	PORT_Memcpy(buf+att->attrib.ulValueLen,stringPtr->pData,
							stringPtr->ulLen);

	crv = sftk_forceAttribute (key,CKA_VALUE,buf,keySize);
	PORT_ZFree(buf,tmpKeySize);
	break;
    case CKM_CONCATENATE_DATA_AND_BASE:
	crv = sftk_DeriveSensitiveCheck(sourceKey,key);
	if (crv != CKR_OK) break;

	stringPtr = (CK_KEY_DERIVATION_STRING_DATA *)pMechanism->pParameter;
	tmpKeySize = att->attrib.ulValueLen+stringPtr->ulLen;
	if (keySize == 0) keySize = tmpKeySize;
	if (keySize > tmpKeySize) {
	    crv = CKR_TEMPLATE_INCONSISTENT;
	    break;
	}
	buf = (unsigned char*)PORT_Alloc(tmpKeySize);
	if (buf == NULL) {
	    crv = CKR_HOST_MEMORY;
	    break;
	}

	PORT_Memcpy(buf,stringPtr->pData,stringPtr->ulLen);
	PORT_Memcpy(buf+stringPtr->ulLen,att->attrib.pValue,
							att->attrib.ulValueLen);

	crv = sftk_forceAttribute (key,CKA_VALUE,buf,keySize);
	PORT_ZFree(buf,tmpKeySize);
	break;
    case CKM_XOR_BASE_AND_DATA:
	crv = sftk_DeriveSensitiveCheck(sourceKey,key);
	if (crv != CKR_OK) break;

	stringPtr = (CK_KEY_DERIVATION_STRING_DATA *)pMechanism->pParameter;
	tmpKeySize = PR_MIN(att->attrib.ulValueLen,stringPtr->ulLen);
	if (keySize == 0) keySize = tmpKeySize;
	if (keySize > tmpKeySize) {
	    crv = CKR_TEMPLATE_INCONSISTENT;
	    break;
	}
	buf = (unsigned char*)PORT_Alloc(keySize);
	if (buf == NULL) {
	    crv = CKR_HOST_MEMORY;
	    break;
	}

	
	PORT_Memcpy(buf,att->attrib.pValue,keySize);
	for (i=0; i < (int)keySize; i++) {
	    buf[i] ^= stringPtr->pData[i];
	}

	crv = sftk_forceAttribute (key,CKA_VALUE,buf,keySize);
	PORT_ZFree(buf,keySize);
	break;

    case CKM_EXTRACT_KEY_FROM_KEY:
      {
	
	CK_ULONG extract = *(CK_EXTRACT_PARAMS *)pMechanism->pParameter;
	CK_ULONG shift   = extract & 0x7;      
	CK_ULONG offset  = extract >> 3;       

	crv = sftk_DeriveSensitiveCheck(sourceKey,key);
	if (crv != CKR_OK) break;

	if (keySize == 0)  {
	    crv = CKR_TEMPLATE_INCOMPLETE;
	    break;
	}
	
	if (att->attrib.ulValueLen < 
			(offset + keySize + ((shift != 0)? 1 :0)) ) {
	    crv = CKR_MECHANISM_PARAM_INVALID;
	    break;
	}
	buf = (unsigned char*)PORT_Alloc(keySize);
	if (buf == NULL) {
	    crv = CKR_HOST_MEMORY;
	    break;
	}

		
	for (i=0; i < (int)keySize; i++) {
	    unsigned char *value =
			 ((unsigned char *)att->attrib.pValue)+offset+i;
	    if (shift) {
	        buf[i] = (value[0] << (shift)) | (value[1] >> (8 - shift));
	    } else {
		buf[i] = value[0];
	    }
	}

	crv = sftk_forceAttribute (key,CKA_VALUE,buf,keySize);
	PORT_ZFree(buf,keySize);
	break;
      }
    case CKM_MD2_KEY_DERIVATION:
	if (keySize == 0) keySize = MD2_LENGTH;
	if (keySize > MD2_LENGTH) {
	    crv = CKR_TEMPLATE_INCONSISTENT;
	    break;
	}
	
	md2 = MD2_NewContext();
	if (md2 == NULL) { 
	    crv = CKR_HOST_MEMORY;
	    break;
	}
	MD2_Begin(md2);
	MD2_Update(md2,(const unsigned char*)att->attrib.pValue,
		   att->attrib.ulValueLen);
	MD2_End(md2,key_block,&outLen,MD2_LENGTH);
	MD2_DestroyContext(md2, PR_TRUE);

	crv = sftk_forceAttribute (key,CKA_VALUE,key_block,keySize);
	break;
    case CKM_MD5_KEY_DERIVATION:
	if (keySize == 0) keySize = MD5_LENGTH;
	if (keySize > MD5_LENGTH) {
	    crv = CKR_TEMPLATE_INCONSISTENT;
	    break;
	}
	MD5_HashBuf(key_block,(const unsigned char*)att->attrib.pValue,
		   att->attrib.ulValueLen);

	crv = sftk_forceAttribute (key,CKA_VALUE,key_block,keySize);
	break;
     case CKM_SHA1_KEY_DERIVATION:
	if (keySize == 0) keySize = SHA1_LENGTH;
	if (keySize > SHA1_LENGTH) {
	    crv = CKR_TEMPLATE_INCONSISTENT;
	    break;
	}
	SHA1_HashBuf(key_block,(const unsigned char*)att->attrib.pValue,
		    att->attrib.ulValueLen);

	crv = sftk_forceAttribute(key,CKA_VALUE,key_block,keySize);
	break;

     case CKM_SHA224_KEY_DERIVATION:
	if (keySize == 0) keySize = SHA224_LENGTH;
	if (keySize > SHA224_LENGTH) {
	    crv = CKR_TEMPLATE_INCONSISTENT;
	    break;
	}
	SHA224_HashBuf(key_block,(const unsigned char*)att->attrib.pValue,
		    att->attrib.ulValueLen);

	crv = sftk_forceAttribute(key,CKA_VALUE,key_block,keySize);
	break;

     case CKM_SHA256_KEY_DERIVATION:
	if (keySize == 0) keySize = SHA256_LENGTH;
	if (keySize > SHA256_LENGTH) {
	    crv = CKR_TEMPLATE_INCONSISTENT;
	    break;
	}
	SHA256_HashBuf(key_block,(const unsigned char*)att->attrib.pValue,
		    att->attrib.ulValueLen);

	crv = sftk_forceAttribute(key,CKA_VALUE,key_block,keySize);
	break;

     case CKM_SHA384_KEY_DERIVATION:
	if (keySize == 0) keySize = SHA384_LENGTH;
	if (keySize > SHA384_LENGTH) {
	    crv = CKR_TEMPLATE_INCONSISTENT;
	    break;
	}
	SHA384_HashBuf(key_block,(const unsigned char*)att->attrib.pValue,
		    att->attrib.ulValueLen);

	crv = sftk_forceAttribute(key,CKA_VALUE,key_block,keySize);
	break;

     case CKM_SHA512_KEY_DERIVATION:
	if (keySize == 0) keySize = SHA512_LENGTH;
	if (keySize > SHA512_LENGTH) {
	    crv = CKR_TEMPLATE_INCONSISTENT;
	    break;
	}
	SHA512_HashBuf(key_block,(const unsigned char*)att->attrib.pValue,
		    att->attrib.ulValueLen);

	crv = sftk_forceAttribute(key,CKA_VALUE,key_block,keySize);
	break;

    case CKM_DH_PKCS_DERIVE:
      {
	SECItem  derived,  dhPublic;
	SECItem  dhPrime,  dhValue;
	
	
	crv = sftk_Attribute2SecItem(NULL, &dhPrime, sourceKey, CKA_PRIME); 
	if (crv != SECSuccess) break;
	crv = sftk_Attribute2SecItem(NULL, &dhValue, sourceKey, CKA_VALUE); 
	if (crv != SECSuccess) {
 	    PORT_Free(dhPrime.data);
	    break;
	}

	dhPublic.data = pMechanism->pParameter;
	dhPublic.len  = pMechanism->ulParameterLen;

	
	rv = DH_Derive(&dhPublic, &dhPrime, &dhValue, &derived, keySize); 

	PORT_Free(dhPrime.data);
	PORT_Free(dhValue.data);
     
	if (rv == SECSuccess) {
	    sftk_forceAttribute(key, CKA_VALUE, derived.data, derived.len);
	    PORT_ZFree(derived.data, derived.len);
	} else
	    crv = CKR_HOST_MEMORY;
	    
	break;
      }

#ifndef NSS_DISABLE_ECC
    case CKM_ECDH1_DERIVE:
    case CKM_ECDH1_COFACTOR_DERIVE:
      {
	SECItem  ecScalar, ecPoint;
	SECItem  tmp;
	PRBool   withCofactor = PR_FALSE;
	unsigned char *secret;
	unsigned char *keyData = NULL;
	int secretlen, curveLen, pubKeyLen;
	CK_ECDH1_DERIVE_PARAMS *mechParams;
	NSSLOWKEYPrivateKey *privKey;
	PLArenaPool *arena = NULL;

	
	mechParams = (CK_ECDH1_DERIVE_PARAMS *) pMechanism->pParameter;
	if ((pMechanism->ulParameterLen != sizeof(CK_ECDH1_DERIVE_PARAMS)) ||
	    ((mechParams->kdf == CKD_NULL) &&
		((mechParams->ulSharedDataLen != 0) || 
		    (mechParams->pSharedData != NULL)))) {
	    crv = CKR_MECHANISM_PARAM_INVALID;
	    break;
	}

	privKey = sftk_GetPrivKey(sourceKey, CKK_EC, &crv);
	if (privKey == NULL) {
	    break;
	}

	
	SECITEM_CopyItem(NULL, &ecScalar, &privKey->u.ec.privateValue);

	ecPoint.data = mechParams->pPublicData;
	ecPoint.len  = mechParams->ulPublicDataLen;

	curveLen = (privKey->u.ec.ecParams.fieldID.size +7)/8;
	pubKeyLen = (2*curveLen) + 1;

	
	if (ecPoint.len < pubKeyLen) {
	    goto ec_loser;
	}
	

	if (ecPoint.len > pubKeyLen) {
	    SECItem newPoint;

	    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
	    if (arena == NULL) {
		goto ec_loser;
	    }

	    rv = SEC_QuickDERDecodeItem(arena, &newPoint, 
					SEC_ASN1_GET(SEC_OctetStringTemplate), 
					&ecPoint);
	    if (rv != SECSuccess) {
		goto ec_loser;
	    }
	    ecPoint = newPoint;
	}

	if (pMechanism->mechanism == CKM_ECDH1_COFACTOR_DERIVE) {
	    withCofactor = PR_TRUE;
	} else {
	    



	    if (EC_ValidatePublicKey(&privKey->u.ec.ecParams, &ecPoint) 
		!= SECSuccess) {
		goto ec_loser;
	    }
	}

	rv = ECDH_Derive(&ecPoint, &privKey->u.ec.ecParams, &ecScalar,
	                 withCofactor, &tmp); 
	PORT_Free(ecScalar.data);
	ecScalar.data = NULL;
	if (privKey != sourceKey->objectInfo) {
	   nsslowkey_DestroyPrivateKey(privKey);
	   privKey=NULL;
	}
	if (arena) {
	    PORT_FreeArena(arena,PR_FALSE);
	    arena=NULL;
	}

	if (rv != SECSuccess) {
	    crv = sftk_MapCryptError(PORT_GetError());
	    break;
	}


	


	if (mechParams->kdf == CKD_NULL) {
	    




	    secret = tmp.data;
	    secretlen = tmp.len;
	} else {
	    secretlen = keySize;
	    crv = sftk_ANSI_X9_63_kdf(&secret, keySize,
			&tmp, mechParams->pSharedData,
			mechParams->ulSharedDataLen, mechParams->kdf);
	    PORT_ZFree(tmp.data, tmp.len);
	    if (crv != CKR_OK) {
		break;
	    }
	    tmp.data = secret;
	    tmp.len = secretlen;
	}

	






	if (keySize) {
	    if (secretlen < keySize) {
	        keyData = PORT_ZAlloc(keySize);
		if (!keyData) {
		    PORT_ZFree(tmp.data, tmp.len);
		    crv = CKR_HOST_MEMORY;
		    break;
		}
		PORT_Memcpy(&keyData[keySize-secretlen],secret,secretlen);
		secret = keyData;
	    } else {
		secret += (secretlen - keySize);
	    }
	    secretlen = keySize;
	}

	sftk_forceAttribute(key, CKA_VALUE, secret, secretlen);
	PORT_ZFree(tmp.data, tmp.len);
	if (keyData) {
	    PORT_ZFree(keyData, keySize);
	}
	break;

ec_loser:
	crv = CKR_ARGUMENTS_BAD;
	PORT_Free(ecScalar.data);
	if (privKey != sourceKey->objectInfo)
	    nsslowkey_DestroyPrivateKey(privKey);
	if (arena) {
	    PORT_FreeArena(arena, PR_FALSE);
	}
	break;

      }
#endif 

    
    case CKM_NSS_HKDF_SHA1:   hashType = HASH_AlgSHA1;   goto hkdf;
    case CKM_NSS_HKDF_SHA256: hashType = HASH_AlgSHA256; goto hkdf;
    case CKM_NSS_HKDF_SHA384: hashType = HASH_AlgSHA384; goto hkdf;
    case CKM_NSS_HKDF_SHA512: hashType = HASH_AlgSHA512; goto hkdf;
hkdf: {
        const CK_NSS_HKDFParams * params =
            (const CK_NSS_HKDFParams *) pMechanism->pParameter;
        const SECHashObject * rawHash;
        unsigned hashLen;
        CK_BYTE buf[HASH_LENGTH_MAX];
        CK_BYTE * prk;  
        CK_ULONG prkLen;
        CK_BYTE * okm;  

        rawHash = HASH_GetRawHashObject(hashType);
        if (rawHash == NULL || rawHash->length > sizeof buf) {
            crv = CKR_FUNCTION_FAILED;
            break;
        }
        hashLen = rawHash->length;

        if (pMechanism->ulParameterLen != sizeof(CK_NSS_HKDFParams) ||
            !params || (!params->bExpand && !params->bExtract) ||
            (params->bExtract && params->ulSaltLen > 0 && !params->pSalt) ||
            (params->bExpand && params->ulInfoLen > 0 && !params->pInfo)) {
            crv = CKR_MECHANISM_PARAM_INVALID;
            break;
        }
        if (keySize == 0 || keySize > sizeof key_block ||
            (!params->bExpand && keySize > hashLen) ||
            (params->bExpand && keySize > 255 * hashLen)) {
            crv = CKR_TEMPLATE_INCONSISTENT;
            break;
        }
        crv = sftk_DeriveSensitiveCheck(sourceKey, key);
        if (crv != CKR_OK)
            break;

        
        if (params->bExtract) {
            CK_BYTE * salt;
            CK_ULONG saltLen;
            HMACContext * hmac;
            unsigned int bufLen;

            salt = params->pSalt;
            saltLen = params->ulSaltLen;
            if (salt == NULL) {
                saltLen = hashLen;
                salt = buf;
                memset(salt, 0, saltLen);
            }
            hmac = HMAC_Create(rawHash, salt, saltLen, isFIPS);
            if (!hmac) {
                crv = CKR_HOST_MEMORY;
                break;
            }
            HMAC_Begin(hmac);
            HMAC_Update(hmac, (const unsigned char*) att->attrib.pValue,
		        att->attrib.ulValueLen);
            HMAC_Finish(hmac, buf, &bufLen, sizeof(buf));
            HMAC_Destroy(hmac, PR_TRUE);
            PORT_Assert(bufLen == rawHash->length);
            prk = buf;
            prkLen = bufLen;
        } else {
            
            prk = (CK_BYTE*) att->attrib.pValue;
            prkLen = att->attrib.ulValueLen;
        }
        
        
        if (!params->bExpand) {
            okm = prk;
        } else {
            



            HMACContext * hmac;
            CK_BYTE i;
            unsigned iterations = PR_ROUNDUP(keySize, hashLen) / hashLen;
            hmac = HMAC_Create(rawHash, prk, prkLen, isFIPS);
            if (hmac == NULL) {
                crv = CKR_HOST_MEMORY;
                break;
            }
            for (i = 1; i <= iterations; ++i) {
                unsigned len;
                HMAC_Begin(hmac);
                if (i > 1) {
                    HMAC_Update(hmac, key_block + ((i-2) * hashLen), hashLen);
                }
                if (params->ulInfoLen != 0) {
                    HMAC_Update(hmac, params->pInfo, params->ulInfoLen);
                }
                HMAC_Update(hmac, &i, 1);
                HMAC_Finish(hmac, key_block + ((i-1) * hashLen), &len,
                            hashLen);
                PORT_Assert(len == hashLen);
            }
            HMAC_Destroy(hmac, PR_TRUE);
            okm = key_block;
        }
        
        crv = sftk_forceAttribute(key, CKA_VALUE, okm, keySize);
        break;
      } 

    case CKM_NSS_JPAKE_ROUND2_SHA1: hashType = HASH_AlgSHA1; goto jpake2;
    case CKM_NSS_JPAKE_ROUND2_SHA256: hashType = HASH_AlgSHA256; goto jpake2;
    case CKM_NSS_JPAKE_ROUND2_SHA384: hashType = HASH_AlgSHA384; goto jpake2;
    case CKM_NSS_JPAKE_ROUND2_SHA512: hashType = HASH_AlgSHA512; goto jpake2;
jpake2:
        if (pMechanism->pParameter == NULL ||
            pMechanism->ulParameterLen != sizeof(CK_NSS_JPAKERound2Params))
            crv = CKR_MECHANISM_PARAM_INVALID;
        if (crv == CKR_OK && sftk_isTrue(key, CKA_TOKEN))
            crv = CKR_TEMPLATE_INCONSISTENT;
	if (crv == CKR_OK)
            crv = sftk_DeriveSensitiveCheck(sourceKey, key);
	if (crv == CKR_OK)
            crv = jpake_Round2(hashType,
                        (CK_NSS_JPAKERound2Params *) pMechanism->pParameter,
                        sourceKey, key);
        break;

    case CKM_NSS_JPAKE_FINAL_SHA1: hashType = HASH_AlgSHA1; goto jpakeFinal;
    case CKM_NSS_JPAKE_FINAL_SHA256: hashType = HASH_AlgSHA256; goto jpakeFinal;
    case CKM_NSS_JPAKE_FINAL_SHA384: hashType = HASH_AlgSHA384; goto jpakeFinal;
    case CKM_NSS_JPAKE_FINAL_SHA512: hashType = HASH_AlgSHA512; goto jpakeFinal;
jpakeFinal:
        if (pMechanism->pParameter == NULL ||
            pMechanism->ulParameterLen != sizeof(CK_NSS_JPAKEFinalParams))
            crv = CKR_MECHANISM_PARAM_INVALID;
        






        if (crv == CKR_OK)
            crv = jpake_Final(hashType,
                        (CK_NSS_JPAKEFinalParams *) pMechanism->pParameter,
                        sourceKey, key);
        break;

    default:
	crv = CKR_MECHANISM_INVALID;
    }
    if (att) {
        sftk_FreeAttribute(att);
    }
    sftk_FreeObject(sourceKey);
    if (crv != CKR_OK) { 
	if (key) sftk_FreeObject(key);
	return crv;
    }

    
    if (key) {
	SFTKSessionObject *sessKey = sftk_narrowToSessionObject(key);
	PORT_Assert(sessKey);
	
	sessKey->wasDerived = PR_TRUE;
	session = sftk_SessionFromHandle(hSession);
	if (session == NULL) {
	    sftk_FreeObject(key);
	    return CKR_HOST_MEMORY;
	}

	crv = sftk_handleObject(key,session);
	sftk_FreeSession(session);
	*phKey = key->handle;
	sftk_FreeObject(key);
    }
    return crv;
}




CK_RV NSC_GetFunctionStatus(CK_SESSION_HANDLE hSession)
{
    CHECK_FORK();

    return CKR_FUNCTION_NOT_PARALLEL;
}


CK_RV NSC_CancelFunction(CK_SESSION_HANDLE hSession)
{
    CHECK_FORK();

    return CKR_FUNCTION_NOT_PARALLEL;
}






CK_RV NSC_GetOperationState(CK_SESSION_HANDLE hSession, 
	CK_BYTE_PTR  pOperationState, CK_ULONG_PTR pulOperationStateLen)
{
    SFTKSessionContext *context;
    SFTKSession *session;
    CK_RV crv;
    CK_ULONG pOSLen = *pulOperationStateLen;

    CHECK_FORK();

    
    crv = sftk_GetContext(hSession, &context, SFTK_HASH, PR_TRUE, &session);
    if (crv != CKR_OK) return crv;

    *pulOperationStateLen = context->cipherInfoLen + sizeof(CK_MECHANISM_TYPE)
				+ sizeof(SFTKContextType);
    if (pOperationState == NULL) {
        sftk_FreeSession(session);
	return CKR_OK;
    } else {
	if (pOSLen < *pulOperationStateLen) {
	    return CKR_BUFFER_TOO_SMALL;
	}
    }
    PORT_Memcpy(pOperationState,&context->type,sizeof(SFTKContextType));
    pOperationState += sizeof(SFTKContextType);
    PORT_Memcpy(pOperationState,&context->currentMech,
						sizeof(CK_MECHANISM_TYPE));
    pOperationState += sizeof(CK_MECHANISM_TYPE);
    PORT_Memcpy(pOperationState,context->cipherInfo,context->cipherInfoLen);
    sftk_FreeSession(session);
    return CKR_OK;
}


#define sftk_Decrement(stateSize,len) \
	stateSize = ((stateSize) > (CK_ULONG)(len)) ? \
				((stateSize) - (CK_ULONG)(len)) : 0;




CK_RV NSC_SetOperationState(CK_SESSION_HANDLE hSession, 
	CK_BYTE_PTR  pOperationState, CK_ULONG  ulOperationStateLen,
        CK_OBJECT_HANDLE hEncryptionKey, CK_OBJECT_HANDLE hAuthenticationKey)
{
    SFTKSessionContext *context;
    SFTKSession *session;
    SFTKContextType type;
    CK_MECHANISM mech;
    CK_RV crv = CKR_OK;

    CHECK_FORK();

    while (ulOperationStateLen != 0) {
	
	PORT_Memcpy(&type,pOperationState, sizeof(SFTKContextType));

	
	session = sftk_SessionFromHandle(hSession);
	if (session == NULL) return CKR_SESSION_HANDLE_INVALID;
	context = sftk_ReturnContextByType(session, type);
	sftk_SetContextByType(session, type, NULL);
	if (context) { 
	     sftk_FreeContext(context);
	}
	pOperationState += sizeof(SFTKContextType);
	sftk_Decrement(ulOperationStateLen,sizeof(SFTKContextType));


	
	PORT_Memcpy(&mech.mechanism,pOperationState,sizeof(CK_MECHANISM_TYPE));
	pOperationState += sizeof(CK_MECHANISM_TYPE);
	sftk_Decrement(ulOperationStateLen, sizeof(CK_MECHANISM_TYPE));
	
	mech.pParameter = NULL;
	mech.ulParameterLen = 0;
	switch (type) {
	case SFTK_HASH:
	    crv = NSC_DigestInit(hSession,&mech);
	    if (crv != CKR_OK) break;
	    crv = sftk_GetContext(hSession, &context, SFTK_HASH, PR_TRUE, 
								NULL);
	    if (crv != CKR_OK) break;
	    PORT_Memcpy(context->cipherInfo,pOperationState,
						context->cipherInfoLen);
	    pOperationState += context->cipherInfoLen;
	    sftk_Decrement(ulOperationStateLen,context->cipherInfoLen);
	    break;
	default:
	    
	    crv = CKR_SAVED_STATE_INVALID;
         }
         sftk_FreeSession(session);
	 if (crv != CKR_OK) break;
    }
    return crv;
}





CK_RV NSC_DigestEncryptUpdate(CK_SESSION_HANDLE hSession, CK_BYTE_PTR  pPart,
    CK_ULONG  ulPartLen, CK_BYTE_PTR  pEncryptedPart,
					 CK_ULONG_PTR pulEncryptedPartLen)
{
    CK_RV crv;

    CHECK_FORK();

    crv = NSC_EncryptUpdate(hSession,pPart,ulPartLen, pEncryptedPart,	
						      pulEncryptedPartLen);
    if (crv != CKR_OK) return crv;
    crv = NSC_DigestUpdate(hSession,pPart,ulPartLen);

    return crv;
}




CK_RV NSC_DecryptDigestUpdate(CK_SESSION_HANDLE hSession,
    CK_BYTE_PTR  pEncryptedPart, CK_ULONG  ulEncryptedPartLen,
    				CK_BYTE_PTR  pPart, CK_ULONG_PTR pulPartLen)
{
    CK_RV crv;

    CHECK_FORK();

    crv = NSC_DecryptUpdate(hSession,pEncryptedPart, ulEncryptedPartLen, 
							pPart,	pulPartLen);
    if (crv != CKR_OK) return crv;
    crv = NSC_DigestUpdate(hSession,pPart,*pulPartLen);

    return crv;
}




CK_RV NSC_SignEncryptUpdate(CK_SESSION_HANDLE hSession, CK_BYTE_PTR  pPart,
	 CK_ULONG  ulPartLen, CK_BYTE_PTR  pEncryptedPart,
					 CK_ULONG_PTR pulEncryptedPartLen)
{
    CK_RV crv;

    CHECK_FORK();

    crv = NSC_EncryptUpdate(hSession,pPart,ulPartLen, pEncryptedPart,	
						      pulEncryptedPartLen);
    if (crv != CKR_OK) return crv;
    crv = NSC_SignUpdate(hSession,pPart,ulPartLen);

    return crv;
}




CK_RV NSC_DecryptVerifyUpdate(CK_SESSION_HANDLE hSession, 
	CK_BYTE_PTR  pEncryptedData, CK_ULONG  ulEncryptedDataLen, 
				CK_BYTE_PTR  pData, CK_ULONG_PTR pulDataLen)
{
    CK_RV crv;

    CHECK_FORK();

    crv = NSC_DecryptUpdate(hSession,pEncryptedData, ulEncryptedDataLen, 
							pData,	pulDataLen);
    if (crv != CKR_OK) return crv;
    crv = NSC_VerifyUpdate(hSession, pData, *pulDataLen);

    return crv;
}




CK_RV NSC_DigestKey(CK_SESSION_HANDLE hSession, CK_OBJECT_HANDLE hKey) 
{
    SFTKSession *session = NULL;
    SFTKObject *key = NULL;
    SFTKAttribute *att;
    CK_RV crv;

    CHECK_FORK();

    session = sftk_SessionFromHandle(hSession);
    if (session == NULL) return CKR_SESSION_HANDLE_INVALID;

    key = sftk_ObjectFromHandle(hKey,session);
    sftk_FreeSession(session);
    if (key == NULL)  return CKR_KEY_HANDLE_INVALID;

    

    
    if (key->objclass != CKO_SECRET_KEY) {
	sftk_FreeObject(key);
	return CKR_KEY_TYPE_INCONSISTENT;
    }
    
    att = sftk_FindAttribute(key,CKA_VALUE);
    sftk_FreeObject(key);
    if (!att) {
        return CKR_KEY_HANDLE_INVALID;        
    }
    crv = NSC_DigestUpdate(hSession,(CK_BYTE_PTR)att->attrib.pValue,
			   att->attrib.ulValueLen);
    sftk_FreeAttribute(att);
    return crv;
}
