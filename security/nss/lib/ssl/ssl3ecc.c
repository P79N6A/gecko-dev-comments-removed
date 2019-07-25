











































#include "nss.h"
#include "cert.h"
#include "ssl.h"
#include "cryptohi.h"	
#include "keyhi.h"
#include "secder.h"
#include "secitem.h"

#include "sslimpl.h"
#include "sslproto.h"
#include "sslerr.h"
#include "prtime.h"
#include "prinrval.h"
#include "prerror.h"
#include "pratom.h"
#include "prthread.h"
#include "prinit.h"

#include "pk11func.h"
#include "secmod.h"
#include "ec.h"
#include "blapi.h"

#include <stdio.h>

#ifdef NSS_ENABLE_ECC

#ifndef PK11_SETATTRS
#define PK11_SETATTRS(x,id,v,l) (x)->type = (id); \
		(x)->pValue=(v); (x)->ulValueLen = (l);
#endif

#define SSL_GET_SERVER_PUBLIC_KEY(sock, type) \
    (ss->serverCerts[type].serverKeyPair ? \
    ss->serverCerts[type].serverKeyPair->pubKey : NULL)

#define SSL_IS_CURVE_NEGOTIATED(curvemsk, curveName) \
    ((curveName > ec_noName) && \
     (curveName < ec_pastLastName) && \
     ((1UL << curveName) & curvemsk) != 0)



static SECStatus ssl3_CreateECDHEphemeralKeys(sslSocket *ss, ECName ec_curve);

#define supportedCurve(x) (((x) > ec_noName) && ((x) < ec_pastLastName))




static const SECOidTag ecName2OIDTag[] = {
	0,  
	SEC_OID_SECG_EC_SECT163K1,  
	SEC_OID_SECG_EC_SECT163R1,  
	SEC_OID_SECG_EC_SECT163R2,  
	SEC_OID_SECG_EC_SECT193R1,  
	SEC_OID_SECG_EC_SECT193R2,  
	SEC_OID_SECG_EC_SECT233K1,  
	SEC_OID_SECG_EC_SECT233R1,  
	SEC_OID_SECG_EC_SECT239K1,  
	SEC_OID_SECG_EC_SECT283K1,  
	SEC_OID_SECG_EC_SECT283R1,  
	SEC_OID_SECG_EC_SECT409K1,  
	SEC_OID_SECG_EC_SECT409R1,  
	SEC_OID_SECG_EC_SECT571K1,  
	SEC_OID_SECG_EC_SECT571R1,  
	SEC_OID_SECG_EC_SECP160K1,  
	SEC_OID_SECG_EC_SECP160R1,  
	SEC_OID_SECG_EC_SECP160R2,  
	SEC_OID_SECG_EC_SECP192K1,  
	SEC_OID_SECG_EC_SECP192R1,  
	SEC_OID_SECG_EC_SECP224K1,  
	SEC_OID_SECG_EC_SECP224R1,  
	SEC_OID_SECG_EC_SECP256K1,  
	SEC_OID_SECG_EC_SECP256R1,  
	SEC_OID_SECG_EC_SECP384R1,  
	SEC_OID_SECG_EC_SECP521R1,  
};

static const PRUint16 curve2bits[] = {
	  0, 
	163, 
	163, 
	163, 
	193, 
	193, 
	233, 
	233, 
	239, 
	283, 
	283, 
	409, 
	409, 
	571, 
	571, 
	160, 
	160, 
	160, 
	192, 
	192, 
	224, 
	224, 
	256, 
	256, 
	384, 
	521, 
      65535  
};

typedef struct Bits2CurveStr {
    PRUint16    bits;
    ECName      curve;
} Bits2Curve;

static const Bits2Curve bits2curve [] = {
   {	192,     ec_secp192r1      },
   {	160,     ec_secp160r2      },
   {	160,     ec_secp160k1           },
   {	160,     ec_secp160r1           },
   {	163,     ec_sect163k1           },
   {	163,     ec_sect163r1           },
   {	163,     ec_sect163r2           },
   {	192,     ec_secp192k1           },
   {	193,     ec_sect193r1           },
   {	193,     ec_sect193r2           },
   {	224,     ec_secp224r1      },
   {	224,     ec_secp224k1           },
   {	233,     ec_sect233k1           },
   {	233,     ec_sect233r1           },
   {	239,     ec_sect239k1           },
   {	256,     ec_secp256r1      },
   {	256,     ec_secp256k1           },
   {	283,     ec_sect283k1           },
   {	283,     ec_sect283r1           },
   {	384,     ec_secp384r1      },
   {	409,     ec_sect409k1           },
   {	409,     ec_sect409r1           },
   {	521,     ec_secp521r1      },
   {	571,     ec_sect571k1           },
   {	571,     ec_sect571r1           },
   {  65535,     ec_noName    }
};

typedef struct ECDHEKeyPairStr {
    ssl3KeyPair *  pair;
    int            error;  
    PRCallOnceType once;
} ECDHEKeyPair;


