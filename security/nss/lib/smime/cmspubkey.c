









































#include "cmslocal.h"

#include "cert.h"
#include "key.h"
#include "secasn1.h"
#include "secitem.h"
#include "secoid.h"
#include "pk11func.h"
#include "secerr.h"









SECStatus
NSS_CMSUtil_EncryptSymKey_RSA(PLArenaPool *poolp, CERTCertificate *cert, 
                              PK11SymKey *bulkkey,
                              SECItem *encKey)
{
    SECStatus rv;
    SECKEYPublicKey *publickey;

    publickey = CERT_ExtractPublicKey(cert);
    if (publickey == NULL)
	return SECFailure;

    rv = NSS_CMSUtil_EncryptSymKey_RSAPubKey(poolp, publickey, bulkkey, encKey);
    SECKEY_DestroyPublicKey(publickey);
    return rv;
}

SECStatus
NSS_CMSUtil_EncryptSymKey_RSAPubKey(PLArenaPool *poolp, 
                                    SECKEYPublicKey *publickey, 
                                    PK11SymKey *bulkkey, SECItem *encKey)
{
    SECStatus rv;
    int data_len;
    KeyType keyType;
    void *mark = NULL;


    mark = PORT_ArenaMark(poolp);
    if (!mark)
	goto loser;

    
    keyType = SECKEY_GetPublicKeyType(publickey);
    PORT_Assert(keyType == rsaKey);
    if (keyType != rsaKey) {
	goto loser;
    }
    
    data_len = SECKEY_PublicKeyStrength(publickey);	
    encKey->data = (unsigned char*)PORT_ArenaAlloc(poolp, data_len);
    encKey->len = data_len;
    if (encKey->data == NULL)
	goto loser;

    
    rv = PK11_PubWrapSymKey(PK11_AlgtagToMechanism(SEC_OID_PKCS1_RSA_ENCRYPTION),
				publickey, bulkkey, encKey);

    if (rv != SECSuccess)
	goto loser;

    PORT_ArenaUnmark(poolp, mark);
    return SECSuccess;

loser:
    if (mark) {
	PORT_ArenaRelease(poolp, mark);
    }
    return SECFailure;
}








PK11SymKey *
NSS_CMSUtil_DecryptSymKey_RSA(SECKEYPrivateKey *privkey, SECItem *encKey, SECOidTag bulkalgtag)
{
    
    CK_MECHANISM_TYPE target;
    PORT_Assert(bulkalgtag != SEC_OID_UNKNOWN);
    target = PK11_AlgtagToMechanism(bulkalgtag);
    if (bulkalgtag == SEC_OID_UNKNOWN || target == CKM_INVALID_MECHANISM) {
	PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	return NULL;
    }
    return PK11_PubUnwrapSymKey(privkey, encKey, target, CKA_DECRYPT, 0);
}



