






#include "ssl.h" 	
#include "certt.h"	
#include "keythi.h"	
#include "sslimpl.h"
#ifndef NO_PKCS11_BYPASS
#include "blapi.h"
#endif

#include "keyhi.h"
#include "pk11func.h"
#include "secasn1.h"
#include "cert.h"
#include "secmodt.h"

#include "sslproto.h"
#include "sslerr.h"

#ifndef NO_PKCS11_BYPASS

#ifdef NOT_A_MACRO
static void
buildSSLKey(unsigned char * keyBlock, unsigned int keyLen, SECItem * result,
            const char * label)
{
    result->type = siBuffer;
    result->data = keyBlock;
    result->len  = keyLen;
    PRINT_BUF(100, (NULL, label, keyBlock, keyLen));
}
#else
#define buildSSLKey(keyBlock, keyLen, result, label) \
{ \
    (result)->type = siBuffer; \
    (result)->data = keyBlock; \
    (result)->len  = keyLen; \
    PRINT_BUF(100, (NULL, label, keyBlock, keyLen)); \
}
#endif




#ifndef NUM_MIXERS
#define NUM_MIXERS 9
#endif
static const char * const mixers[NUM_MIXERS] = { 
    "A", 
    "BB", 
    "CCC", 
    "DDDD", 
    "EEEEE", 
    "FFFFFF", 
    "GGGGGGG",
    "HHHHHHHH",
    "IIIIIIIII" 
};