static ECDHEKeyPair gECDHEKeyPairs[ec_pastLastName];

SECStatus 
ssl3_ECName2Params(PRArenaPool * arena, ECName curve, SECKEYECParams * params)
{
    SECOidData *oidData = NULL;

    if ((curve <= ec_noName) || (curve >= ec_pastLastName) ||
	((oidData = SECOID_FindOIDByTag(ecName2OIDTag[curve])) == NULL)) {
        PORT_SetError(SEC_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);
	return SECFailure;
    }

    SECITEM_AllocItem(arena, params, (2 + oidData->oid.len));
    




    params->data[0] = SEC_ASN1_OBJECT_ID;
    params->data[1] = oidData->oid.len;
    memcpy(params->data + 2, oidData->oid.data, oidData->oid.len);

    return SECSuccess;
}

static ECName 
params2ecName(SECKEYECParams * params)
{
    SECItem oid = { siBuffer, NULL, 0};
    SECOidData *oidData = NULL;
    ECName i;

    




    if (params->data[0] != SEC_ASN1_OBJECT_ID) return ec_noName;
    oid.len = params->len - 2;
    oid.data = params->data + 2;
    if ((oidData = SECOID_FindOID(&oid)) == NULL) return ec_noName;
    for (i = ec_noName + 1; i < ec_pastLastName; i++) {
	if (ecName2OIDTag[i] == oidData->offset)
	    return i;
    }

    return ec_noName;
}


static SECStatus
ssl3_ComputeECDHKeyHash(SECItem ec_params, SECItem server_ecpoint,
			     SSL3Random *client_rand, SSL3Random *server_rand,
			     SSL3Hashes *hashes, PRBool bypassPKCS11)
{
    PRUint8     * hashBuf;
    PRUint8     * pBuf;
    SECStatus     rv 		= SECSuccess;
    unsigned int  bufLen;
    





    PRUint8       buf[2*SSL3_RANDOM_LENGTH + 2 + 1 + 256];

    bufLen = 2*SSL3_RANDOM_LENGTH + ec_params.len + 1 + server_ecpoint.len;
    if (bufLen <= sizeof buf) {
    	hashBuf = buf;
    } else {
    	hashBuf = PORT_Alloc(bufLen);
	if (!hashBuf) {
	    return SECFailure;
	}
    }

    memcpy(hashBuf, client_rand, SSL3_RANDOM_LENGTH); 
    	pBuf = hashBuf + SSL3_RANDOM_LENGTH;
    memcpy(pBuf, server_rand, SSL3_RANDOM_LENGTH);
    	pBuf += SSL3_RANDOM_LENGTH;
    memcpy(pBuf, ec_params.data, ec_params.len);
    	pBuf += ec_params.len;
    pBuf[0] = (PRUint8)(server_ecpoint.len);
    pBuf += 1;
    memcpy(pBuf, server_ecpoint.data, server_ecpoint.len);
    	pBuf += server_ecpoint.len;
    PORT_Assert((unsigned int)(pBuf - hashBuf) == bufLen);

    rv = ssl3_ComputeCommonKeyHash(hashBuf, bufLen, hashes, bypassPKCS11);

    PRINT_BUF(95, (NULL, "ECDHkey hash: ", hashBuf, bufLen));
    PRINT_BUF(95, (NULL, "ECDHkey hash: MD5 result", hashes->md5, MD5_LENGTH));
    PRINT_BUF(95, (NULL, "ECDHkey hash: SHA1 result", hashes->sha, SHA1_LENGTH));

    if (hashBuf != buf)
    	PORT_Free(hashBuf);
    return rv;
}



