







































#include "secport.h"
#include "seccomon.h"
#include "secmod.h"
#include "secmodi.h"
#include "secmodti.h"
#include "pkcs11.h"
#include "pk11func.h"
#include "cert.h"
#include "certi.h"
#include "secitem.h"
#include "key.h" 
#include "secoid.h"
#include "pkcs7t.h"
#include "cmsreclist.h"

#include "certdb.h"
#include "secerr.h"
#include "sslerr.h"

#include "pki3hack.h"
#include "dev3hack.h"

#include "devm.h" 
#include "nsspki.h"
#include "pki.h"
#include "pkim.h"
#include "pkitm.h"
#include "pkistore.h" 
#include "devt.h"

extern const NSSError NSS_ERROR_NOT_FOUND;
extern const NSSError NSS_ERROR_INVALID_CERTIFICATE;

struct nss3_cert_cbstr {
    SECStatus(* callback)(CERTCertificate*, void *);
    nssList *cached;
    void *arg;
};




static PRStatus convert_cert(NSSCertificate *c, void *arg)
{
    CERTCertificate *nss3cert;
    SECStatus secrv;
    struct nss3_cert_cbstr *nss3cb = (struct nss3_cert_cbstr *)arg;
    
    nss3cert = STAN_GetCERTCertificate(c);
    if (!nss3cert) return PR_FAILURE;
    secrv = (*nss3cb->callback)(nss3cert, nss3cb->arg);
    return (secrv) ? PR_FAILURE : PR_SUCCESS;
}





static int toHex(int x) { return (x < 10) ? (x+'0') : (x+'a'-10); }
#define MAX_CERT_ID 4
#define DEFAULT_STRING "Cert ID "
static char *
pk11_buildNickname(PK11SlotInfo *slot,CK_ATTRIBUTE *cert_label,
			CK_ATTRIBUTE *key_label, CK_ATTRIBUTE *cert_id)
{
    int prefixLen = PORT_Strlen(slot->token_name);
    int suffixLen = 0;
    char *suffix = NULL;
    char buildNew[sizeof(DEFAULT_STRING)+MAX_CERT_ID*2];
    char *next,*nickname;

    if (cert_label && (cert_label->ulValueLen)) {
	suffixLen = cert_label->ulValueLen;
	suffix = (char*)cert_label->pValue;
    } else if (key_label && (key_label->ulValueLen)) {
	suffixLen = key_label->ulValueLen;
	suffix = (char*)key_label->pValue;
    } else if (cert_id && cert_id->ulValueLen > 0) {
	int i,first = cert_id->ulValueLen - MAX_CERT_ID;
	int offset = sizeof(DEFAULT_STRING);
	char *idValue = (char *)cert_id->pValue;

	PORT_Memcpy(buildNew,DEFAULT_STRING,sizeof(DEFAULT_STRING)-1);
	next = buildNew + offset;
	if (first < 0) first = 0;
	for (i=first; i < (int) cert_id->ulValueLen; i++) {
		*next++ = toHex((idValue[i] >> 4) & 0xf);
		*next++ = toHex(idValue[i] & 0xf);
	}
	*next++ = 0;
	suffix = buildNew;
	suffixLen = PORT_Strlen(buildNew);
    } else {
	PORT_SetError( SEC_ERROR_LIBRARY_FAILURE );
	return NULL;
    }

    
    next = nickname = (char *)PORT_Alloc(prefixLen+1+suffixLen+1);
    if (nickname == NULL) return NULL;

    PORT_Memcpy(next,slot->token_name,prefixLen);
    next += prefixLen;
    *next++ = ':';
    PORT_Memcpy(next,suffix,suffixLen);
    next += suffixLen;
    *next++ = 0;
    return nickname;
}

PRBool
PK11_IsUserCert(PK11SlotInfo *slot, CERTCertificate *cert,
						CK_OBJECT_HANDLE certID)
{
    CK_OBJECT_CLASS theClass;

    if (slot == NULL) return PR_FALSE;
    if (cert == NULL) return PR_FALSE;

    theClass = CKO_PRIVATE_KEY;
    if (pk11_LoginStillRequired(slot,NULL)) {
	theClass = CKO_PUBLIC_KEY;
    }
    if (PK11_MatchItem(slot, certID , theClass) != CK_INVALID_HANDLE) {
	return PR_TRUE;
    }

   if (theClass == CKO_PUBLIC_KEY) {
	SECKEYPublicKey *pubKey= CERT_ExtractPublicKey(cert);
	CK_ATTRIBUTE theTemplate;

	if (pubKey == NULL) {
	   return PR_FALSE;
	}

	PK11_SETATTRS(&theTemplate,0,NULL,0);
	switch (pubKey->keyType) {
	case rsaKey:
	    PK11_SETATTRS(&theTemplate,CKA_MODULUS, pubKey->u.rsa.modulus.data,
						pubKey->u.rsa.modulus.len);
	    break;
	case dsaKey:
	    PK11_SETATTRS(&theTemplate,CKA_VALUE, pubKey->u.dsa.publicValue.data,
						pubKey->u.dsa.publicValue.len);
	    break;
	case dhKey:
	    PK11_SETATTRS(&theTemplate,CKA_VALUE, pubKey->u.dh.publicValue.data,
						pubKey->u.dh.publicValue.len);
	    break;
	case ecKey:
	    PK11_SETATTRS(&theTemplate,CKA_EC_POINT, 
			  pubKey->u.ec.publicValue.data,
			  pubKey->u.ec.publicValue.len);
	    break;
	case keaKey:
	case fortezzaKey:
	case nullKey:
	    
	    break;
	}

	if (theTemplate.ulValueLen == 0) {
	    SECKEY_DestroyPublicKey(pubKey);
	    return PR_FALSE;
	}
	pk11_SignedToUnsigned(&theTemplate);
	if (pk11_FindObjectByTemplate(slot,&theTemplate,1) != CK_INVALID_HANDLE) {
	    SECKEY_DestroyPublicKey(pubKey);
	    return PR_TRUE;
	}
	SECKEY_DestroyPublicKey(pubKey);
    }
    return PR_FALSE;
}







PRBool
pk11_isID0(PK11SlotInfo *slot, CK_OBJECT_HANDLE certID)
{
    CK_ATTRIBUTE keyID = {CKA_ID, NULL, 0};
    PRBool isZero = PR_FALSE;
    int i;
    CK_RV crv;


    crv = PK11_GetAttributes(NULL,slot,certID,&keyID,1);
    if (crv != CKR_OK) {
	return isZero;
    }

    if (keyID.ulValueLen != 0) {
	char *value = (char *)keyID.pValue;
	isZero = PR_TRUE; 
	for (i=0; i < (int) keyID.ulValueLen; i++) {
	    if (value[i] != 0) {
		isZero = PR_FALSE; 
		break;
	    }
	}
    }
    PORT_Free(keyID.pValue);
    return isZero;

}





static CERTCertificate
*pk11_fastCert(PK11SlotInfo *slot, CK_OBJECT_HANDLE certID, 
			CK_ATTRIBUTE *privateLabel, char **nickptr)
{
    NSSCertificate *c;
    nssCryptokiObject *co = NULL;
    nssPKIObject *pkio;
    NSSToken *token;
    NSSTrustDomain *td = STAN_GetDefaultTrustDomain();

    
    token = PK11Slot_GetNSSToken(slot);
    if (token->defaultSession) {
	co = nssCryptokiObject_Create(token, token->defaultSession, certID);
    } else {
	PORT_SetError(SEC_ERROR_NO_TOKEN);
    }
    if (!co) {
	return NULL;
    }

    
    pkio = nssPKIObject_Create(NULL, co, td, NULL, nssPKIMonitor);
    if (!pkio) {
	nssCryptokiObject_Destroy(co);
	return NULL;
    }

    
    c = nssCertificate_Create(pkio);
    if (!c) {
	nssPKIObject_Destroy(pkio);
	return NULL;
    }

    nssTrustDomain_AddCertsToCache(td, &c, 1);

    
    if ((nickptr) && (co->label)) {
	CK_ATTRIBUTE label, id;
	label.type = CKA_LABEL;
	label.pValue = co->label;
	label.ulValueLen = PORT_Strlen(co->label);
	id.type = CKA_ID;
	id.pValue = c->id.data;
	id.ulValueLen = c->id.size;
	*nickptr = pk11_buildNickname(slot, &label, privateLabel, &id);
    }
    return STAN_GetCERTCertificateOrRelease(c);
}