SECStatus
ssl3_KeyAndMacDeriveBypass(
    ssl3CipherSpec *      pwSpec,
    const unsigned char * cr,
    const unsigned char * sr,
    PRBool                isTLS,
    PRBool                isExport)
{
    const ssl3BulkCipherDef *cipher_def = pwSpec->cipher_def;
    unsigned char * key_block    = pwSpec->key_block;
    unsigned char * key_block2   = NULL;
    unsigned int    block_bytes  = 0;
    unsigned int    block_needed = 0;
    unsigned int    i;
    unsigned int    keySize;            
    unsigned int    effKeySize;		
    unsigned int    macSize;		
    unsigned int    IVSize;		
    SECStatus       rv    = SECFailure;
    SECStatus       status = SECSuccess;
    PRBool          isFIPS = PR_FALSE;

    SECItem         srcr;
    SECItem         crsr;

    unsigned char     srcrdata[SSL3_RANDOM_LENGTH * 2];
    unsigned char     crsrdata[SSL3_RANDOM_LENGTH * 2];
    PRUint64          md5buf[22];
    PRUint64          shabuf[40];

#define md5Ctx ((MD5Context *)md5buf)
#define shaCtx ((SHA1Context *)shabuf)

    static const SECItem zed  = { siBuffer, NULL, 0 };

    if (pwSpec->msItem.data == NULL ||
        pwSpec->msItem.len  != SSL3_MASTER_SECRET_LENGTH) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return rv;
    }

    PRINT_BUF(100, (NULL, "Master Secret", pwSpec->msItem.data, 
                                           pwSpec->msItem.len));

    
    macSize    = pwSpec->mac_size;
    keySize    = cipher_def->key_size;
    effKeySize = cipher_def->secret_key_size;
    IVSize     = cipher_def->iv_size;
    if (keySize == 0) {
	effKeySize = IVSize = 0; 
    }
    block_needed = 2 * (macSize + effKeySize + ((!isExport) * IVSize));

    


    pwSpec->client.write_key_item     = zed;
    pwSpec->client.write_mac_key_item = zed;
    pwSpec->server.write_key_item     = zed;
    pwSpec->server.write_mac_key_item = zed;

    
    srcr.type   = siBuffer;
    srcr.data   = srcrdata;
    srcr.len    = sizeof srcrdata;
    PORT_Memcpy(srcrdata, sr, SSL3_RANDOM_LENGTH);
    PORT_Memcpy(srcrdata + SSL3_RANDOM_LENGTH, cr, SSL3_RANDOM_LENGTH);

    
    crsr.type   = siBuffer;
    crsr.data   = crsrdata;
    crsr.len    = sizeof crsrdata;
    PORT_Memcpy(crsrdata, cr, SSL3_RANDOM_LENGTH);
    PORT_Memcpy(crsrdata + SSL3_RANDOM_LENGTH, sr, SSL3_RANDOM_LENGTH);
    PRINT_BUF(100, (NULL, "Key & MAC CRSR", crsr.data, crsr.len));

    


    if (isTLS) {
	SECItem       keyblk;

	keyblk.type = siBuffer;
	keyblk.data = key_block;
	keyblk.len  = block_needed;

	status = TLS_PRF(&pwSpec->msItem, "key expansion", &srcr, &keyblk,
			  isFIPS);
	if (status != SECSuccess) {
	    goto key_and_mac_derive_fail;
	}
	block_bytes = keyblk.len;
    } else {
	








	unsigned int made = 0;
	for (i = 0; made < block_needed && i < NUM_MIXERS; ++i) {
	    unsigned int    outLen;
	    unsigned char   sha_out[SHA1_LENGTH];

	    SHA1_Begin(shaCtx);
	    SHA1_Update(shaCtx, (unsigned char*)(mixers[i]), i+1);
	    SHA1_Update(shaCtx, pwSpec->msItem.data, pwSpec->msItem.len);
	    SHA1_Update(shaCtx, srcr.data, srcr.len);
	    SHA1_End(shaCtx, sha_out, &outLen, SHA1_LENGTH);
	    PORT_Assert(outLen == SHA1_LENGTH);

	    MD5_Begin(md5Ctx);
	    MD5_Update(md5Ctx, pwSpec->msItem.data, pwSpec->msItem.len);
	    MD5_Update(md5Ctx, sha_out, outLen);
	    MD5_End(md5Ctx, key_block + made, &outLen, MD5_LENGTH);
	    PORT_Assert(outLen == MD5_LENGTH);
	    made += MD5_LENGTH;
	}
	block_bytes = made;
    }
    PORT_Assert(block_bytes >= block_needed);
    PORT_Assert(block_bytes <= sizeof pwSpec->key_block);
    PRINT_BUF(100, (NULL, "key block", key_block, block_bytes));

    


    key_block2 = key_block + block_bytes;
    i = 0;			

    



    buildSSLKey(&key_block[i],macSize, &pwSpec->client.write_mac_key_item, \
                "Client Write MAC Secret");
    i += macSize;

    


    buildSSLKey(&key_block[i],macSize, &pwSpec->server.write_mac_key_item, \
                "Server Write MAC Secret");
    i += macSize;

    if (!keySize) {
	
	buildSSLKey(NULL, 0, &pwSpec->client.write_key_item, \
	            "Client Write Key (MAC only)");
	buildSSLKey(NULL, 0, &pwSpec->server.write_key_item, \
	            "Server Write Key (MAC only)");
	buildSSLKey(NULL, 0, &pwSpec->client.write_iv_item, \
	            "Client Write IV (MAC only)");
	buildSSLKey(NULL, 0, &pwSpec->server.write_iv_item, \
	            "Server Write IV (MAC only)");
    } else if (!isExport) {
	



	buildSSLKey(&key_block[i], keySize, &pwSpec->client.write_key_item, \
	            "Domestic Client Write Key");
	i += keySize;

	


	buildSSLKey(&key_block[i], keySize, &pwSpec->server.write_key_item, \
	            "Domestic Server Write Key");
	i += keySize;

	if (IVSize > 0) {
	    


	    buildSSLKey(&key_block[i], IVSize, &pwSpec->client.write_iv_item, \
	                "Domestic Client Write IV");
	    i += IVSize;

	    


	    buildSSLKey(&key_block[i], IVSize, &pwSpec->server.write_iv_item, \
	                "Domestic Server Write IV");
	    i += IVSize;
	}
	PORT_Assert(i <= block_bytes);

    } else if (!isTLS) { 
	


	unsigned int    outLen;

	




	MD5_Begin(md5Ctx);
	MD5_Update(md5Ctx, &key_block[i], effKeySize);
	MD5_Update(md5Ctx, crsr.data, crsr.len);
	MD5_End(md5Ctx, key_block2, &outLen, MD5_LENGTH);
	i += effKeySize;
	buildSSLKey(key_block2, keySize, &pwSpec->client.write_key_item, \
	            "SSL3 Export Client Write Key");
	key_block2 += keySize;

	




	MD5_Begin(md5Ctx);
	MD5_Update(md5Ctx, &key_block[i], effKeySize);
	MD5_Update(md5Ctx, srcr.data, srcr.len);
	MD5_End(md5Ctx, key_block2, &outLen, MD5_LENGTH);
	i += effKeySize;
	buildSSLKey(key_block2, keySize, &pwSpec->server.write_key_item, \
	            "SSL3 Export Server Write Key");
	key_block2 += keySize;
	PORT_Assert(i <= block_bytes);

	if (IVSize) {
	    



	    MD5_Begin(md5Ctx);
	    MD5_Update(md5Ctx, crsr.data, crsr.len);
	    MD5_End(md5Ctx, key_block2, &outLen, MD5_LENGTH);
	    buildSSLKey(key_block2, IVSize, &pwSpec->client.write_iv_item, \
	                "SSL3 Export Client Write IV");
	    key_block2 += IVSize;

	    



	    MD5_Begin(md5Ctx);
	    MD5_Update(md5Ctx, srcr.data, srcr.len);
	    MD5_End(md5Ctx, key_block2, &outLen, MD5_LENGTH);
	    buildSSLKey(key_block2, IVSize, &pwSpec->server.write_iv_item, \
	                "SSL3 Export Server Write IV");
	    key_block2 += IVSize;
	}

	PORT_Assert(key_block2 - key_block <= sizeof pwSpec->key_block);
    } else {
	


	SECItem       secret ;
	SECItem       keyblk ;

	secret.type = siBuffer;
	keyblk.type = siBuffer;
	





	secret.data = &key_block[i];
	secret.len  = effKeySize;
	i          += effKeySize;
	keyblk.data = key_block2;
	keyblk.len  = keySize;
	status = TLS_PRF(&secret, "client write key", &crsr, &keyblk, isFIPS);
	if (status != SECSuccess) {
	    goto key_and_mac_derive_fail;
	}
	buildSSLKey(key_block2, keySize, &pwSpec->client.write_key_item, \
	            "TLS Export Client Write Key");
	key_block2 += keySize;

	





	secret.data = &key_block[i];
	secret.len  = effKeySize;
	i          += effKeySize;
	keyblk.data = key_block2;
	keyblk.len  = keySize;
	status = TLS_PRF(&secret, "server write key", &crsr, &keyblk, isFIPS);
	if (status != SECSuccess) {
	    goto key_and_mac_derive_fail;
	}
	buildSSLKey(key_block2, keySize, &pwSpec->server.write_key_item, \
	            "TLS Export Server Write Key");
	key_block2 += keySize;

	




	if (IVSize) {
	    secret.data = NULL;
	    secret.len  = 0;
	    keyblk.data = key_block2;
	    keyblk.len  = 2 * IVSize;
	    status = TLS_PRF(&secret, "IV block", &crsr, &keyblk, isFIPS);
	    if (status != SECSuccess) {
		goto key_and_mac_derive_fail;
	    }
	    buildSSLKey(key_block2,          IVSize, \
	                &pwSpec->client.write_iv_item, \
			"TLS Export Client Write IV");
	    buildSSLKey(key_block2 + IVSize, IVSize, \
	                &pwSpec->server.write_iv_item, \
			"TLS Export Server Write IV");
	    key_block2 += 2 * IVSize;
	}
	PORT_Assert(key_block2 - key_block <= sizeof pwSpec->key_block);
    }
    rv = SECSuccess;