SECStatus
ssl3_SendECDHClientKeyExchange(sslSocket * ss, SECKEYPublicKey * svrPubKey)
{
    PK11SymKey *	pms 		= NULL;
    SECStatus           rv    		= SECFailure;
    PRBool              isTLS;
    CK_MECHANISM_TYPE	target;
    SECKEYPublicKey	*pubKey = NULL;		
    SECKEYPrivateKey	*privKey = NULL;	

    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveXmitBufLock(ss));

    isTLS = (PRBool)(ss->ssl3.pwSpec->version > SSL_LIBRARY_VERSION_3_0);

    
    if (svrPubKey->keyType != ecKey) {
	PORT_SetError(SEC_ERROR_BAD_KEY);
	goto loser;
    }
    
    privKey = SECKEY_CreateECPrivateKey(&svrPubKey->u.ec.DEREncodedParams, 
	                                &pubKey, NULL);
    if (!privKey || !pubKey) {
	    ssl_MapLowLevelError(SEC_ERROR_KEYGEN_FAIL);
	    rv = SECFailure;
	    goto loser;
    }
    PRINT_BUF(50, (ss, "ECDH public value:",
					pubKey->u.ec.publicValue.data,
					pubKey->u.ec.publicValue.len));

    if (isTLS) target = CKM_TLS_MASTER_KEY_DERIVE_DH;
    else target = CKM_SSL3_MASTER_KEY_DERIVE_DH;

    
    pms = PK11_PubDeriveWithKDF(privKey, svrPubKey, PR_FALSE, NULL, NULL,
			    CKM_ECDH1_DERIVE, target, CKA_DERIVE, 0,
			    CKD_NULL, NULL, NULL);

    if (pms == NULL) {
	SSL3AlertDescription desc  = illegal_parameter;
	(void)SSL3_SendAlert(ss, alert_fatal, desc);
	ssl_MapLowLevelError(SSL_ERROR_CLIENT_KEY_EXCHANGE_FAILURE);
	goto loser;
    }

    SECKEY_DestroyPrivateKey(privKey);
    privKey = NULL;

    rv = ssl3_InitPendingCipherSpec(ss,  pms);
    PK11_FreeSymKey(pms); pms = NULL;

    if (rv != SECSuccess) {
	ssl_MapLowLevelError(SSL_ERROR_CLIENT_KEY_EXCHANGE_FAILURE);
	goto loser;
    }

    rv = ssl3_AppendHandshakeHeader(ss, client_key_exchange, 
					pubKey->u.ec.publicValue.len + 1);
    if (rv != SECSuccess) {
        goto loser;	
    }

    rv = ssl3_AppendHandshakeVariable(ss, 
					pubKey->u.ec.publicValue.data,
					pubKey->u.ec.publicValue.len, 1);
    SECKEY_DestroyPublicKey(pubKey);
    pubKey = NULL;

    if (rv != SECSuccess) {
        goto loser;	
    }

    rv = SECSuccess;

loser:
    if(pms) PK11_FreeSymKey(pms);
    if(privKey) SECKEY_DestroyPrivateKey(privKey);
    if(pubKey) SECKEY_DestroyPublicKey(pubKey);
    return rv;
}





SECStatus
ssl3_HandleECDHClientKeyExchange(sslSocket *ss, SSL3Opaque *b,
				     PRUint32 length,
                                     SECKEYPublicKey *srvrPubKey,
                                     SECKEYPrivateKey *srvrPrivKey)
{
    PK11SymKey *      pms;
    SECStatus         rv;
    SECKEYPublicKey   clntPubKey;
    CK_MECHANISM_TYPE	target;
    PRBool isTLS;

    PORT_Assert( ss->opt.noLocks || ssl_HaveRecvBufLock(ss) );
    PORT_Assert( ss->opt.noLocks || ssl_HaveSSL3HandshakeLock(ss) );

    clntPubKey.keyType = ecKey;
    clntPubKey.u.ec.DEREncodedParams.len = 
	srvrPubKey->u.ec.DEREncodedParams.len;
    clntPubKey.u.ec.DEREncodedParams.data = 
	srvrPubKey->u.ec.DEREncodedParams.data;

    rv = ssl3_ConsumeHandshakeVariable(ss, &clntPubKey.u.ec.publicValue, 
	                               1, &b, &length);
    if (rv != SECSuccess) {
	SEND_ALERT
	return SECFailure;	
    }

    isTLS = (PRBool)(ss->ssl3.prSpec->version > SSL_LIBRARY_VERSION_3_0);

    if (isTLS) target = CKM_TLS_MASTER_KEY_DERIVE_DH;
    else target = CKM_SSL3_MASTER_KEY_DERIVE_DH;

    
    pms = PK11_PubDeriveWithKDF(srvrPrivKey, &clntPubKey, PR_FALSE, NULL, NULL,
			    CKM_ECDH1_DERIVE, target, CKA_DERIVE, 0,
			    CKD_NULL, NULL, NULL);

    if (pms == NULL) {
	
	ssl_MapLowLevelError(SSL_ERROR_CLIENT_KEY_EXCHANGE_FAILURE);
	return SECFailure;
    }

    rv = ssl3_InitPendingCipherSpec(ss,  pms);
    PK11_FreeSymKey(pms);
    if (rv != SECSuccess) {
	SEND_ALERT
	return SECFailure; 
    }
    return SECSuccess;
}

ECName
ssl3_GetCurveWithECKeyStrength(PRUint32 curvemsk, int requiredECCbits)
{
    int    i;
    
    for ( i = 0; bits2curve[i].curve != ec_noName; i++) {
	if (bits2curve[i].bits < requiredECCbits)
	    continue;
    	if (SSL_IS_CURVE_NEGOTIATED(curvemsk, bits2curve[i].curve)) {
	    return bits2curve[i].curve;
	}
    }
    PORT_SetError(SSL_ERROR_NO_CYPHER_OVERLAP);
    return ec_noName;
}