CERTCertificate *
PK11_MakeCertFromHandle(PK11SlotInfo *slot,CK_OBJECT_HANDLE certID,
						CK_ATTRIBUTE *privateLabel)
{
    char * nickname = NULL;
    CERTCertificate *cert = NULL;
    CERTCertTrust *trust;
    PRBool isFortezzaRootCA = PR_FALSE;
    PRBool swapNickname = PR_FALSE;

    cert = pk11_fastCert(slot,certID,privateLabel, &nickname);
    if (cert == NULL) goto loser;
	
    if (nickname) {
	if (cert->nickname != NULL) {
		cert->dbnickname = cert->nickname;
	} 
	cert->nickname = PORT_ArenaStrdup(cert->arena,nickname);
	PORT_Free(nickname);
	nickname = NULL;
	swapNickname = PR_TRUE;
    }

    


    if (cert->slot == NULL) {
	cert->slot = PK11_ReferenceSlot(slot);
	cert->pkcs11ID = certID;
	cert->ownSlot = PR_TRUE;
	cert->series = slot->series;
    }

    trust = (CERTCertTrust*)PORT_ArenaAlloc(cert->arena, sizeof(CERTCertTrust));
    if (trust == NULL) goto loser;
    PORT_Memset(trust,0, sizeof(CERTCertTrust));
    cert->trust = trust;

    

    if(! pk11_HandleTrustObject(slot, cert, trust) ) {
	unsigned int type;

	
	if (CERT_IsCACert(cert, &type)) {
	    unsigned int trustflags = CERTDB_VALID_CA;
	   
	    

 
	    if (pk11_isID0(slot,certID) && 
		cert->isRoot) {
		trustflags |= CERTDB_TRUSTED_CA;
		


		if (PK11_DoesMechanism(slot,CKM_KEA_KEY_DERIVE)) {
		    trust->objectSigningFlags |= CERTDB_VALID_CA;
		    isFortezzaRootCA = PR_TRUE;
		}
	    }
	    if ((type & NS_CERT_TYPE_SSL_CA) == NS_CERT_TYPE_SSL_CA) {
		trust->sslFlags |= trustflags;
	    }
	    if ((type & NS_CERT_TYPE_EMAIL_CA) == NS_CERT_TYPE_EMAIL_CA) {
		trust->emailFlags |= trustflags;
	    }
	    if ((type & NS_CERT_TYPE_OBJECT_SIGNING_CA) 
					== NS_CERT_TYPE_OBJECT_SIGNING_CA) {
		trust->objectSigningFlags |= trustflags;
	    }
	}
    }

    if (PK11_IsUserCert(slot,cert,certID)) {
	trust->sslFlags |= CERTDB_USER;
	trust->emailFlags |= CERTDB_USER;
	
    }

    return cert;

loser:
    if (nickname) PORT_Free(nickname);
    if (cert) CERT_DestroyCertificate(cert);
    return NULL;
}

	



CERTCertificate *
PK11_GetCertFromPrivateKey(SECKEYPrivateKey *privKey)
{
    PK11SlotInfo *slot = privKey->pkcs11Slot;
    CK_OBJECT_HANDLE handle = privKey->pkcs11ID;
    CK_OBJECT_HANDLE certID = 
		PK11_MatchItem(slot,handle,CKO_CERTIFICATE);
    CERTCertificate *cert;

    if (certID == CK_INVALID_HANDLE) {
	PORT_SetError(SSL_ERROR_NO_CERTIFICATE);
	return NULL;
    }
    cert = PK11_MakeCertFromHandle(slot,certID,NULL);
    return (cert);

}





SECStatus
PK11_DeleteTokenCertAndKey(CERTCertificate *cert,void *wincx)
{
    SECKEYPrivateKey *privKey = PK11_FindKeyByAnyCert(cert,wincx);
    CK_OBJECT_HANDLE pubKey;
    PK11SlotInfo *slot = NULL;

    pubKey = pk11_FindPubKeyByAnyCert(cert, &slot, wincx);
    if (privKey) {
	
	SEC_DeletePermCertificate(cert);
	PK11_DeleteTokenPrivateKey(privKey, PR_FALSE);
    }
    if ((pubKey != CK_INVALID_HANDLE) && (slot != NULL)) { 
    	PK11_DestroyTokenObject(slot,pubKey);
        PK11_FreeSlot(slot);
    }
    return SECSuccess;
}




typedef struct pk11DoCertCallbackStr {
	SECStatus(* callback)(PK11SlotInfo *slot, CERTCertificate*, void *);
	SECStatus(* noslotcallback)(CERTCertificate*, void *);
	SECStatus(* itemcallback)(CERTCertificate*, SECItem *, void *);
	void *callbackArg;
} pk11DoCertCallback;


typedef struct pk11CertCallbackStr {
	SECStatus(* callback)(CERTCertificate*,SECItem *,void *);
	void *callbackArg;
} pk11CertCallback;

struct fake_der_cb_argstr
{
    SECStatus(* callback)(CERTCertificate*, SECItem *, void *);
    void *arg;
};

static SECStatus fake_der_cb(CERTCertificate *c, void *a)
{
    struct fake_der_cb_argstr *fda = (struct fake_der_cb_argstr *)a;
    return (*fda->callback)(c, &c->derCert, fda->arg);
}




SECStatus
PK11_TraverseSlotCerts(SECStatus(* callback)(CERTCertificate*,SECItem *,void *),
						void *arg, void *wincx) 
{
    NSSTrustDomain *defaultTD = STAN_GetDefaultTrustDomain();
    struct fake_der_cb_argstr fda;
    struct nss3_cert_cbstr pk11cb;

    
    (void) pk11_TraverseAllSlots( NULL, NULL, PR_TRUE, wincx);

    fda.callback = callback;
    fda.arg = arg;
    pk11cb.callback = fake_der_cb;
    pk11cb.arg = &fda;
    NSSTrustDomain_TraverseCertificates(defaultTD, convert_cert, &pk11cb);
    return SECSuccess;
}

static void
transfer_token_certs_to_collection(nssList *certList, NSSToken *token, 
                                   nssPKIObjectCollection *collection)
{
    NSSCertificate **certs;
    PRUint32 i, count;
    NSSToken **tokens, **tp;
    count = nssList_Count(certList);
    if (count == 0) {
	return;
    }
    certs = nss_ZNEWARRAY(NULL, NSSCertificate *, count);
    if (!certs) {
	return;
    }
    nssList_GetArray(certList, (void **)certs, count);
    for (i=0; i<count; i++) {
	tokens = nssPKIObject_GetTokens(&certs[i]->object, NULL);
	if (tokens) {
	    for (tp = tokens; *tp; tp++) {
		if (*tp == token) {
		    nssPKIObjectCollection_AddObject(collection, 
		                                     (nssPKIObject *)certs[i]);
		}
	    }
	    nssTokenArray_Destroy(tokens);
	}
	CERT_DestroyCertificate(STAN_GetCERTCertificateOrRelease(certs[i]));
    }
    nss_ZFreeIf(certs);
}

CERTCertificate *
PK11_FindCertFromNickname(const char *nickname, void *wincx) 
{
    PRStatus status;
    CERTCertificate *rvCert = NULL;
    NSSCertificate *cert = NULL;
    NSSCertificate **certs = NULL;
    static const NSSUsage usage = {PR_TRUE  };
    NSSToken *token;
    NSSTrustDomain *defaultTD = STAN_GetDefaultTrustDomain();
    PK11SlotInfo *slot = NULL;
    SECStatus rv;
    char *nickCopy;
    char *delimit = NULL;
    char *tokenName;

    nickCopy = PORT_Strdup(nickname);
    if (!nickCopy) {
        
        return NULL;
    }
    if ((delimit = PORT_Strchr(nickCopy,':')) != NULL) {
	tokenName = nickCopy;
	nickname = delimit + 1;
	*delimit = '\0';
	
	token = NSSTrustDomain_FindTokenByName(defaultTD, (NSSUTF8 *)tokenName);
	if (token) {
	    slot = PK11_ReferenceSlot(token->pk11slot);
	} else {
	    PORT_SetError(SEC_ERROR_NO_TOKEN);
	}
	*delimit = ':';
    } else {
	slot = PK11_GetInternalKeySlot();
	token = PK11Slot_GetNSSToken(slot);
    }
    if (token) {
	nssList *certList;
	nssCryptokiObject **instances;
	nssPKIObjectCollection *collection;
	nssTokenSearchType tokenOnly = nssTokenSearchType_TokenOnly;
	if (!PK11_IsPresent(slot)) {
	    goto loser;
	}
   	rv = pk11_AuthenticateUnfriendly(slot, PR_TRUE, wincx);
	if (rv != SECSuccess) {
	    goto loser;
	}
	collection = nssCertificateCollection_Create(defaultTD, NULL);
	if (!collection) {
	    goto loser;
	}
	certList = nssList_Create(NULL, PR_FALSE);
	if (!certList) {
	    nssPKIObjectCollection_Destroy(collection);
	    goto loser;
	}
	(void)nssTrustDomain_GetCertsForNicknameFromCache(defaultTD, 
	                                                  nickname, 
	                                                  certList);
	transfer_token_certs_to_collection(certList, token, collection);
	instances = nssToken_FindCertificatesByNickname(token,
	                                                NULL,
	                                                nickname,
	                                                tokenOnly,
	                                                0,
	                                                &status);
	nssPKIObjectCollection_AddInstances(collection, instances, 0);
	nss_ZFreeIf(instances);
	
	if (nssPKIObjectCollection_Count(collection) == 0 &&
	    PORT_Strchr(nickname, '@') != NULL) 
	{
	    char* lowercaseName = CERT_FixupEmailAddr(nickname);
	    if (lowercaseName) {
		(void)nssTrustDomain_GetCertsForEmailAddressFromCache(defaultTD, 
								      lowercaseName, 
								      certList);
		transfer_token_certs_to_collection(certList, token, collection);
		instances = nssToken_FindCertificatesByEmail(token,
							     NULL,
							     lowercaseName,
							     tokenOnly,
							     0,
							     &status);
		nssPKIObjectCollection_AddInstances(collection, instances, 0);
		nss_ZFreeIf(instances);
		PORT_Free(lowercaseName);
	    }
	}
	certs = nssPKIObjectCollection_GetCertificates(collection, 
	                                               NULL, 0, NULL);
	nssPKIObjectCollection_Destroy(collection);
	if (certs) {
	    cert = nssCertificateArray_FindBestCertificate(certs, NULL, 
	                                                   &usage, NULL);
	    if (cert) {
		rvCert = STAN_GetCERTCertificateOrRelease(cert);
	    }
	    nssCertificateArray_Destroy(certs);
	}
	nssList_Destroy(certList);
    }
    if (slot) {
	PK11_FreeSlot(slot);
    }
    if (nickCopy) PORT_Free(nickCopy);
    return rvCert;
loser:
    if (slot) {
	PK11_FreeSlot(slot);
    }
    if (nickCopy) PORT_Free(nickCopy);
    return NULL;
}