key_and_mac_derive_fail:

    MD5_DestroyContext(md5Ctx, PR_FALSE);
    SHA1_DestroyContext(shaCtx, PR_FALSE);

    if (rv != SECSuccess) {
	PORT_SetError(SSL_ERROR_SESSION_KEY_GEN_FAILURE);
    }

    return rv;
}






SECStatus
ssl3_MasterKeyDeriveBypass( 
    ssl3CipherSpec *      pwSpec,
    const unsigned char * cr,
    const unsigned char * sr,
    const SECItem *       pms,
    PRBool                isTLS,
    PRBool                isRSA)
{
    unsigned char * key_block    = pwSpec->key_block;
    SECStatus       rv    = SECSuccess;
    PRBool          isFIPS = PR_FALSE;

    SECItem         crsr;

    unsigned char     crsrdata[SSL3_RANDOM_LENGTH * 2];
    PRUint64          md5buf[22];
    PRUint64          shabuf[40];

#define md5Ctx ((MD5Context *)md5buf)
#define shaCtx ((SHA1Context *)shabuf)

    
    if (isRSA) { 
	PORT_Assert(pms->len == SSL3_RSA_PMS_LENGTH);
	if (pms->len != SSL3_RSA_PMS_LENGTH) {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    return SECFailure;
	}
	
    }

    
    crsr.type   = siBuffer;
    crsr.data   = crsrdata;
    crsr.len    = sizeof crsrdata;
    PORT_Memcpy(crsrdata, cr, SSL3_RANDOM_LENGTH);
    PORT_Memcpy(crsrdata + SSL3_RANDOM_LENGTH, sr, SSL3_RANDOM_LENGTH);
    PRINT_BUF(100, (NULL, "Master Secret CRSR", crsr.data, crsr.len));

    
    if (isTLS) {
	SECItem master = { siBuffer, NULL, 0 };

	master.data = key_block;
	master.len = SSL3_MASTER_SECRET_LENGTH;

	rv = TLS_PRF(pms, "master secret", &crsr, &master, isFIPS);
	if (rv != SECSuccess) {
	    PORT_SetError(SSL_ERROR_SESSION_KEY_GEN_FAILURE);
	}
    } else {
	int i;
	unsigned int made = 0;
	for (i = 0; i < 3; i++) {
	    unsigned int    outLen;
	    unsigned char   sha_out[SHA1_LENGTH];

	    SHA1_Begin(shaCtx);
	    SHA1_Update(shaCtx, (unsigned char*) mixers[i], i+1);
	    SHA1_Update(shaCtx, pms->data, pms->len);
	    SHA1_Update(shaCtx, crsr.data, crsr.len);
	    SHA1_End(shaCtx, sha_out, &outLen, SHA1_LENGTH);
	    PORT_Assert(outLen == SHA1_LENGTH);

	    MD5_Begin(md5Ctx);
	    MD5_Update(md5Ctx, pms->data, pms->len);
	    MD5_Update(md5Ctx, sha_out, outLen);
	    MD5_End(md5Ctx, key_block + made, &outLen, MD5_LENGTH);
	    PORT_Assert(outLen == MD5_LENGTH);
	    made += outLen;
	}
    }

    
    PORT_Memcpy(pwSpec->raw_master_secret, key_block, 
		SSL3_MASTER_SECRET_LENGTH);
    pwSpec->msItem.data = pwSpec->raw_master_secret;
    pwSpec->msItem.len  = SSL3_MASTER_SECRET_LENGTH;
    PRINT_BUF(100, (NULL, "Master Secret", pwSpec->msItem.data, 
                                           pwSpec->msItem.len));

    return rv;
}