ECName
ssl3_GetCurveNameForServerSocket(sslSocket *ss)
{
    SECKEYPublicKey * svrPublicKey = NULL;
    ECName ec_curve = ec_noName;
    int    signatureKeyStrength = 521;
    int    requiredECCbits = ss->sec.secretKeyBits * 2;

    if (ss->ssl3.hs.kea_def->kea == kea_ecdhe_ecdsa) {
	svrPublicKey = SSL_GET_SERVER_PUBLIC_KEY(ss, kt_ecdh);
	if (svrPublicKey)
	    ec_curve = params2ecName(&svrPublicKey->u.ec.DEREncodedParams);
	if (!SSL_IS_CURVE_NEGOTIATED(ss->ssl3.hs.negotiatedECCurves, ec_curve)) {
	    PORT_SetError(SSL_ERROR_NO_CYPHER_OVERLAP);
	    return ec_noName;
	}
	signatureKeyStrength = curve2bits[ ec_curve ];
    } else {
        
        int serverKeyStrengthInBits;
 
        svrPublicKey = SSL_GET_SERVER_PUBLIC_KEY(ss, kt_rsa);
        if (!svrPublicKey) {
            PORT_SetError(SSL_ERROR_NO_CYPHER_OVERLAP);
            return ec_noName;
        }
 
        
        serverKeyStrengthInBits = svrPublicKey->u.rsa.modulus.len;
        if (svrPublicKey->u.rsa.modulus.data[0] == 0) {
            serverKeyStrengthInBits--;
        }
        
        serverKeyStrengthInBits *= BPB;
 
        signatureKeyStrength =
	    SSL_RSASTRENGTH_TO_ECSTRENGTH(serverKeyStrengthInBits);
    }
    if ( requiredECCbits > signatureKeyStrength ) 
         requiredECCbits = signatureKeyStrength;

    return ssl3_GetCurveWithECKeyStrength(ss->ssl3.hs.negotiatedECCurves,
					  requiredECCbits);
}


static SECStatus 
ssl3_ShutdownECDHECurves(void *appData, void *nssData)
{
    int i;
    ECDHEKeyPair *keyPair = &gECDHEKeyPairs[0];

    for (i=0; i < ec_pastLastName; i++, keyPair++) {
	if (keyPair->pair) {
	    ssl3_FreeKeyPair(keyPair->pair);
	}
    }
    memset(gECDHEKeyPairs, 0, sizeof gECDHEKeyPairs);
    return SECSuccess;
}

static PRStatus
ssl3_ECRegister(void)
{
    SECStatus rv;
    rv = NSS_RegisterShutdown(ssl3_ShutdownECDHECurves, gECDHEKeyPairs);
    if (rv != SECSuccess) {
	gECDHEKeyPairs[ec_noName].error = PORT_GetError();
    }
    return (PRStatus)rv;
}


static PRStatus 
ssl3_CreateECDHEphemeralKeyPair(void * arg)
{
    SECKEYPrivateKey *    privKey  = NULL;
    SECKEYPublicKey *     pubKey   = NULL;
    ssl3KeyPair *	  keyPair  = NULL;
    ECName                ec_curve = (ECName)arg;
    SECKEYECParams        ecParams = { siBuffer, NULL, 0 };

    PORT_Assert(gECDHEKeyPairs[ec_curve].pair == NULL);

    
    if (ssl3_ECName2Params(NULL, ec_curve, &ecParams) != SECSuccess) {
	gECDHEKeyPairs[ec_curve].error = PORT_GetError();
	return PR_FAILURE;
    }

    privKey = SECKEY_CreateECPrivateKey(&ecParams, &pubKey, NULL);    
    SECITEM_FreeItem(&ecParams, PR_FALSE);

    if (!privKey || !pubKey || !(keyPair = ssl3_NewKeyPair(privKey, pubKey))) {
	if (privKey) {
	    SECKEY_DestroyPrivateKey(privKey);
	}
	if (pubKey) {
	    SECKEY_DestroyPublicKey(pubKey);
	}
	ssl_MapLowLevelError(SEC_ERROR_KEYGEN_FAIL);
	gECDHEKeyPairs[ec_curve].error = PORT_GetError();
	return PR_FAILURE;
    }

    gECDHEKeyPairs[ec_curve].pair = keyPair;
    return PR_SUCCESS;
}










static SECStatus
ssl3_CreateECDHEphemeralKeys(sslSocket *ss, ECName ec_curve)
{
    ssl3KeyPair *	  keyPair        = NULL;

    
    if (gECDHEKeyPairs[ec_curve].pair == NULL) {
	PRStatus status;

	status = PR_CallOnce(&gECDHEKeyPairs[ec_noName].once, ssl3_ECRegister);
        if (status != PR_SUCCESS) {
	    PORT_SetError(gECDHEKeyPairs[ec_noName].error);
	    return SECFailure;
    	}
	status = PR_CallOnceWithArg(&gECDHEKeyPairs[ec_curve].once,
	                            ssl3_CreateECDHEphemeralKeyPair,
				    (void *)ec_curve);
        if (status != PR_SUCCESS) {
	    PORT_SetError(gECDHEKeyPairs[ec_curve].error);
	    return SECFailure;
    	}
    }

    keyPair = gECDHEKeyPairs[ec_curve].pair;
    PORT_Assert(keyPair != NULL);
    if (!keyPair) 
    	return SECFailure;
    ss->ephemeralECDHKeyPair = ssl3_GetKeyPairRef(keyPair);

    return SECSuccess;
}