CERTCertList *
PK11_FindCertsFromNickname(const char *nickname, void *wincx) 
{
    char *nickCopy;
    char *delimit = NULL;
    char *tokenName;
    int i;
    CERTCertList *certList = NULL;
    nssPKIObjectCollection *collection = NULL;
    NSSCertificate **foundCerts = NULL;
    NSSTrustDomain *defaultTD = STAN_GetDefaultTrustDomain();
    NSSCertificate *c;
    NSSToken *token;
    PK11SlotInfo *slot;
    SECStatus rv;

    nickCopy = PORT_Strdup(nickname);
    if (!nickCopy) {
        
        return NULL;
    }
    if ((delimit = PORT_Strchr(nickCopy,':')) != NULL) {
	tokenName = nickCopy;
	nickname = delimit + 1;
	*delimit = '\0';
	
	token = NSSTrustDomain_FindTokenByName(defaultTD, (NSSUTF8 *)tokenName);
	if (token) {
	    slot = PK11_ReferenceSlot(token->pk11slot);
	} else {
	    PORT_SetError(SEC_ERROR_NO_TOKEN);
	    slot = NULL;
	}
	*delimit = ':';
    } else {
	slot = PK11_GetInternalKeySlot();
	token = PK11Slot_GetNSSToken(slot);
    }
    if (token) {
	PRStatus status;
	nssList *nameList;
	nssCryptokiObject **instances;
	nssTokenSearchType tokenOnly = nssTokenSearchType_TokenOnly;
   	rv = pk11_AuthenticateUnfriendly(slot, PR_TRUE, wincx);
	if (rv != SECSuccess) {
	    PK11_FreeSlot(slot);
    	    if (nickCopy) PORT_Free(nickCopy);
	    return NULL;
	}
	collection = nssCertificateCollection_Create(defaultTD, NULL);
	if (!collection) {
	    PK11_FreeSlot(slot);
	    if (nickCopy) PORT_Free(nickCopy);
	    return NULL;
	}
	nameList = nssList_Create(NULL, PR_FALSE);
	if (!nameList) {
	    PK11_FreeSlot(slot);
	    if (nickCopy) PORT_Free(nickCopy);
	    return NULL;
	}
	(void)nssTrustDomain_GetCertsForNicknameFromCache(defaultTD,
	                                                  nickname, 
	                                                  nameList);
	transfer_token_certs_to_collection(nameList, token, collection);
	instances = nssToken_FindCertificatesByNickname(token,
	                                                NULL,
	                                                nickname,
	                                                tokenOnly,
	                                                0,
	                                                &status);
	nssPKIObjectCollection_AddInstances(collection, instances, 0);
	nss_ZFreeIf(instances);

        
        if (nssPKIObjectCollection_Count(collection) == 0 &&
            PORT_Strchr(nickname, '@') != NULL) 
        {
            char* lowercaseName = CERT_FixupEmailAddr(nickname);
            if (lowercaseName) {
                (void)nssTrustDomain_GetCertsForEmailAddressFromCache(defaultTD, 
                                                                      lowercaseName, 
                                                                      nameList);
                transfer_token_certs_to_collection(nameList, token, collection);
                instances = nssToken_FindCertificatesByEmail(token,
                                                             NULL,
                                                             lowercaseName,
                                                             tokenOnly,
                                                             0,
                                                             &status);
                nssPKIObjectCollection_AddInstances(collection, instances, 0);
                nss_ZFreeIf(instances);
                PORT_Free(lowercaseName);
            }
        }

        nssList_Destroy(nameList);
	foundCerts = nssPKIObjectCollection_GetCertificates(collection,
	                                                    NULL, 0, NULL);
	nssPKIObjectCollection_Destroy(collection);
    }
    if (slot) {
	PK11_FreeSlot(slot);
    }
    if (nickCopy) PORT_Free(nickCopy);
    if (foundCerts) {
	PRTime now = PR_Now();
	certList = CERT_NewCertList();
	for (i=0, c = *foundCerts; c; c = foundCerts[++i]) {
	    if (certList) {
	 	CERTCertificate *certCert = STAN_GetCERTCertificateOrRelease(c);
		
		if (certCert) {
		    
		    CERT_AddCertToListSorted(certList, certCert,
				CERT_SortCBValidity, &now);
		}
	    } else {
		nssCertificate_Destroy(c);
	    }
	}
	if (certList && CERT_LIST_HEAD(certList) == NULL) {
	    CERT_DestroyCertList(certList);
	    certList = NULL;
	}
	
	nss_ZFreeIf(foundCerts);
    }
    return certList;
}






SECItem *
PK11_GetPubIndexKeyID(CERTCertificate *cert) 
{
    SECKEYPublicKey *pubk;
    SECItem *newItem = NULL;

    pubk = CERT_ExtractPublicKey(cert);
    if (pubk == NULL) return NULL;

    switch (pubk->keyType) {
    case rsaKey:
	newItem = SECITEM_DupItem(&pubk->u.rsa.modulus);
	break;
    case dsaKey:
        newItem = SECITEM_DupItem(&pubk->u.dsa.publicValue);
	break;
    case dhKey:
        newItem = SECITEM_DupItem(&pubk->u.dh.publicValue);
	break;
    case ecKey:
        newItem = SECITEM_DupItem(&pubk->u.ec.publicValue);
	break;
    case fortezzaKey:
    default:
	newItem = NULL; 
    }
    SECKEY_DestroyPublicKey(pubk);
    
    return newItem;
}




SECItem *
pk11_mkcertKeyID(CERTCertificate *cert) 
{
    SECItem *pubKeyData = PK11_GetPubIndexKeyID(cert) ;
    SECItem *certCKA_ID;

    if (pubKeyData == NULL) return NULL;
    
    certCKA_ID = PK11_MakeIDFromPubKey(pubKeyData);
    SECITEM_FreeItem(pubKeyData,PR_TRUE);
    return certCKA_ID;
}




SECStatus
PK11_ImportCert(PK11SlotInfo *slot, CERTCertificate *cert, 
		CK_OBJECT_HANDLE key, const char *nickname, 
                PRBool includeTrust) 
{
    PRStatus status;
    NSSCertificate *c;
    nssCryptokiObject *keyobj, *certobj;
    NSSToken *token = PK11Slot_GetNSSToken(slot);
    SECItem *keyID = pk11_mkcertKeyID(cert);
    char *emailAddr = NULL;
    nssCertificateStoreTrace lockTrace = {NULL, NULL, PR_FALSE, PR_FALSE};
    nssCertificateStoreTrace unlockTrace = {NULL, NULL, PR_FALSE, PR_FALSE};

    if (keyID == NULL) {
	goto loser;
    }

    if (PK11_IsInternal(slot) && cert->emailAddr && cert->emailAddr[0]) {
	emailAddr = cert->emailAddr;
    }

    
    if (cert->nssCertificate) {
	c = cert->nssCertificate;
    } else {
	c = STAN_GetNSSCertificate(cert);
	if (c == NULL) {
	    goto loser;
	}
    }

    if (c->object.cryptoContext) {
	
	NSSCryptoContext *cc = c->object.cryptoContext;
	nssCertificateStore_Lock(cc->certStore, &lockTrace);
	nssCertificateStore_RemoveCertLOCKED(cc->certStore, c);
	nssCertificateStore_Unlock(cc->certStore, &lockTrace, &unlockTrace);
	c->object.cryptoContext = NULL;
	cert->istemp = PR_FALSE;
	cert->isperm = PR_TRUE;
    }

    
    nssItem_Create(c->object.arena, &c->id, keyID->len, keyID->data);
    if (!c->id.data) {
	goto loser;
    }

    if (key != CK_INVALID_HANDLE) {
	
	keyobj = nss_ZNEW(NULL, nssCryptokiObject);
	if (!keyobj) {
	    goto loser;
	}
	keyobj->token = nssToken_AddRef(token);
	keyobj->handle = key;
	keyobj->isTokenObject = PR_TRUE;

	
	status = nssCryptokiPrivateKey_SetCertificate(keyobj, NULL, nickname, 
	                                              &c->id, &c->subject);
	nssCryptokiObject_Destroy(keyobj);
	if (status != PR_SUCCESS) {
	    goto loser;
	}
    }

    
    certobj = nssToken_ImportCertificate(token, NULL,
                                         NSSCertificateType_PKIX,
                                         &c->id,
                                         nickname,
                                         &c->encoding,
                                         &c->issuer,
                                         &c->subject,
                                         &c->serial,
					 emailAddr,
                                         PR_TRUE);
    if (!certobj) {
	if (NSS_GetError() == NSS_ERROR_INVALID_CERTIFICATE) {
	    PORT_SetError(SEC_ERROR_REUSED_ISSUER_AND_SERIAL);
	    SECITEM_FreeItem(keyID,PR_TRUE);
	    return SECFailure;
	}
	goto loser;
    }
    


    nssPKIObject_AddInstance(&c->object, certobj);
    nssTrustDomain_AddCertsToCache(STAN_GetDefaultTrustDomain(), &c, 1);
    (void)STAN_ForceCERTCertificateUpdate(c);
    SECITEM_FreeItem(keyID,PR_TRUE);
    return SECSuccess;
loser:
    CERT_MapStanError();
    SECITEM_FreeItem(keyID,PR_TRUE);
    if (PORT_GetError() != SEC_ERROR_TOKEN_NOT_LOGGED_IN) {
	PORT_SetError(SEC_ERROR_ADDING_CERT);
    }
    return SECFailure;
}

SECStatus
PK11_ImportDERCert(PK11SlotInfo *slot, SECItem *derCert,
		CK_OBJECT_HANDLE key, char *nickname, PRBool includeTrust) 
{
    CERTCertificate *cert;
    SECStatus rv;

    cert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
                                   derCert, NULL, PR_FALSE, PR_TRUE);
    if (cert == NULL) return SECFailure;

    rv = PK11_ImportCert(slot, cert, key, nickname, includeTrust);
    CERT_DestroyCertificate (cert);
    return rv;
}