static SECStatus
ssl_canExtractMS(PK11SymKey *pms, PRBool isTLS, PRBool isDH, PRBool *pcbp)
{   SECStatus	      rv;
    PK11SymKey *    ms = NULL;
    SECItem         params = {siBuffer, NULL, 0};
    CK_SSL3_MASTER_KEY_DERIVE_PARAMS master_params;
    unsigned char   rand[SSL3_RANDOM_LENGTH];
    CK_VERSION      pms_version;
    CK_MECHANISM_TYPE master_derive;
    CK_MECHANISM_TYPE key_derive;
    CK_FLAGS          keyFlags;
    
    if (pms == NULL)
	return(SECFailure);

    PORT_Memset(rand, 0, SSL3_RANDOM_LENGTH);

    if (isTLS) {
	if(isDH) master_derive = CKM_TLS_MASTER_KEY_DERIVE_DH;
	else master_derive = CKM_TLS_MASTER_KEY_DERIVE;
	key_derive    = CKM_TLS_KEY_AND_MAC_DERIVE;
	keyFlags      = CKF_SIGN | CKF_VERIFY;
    } else {
	if (isDH) master_derive = CKM_SSL3_MASTER_KEY_DERIVE_DH;
	else master_derive = CKM_SSL3_MASTER_KEY_DERIVE;
	key_derive    = CKM_SSL3_KEY_AND_MAC_DERIVE;
	keyFlags      = 0;
    }

    master_params.pVersion                     = &pms_version;
    master_params.RandomInfo.pClientRandom     = rand;
    master_params.RandomInfo.ulClientRandomLen = SSL3_RANDOM_LENGTH;
    master_params.RandomInfo.pServerRandom     = rand;
    master_params.RandomInfo.ulServerRandomLen = SSL3_RANDOM_LENGTH;

    params.data = (unsigned char *) &master_params;
    params.len  = sizeof master_params;

    ms = PK11_DeriveWithFlags(pms, master_derive, &params, key_derive,
			      CKA_DERIVE, 0, keyFlags);
    if (ms == NULL)
	return(SECFailure);

    rv = PK11_ExtractKeyValue(ms);
    *pcbp = (rv == SECSuccess);
    PK11_FreeSymKey(ms);
    
    return(rv);

}
#endif  