SECStatus
ssl3_HandleECDHServerKeyExchange(sslSocket *ss, SSL3Opaque *b, PRUint32 length)
{
    PRArenaPool *    arena     = NULL;
    SECKEYPublicKey *peerKey   = NULL;
    PRBool           isTLS;
    SECStatus        rv;
    int              errCode   = SSL_ERROR_RX_MALFORMED_SERVER_KEY_EXCH;
    SSL3AlertDescription desc  = illegal_parameter;
    SSL3Hashes       hashes;
    SECItem          signature = {siBuffer, NULL, 0};

    SECItem          ec_params = {siBuffer, NULL, 0};
    SECItem          ec_point  = {siBuffer, NULL, 0};
    unsigned char    paramBuf[3]; 

    isTLS = (PRBool)(ss->ssl3.prSpec->version > SSL_LIBRARY_VERSION_3_0);

    


    ec_params.len  = sizeof paramBuf;
    ec_params.data = paramBuf;
    rv = ssl3_ConsumeHandshake(ss, ec_params.data, ec_params.len, &b, &length);
    if (rv != SECSuccess) {
	goto loser;		
    }

    
    if ((ec_params.data[0] != ec_type_named) || 
	(ec_params.data[1] != 0) ||
	!supportedCurve(ec_params.data[2])) {
	    errCode = SEC_ERROR_UNSUPPORTED_ELLIPTIC_CURVE;
	    desc = handshake_failure;
	    goto alert_loser;
    }

    rv = ssl3_ConsumeHandshakeVariable(ss, &ec_point, 1, &b, &length);
    if (rv != SECSuccess) {
	goto loser;		
    }
    
    if (ec_point.data[0] != EC_POINT_FORM_UNCOMPRESSED) {
	    errCode = SEC_ERROR_UNSUPPORTED_EC_POINT_FORM;
	    desc = handshake_failure;
	    goto alert_loser;
    }

    rv = ssl3_ConsumeHandshakeVariable(ss, &signature, 2, &b, &length);
    if (rv != SECSuccess) {
	goto loser;		
    }

    if (length != 0) {
	if (isTLS)
	    desc = decode_error;
	goto alert_loser;		
    }

    PRINT_BUF(60, (NULL, "Server EC params", ec_params.data, 
	ec_params.len));
    PRINT_BUF(60, (NULL, "Server EC point", ec_point.data, ec_point.len));

    
    
    desc = isTLS ? decrypt_error : handshake_failure;

    


    rv = ssl3_ComputeECDHKeyHash(ec_params, ec_point,
				      &ss->ssl3.hs.client_random,
				      &ss->ssl3.hs.server_random, 
				      &hashes, ss->opt.bypassPKCS11);

    if (rv != SECSuccess) {
	errCode =
	    ssl_MapLowLevelError(SSL_ERROR_SERVER_KEY_EXCHANGE_FAILURE);
	goto alert_loser;
    }
    rv = ssl3_VerifySignedHashes(&hashes, ss->sec.peerCert, &signature,
				isTLS, ss->pkcs11PinArg);
    if (rv != SECSuccess)  {
	errCode =
	    ssl_MapLowLevelError(SSL_ERROR_SERVER_KEY_EXCHANGE_FAILURE);
	goto alert_loser;
    }

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	goto no_memory;
    }

    ss->sec.peerKey = peerKey = PORT_ArenaZNew(arena, SECKEYPublicKey);
    if (peerKey == NULL) {
	goto no_memory;
    }

    peerKey->arena                 = arena;
    peerKey->keyType               = ecKey;

    
    if (ssl3_ECName2Params(arena, ec_params.data[2], 
	    &peerKey->u.ec.DEREncodedParams) != SECSuccess) {
	


	errCode = SEC_ERROR_UNSUPPORTED_ELLIPTIC_CURVE;
	goto alert_loser;
    }

    
    if (SECITEM_CopyItem(arena, &peerKey->u.ec.publicValue,  &ec_point))
    {
	PORT_FreeArena(arena, PR_FALSE);
	goto no_memory;
    }
    peerKey->pkcs11Slot         = NULL;
    peerKey->pkcs11ID           = CK_INVALID_HANDLE;

    ss->sec.peerKey = peerKey;
    ss->ssl3.hs.ws = wait_cert_request;

    return SECSuccess;

alert_loser:
    (void)SSL3_SendAlert(ss, alert_fatal, desc);
loser:
    PORT_SetError( errCode );
    return SECFailure;

no_memory:	
    ssl_MapLowLevelError(SSL_ERROR_SERVER_KEY_EXCHANGE_FAILURE);
    return SECFailure;
}