CK_OBJECT_HANDLE
pk11_getcerthandle(PK11SlotInfo *slot, CERTCertificate *cert, 
					CK_ATTRIBUTE *theTemplate,int tsize)
{
    CK_OBJECT_HANDLE certh;

    if (cert->slot == slot) {
	certh = cert->pkcs11ID;
	if ((certh == CK_INVALID_HANDLE) ||
			(cert->series != slot->series)) {
    	     certh = pk11_FindObjectByTemplate(slot,theTemplate,tsize);
	     cert->pkcs11ID = certh;
	     cert->series = slot->series;
	}
    } else {
    	certh = pk11_FindObjectByTemplate(slot,theTemplate,tsize);
    }
    return certh;
}




SECKEYPrivateKey *
PK11_FindPrivateKeyFromCert(PK11SlotInfo *slot, CERTCertificate *cert,
								 void *wincx) 
{
    int err;
    CK_OBJECT_CLASS certClass = CKO_CERTIFICATE;
    CK_ATTRIBUTE theTemplate[] = {
	{ CKA_VALUE, NULL, 0 },
	{ CKA_CLASS, NULL, 0 }
    };
    
    int tsize = sizeof(theTemplate)/sizeof(theTemplate[0]);
    CK_OBJECT_HANDLE certh;
    CK_OBJECT_HANDLE keyh;
    CK_ATTRIBUTE *attrs = theTemplate;
    PRBool needLogin;
    SECStatus rv;

    PK11_SETATTRS(attrs, CKA_VALUE, cert->derCert.data, 
						cert->derCert.len); attrs++;
    PK11_SETATTRS(attrs, CKA_CLASS, &certClass, sizeof(certClass));

    


    rv = pk11_AuthenticateUnfriendly(slot, PR_TRUE, wincx);
    if (rv != SECSuccess) {
	return NULL;
    }

    certh = pk11_getcerthandle(slot,cert,theTemplate,tsize);
    if (certh == CK_INVALID_HANDLE) {
	return NULL;
    }
    






    needLogin = pk11_LoginStillRequired(slot,wincx);
    keyh = PK11_MatchItem(slot,certh,CKO_PRIVATE_KEY);
    if ((keyh == CK_INVALID_HANDLE) && needLogin &&
                        (SSL_ERROR_NO_CERTIFICATE == (err = PORT_GetError()) ||
			 SEC_ERROR_TOKEN_NOT_LOGGED_IN == err )) {
	
	rv = PK11_Authenticate(slot, PR_TRUE, wincx);
	if (rv != SECSuccess) {
	    return NULL;
	}
	keyh = PK11_MatchItem(slot,certh,CKO_PRIVATE_KEY);
    }
    if (keyh == CK_INVALID_HANDLE) { 
	return NULL; 
    }
    return PK11_MakePrivKey(slot, nullKey, PR_TRUE, keyh, wincx);
} 





PK11SlotInfo *
PK11_KeyForCertExists(CERTCertificate *cert, CK_OBJECT_HANDLE *keyPtr, 
								void *wincx) 
{
    PK11SlotList *list;
    PK11SlotListElement *le;
    SECItem *keyID;
    CK_OBJECT_HANDLE key;
    PK11SlotInfo *slot = NULL;
    SECStatus rv;
    int err;

    keyID = pk11_mkcertKeyID(cert);
    
    list = PK11_GetAllTokens(CKM_INVALID_MECHANISM,PR_FALSE,PR_TRUE,wincx);
    if ((keyID == NULL) || (list == NULL)) {
	if (keyID) SECITEM_FreeItem(keyID,PR_TRUE);
	if (list) PK11_FreeSlotList(list);
    	return NULL;
    }

    
    for (le = list->head ; le; le = le->next) {
	






	PRBool needLogin = pk11_LoginStillRequired(le->slot,wincx);
	key = pk11_FindPrivateKeyFromCertID(le->slot,keyID);
	if ((key == CK_INVALID_HANDLE) && needLogin &&
            		(SSL_ERROR_NO_CERTIFICATE == (err = PORT_GetError()) ||
			 SEC_ERROR_TOKEN_NOT_LOGGED_IN == err )) {
	    
	    rv = PK11_Authenticate(le->slot, PR_TRUE, wincx);
	    if (rv != SECSuccess) continue;
	    key = pk11_FindPrivateKeyFromCertID(le->slot,keyID);
	}
	if (key != CK_INVALID_HANDLE) {
	    slot = PK11_ReferenceSlot(le->slot);
	    if (keyPtr) *keyPtr = key;
	    break;
	}
    }

    SECITEM_FreeItem(keyID,PR_TRUE);
    PK11_FreeSlotList(list);
    return slot;

}




PK11SlotInfo *
PK11_KeyForDERCertExists(SECItem *derCert, CK_OBJECT_HANDLE *keyPtr, 
								void *wincx) 
{
    CERTCertificate *cert;
    PK11SlotInfo *slot = NULL;

    


    cert = CERT_DecodeDERCertificate(derCert, PR_FALSE, NULL);
    if (cert == NULL) return NULL;

    slot = PK11_KeyForCertExists(cert, keyPtr, wincx);
    CERT_DestroyCertificate (cert);
    return slot;
}

PK11SlotInfo *
PK11_ImportCertForKey(CERTCertificate *cert, const char *nickname,
                      void *wincx) 
{
    PK11SlotInfo *slot = NULL;
    CK_OBJECT_HANDLE key;

    slot = PK11_KeyForCertExists(cert,&key,wincx);

    if (slot) {
	if (PK11_ImportCert(slot,cert,key,nickname,PR_FALSE) != SECSuccess) {
	    PK11_FreeSlot(slot);
	    slot = NULL;
	}
    } else {
	PORT_SetError(SEC_ERROR_ADDING_CERT);
    }

    return slot;
}

PK11SlotInfo *
PK11_ImportDERCertForKey(SECItem *derCert, char *nickname,void *wincx) 
{
    CERTCertificate *cert;
    PK11SlotInfo *slot = NULL;

    cert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
                                   derCert, NULL, PR_FALSE, PR_TRUE);
    if (cert == NULL) return NULL;

    slot = PK11_ImportCertForKey(cert, nickname, wincx);
    CERT_DestroyCertificate (cert);
    return slot;
}

static CK_OBJECT_HANDLE
pk11_FindCertObjectByTemplate(PK11SlotInfo **slotPtr, 
		CK_ATTRIBUTE *searchTemplate, int count, void *wincx) 
{
    PK11SlotList *list;
    PK11SlotListElement *le;
    CK_OBJECT_HANDLE certHandle = CK_INVALID_HANDLE;
    PK11SlotInfo *slot = NULL;
    SECStatus rv;

    *slotPtr = NULL;

    
    list = PK11_GetAllTokens(CKM_INVALID_MECHANISM,PR_FALSE,PR_TRUE,wincx);
    if (list == NULL) {
    	return CK_INVALID_HANDLE;
    }


    
    for (le = list->head ; le; le = le->next) {
	rv = pk11_AuthenticateUnfriendly(le->slot, PR_TRUE, wincx);
	if (rv != SECSuccess) continue;

	certHandle = pk11_FindObjectByTemplate(le->slot,searchTemplate,count);
	if (certHandle != CK_INVALID_HANDLE) {
	    slot = PK11_ReferenceSlot(le->slot);
	    break;
	}
    }

    PK11_FreeSlotList(list);

    if (slot == NULL) {
	return CK_INVALID_HANDLE;
    }
    *slotPtr = slot;
    return certHandle;
}

CERTCertificate *
PK11_FindCertByIssuerAndSNOnToken(PK11SlotInfo *slot, 
				CERTIssuerAndSN *issuerSN, void *wincx)
{
    CERTCertificate *rvCert = NULL;
    NSSCertificate *cert = NULL;
    NSSDER issuer, serial;
    NSSTrustDomain *td = STAN_GetDefaultTrustDomain();
    NSSToken *token = slot->nssToken;
    nssSession *session;
    nssCryptokiObject *instance = NULL;
    nssPKIObject *object = NULL;
    SECItem *derSerial;
    PRStatus status;

    if (!issuerSN || !issuerSN->derIssuer.data || !issuerSN->derIssuer.len ||
        !issuerSN->serialNumber.data || !issuerSN->serialNumber.len || 
	issuerSN->derIssuer.len    > CERT_MAX_DN_BYTES ||
	issuerSN->serialNumber.len > CERT_MAX_SERIAL_NUMBER_BYTES ) {
    	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return NULL;
    }

    
    if (token == NULL) {
	PORT_SetError(SEC_ERROR_NO_TOKEN);
	return NULL;
    }


    



    derSerial = SEC_ASN1EncodeItem(NULL, NULL,
                                   &issuerSN->serialNumber,
                                   SEC_ASN1_GET(SEC_IntegerTemplate));
    if (!derSerial) {
	return NULL;
    }

    NSSITEM_FROM_SECITEM(&issuer, &issuerSN->derIssuer);
    NSSITEM_FROM_SECITEM(&serial, derSerial);

    session = nssToken_GetDefaultSession(token);
    if (!session) {
	goto loser;
    }

    instance = nssToken_FindCertificateByIssuerAndSerialNumber(token,session,
		&issuer, &serial, nssTokenSearchType_TokenForced, &status);

    SECITEM_FreeItem(derSerial, PR_TRUE);

    if (!instance) {
	goto loser;
    }
    object = nssPKIObject_Create(NULL, instance, td, NULL, nssPKIMonitor);
    if (!object) {
	goto loser;
    }
    instance = NULL; 
    cert = nssCertificate_Create(object);
    if (!cert) {
	goto loser;
    }
    object = NULL; 
    nssTrustDomain_AddCertsToCache(td, &cert,1);
    
    rvCert = STAN_GetCERTCertificate(cert);
    if (!rvCert) {
	goto loser;
    }
    return rvCert;

loser:
    if (instance) {
	nssCryptokiObject_Destroy(instance);
    }
    if (object) {
	nssPKIObject_Destroy(object);
    }
    if (cert) {
	nssCertificate_Destroy(cert);
    }
    return NULL;
}