SECStatus
NSS_CMSUtil_EncryptSymKey_ESDH(PLArenaPool *poolp, CERTCertificate *cert, PK11SymKey *key,
			SECItem *encKey, SECItem **ukm, SECAlgorithmID *keyEncAlg,
			SECItem *pubKey)
{
#if 0 
    SECOidTag certalgtag;	
    SECOidTag encalgtag;	
    SECStatus rv;
    SECItem *params = NULL;
    int data_len;
    SECStatus err;
    PK11SymKey *tek;
    CERTCertificate *ourCert;
    SECKEYPublicKey *ourPubKey;
    NSSCMSKEATemplateSelector whichKEA = NSSCMSKEAInvalid;

    certalgtag = SECOID_GetAlgorithmTag(&(cert->subjectPublicKeyInfo.algorithm));
    PORT_Assert(certalgtag == SEC_OID_X942_DIFFIE_HELMAN_KEY);

    
    encalgtag = SEC_OID_CMS_EPHEMERAL_STATIC_DIFFIE_HELLMAN;

    
    publickey = CERT_ExtractPublicKey(cert);
    if (publickey == NULL) goto loser;

    
    ourCert = PK11_FindBestKEAMatch(cert, wincx);
    if (ourCert == NULL) goto loser;

    arena = PORT_NewArena(1024);
    if (arena == NULL) goto loser;

    
    
    ourPubKey = CERT_ExtractPublicKey(ourCert);
    if (ourPubKey == NULL)
    {
	goto loser;
    }
    SECITEM_CopyItem(arena, pubKey, &(ourPubKey->u.fortezza.KEAKey));
    SECKEY_DestroyPublicKey(ourPubKey); 
    ourPubKey = NULL;

    
    ourPrivKey = PK11_FindKeyByAnyCert(ourCert,wincx);
    CERT_DestroyCertificate(ourCert); 
    if (!ourPrivKey) goto loser;

    
    if (ukm) {
	ukm->data = (unsigned char*)PORT_ArenaZAlloc(arena,);
	ukm->len = ;
    }

    

    kek = PK11_PubDerive(ourPrivKey, publickey, PR_TRUE,
			 ukm, NULL,
			 CKM_KEA_KEY_DERIVE, CKM_SKIPJACK_WRAP,
			 CKA_WRAP, 0, wincx);

    SECKEY_DestroyPublicKey(publickey);
    SECKEY_DestroyPrivateKey(ourPrivKey);
    publickey = NULL;
    ourPrivKey = NULL;
    
    if (!kek)
	goto loser;

    
    encKey->data = (unsigned char*)PORT_ArenaAlloc(poolp, SMIME_FORTEZZA_MAX_KEY_SIZE);
    encKey->len = SMIME_FORTEZZA_MAX_KEY_SIZE;

    if (encKey->data == NULL)
    {
	PK11_FreeSymKey(kek);
	goto loser;
    }


    
    
    switch (PK11_AlgtagToMechanism(enccinfo->encalg))
    {
    case CKM_SKIPJACK_CFB8:
	err = PK11_WrapSymKey(CKM_CMS3DES_WRAP, NULL, kek, bulkkey, encKey);
	whichKEA = NSSCMSKEAUsesSkipjack;
	break;
    case CKM_SKIPJACK_CFB8:
	err = PK11_WrapSymKey(CKM_CMSRC2_WRAP, NULL, kek, bulkkey, encKey);
	whichKEA = NSSCMSKEAUsesSkipjack;
	break;
    default:
	
        err = SECFailure;
        
	break;
    }

    PK11_FreeSymKey(kek);	
    if (err != SECSuccess)
	goto loser;

    PORT_Assert(whichKEA != NSSCMSKEAInvalid);

    
    
    params = SEC_ASN1EncodeItem(arena, NULL, &keaParams, sec_pkcs7_get_kea_template(whichKEA));
    if (params == NULL)
	goto loser;

    
    rv = SECOID_SetAlgorithmID(poolp, keyEncAlg, SEC_OID_CMS_EPHEMERAL_STATIC_DIFFIE_HELLMAN, params);
    if (rv != SECSuccess)
	goto loser;

    
loser:
    if (arena) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    if (publickey) {
        SECKEY_DestroyPublicKey(publickey);
    }
    if (ourPrivKey) {
        SECKEY_DestroyPrivateKey(ourPrivKey);
    }
#endif
    return SECFailure;
}

PK11SymKey *
NSS_CMSUtil_DecryptSymKey_ESDH(SECKEYPrivateKey *privkey, SECItem *encKey, SECAlgorithmID *keyEncAlg, SECOidTag bulkalgtag, void *pwfn_arg)
{
#if 0 
    SECStatus err;
    CK_MECHANISM_TYPE bulkType;
    PK11SymKey *tek;
    SECKEYPublicKey *originatorPubKey;
    NSSCMSSMIMEKEAParameters keaParams;

   
   originatorPubKey = PK11_MakeKEAPubKey(keaParams.originatorKEAKey.data,
			   keaParams.originatorKEAKey.len);
   if (originatorPubKey == NULL)
      goto loser;
    
   


   tek = PK11_PubDerive(privkey, originatorPubKey, PR_FALSE,
			 &keaParams.originatorRA, NULL,
			 CKM_KEA_KEY_DERIVE, CKM_SKIPJACK_WRAP,
			 CKA_WRAP, 0, pwfn_arg);
   SECKEY_DestroyPublicKey(originatorPubKey);	
   if (tek == NULL)
	goto loser;
    
    

    
    
    bulkkey = PK11_UnwrapSymKey(tek, CKM_SKIPJACK_WRAP, NULL,
				encKey, CKM_SKIPJACK_CBC64, CKA_DECRYPT, 0);

    return bulkkey;

loser:
#endif
    return NULL;
}