SECStatus
ssl3_SendECDHServerKeyExchange(sslSocket *ss)
{
const ssl3KEADef *     kea_def     = ss->ssl3.hs.kea_def;
    SECStatus          rv          = SECFailure;
    int                length;
    PRBool             isTLS;
    SECItem            signed_hash = {siBuffer, NULL, 0};
    SSL3Hashes         hashes;

    SECKEYPublicKey *  ecdhePub;
    SECItem            ec_params = {siBuffer, NULL, 0};
    unsigned char      paramBuf[3];
    ECName             curve;
    SSL3KEAType        certIndex;


    
    curve = ssl3_GetCurveNameForServerSocket(ss);
    if (curve == ec_noName) {
    	goto loser;
    }
    rv = ssl3_CreateECDHEphemeralKeys(ss, curve);
    if (rv != SECSuccess) {
	goto loser; 	
    }	    
    ecdhePub = ss->ephemeralECDHKeyPair->pubKey;
    PORT_Assert(ecdhePub != NULL);
    if (!ecdhePub) {
	PORT_SetError(SSL_ERROR_SERVER_KEY_EXCHANGE_FAILURE);
	return SECFailure;
    }	
    
    ec_params.len  = sizeof paramBuf;
    ec_params.data = paramBuf;
    curve = params2ecName(&ecdhePub->u.ec.DEREncodedParams);
    if (curve != ec_noName) {
	ec_params.data[0] = ec_type_named;
	ec_params.data[1] = 0x00;
	ec_params.data[2] = curve;
    } else {
	PORT_SetError(SEC_ERROR_UNSUPPORTED_ELLIPTIC_CURVE);
	goto loser;
    }		

    rv = ssl3_ComputeECDHKeyHash(ec_params, ecdhePub->u.ec.publicValue,
				      &ss->ssl3.hs.client_random,
				      &ss->ssl3.hs.server_random,
				      &hashes, ss->opt.bypassPKCS11);
    if (rv != SECSuccess) {
	ssl_MapLowLevelError(SSL_ERROR_SERVER_KEY_EXCHANGE_FAILURE);
	goto loser;
    }

    isTLS = (PRBool)(ss->ssl3.pwSpec->version > SSL_LIBRARY_VERSION_3_0);

    



    if (kea_def->kea == kea_ecdhe_rsa)
	certIndex = kt_rsa;
    else 
	certIndex = kt_ecdh;

    rv = ssl3_SignHashes(&hashes, ss->serverCerts[certIndex].SERVERKEY, 
			 &signed_hash, isTLS);
    if (rv != SECSuccess) {
	goto loser;		
    }
    if (signed_hash.data == NULL) {
	
	PORT_SetError(SSL_ERROR_SERVER_KEY_EXCHANGE_FAILURE);
	goto loser;
    }

    length = ec_params.len + 
	     1 + ecdhePub->u.ec.publicValue.len + 
	     2 + signed_hash.len;

    rv = ssl3_AppendHandshakeHeader(ss, server_key_exchange, length);
    if (rv != SECSuccess) {
	goto loser; 	
    }

    rv = ssl3_AppendHandshake(ss, ec_params.data, ec_params.len);
    if (rv != SECSuccess) {
	goto loser; 	
    }

    rv = ssl3_AppendHandshakeVariable(ss, ecdhePub->u.ec.publicValue.data,
				      ecdhePub->u.ec.publicValue.len, 1);
    if (rv != SECSuccess) {
	goto loser; 	
    }

    rv = ssl3_AppendHandshakeVariable(ss, signed_hash.data,
				      signed_hash.len, 2);
    if (rv != SECSuccess) {
	goto loser; 	
    }

    PORT_Free(signed_hash.data);
    return SECSuccess;

loser:
    if (signed_hash.data != NULL) 
    	PORT_Free(signed_hash.data);
    return SECFailure;
}



static const ssl3CipherSuite ecdh_suites[] = {
    TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA,
    TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA,
    TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA,
    TLS_ECDH_ECDSA_WITH_NULL_SHA,
    TLS_ECDH_ECDSA_WITH_RC4_128_SHA,
    TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA,
    TLS_ECDH_RSA_WITH_AES_128_CBC_SHA,
    TLS_ECDH_RSA_WITH_AES_256_CBC_SHA,
    TLS_ECDH_RSA_WITH_NULL_SHA,
    TLS_ECDH_RSA_WITH_RC4_128_SHA,
    0 
};

static const ssl3CipherSuite ecdh_ecdsa_suites[] = {
    TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA,
    TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA,
    TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA,
    TLS_ECDH_ECDSA_WITH_NULL_SHA,
    TLS_ECDH_ECDSA_WITH_RC4_128_SHA,
    0 
};

static const ssl3CipherSuite ecdh_rsa_suites[] = {
    TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA,
    TLS_ECDH_RSA_WITH_AES_128_CBC_SHA,
    TLS_ECDH_RSA_WITH_AES_256_CBC_SHA,
    TLS_ECDH_RSA_WITH_NULL_SHA,
    TLS_ECDH_RSA_WITH_RC4_128_SHA,
    0 
};

static const ssl3CipherSuite ecdhe_ecdsa_suites[] = {
    TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA,
    TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA,
    TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA,
    TLS_ECDHE_ECDSA_WITH_NULL_SHA,
    TLS_ECDHE_ECDSA_WITH_RC4_128_SHA,
    0 
};