static CERTCertificate *
pk11_FindCertObjectByRecipientNew(PK11SlotInfo *slot, NSSCMSRecipient **recipientlist, int *rlIndex, void *pwarg)
{
    NSSCMSRecipient *ri = NULL;
    int i;

    for (i=0; (ri = recipientlist[i]) != NULL; i++) {
	CERTCertificate *cert = NULL;
	if (ri->kind == RLSubjKeyID) {
	    SECItem *derCert = cert_FindDERCertBySubjectKeyID(ri->id.subjectKeyID);
	    if (derCert) {
		cert = PK11_FindCertFromDERCertItem(slot, derCert, pwarg);
		SECITEM_FreeItem(derCert, PR_TRUE);
	    }
	} else {
	    cert = PK11_FindCertByIssuerAndSNOnToken(slot, ri->id.issuerAndSN, 
						     pwarg);
	}
	if (cert) {
	    
	    if ((cert->trust == NULL) ||
       		((cert->trust->emailFlags & CERTDB_USER) != CERTDB_USER)) {
		 CERT_DestroyCertificate(cert);
		continue;
	    }
	    ri->slot = PK11_ReferenceSlot(slot);
	    *rlIndex = i;
	    return cert;
	}
    }
    *rlIndex = -1;
    return NULL;
}







static CERTCertificate *
pk11_AllFindCertObjectByRecipientNew(NSSCMSRecipient **recipientlist, void *wincx, int *rlIndex)
{
    PK11SlotList *list;
    PK11SlotListElement *le;
    CERTCertificate *cert = NULL;
    SECStatus rv;

    
    list = PK11_GetAllTokens(CKM_INVALID_MECHANISM,PR_FALSE,PR_TRUE,wincx);
    if (list == NULL) {
    	return CK_INVALID_HANDLE;
    }

    
    for (le = list->head ; le; le = le->next) {
	rv = pk11_AuthenticateUnfriendly(le->slot, PR_TRUE, wincx);
	if (rv != SECSuccess) continue;

	cert = pk11_FindCertObjectByRecipientNew(le->slot, 
					recipientlist, rlIndex, wincx);
	if (cert)
	    break;
    }

    PK11_FreeSlotList(list);

    return cert;
}





static CERTCertificate *
pk11_FindCertObjectByRecipient(PK11SlotInfo *slot, 
	SEC_PKCS7RecipientInfo **recipientArray,
	SEC_PKCS7RecipientInfo **rip, void *pwarg)
{
    SEC_PKCS7RecipientInfo *ri = NULL;
    int i;

    for (i=0; (ri = recipientArray[i]) != NULL; i++) {
	CERTCertificate *cert;

	cert = PK11_FindCertByIssuerAndSNOnToken(slot, ri->issuerAndSN, 
								pwarg);
        if (cert) {
	    
	    if ((cert->trust == NULL) ||
       		((cert->trust->emailFlags & CERTDB_USER) != CERTDB_USER)) {
		 CERT_DestroyCertificate(cert);
		continue;
	    }
	    *rip = ri;
	    return cert;
	}

    }
    *rip = NULL;
    return NULL;
}




static CERTCertificate *
pk11_AllFindCertObjectByRecipient(PK11SlotInfo **slotPtr, 
	SEC_PKCS7RecipientInfo **recipientArray,SEC_PKCS7RecipientInfo **rip,
							void *wincx) 
{
    PK11SlotList *list;
    PK11SlotListElement *le;
    CERTCertificate * cert = NULL;
    PK11SlotInfo *slot = NULL;
    SECStatus rv;

    *slotPtr = NULL;

    
    list = PK11_GetAllTokens(CKM_INVALID_MECHANISM,PR_FALSE,PR_TRUE,wincx);
    if (list == NULL) {
    	return CK_INVALID_HANDLE;
    }

    *rip = NULL;

    
    for (le = list->head ; le; le = le->next) {
	rv = pk11_AuthenticateUnfriendly(le->slot, PR_TRUE, wincx);
	if (rv != SECSuccess) continue;

	cert = pk11_FindCertObjectByRecipient(le->slot, recipientArray, 
							rip, wincx);
	if (cert) {
	    slot = PK11_ReferenceSlot(le->slot);
	    break;
	}
    }

    PK11_FreeSlotList(list);

    if (slot == NULL) {
	return NULL;
    }
    *slotPtr = slot;
    PORT_Assert(cert != NULL);
    return cert;
}








CERTCertificate *
PK11_FindCertAndKeyByRecipientList(PK11SlotInfo **slotPtr, 
	SEC_PKCS7RecipientInfo **array, SEC_PKCS7RecipientInfo **rip,
				SECKEYPrivateKey**privKey, void *wincx)
{
    CERTCertificate *cert = NULL;

    *privKey = NULL;
    *slotPtr = NULL;
    cert = pk11_AllFindCertObjectByRecipient(slotPtr,array,rip,wincx);
    if (!cert) {
	return NULL;
    }

    *privKey = PK11_FindKeyByAnyCert(cert, wincx);
    if (*privKey == NULL) {
	goto loser;
    }

    return cert;
loser:
    if (cert) CERT_DestroyCertificate(cert);
    if (*slotPtr) PK11_FreeSlot(*slotPtr);
    *slotPtr = NULL;
    return NULL;
}

static PRCallOnceType keyIDHashCallOnce;

static PRStatus PR_CALLBACK
pk11_keyIDHash_populate(void *wincx)
{
    CERTCertList     *certList;
    CERTCertListNode *node = NULL;
    SECItem           subjKeyID = {siBuffer, NULL, 0};

    certList = PK11_ListCerts(PK11CertListUser, wincx);
    if (!certList) {
	return PR_FAILURE;
    }

    for (node = CERT_LIST_HEAD(certList);
         !CERT_LIST_END(node, certList);
         node = CERT_LIST_NEXT(node)) {
	if (CERT_FindSubjectKeyIDExtension(node->cert, 
	                                   &subjKeyID) == SECSuccess && 
	    subjKeyID.data != NULL) {
	    cert_AddSubjectKeyIDMapping(&subjKeyID, node->cert);
	    SECITEM_FreeItem(&subjKeyID, PR_FALSE);
	}
    }
    CERT_DestroyCertList(certList);
    return PR_SUCCESS;
}






int
PK11_FindCertAndKeyByRecipientListNew(NSSCMSRecipient **recipientlist, void *wincx)
{
    CERTCertificate *cert;
    NSSCMSRecipient *rl;
    PRStatus rv;
    int rlIndex;

    rv = PR_CallOnceWithArg(&keyIDHashCallOnce, pk11_keyIDHash_populate, wincx);
    if (rv != PR_SUCCESS)
	return -1;

    cert = pk11_AllFindCertObjectByRecipientNew(recipientlist, wincx, &rlIndex);
    if (!cert) {
	return -1;
    }

    rl = recipientlist[rlIndex];

    

    rl->privkey = PK11_FindKeyByAnyCert(cert, wincx);
    if (rl->privkey == NULL) {
	goto loser;
    }

    
    rl->cert = cert;
    return rlIndex;

loser:
    if (cert) CERT_DestroyCertificate(cert);
    if (rl->slot) PK11_FreeSlot(rl->slot);
    rl->slot = NULL;
    return -1;
}

CERTCertificate *
PK11_FindCertByIssuerAndSN(PK11SlotInfo **slotPtr, CERTIssuerAndSN *issuerSN,
							 void *wincx)
{
    CERTCertificate *rvCert = NULL;
    NSSCertificate *cert;
    NSSDER issuer, serial;
    NSSCryptoContext *cc;
    SECItem *derSerial;

    if (!issuerSN || !issuerSN->derIssuer.data || !issuerSN->derIssuer.len ||
        !issuerSN->serialNumber.data || !issuerSN->serialNumber.len || 
	issuerSN->derIssuer.len    > CERT_MAX_DN_BYTES ||
	issuerSN->serialNumber.len > CERT_MAX_SERIAL_NUMBER_BYTES ) {
    	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return NULL;
    }

    if (slotPtr) *slotPtr = NULL;

    



    derSerial = SEC_ASN1EncodeItem(NULL, NULL,
                                   &issuerSN->serialNumber,
                                   SEC_ASN1_GET(SEC_IntegerTemplate));
    if (!derSerial) {
	return NULL;
    }

    NSSITEM_FROM_SECITEM(&issuer, &issuerSN->derIssuer);
    NSSITEM_FROM_SECITEM(&serial, derSerial);

    cc = STAN_GetDefaultCryptoContext();
    cert = NSSCryptoContext_FindCertificateByIssuerAndSerialNumber(cc, 
                                                                &issuer, 
                                                                &serial);
    if (cert) {
	SECITEM_FreeItem(derSerial, PR_TRUE);
	return STAN_GetCERTCertificateOrRelease(cert);
    }

    do {
	
	if (rvCert) {
	    CERT_DestroyCertificate(rvCert);
	    rvCert = NULL;
 	}

	cert = NSSTrustDomain_FindCertificateByIssuerAndSerialNumber(
                                                  STAN_GetDefaultTrustDomain(),
                                                  &issuer,
                                                  &serial);
	if (!cert) {
	    break;
	}

	rvCert = STAN_GetCERTCertificateOrRelease(cert);
	if (rvCert == NULL) {
	   break;
	}

	
    } while (!PK11_IsPresent(rvCert->slot));

    if (rvCert && slotPtr) *slotPtr = PK11_ReferenceSlot(rvCert->slot);

    SECITEM_FreeItem(derSerial, PR_TRUE);
    return rvCert;
}