SECStatus 
SSL_CanBypass(CERTCertificate *cert, SECKEYPrivateKey *srvPrivkey,
	      PRUint32 protocolmask, PRUint16 *ciphersuites, int nsuites,
              PRBool *pcanbypass, void *pwArg)
{
#ifdef NO_PKCS11_BYPASS
    if (!pcanbypass) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    *pcanbypass = PR_FALSE;
    return SECSuccess;
#else
    SECStatus	      rv;
    int		      i;
    PRUint16	      suite;
    PK11SymKey *      pms = NULL;
    SECKEYPublicKey * srvPubkey = NULL;
    KeyType	      privKeytype;
    PK11SlotInfo *    slot = NULL;
    SECItem           param;
    CK_VERSION 	      version;
    CK_MECHANISM_TYPE mechanism_array[2];
    SECItem           enc_pms = {siBuffer, NULL, 0};
    PRBool	      isTLS = PR_FALSE;
    SSLCipherSuiteInfo csdef;
    PRBool	      testrsa = PR_FALSE;
    PRBool	      testrsa_export = PR_FALSE;
    PRBool	      testecdh = PR_FALSE;
    PRBool	      testecdhe = PR_FALSE;
#ifdef NSS_ENABLE_ECC
    SECKEYECParams ecParams = { siBuffer, NULL, 0 };
#endif

    if (!cert || !srvPrivkey || !ciphersuites || !pcanbypass) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    
    srvPubkey = CERT_ExtractPublicKey(cert);
    if (!srvPubkey)
        return SECFailure;
	
    *pcanbypass = PR_TRUE;
    rv = SECFailure;
    
    
    


    for (i=0; i < nsuites && (suite = *ciphersuites++) != 0; i++) {
	
	if (SSL_GetCipherSuiteInfo(suite, &csdef, sizeof(csdef)) != SECSuccess
	    || SSL_IS_SSL2_CIPHER(suite) )
	    continue;
	switch (csdef.keaType) {
	case ssl_kea_rsa:
	    switch (csdef.cipherSuite) {
	    case TLS_RSA_EXPORT1024_WITH_RC4_56_SHA:
	    case TLS_RSA_EXPORT1024_WITH_DES_CBC_SHA:
	    case SSL_RSA_EXPORT_WITH_RC4_40_MD5:
	    case SSL_RSA_EXPORT_WITH_RC2_CBC_40_MD5:
		testrsa_export = PR_TRUE;
	    }
	    if (!testrsa_export)
		testrsa = PR_TRUE;
	    break;
	case ssl_kea_ecdh:
	    if (strcmp(csdef.keaTypeName, "ECDHE") == 0) 
		testecdhe = PR_TRUE;
	    else
		testecdh = PR_TRUE;
	    break;
	case ssl_kea_dh:
	    
	default:
	    continue;
	}
    }
    
    




    privKeytype = SECKEY_GetPrivateKeyType(srvPrivkey);
    protocolmask &= SSL_CBP_SSL3|SSL_CBP_TLS1_0;
    while (protocolmask) {
	if (protocolmask & SSL_CBP_SSL3) {
	    isTLS = PR_FALSE;
	    protocolmask ^= SSL_CBP_SSL3;
	} else {
	    isTLS = PR_TRUE;
	    protocolmask ^= SSL_CBP_TLS1_0;
	}

	if (privKeytype == rsaKey && testrsa_export) {
	    if (PK11_GetPrivateModulusLen(srvPrivkey) > EXPORT_RSA_KEY_LENGTH) {
		*pcanbypass = PR_FALSE;
		rv = SECSuccess;
		break;
	    } else
		testrsa = PR_TRUE;
	}
	for (; privKeytype == rsaKey && testrsa; ) {
	    
	    unsigned char     rsaPmsBuf[SSL3_RSA_PMS_LENGTH];
	    unsigned int      outLen = 0;
	    CK_MECHANISM_TYPE target;
	    SECStatus	      irv;
	    
	    mechanism_array[0] = CKM_SSL3_PRE_MASTER_KEY_GEN;
	    mechanism_array[1] = CKM_RSA_PKCS;

	    slot = PK11_GetBestSlotMultiple(mechanism_array, 2, pwArg);
	    if (slot == NULL) {
		PORT_SetError(SSL_ERROR_TOKEN_SLOT_NOT_FOUND);
		break;
	    }

	    
	    version.major = 3 ;
	    version.minor = 0 ;
	    param.data = (unsigned char *)&version;
	    param.len  = sizeof version;
	    pms = PK11_KeyGen(slot, CKM_SSL3_PRE_MASTER_KEY_GEN, &param, 0, pwArg);
	    PK11_FreeSlot(slot);
	    if (!pms)
		break;
	    
	    enc_pms.len  = SECKEY_PublicKeyStrength(srvPubkey);
	    enc_pms.data = (unsigned char*)PORT_Alloc(enc_pms.len);
	    if (enc_pms.data == NULL) {
	        PORT_SetError(PR_OUT_OF_MEMORY_ERROR);
	        break;
	    }
	    irv = PK11_PubWrapSymKey(CKM_RSA_PKCS, srvPubkey, pms, &enc_pms);
	    if (irv != SECSuccess) 
		break;
	    PK11_FreeSymKey(pms);
	    pms = NULL;
	    
	    rv = PK11_PrivDecryptPKCS1(srvPrivkey, rsaPmsBuf, &outLen,
				       sizeof rsaPmsBuf,
				       (unsigned char *)enc_pms.data,
				       enc_pms.len);
	    
	    if (rv == SECSuccess) {
		*pcanbypass = PR_TRUE;
		break;
	    }
	    
	    target = isTLS ? CKM_TLS_MASTER_KEY_DERIVE
			: CKM_SSL3_MASTER_KEY_DERIVE;
	    pms = PK11_PubUnwrapSymKey(srvPrivkey, &enc_pms,
				       target, CKA_DERIVE, 0);
	    rv = ssl_canExtractMS(pms, isTLS, PR_FALSE, pcanbypass);
	    if (rv == SECSuccess && *pcanbypass == PR_FALSE)
		goto done;
	    break;
	}

	


	if (enc_pms.data != NULL) {
	    SECITEM_FreeItem(&enc_pms, PR_FALSE);
        }
#ifdef NSS_ENABLE_ECC
	for (; (privKeytype == ecKey && ( testecdh || testecdhe)) ||
	       (privKeytype == rsaKey && testecdhe); ) {
	    CK_MECHANISM_TYPE target;
	    SECKEYPublicKey  *keapub = NULL;
	    SECKEYPrivateKey *keapriv;
	    SECKEYPublicKey  *cpub = NULL; 
	    SECKEYPrivateKey *cpriv = NULL;
	    SECKEYECParams   *pecParams = NULL;

	    if (privKeytype == ecKey && testecdhe) {
		
		pecParams = &srvPubkey->u.ec.DEREncodedParams;
	    } else if (privKeytype == rsaKey && testecdhe) {
		
		ECName       ec_curve;
		int		 serverKeyStrengthInBits;
		int		 signatureKeyStrength;
		int		 requiredECCbits;

		
		requiredECCbits = PK11_GetPrivateModulusLen(srvPrivkey);
		if (requiredECCbits < 0)
		    break;
		requiredECCbits *= BPB;
		serverKeyStrengthInBits = srvPubkey->u.rsa.modulus.len;
		if (srvPubkey->u.rsa.modulus.data[0] == 0) {
		    serverKeyStrengthInBits--;
		}
		
		serverKeyStrengthInBits *= BPB;

		signatureKeyStrength =
		    SSL_RSASTRENGTH_TO_ECSTRENGTH(serverKeyStrengthInBits);

		if ( requiredECCbits > signatureKeyStrength ) 
		     requiredECCbits = signatureKeyStrength;

		ec_curve =
		    ssl3_GetCurveWithECKeyStrength(
					ssl3_GetSupportedECCurveMask(NULL),
				  	requiredECCbits);
		rv = ssl3_ECName2Params(NULL, ec_curve, &ecParams);
		if (rv == SECFailure) {
		    break;
		}
		pecParams = &ecParams;
	    }

	    if (testecdhe) {
		
		keapriv = SECKEY_CreateECPrivateKey(pecParams, &keapub, NULL); 
		if (!keapriv || !keapub) {
		    if (keapriv)
			SECKEY_DestroyPrivateKey(keapriv);
		    if (keapub)
			SECKEY_DestroyPublicKey(keapub);
		    PORT_SetError(SEC_ERROR_KEYGEN_FAIL);
		    rv = SECFailure;
		    break;
		}
	    } else {
		
		keapub = srvPubkey;
		keapriv = srvPrivkey;
		pecParams = &srvPubkey->u.ec.DEREncodedParams;
	    }

	    
	    
	    cpriv = SECKEY_CreateECPrivateKey(pecParams, &cpub, NULL);
	    if (!cpriv || !cpub) {
		if (testecdhe) {
		    SECKEY_DestroyPrivateKey(keapriv);
		    SECKEY_DestroyPublicKey(keapub);
		}
		PORT_SetError(SEC_ERROR_KEYGEN_FAIL);
		rv = SECFailure;
		break;
	    }
	    
	    
	    target = isTLS ? CKM_TLS_MASTER_KEY_DERIVE_DH
			   : CKM_SSL3_MASTER_KEY_DERIVE_DH;
	    pms = PK11_PubDeriveWithKDF(keapriv, cpub, PR_FALSE, NULL, NULL,
				    CKM_ECDH1_DERIVE,
				    target,
				    CKA_DERIVE, 0, CKD_NULL, NULL, NULL);
	    rv = ssl_canExtractMS(pms, isTLS, PR_TRUE, pcanbypass);
	    SECKEY_DestroyPrivateKey(cpriv);
	    SECKEY_DestroyPublicKey(cpub);
	    if (testecdhe) {
		SECKEY_DestroyPrivateKey(keapriv);
		SECKEY_DestroyPublicKey(keapub);
	    }
	    if (rv == SECSuccess && *pcanbypass == PR_FALSE)
		goto done;
	    break;
	}
	
	if (ecParams.data != NULL) {
	    PORT_Free(ecParams.data);
	    ecParams.data = NULL;
	}
#endif 
	if (pms)
	    PK11_FreeSymKey(pms);
    }

    
    rv = SECSuccess;
    
  done:
    if (pms)
	PK11_FreeSymKey(pms);

    


    if (enc_pms.data != NULL) {
    	SECITEM_FreeItem(&enc_pms, PR_FALSE);
    }
#ifdef NSS_ENABLE_ECC
    if (ecParams.data != NULL) {
        PORT_Free(ecParams.data);
        ecParams.data = NULL;
    }
#endif 

    if (srvPubkey) {
    	SECKEY_DestroyPublicKey(srvPubkey);
	srvPubkey = NULL;
    }


    return rv;
#endif 
}