static const ssl3CipherSuite ecdhe_rsa_suites[] = {
    TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA,
    TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA,
    TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA,
    TLS_ECDHE_RSA_WITH_NULL_SHA,
    TLS_ECDHE_RSA_WITH_RC4_128_SHA,
    0 
};


static const ssl3CipherSuite ecSuites[] = {
    TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA,
    TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA,
    TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA,
    TLS_ECDHE_ECDSA_WITH_NULL_SHA,
    TLS_ECDHE_ECDSA_WITH_RC4_128_SHA,
    TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA,
    TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA,
    TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA,
    TLS_ECDHE_RSA_WITH_NULL_SHA,
    TLS_ECDHE_RSA_WITH_RC4_128_SHA,
    TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA,
    TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA,
    TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA,
    TLS_ECDH_ECDSA_WITH_NULL_SHA,
    TLS_ECDH_ECDSA_WITH_RC4_128_SHA,
    TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA,
    TLS_ECDH_RSA_WITH_AES_128_CBC_SHA,
    TLS_ECDH_RSA_WITH_AES_256_CBC_SHA,
    TLS_ECDH_RSA_WITH_NULL_SHA,
    TLS_ECDH_RSA_WITH_RC4_128_SHA,
    0 
};


SECStatus
ssl3_DisableECCSuites(sslSocket * ss, const ssl3CipherSuite * suite)
{
    if (!suite)
    	suite = ecSuites;
    for (; *suite; ++suite) {
	SECStatus rv      = ssl3_CipherPrefSet(ss, *suite, PR_FALSE);

	PORT_Assert(rv == SECSuccess); 
    }
    return SECSuccess;
}




void
ssl3_FilterECCipherSuitesByServerCerts(sslSocket * ss)
{
    CERTCertificate * svrCert;

    svrCert = ss->serverCerts[kt_rsa].serverCert;
    if (!svrCert) {
	ssl3_DisableECCSuites(ss, ecdhe_rsa_suites);
    }

    svrCert = ss->serverCerts[kt_ecdh].serverCert;
    if (!svrCert) {
	ssl3_DisableECCSuites(ss, ecdh_suites);
	ssl3_DisableECCSuites(ss, ecdhe_ecdsa_suites);
    } else {
	SECOidTag sigTag = SECOID_GetAlgorithmTag(&svrCert->signature);

	switch (sigTag) {
	case SEC_OID_PKCS1_RSA_ENCRYPTION:
	case SEC_OID_PKCS1_MD2_WITH_RSA_ENCRYPTION:
	case SEC_OID_PKCS1_MD4_WITH_RSA_ENCRYPTION:
	case SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION:
	case SEC_OID_PKCS1_SHA1_WITH_RSA_ENCRYPTION:
	case SEC_OID_PKCS1_SHA224_WITH_RSA_ENCRYPTION:
	case SEC_OID_PKCS1_SHA256_WITH_RSA_ENCRYPTION:
	case SEC_OID_PKCS1_SHA384_WITH_RSA_ENCRYPTION:
	case SEC_OID_PKCS1_SHA512_WITH_RSA_ENCRYPTION:
	    ssl3_DisableECCSuites(ss, ecdh_ecdsa_suites);
	    break;
	case SEC_OID_ANSIX962_ECDSA_SHA1_SIGNATURE:
	case SEC_OID_ANSIX962_ECDSA_SHA224_SIGNATURE:
	case SEC_OID_ANSIX962_ECDSA_SHA256_SIGNATURE:
	case SEC_OID_ANSIX962_ECDSA_SHA384_SIGNATURE:
	case SEC_OID_ANSIX962_ECDSA_SHA512_SIGNATURE:
	case SEC_OID_ANSIX962_ECDSA_SIGNATURE_RECOMMENDED_DIGEST:
	case SEC_OID_ANSIX962_ECDSA_SIGNATURE_SPECIFIED_DIGEST:
	    ssl3_DisableECCSuites(ss, ecdh_rsa_suites);
	    break;
	default:
	    ssl3_DisableECCSuites(ss, ecdh_suites);
	    break;
	}
    }
}



PRBool
ssl3_IsECCEnabled(sslSocket * ss)
{
    const ssl3CipherSuite * suite;

    for (suite = ecSuites; *suite; ++suite) {
	PRBool    enabled = PR_FALSE;
	SECStatus rv      = ssl3_CipherPrefGet(ss, *suite, &enabled);

	PORT_Assert(rv == SECSuccess); 
	if (rv == SECSuccess && enabled)
	    return PR_TRUE;
    }
    return PR_FALSE;
}

#define BE(n) 0, n

#ifndef NSS_ECC_MORE_THAN_SUITE_B



static const PRUint8 EClist[12] = {
    BE(10),         
    BE( 8),         
    BE( 6),         
    BE(23), BE(24), BE(25)
};
#else