CK_OBJECT_HANDLE
PK11_FindObjectForCert(CERTCertificate *cert, void *wincx, PK11SlotInfo **pSlot)
{
    CK_OBJECT_HANDLE certHandle;
    CK_ATTRIBUTE searchTemplate	= { CKA_VALUE, NULL, 0 };
    
    PK11_SETATTRS(&searchTemplate, CKA_VALUE, cert->derCert.data,
		  cert->derCert.len);

    if (cert->slot) {
	certHandle = pk11_getcerthandle(cert->slot,cert,&searchTemplate,1);
	if (certHandle != CK_INVALID_HANDLE) {
	    *pSlot = PK11_ReferenceSlot(cert->slot);
	    return certHandle;
	}
    }

    certHandle = pk11_FindCertObjectByTemplate(pSlot,&searchTemplate,1,wincx);
    if (certHandle != CK_INVALID_HANDLE) {
	if (cert->slot == NULL) {
	    cert->slot = PK11_ReferenceSlot(*pSlot);
	    cert->pkcs11ID = certHandle;
	    cert->ownSlot = PR_TRUE;
	    cert->series = cert->slot->series;
	}
    }

    return(certHandle);
}

SECKEYPrivateKey *
PK11_FindKeyByAnyCert(CERTCertificate *cert, void *wincx)
{
    CK_OBJECT_HANDLE certHandle;
    CK_OBJECT_HANDLE keyHandle;
    PK11SlotInfo *slot = NULL;
    SECKEYPrivateKey *privKey = NULL;
    PRBool needLogin;
    SECStatus rv;
    int err;

    certHandle = PK11_FindObjectForCert(cert, wincx, &slot);
    if (certHandle == CK_INVALID_HANDLE) {
	 return NULL;
    }
    






    needLogin = pk11_LoginStillRequired(slot,wincx);
    keyHandle = PK11_MatchItem(slot,certHandle,CKO_PRIVATE_KEY);
    if ((keyHandle == CK_INVALID_HANDLE) &&  needLogin &&
			(SSL_ERROR_NO_CERTIFICATE == (err = PORT_GetError()) ||
			 SEC_ERROR_TOKEN_NOT_LOGGED_IN == err ) ) {
	
	rv = PK11_Authenticate(slot, PR_TRUE, wincx);
	if (rv == SECSuccess) {
            keyHandle = PK11_MatchItem(slot,certHandle,CKO_PRIVATE_KEY);
	}
    }
    if (keyHandle != CK_INVALID_HANDLE) { 
        privKey =  PK11_MakePrivKey(slot, nullKey, PR_TRUE, keyHandle, wincx);
    }
    if (slot) {
	PK11_FreeSlot(slot);
    }
    return privKey;
}

CK_OBJECT_HANDLE
pk11_FindPubKeyByAnyCert(CERTCertificate *cert, PK11SlotInfo **slot, void *wincx)
{
    CK_OBJECT_HANDLE certHandle;
    CK_OBJECT_HANDLE keyHandle;

    certHandle = PK11_FindObjectForCert(cert, wincx, slot);
    if (certHandle == CK_INVALID_HANDLE) {
	 return CK_INVALID_HANDLE;
    }
    keyHandle = PK11_MatchItem(*slot,certHandle,CKO_PUBLIC_KEY);
    if (keyHandle == CK_INVALID_HANDLE) { 
	PK11_FreeSlot(*slot);
	return CK_INVALID_HANDLE;
    }
    return keyHandle;
}




int
PK11_NumberCertsForCertSubject(CERTCertificate *cert)
{
    CK_OBJECT_CLASS certClass = CKO_CERTIFICATE;
    CK_ATTRIBUTE theTemplate[] = {
	{ CKA_CLASS, NULL, 0 },
	{ CKA_SUBJECT, NULL, 0 },
    };
    CK_ATTRIBUTE *attr = theTemplate;
   int templateSize = sizeof(theTemplate)/sizeof(theTemplate[0]);

    PK11_SETATTRS(attr,CKA_CLASS, &certClass, sizeof(certClass)); attr++;
    PK11_SETATTRS(attr,CKA_SUBJECT,cert->derSubject.data,cert->derSubject.len);

    if (cert->slot == NULL) {
	PK11SlotList *list = PK11_GetAllTokens(CKM_INVALID_MECHANISM,
							PR_FALSE,PR_TRUE,NULL);
	PK11SlotListElement *le;
	int count = 0;

	if (!list) {
            
            return 0;
	}

	
	for (le = list->head; le; le = le->next) {
	    count += PK11_NumberObjectsFor(le->slot,theTemplate,templateSize);
	}
	PK11_FreeSlotList(list);
	return count;
    }

    return PK11_NumberObjectsFor(cert->slot,theTemplate,templateSize);
}




SECStatus
PK11_TraverseCertsForSubject(CERTCertificate *cert,
        SECStatus(* callback)(CERTCertificate*, void *), void *arg)
{
    if(!cert) {
	return SECFailure;
    }
    if (cert->slot == NULL) {
	PK11SlotList *list = PK11_GetAllTokens(CKM_INVALID_MECHANISM,
							PR_FALSE,PR_TRUE,NULL);
	PK11SlotListElement *le;

	if (!list) {
            
            return SECFailure;
	}
	
	for (le = list->head; le; le = le->next) {
	    PK11_TraverseCertsForSubjectInSlot(cert,le->slot,callback,arg);
	}
	PK11_FreeSlotList(list);
	return SECSuccess;

    }

    return PK11_TraverseCertsForSubjectInSlot(cert, cert->slot, callback, arg);
}

SECStatus
PK11_TraverseCertsForSubjectInSlot(CERTCertificate *cert, PK11SlotInfo *slot,
	SECStatus(* callback)(CERTCertificate*, void *), void *arg)
{
    PRStatus nssrv = PR_SUCCESS;
    NSSToken *token;
    NSSDER subject;
    NSSTrustDomain *td;
    nssList *subjectList;
    nssPKIObjectCollection *collection;
    nssCryptokiObject **instances;
    NSSCertificate **certs;
    nssTokenSearchType tokenOnly = nssTokenSearchType_TokenOnly;
    td = STAN_GetDefaultTrustDomain();
    NSSITEM_FROM_SECITEM(&subject, &cert->derSubject);
    token = PK11Slot_GetNSSToken(slot);
    if (!nssToken_IsPresent(token)) {
	return SECSuccess;
    }
    collection = nssCertificateCollection_Create(td, NULL);
    if (!collection) {
	return SECFailure;
    }
    subjectList = nssList_Create(NULL, PR_FALSE);
    if (!subjectList) {
	nssPKIObjectCollection_Destroy(collection);
	return SECFailure;
    }
    (void)nssTrustDomain_GetCertsForSubjectFromCache(td, &subject, 
                                                     subjectList);
    transfer_token_certs_to_collection(subjectList, token, collection);
    instances = nssToken_FindCertificatesBySubject(token, NULL,
	                                           &subject, 
	                                           tokenOnly, 0, &nssrv);
    nssPKIObjectCollection_AddInstances(collection, instances, 0);
    nss_ZFreeIf(instances);
    nssList_Destroy(subjectList);
    certs = nssPKIObjectCollection_GetCertificates(collection,
                                                   NULL, 0, NULL);
    nssPKIObjectCollection_Destroy(collection);
    if (certs) {
	CERTCertificate *oldie;
	NSSCertificate **cp;
	for (cp = certs; *cp; cp++) {
	    oldie = STAN_GetCERTCertificate(*cp);
	    if (!oldie) {
		continue;
	    }
	    if ((*callback)(oldie, arg) != SECSuccess) {
		nssrv = PR_FAILURE;
		break;
	    }
	}
	nssCertificateArray_Destroy(certs);
    }
    return (nssrv == PR_SUCCESS) ? SECSuccess : SECFailure;
}

SECStatus
PK11_TraverseCertsForNicknameInSlot(SECItem *nickname, PK11SlotInfo *slot,
	SECStatus(* callback)(CERTCertificate*, void *), void *arg)
{
    struct nss3_cert_cbstr pk11cb;
    PRStatus nssrv = PR_SUCCESS;
    NSSToken *token;
    NSSTrustDomain *td;
    NSSUTF8 *nick;
    PRBool created = PR_FALSE;
    nssCryptokiObject **instances;
    nssPKIObjectCollection *collection = NULL;
    NSSCertificate **certs;
    nssList *nameList = NULL;
    nssTokenSearchType tokenOnly = nssTokenSearchType_TokenOnly;
    pk11cb.callback = callback;
    pk11cb.arg = arg;
    token = PK11Slot_GetNSSToken(slot);
    if (!nssToken_IsPresent(token)) {
	return SECSuccess;
    }
    if (nickname->data[nickname->len-1] != '\0') {
	nick = nssUTF8_Create(NULL, nssStringType_UTF8String, 
	                      nickname->data, nickname->len);
	created = PR_TRUE;
    } else {
	nick = (NSSUTF8 *)nickname->data;
    }
    td = STAN_GetDefaultTrustDomain();
    collection = nssCertificateCollection_Create(td, NULL);
    if (!collection) {
	goto loser;
    }
    nameList = nssList_Create(NULL, PR_FALSE);
    if (!nameList) {
	goto loser;
    }
    (void)nssTrustDomain_GetCertsForNicknameFromCache(td, nick, nameList);
    transfer_token_certs_to_collection(nameList, token, collection);
    instances = nssToken_FindCertificatesByNickname(token, NULL,
	                                            nick,
	                                            tokenOnly, 0, &nssrv);
    nssPKIObjectCollection_AddInstances(collection, instances, 0);
    nss_ZFreeIf(instances);
    nssList_Destroy(nameList);
    certs = nssPKIObjectCollection_GetCertificates(collection,
                                                   NULL, 0, NULL);
    nssPKIObjectCollection_Destroy(collection);
    if (certs) {
	CERTCertificate *oldie;
	NSSCertificate **cp;
	for (cp = certs; *cp; cp++) {
	    oldie = STAN_GetCERTCertificate(*cp);
	    if (!oldie) {
		continue;
	    }
	    if ((*callback)(oldie, arg) != SECSuccess) {
		nssrv = PR_FAILURE;
		break;
	    }
	}
	nssCertificateArray_Destroy(certs);
    }
    if (created) nss_ZFreeIf(nick);
    return (nssrv == PR_SUCCESS) ? SECSuccess : SECFailure;
loser:
    if (created) {
	nss_ZFreeIf(nick);
    }
    if (collection) {
	nssPKIObjectCollection_Destroy(collection);
    }
    if (nameList) {
	nssList_Destroy(nameList);
    }
    return SECFailure;
}