static const PRUint8 EClist[56] = {
    BE(10),         
    BE(52),         
    BE(50),         
            BE( 1), BE( 2), BE( 3), BE( 4), BE( 5), BE( 6), BE( 7), 
    BE( 8), BE( 9), BE(10), BE(11), BE(12), BE(13), BE(14), BE(15), 
    BE(16), BE(17), BE(18), BE(19), BE(20), BE(21), BE(22), BE(23), 
    BE(24), BE(25)
};
#endif

static const PRUint8 ECPtFmt[6] = {
    BE(11),         
    BE( 2),         
             1,     
                 0  
};




PRInt32
ssl3_SendSupportedCurvesXtn(
			sslSocket * ss,
			PRBool      append,
			PRUint32    maxBytes)
{
    if (!ss || !ssl3_IsECCEnabled(ss))
    	return 0;
    if (append && maxBytes >= (sizeof EClist)) {
	SECStatus rv = ssl3_AppendHandshake(ss, EClist, (sizeof EClist));
	if (rv != SECSuccess)
	    return -1;
	if (!ss->sec.isServer) {
	    TLSExtensionData *xtnData = &ss->xtnData;
	    xtnData->advertised[xtnData->numAdvertised++] =
		ssl_elliptic_curves_xtn;
	}
    }
    return (sizeof EClist);
}




PRInt32
ssl3_SendSupportedPointFormatsXtn(
			sslSocket * ss,
			PRBool      append,
			PRUint32    maxBytes)
{
    if (!ss || !ssl3_IsECCEnabled(ss))
    	return 0;
    if (append && maxBytes >= (sizeof ECPtFmt)) {
	SECStatus rv = ssl3_AppendHandshake(ss, ECPtFmt, (sizeof ECPtFmt));
	if (rv != SECSuccess)
	    return -1;
	if (!ss->sec.isServer) {
	    TLSExtensionData *xtnData = &ss->xtnData;
	    xtnData->advertised[xtnData->numAdvertised++] =
		ssl_ec_point_formats_xtn;
	}
    }
    return (sizeof ECPtFmt);
}




SECStatus
ssl3_HandleSupportedPointFormatsXtn(sslSocket *ss, PRUint16 ex_type,
                                    SECItem *data)
{
    int i;

    if (data->len < 2 || data->len > 255 || !data->data ||
        data->len != (unsigned int)data->data[0] + 1) {
    	
	goto loser;
    }
    for (i = data->len; --i > 0; ) {
    	if (data->data[i] == 0) {
	    
	    SECStatus rv;
	    rv = ssl3_RegisterServerHelloExtensionSender(ss, ex_type,
			      &ssl3_SendSupportedPointFormatsXtn);
	    return rv;
	}
    }
loser:
    
    ssl3_DisableECCSuites(ss, ecSuites);
    return SECFailure;
}


#define SSL3_GET_SERVER_PUBLICKEY(sock, type) \
    (ss->serverCerts[type].serverKeyPair ? \
    ss->serverCerts[type].serverKeyPair->pubKey : NULL)


ECName ssl3_GetSvrCertCurveName(sslSocket *ss) 
{
    SECKEYPublicKey       *srvPublicKey; 
    ECName		  ec_curve       = ec_noName;

    srvPublicKey = SSL3_GET_SERVER_PUBLICKEY(ss, kt_ecdh);
    if (srvPublicKey) {
	ec_curve = params2ecName(&srvPublicKey->u.ec.DEREncodedParams);
    }
    return ec_curve;
}




SECStatus
ssl3_HandleSupportedCurvesXtn(sslSocket *ss, PRUint16 ex_type, SECItem *data)
{
    PRInt32  list_len;
    PRUint32 peerCurves   = 0;
    PRUint32 mutualCurves = 0;
    PRUint16 svrCertCurveName;

    if (!data->data || data->len < 4 || data->len > 65535)
    	goto loser;
    
    list_len = ssl3_ConsumeHandshakeNumber(ss, 2, &data->data, &data->len);
    if (list_len < 0 || data->len != list_len || (data->len % 2) != 0) {
    	
	goto loser;
    }
    
    while (data->len) {
	PRInt32  curve_name = 
		 ssl3_ConsumeHandshakeNumber(ss, 2, &data->data, &data->len);
	if (curve_name > ec_noName && curve_name < ec_pastLastName) {
	    peerCurves |= (1U << curve_name);
	}
    }
    
    mutualCurves = ss->ssl3.hs.negotiatedECCurves &= peerCurves;
    if (!mutualCurves) { 
    	goto loser;
    }

    


    svrCertCurveName = ssl3_GetSvrCertCurveName(ss);
    if (svrCertCurveName != ec_noName &&
        (mutualCurves & (1U << svrCertCurveName)) != 0) {
	return SECSuccess;
    }
    


    ssl3_DisableECCSuites(ss, ecdh_ecdsa_suites);
    ssl3_DisableECCSuites(ss, ecdhe_ecdsa_suites);
    return SECFailure;

loser:
    
    ssl3_DisableECCSuites(ss, ecSuites);
    return SECFailure;
}

#endif 