SECStatus
PK11_TraverseCertsInSlot(PK11SlotInfo *slot,
	SECStatus(* callback)(CERTCertificate*, void *), void *arg)
{
    PRStatus nssrv;
    NSSTrustDomain *td = STAN_GetDefaultTrustDomain();
    NSSToken *tok;
    nssList *certList = NULL;
    nssCryptokiObject **instances;
    nssPKIObjectCollection *collection;
    NSSCertificate **certs;
    nssTokenSearchType tokenOnly = nssTokenSearchType_TokenOnly;
    tok = PK11Slot_GetNSSToken(slot);
    if (!nssToken_IsPresent(tok)) {
	return SECSuccess;
    }
    collection = nssCertificateCollection_Create(td, NULL);
    if (!collection) {
	return SECFailure;
    }
    certList = nssList_Create(NULL, PR_FALSE);
    if (!certList) {
	nssPKIObjectCollection_Destroy(collection);
	return SECFailure;
    }
    (void *)nssTrustDomain_GetCertsFromCache(td, certList);
    transfer_token_certs_to_collection(certList, tok, collection);
    instances = nssToken_FindObjects(tok, NULL, CKO_CERTIFICATE,
                                     tokenOnly, 0, &nssrv);
    nssPKIObjectCollection_AddInstances(collection, instances, 0);
    nss_ZFreeIf(instances);
    nssList_Destroy(certList);
    certs = nssPKIObjectCollection_GetCertificates(collection,
                                                   NULL, 0, NULL);
    nssPKIObjectCollection_Destroy(collection);
    if (certs) {
	CERTCertificate *oldie;
	NSSCertificate **cp;
	for (cp = certs; *cp; cp++) {
	    oldie = STAN_GetCERTCertificate(*cp);
	    if (!oldie) {
		continue;
	    }
	    if ((*callback)(oldie, arg) != SECSuccess) {
		nssrv = PR_FAILURE;
		break;
	    }
	}
	nssCertificateArray_Destroy(certs);
    }
    return (nssrv == PR_SUCCESS) ? SECSuccess : SECFailure;
}




CERTCertificate *
PK11_FindCertFromDERCert(PK11SlotInfo *slot, CERTCertificate *cert,
								 void *wincx)
{
    return PK11_FindCertFromDERCertItem(slot, &cert->derCert, wincx);
}

CERTCertificate *
PK11_FindCertFromDERCertItem(PK11SlotInfo *slot, SECItem *inDerCert,
								 void *wincx)

{
    NSSCertificate *c;
    NSSDER derCert;
    NSSToken *tok;
    NSSTrustDomain *td = STAN_GetDefaultTrustDomain();
    SECStatus rv;

    tok = PK11Slot_GetNSSToken(slot);
    NSSITEM_FROM_SECITEM(&derCert, inDerCert);
    rv = pk11_AuthenticateUnfriendly(slot, PR_TRUE, wincx);
    if (rv != SECSuccess) {
	PK11_FreeSlot(slot);
	return NULL;
    }
    c = NSSTrustDomain_FindCertificateByEncodedCertificate(td, &derCert);
    if (c) {
	PRBool isToken = PR_FALSE;
	NSSToken **tp;
	NSSToken **tokens = nssPKIObject_GetTokens(&c->object, NULL);
	if (tokens) {
	    for (tp = tokens; *tp; tp++) {
		if (*tp == tok) {
		    isToken = PR_TRUE;
		    break;
		}
	    }
	    if (!isToken) {
		NSSCertificate_Destroy(c);
		c = NULL;
	    }
	    nssTokenArray_Destroy(tokens);
	}
    }
    return c ? STAN_GetCERTCertificateOrRelease(c) : NULL;
} 





static CK_OBJECT_HANDLE 
pk11_findKeyObjectByDERCert(PK11SlotInfo *slot, CERTCertificate *cert, 
								void *wincx)
{
    SECItem *keyID;
    CK_OBJECT_HANDLE key;
    SECStatus rv;
    PRBool needLogin;
    int err;

    if((slot == NULL) || (cert == NULL)) {
	return CK_INVALID_HANDLE;
    }

    keyID = pk11_mkcertKeyID(cert);
    if(keyID == NULL) {
	return CK_INVALID_HANDLE;
    }

    






    needLogin = pk11_LoginStillRequired(slot,wincx);
    key = pk11_FindPrivateKeyFromCertID(slot, keyID);
    if ((key == CK_INVALID_HANDLE) && needLogin &&
			(SSL_ERROR_NO_CERTIFICATE == (err = PORT_GetError()) ||
			 SEC_ERROR_TOKEN_NOT_LOGGED_IN == err )) {
	
	rv = PK11_Authenticate(slot, PR_TRUE, wincx);
	if (rv != SECSuccess) goto loser;
	key = pk11_FindPrivateKeyFromCertID(slot, keyID);
   }

loser:
    SECITEM_ZfreeItem(keyID, PR_TRUE);
    return key;
}

SECKEYPrivateKey *
PK11_FindKeyByDERCert(PK11SlotInfo *slot, CERTCertificate *cert, 
								void *wincx)
{
    CK_OBJECT_HANDLE keyHandle;

    if((slot == NULL) || (cert == NULL)) {
	return NULL;
    }

    keyHandle = pk11_findKeyObjectByDERCert(slot, cert, wincx);
    if (keyHandle == CK_INVALID_HANDLE) {
	return NULL;
    }

    return PK11_MakePrivKey(slot,nullKey,PR_TRUE,keyHandle,wincx);
}

SECStatus
PK11_ImportCertForKeyToSlot(PK11SlotInfo *slot, CERTCertificate *cert, 
						char *nickname, 
						PRBool addCertUsage,void *wincx)
{
    CK_OBJECT_HANDLE keyHandle;

    if((slot == NULL) || (cert == NULL) || (nickname == NULL)) {
	return SECFailure;
    }

    keyHandle = pk11_findKeyObjectByDERCert(slot, cert, wincx);
    if (keyHandle == CK_INVALID_HANDLE) {
	return SECFailure;
    }

    return PK11_ImportCert(slot, cert, keyHandle, nickname, addCertUsage);
}   



#define SEC_OID_MISSI_KEA 300  /* until we have v3 stuff merged */
PRBool
KEAPQGCompare(CERTCertificate *server,CERTCertificate *cert) {

    if ( SECKEY_KEAParamCompare(server,cert) == SECEqual ) {
        return PR_TRUE;
    } else {
	return PR_FALSE;
    }
}

PRBool
PK11_FortezzaHasKEA(CERTCertificate *cert) 
{
   
   SECOidData *oid;

   if ((cert->trust == NULL) ||
       ((cert->trust->sslFlags & CERTDB_USER) != CERTDB_USER)) {
       return PR_FALSE;
   }

   oid = SECOID_FindOID(&cert->subjectPublicKeyInfo.algorithm.algorithm);
   if (!oid) {
       return PR_FALSE;
   }

   return (PRBool)((oid->offset == SEC_OID_MISSI_KEA_DSS_OLD) || 
		(oid->offset == SEC_OID_MISSI_KEA_DSS) ||
				(oid->offset == SEC_OID_MISSI_KEA)) ;
}




static CERTCertificate
*pk11_GetKEAMate(PK11SlotInfo *slot,CERTCertificate *peer)
{
    int i;
    CERTCertificate *returnedCert = NULL;

    for (i=0; i < slot->cert_count; i++) {
	CERTCertificate *cert = slot->cert_array[i];

	if (PK11_FortezzaHasKEA(cert) && KEAPQGCompare(peer,cert)) {
		returnedCert = CERT_DupCertificate(cert);
		break;
	}
    }
    return returnedCert;
}







CERTCertificate *
PK11_FindBestKEAMatch(CERTCertificate *server, void *wincx)
{
    PK11SlotList *keaList = PK11_GetAllTokens(CKM_KEA_KEY_DERIVE,
							PR_FALSE,PR_TRUE,wincx);
    PK11SlotListElement *le;
    CERTCertificate *returnedCert = NULL;
    SECStatus rv;

    if (!keaList) {
        
        return NULL;
    }

    
    for (le = keaList->head; le; le = le->next) {
        rv = PK11_Authenticate(le->slot, PR_TRUE, wincx);
        if (rv != SECSuccess) continue;
	if (le->slot->session == CK_INVALID_SESSION) {
	    continue;
	}
	returnedCert = pk11_GetKEAMate(le->slot,server);
	if (returnedCert) break;
    }
    PK11_FreeSlotList(keaList);

    return returnedCert;
}





SECStatus
PK11_GetKEAMatchedCerts(PK11SlotInfo *slot1, PK11SlotInfo *slot2,
	CERTCertificate **cert1, CERTCertificate **cert2)
{
    CERTCertificate *returnedCert = NULL;
    int i;

    for (i=0; i < slot1->cert_count; i++) {
	CERTCertificate *cert = slot1->cert_array[i];

	if (PK11_FortezzaHasKEA(cert)) {
	    returnedCert = pk11_GetKEAMate(slot2,cert);
	    if (returnedCert != NULL) {
		*cert2 = returnedCert;
		*cert1 = CERT_DupCertificate(cert);
		return SECSuccess;
	    }
	}
    }
    return SECFailure;
}




CK_OBJECT_HANDLE
PK11_FindCertInSlot(PK11SlotInfo *slot, CERTCertificate *cert, void *wincx)
{
    CK_OBJECT_CLASS certClass = CKO_CERTIFICATE;
    CK_ATTRIBUTE theTemplate[] = {
	{ CKA_VALUE, NULL, 0 },
	{ CKA_CLASS, NULL, 0 }
    };
    
    int tsize = sizeof(theTemplate)/sizeof(theTemplate[0]);
    CK_ATTRIBUTE *attrs = theTemplate;
    SECStatus rv;

    PK11_SETATTRS(attrs, CKA_VALUE, cert->derCert.data,
						cert->derCert.len); attrs++;
    PK11_SETATTRS(attrs, CKA_CLASS, &certClass, sizeof(certClass));

    


    rv = pk11_AuthenticateUnfriendly(slot, PR_TRUE, wincx);
    if (rv != SECSuccess) {
	return CK_INVALID_HANDLE;
    }

    return pk11_getcerthandle(slot,cert,theTemplate,tsize);
}






struct listCertsStr {
    PK11CertListType type;
    CERTCertList *certList;
};

static PRStatus
pk11ListCertCallback(NSSCertificate *c, void *arg)
{
    struct listCertsStr *listCertP = (struct listCertsStr *)arg;
    CERTCertificate *newCert = NULL;
    PK11CertListType type = listCertP->type;
    CERTCertList *certList = listCertP->certList;
    PRBool isUnique = PR_FALSE;
    PRBool isCA = PR_FALSE;
    char *nickname = NULL;
    unsigned int certType;

    if ((type == PK11CertListUnique) || (type == PK11CertListRootUnique) ||
        (type == PK11CertListCAUnique) || (type == PK11CertListUserUnique) ) {
        
	isUnique = PR_TRUE;
    }
    if ((type == PK11CertListCA) || (type == PK11CertListRootUnique) ||
        (type == PK11CertListCAUnique)) {
	isCA = PR_TRUE;
    }

    
    if ( ( (type == PK11CertListUser) || (type == PK11CertListUserUnique) ) && 
		!NSSCertificate_IsPrivateKeyAvailable(c, NULL,NULL)) {
	return PR_SUCCESS;
    }

    



    if ((type == PK11CertListRootUnique) && 
		NSSCertificate_IsPrivateKeyAvailable(c, NULL,NULL)) {
	return PR_SUCCESS;
    }

    
    newCert = STAN_GetCERTCertificate(c);
    if (!newCert) {
	return PR_SUCCESS;
    }
    
    if( isCA  && (!CERT_IsCACert(newCert, &certType)) ) {
	return PR_SUCCESS;
    }
    if (isUnique) {
	CERT_DupCertificate(newCert);

	nickname = STAN_GetCERTCertificateName(certList->arena, c);

	
	if (newCert->slot && !PK11_IsInternal(newCert->slot)) {
	    CERT_AddCertToListTailWithData(certList,newCert,nickname);
	} else {
	    CERT_AddCertToListHeadWithData(certList,newCert,nickname);
	}
    } else {
	
	nssCryptokiObject **ip;
	nssCryptokiObject **instances = nssPKIObject_GetInstances(&c->object);
	if (!instances) {
	    return PR_SUCCESS;
	}
	for (ip = instances; *ip; ip++) {
	    nssCryptokiObject *instance = *ip;
	    PK11SlotInfo *slot = instance->token->pk11slot;

	    
	    CERT_DupCertificate(newCert);

	    nickname = STAN_GetCERTCertificateNameForInstance(
			certList->arena, c, instance);

	    
	    if (slot && !PK11_IsInternal(slot)) {
		CERT_AddCertToListTailWithData(certList,newCert,nickname);
	    } else {
		CERT_AddCertToListHeadWithData(certList,newCert,nickname);
	    }
	}
	nssCryptokiObjectArray_Destroy(instances);
    }
    return PR_SUCCESS;
}


CERTCertList *
PK11_ListCerts(PK11CertListType type, void *pwarg)
{
    NSSTrustDomain *defaultTD = STAN_GetDefaultTrustDomain();
    CERTCertList *certList = NULL;
    struct listCertsStr listCerts;
    certList = CERT_NewCertList();
    listCerts.type = type;
    listCerts.certList = certList;

    
    (void) pk11_TraverseAllSlots( NULL, NULL, PR_TRUE, pwarg);
    NSSTrustDomain_TraverseCertificates(defaultTD, pk11ListCertCallback,
								 &listCerts);
    return certList;
}
    
SECItem *
PK11_GetLowLevelKeyIDForCert(PK11SlotInfo *slot,
					CERTCertificate *cert, void *wincx)
{
    CK_ATTRIBUTE theTemplate[] = {
	{ CKA_VALUE, NULL, 0 },
	{ CKA_CLASS, NULL, 0 }
    };
    
    int tsize = sizeof(theTemplate)/sizeof(theTemplate[0]);
    CK_OBJECT_HANDLE certHandle;
    CK_ATTRIBUTE *attrs = theTemplate;
    PK11SlotInfo *slotRef = NULL;
    SECItem *item;
    SECStatus rv;

    if (slot) {
	PK11_SETATTRS(attrs, CKA_VALUE, cert->derCert.data, 
						cert->derCert.len); attrs++;
 
	rv = pk11_AuthenticateUnfriendly(slot, PR_TRUE, wincx);
	if (rv != SECSuccess) {
	    return NULL;
	}
        certHandle = pk11_getcerthandle(slot,cert,theTemplate,tsize);
    } else {
    	certHandle = PK11_FindObjectForCert(cert, wincx, &slotRef);
	if (certHandle == CK_INVALID_HANDLE) {
	   return pk11_mkcertKeyID(cert);
	}
	slot = slotRef;
    }

    if (certHandle == CK_INVALID_HANDLE) {
	 return NULL;
    }

    item = pk11_GetLowLevelKeyFromHandle(slot,certHandle);
    if (slotRef) PK11_FreeSlot(slotRef);
    return item;
}


typedef struct {
    CERTCertList *list;
    PK11SlotInfo *slot;
} ListCertsArg;

static SECStatus
listCertsCallback(CERTCertificate* cert, void*arg)
{
    ListCertsArg *cdata = (ListCertsArg*)arg;
    char *nickname = NULL;
    nssCryptokiObject *instance, **ci;
    nssCryptokiObject **instances;
    NSSCertificate *c = STAN_GetNSSCertificate(cert);

    if (c == NULL) {
        return SECFailure;
    }
    instances = nssPKIObject_GetInstances(&c->object);
    if (!instances) {
        return SECFailure;
    }
    instance = NULL;
    for (ci = instances; *ci; ci++) {
	if ((*ci)->token->pk11slot == cdata->slot) {
	    instance = *ci;
	    break;
	}
    }
    PORT_Assert(instance != NULL);
    if (!instance) {
	nssCryptokiObjectArray_Destroy(instances);
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return SECFailure;
    }
    nickname = STAN_GetCERTCertificateNameForInstance(cdata->list->arena,
	c, instance);
    nssCryptokiObjectArray_Destroy(instances);

    return CERT_AddCertToListTailWithData(cdata->list, 
				CERT_DupCertificate(cert),nickname);
}

CERTCertList *
PK11_ListCertsInSlot(PK11SlotInfo *slot)
{
    SECStatus status;
    CERTCertList *certs;
    ListCertsArg cdata;

    certs = CERT_NewCertList();
    if(certs == NULL) return NULL;
    cdata.list = certs;
    cdata.slot = slot;

    status = PK11_TraverseCertsInSlot(slot, listCertsCallback,
		&cdata);

    if( status != SECSuccess ) {
	CERT_DestroyCertList(certs);
	certs = NULL;
    }

    return certs;
}

PK11SlotList *
PK11_GetAllSlotsForCert(CERTCertificate *cert, void *arg)
{
    NSSCertificate *c = STAN_GetNSSCertificate(cert);
    
    nssCryptokiObject **ip;
    nssCryptokiObject **instances = nssPKIObject_GetInstances(&c->object);
    PK11SlotList *slotList;
    PRBool found = PR_FALSE;

    if (!cert) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return NULL;
    }

    if (!instances) {
	PORT_SetError(SEC_ERROR_NO_TOKEN);
	return NULL;
    }

    slotList = PK11_NewSlotList();
    if (!slotList) {
	nssCryptokiObjectArray_Destroy(instances);
	return NULL;
    }

    for (ip = instances; *ip; ip++) {
	nssCryptokiObject *instance = *ip;
	PK11SlotInfo *slot = instance->token->pk11slot;
	if (slot) {
	    PK11_AddSlotToList(slotList, slot);
	    found = PR_TRUE;
	}
    }
    if (!found) {
	PK11_FreeSlotList(slotList);
	PORT_SetError(SEC_ERROR_NO_TOKEN);
	slotList = NULL;
    }

    nssCryptokiObjectArray_Destroy(instances);
    return slotList;
}
