









































#include "cmslocal.h"

#include "cert.h"
#include "key.h"
#include "secasn1.h"
#include "secitem.h"
#include "secoid.h"
#include "pk11func.h"
#include "secerr.h"

PRBool
nss_cmsrecipientinfo_usessubjectkeyid(NSSCMSRecipientInfo *ri)
{
    if (ri->recipientInfoType == NSSCMSRecipientInfoID_KeyTrans) {
	NSSCMSRecipientIdentifier *rid;
	rid = &ri->ri.keyTransRecipientInfo.recipientIdentifier;
	if (rid->identifierType == NSSCMSRecipientID_SubjectKeyID) {
	    return PR_TRUE;
	}
    }
    return PR_FALSE;
}









static const SECOidData fakeContent;
NSSCMSRecipientInfo *
nss_cmsrecipientinfo_create(NSSCMSMessage *cmsg, 
			    NSSCMSRecipientIDSelector type,
                            CERTCertificate *cert, 
			    SECKEYPublicKey *pubKey, 
                            SECItem *subjKeyID, 
			    void* pwfn_arg, 
			    SECItem* DERinput)
{
    NSSCMSRecipientInfo *ri;
    void *mark;
    SECOidTag certalgtag;
    SECStatus rv = SECSuccess;
    NSSCMSRecipientEncryptedKey *rek;
    NSSCMSOriginatorIdentifierOrKey *oiok;
    unsigned long version;
    SECItem *dummy;
    PLArenaPool *poolp;
    CERTSubjectPublicKeyInfo *spki, *freeSpki = NULL;
    NSSCMSRecipientIdentifier *rid;
    extern const SEC_ASN1Template NSSCMSRecipientInfoTemplate[];

    if (!cmsg) {
	

	cmsg = NSS_CMSMessage_Create(NULL);
        cmsg->pwfn_arg = pwfn_arg;
	
	cmsg->contentInfo.contentTypeTag = (SECOidData *)&fakeContent;
    }

    poolp = cmsg->poolp;

    mark = PORT_ArenaMark(poolp);

    ri = (NSSCMSRecipientInfo *)PORT_ArenaZAlloc(poolp, sizeof(NSSCMSRecipientInfo));
    if (ri == NULL)
	goto loser;

    ri->cmsg = cmsg;

    if (DERinput) {
        
        SECItem newinput;
        SECStatus rv = SECITEM_CopyItem(poolp, &newinput, DERinput);
        if (SECSuccess != rv)
            goto loser;
        rv = SEC_QuickDERDecodeItem(poolp, ri, NSSCMSRecipientInfoTemplate, &newinput);
        if (SECSuccess != rv)
            goto loser;
    }

    switch (type) {
        case NSSCMSRecipientID_IssuerSN:
        {
            ri->cert = CERT_DupCertificate(cert);
            if (NULL == ri->cert)
                goto loser;
            spki = &(cert->subjectPublicKeyInfo);
            break;
        }
        
        case NSSCMSRecipientID_SubjectKeyID:
        {
            PORT_Assert(pubKey);
            spki = freeSpki = SECKEY_CreateSubjectPublicKeyInfo(pubKey);
            break;
        }

	case NSSCMSRecipientID_BrandNew:
	    goto done;
	    break;

        default:
            
            goto loser;
            break;
    }

    certalgtag = SECOID_GetAlgorithmTag(&(spki->algorithm));

    rid = &ri->ri.keyTransRecipientInfo.recipientIdentifier;
    switch (certalgtag) {
    case SEC_OID_PKCS1_RSA_ENCRYPTION:
	ri->recipientInfoType = NSSCMSRecipientInfoID_KeyTrans;
	rid->identifierType = type;
	if (type == NSSCMSRecipientID_IssuerSN) {
	    rid->id.issuerAndSN = CERT_GetCertIssuerAndSN(poolp, cert);
	    if (rid->id.issuerAndSN == NULL) {
	      break;
	    }
	} else if (type == NSSCMSRecipientID_SubjectKeyID){
	    NSSCMSKeyTransRecipientInfoEx *riExtra;

	    rid->id.subjectKeyID = PORT_ArenaNew(poolp, SECItem);
	    if (rid->id.subjectKeyID == NULL) {
		rv = SECFailure;
		PORT_SetError(SEC_ERROR_NO_MEMORY);
		break;
	    } 
	    SECITEM_CopyItem(poolp, rid->id.subjectKeyID, subjKeyID);
	    if (rid->id.subjectKeyID->data == NULL) {
		rv = SECFailure;
		PORT_SetError(SEC_ERROR_NO_MEMORY);
		break;
	    }
	    riExtra = &ri->ri.keyTransRecipientInfoEx;
	    riExtra->version = 0;
	    riExtra->pubKey = SECKEY_CopyPublicKey(pubKey);
	    if (riExtra->pubKey == NULL) {
		rv = SECFailure;
		PORT_SetError(SEC_ERROR_NO_MEMORY);
		break;
	    }
	} else {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    rv = SECFailure;
	}
	break;
    case SEC_OID_X942_DIFFIE_HELMAN_KEY: 
	PORT_Assert(type == NSSCMSRecipientID_IssuerSN);
	if (type != NSSCMSRecipientID_IssuerSN) {
	    rv = SECFailure;
	    break;
	}
	
	ri->recipientInfoType = NSSCMSRecipientInfoID_KeyAgree;

	if (ri->ri.keyTransRecipientInfo.recipientIdentifier.id.issuerAndSN == NULL) {
	    rv = SECFailure;
	    break;
	}
	





	



	
	if ((rek = NSS_CMSRecipientEncryptedKey_Create(poolp)) == NULL) {
	    rv = SECFailure;
	    break;
	}

	
	rek->recipientIdentifier.identifierType = NSSCMSKeyAgreeRecipientID_IssuerSN;
	if ((rek->recipientIdentifier.id.issuerAndSN = CERT_GetCertIssuerAndSN(poolp, cert)) == NULL) {
	    rv = SECFailure;
	    break;
	}

	oiok = &(ri->ri.keyAgreeRecipientInfo.originatorIdentifierOrKey);

	
	oiok->identifierType = NSSCMSOriginatorIDOrKey_OriginatorPublicKey;

	rv = NSS_CMSArray_Add(poolp, (void ***)&ri->ri.keyAgreeRecipientInfo.recipientEncryptedKeys,
				    (void *)rek);

	break;
    default:
	
	
	PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	rv = SECFailure;
	break;
    }

    if (rv == SECFailure)
	goto loser;

    
    switch (ri->recipientInfoType) {
    case NSSCMSRecipientInfoID_KeyTrans:
	if (ri->ri.keyTransRecipientInfo.recipientIdentifier.identifierType == NSSCMSRecipientID_IssuerSN)
	    version = NSS_CMS_KEYTRANS_RECIPIENT_INFO_VERSION_ISSUERSN;
	else
	    version = NSS_CMS_KEYTRANS_RECIPIENT_INFO_VERSION_SUBJKEY;
	dummy = SEC_ASN1EncodeInteger(poolp, &(ri->ri.keyTransRecipientInfo.version), version);
	if (dummy == NULL)
	    goto loser;
	break;
    case NSSCMSRecipientInfoID_KeyAgree:
	dummy = SEC_ASN1EncodeInteger(poolp, &(ri->ri.keyAgreeRecipientInfo.version),
						NSS_CMS_KEYAGREE_RECIPIENT_INFO_VERSION);
	if (dummy == NULL)
	    goto loser;
	break;
    case NSSCMSRecipientInfoID_KEK:
	
	dummy = SEC_ASN1EncodeInteger(poolp, &(ri->ri.kekRecipientInfo.version),
						NSS_CMS_KEK_RECIPIENT_INFO_VERSION);
	if (dummy == NULL)
	    goto loser;
	break;
    
    }

done:
    PORT_ArenaUnmark (poolp, mark);
    if (freeSpki)
      SECKEY_DestroySubjectPublicKeyInfo(freeSpki);
    return ri;

loser:
    if (ri && ri->cert) {
        CERT_DestroyCertificate(ri->cert);
    }
    if (freeSpki) {
      SECKEY_DestroySubjectPublicKeyInfo(freeSpki);
    }
    PORT_ArenaRelease (poolp, mark);
    if (cmsg->contentInfo.contentTypeTag == &fakeContent) {
	NSS_CMSMessage_Destroy(cmsg);
    }
    return NULL;
}








NSSCMSRecipientInfo *
NSS_CMSRecipientInfo_Create(NSSCMSMessage *cmsg, CERTCertificate *cert)
{
    return nss_cmsrecipientinfo_create(cmsg, NSSCMSRecipientID_IssuerSN, cert, 
                                       NULL, NULL, NULL, NULL);
}

NSSCMSRecipientInfo *
NSS_CMSRecipientInfo_CreateNew(void* pwfn_arg)
{
    return nss_cmsrecipientinfo_create(NULL, NSSCMSRecipientID_BrandNew, NULL, 
                                       NULL, NULL, pwfn_arg, NULL);
}

NSSCMSRecipientInfo *
NSS_CMSRecipientInfo_CreateFromDER(SECItem* input, void* pwfn_arg)
{
    return nss_cmsrecipientinfo_create(NULL, NSSCMSRecipientID_BrandNew, NULL, 
                                       NULL, NULL, pwfn_arg, input);
}


NSSCMSRecipientInfo *
NSS_CMSRecipientInfo_CreateWithSubjKeyID(NSSCMSMessage   *cmsg, 
                                     SECItem         *subjKeyID,
                                     SECKEYPublicKey *pubKey)
{
    return nss_cmsrecipientinfo_create(cmsg, NSSCMSRecipientID_SubjectKeyID, 
                                       NULL, pubKey, subjKeyID, NULL, NULL);
}

NSSCMSRecipientInfo *
NSS_CMSRecipientInfo_CreateWithSubjKeyIDFromCert(NSSCMSMessage *cmsg,
                                             CERTCertificate *cert)
{
    SECKEYPublicKey *pubKey = NULL;
    SECItem subjKeyID = {siBuffer, NULL, 0};
    NSSCMSRecipientInfo *retVal = NULL;

    if (!cmsg || !cert) {
	return NULL;
    }
    pubKey = CERT_ExtractPublicKey(cert);
    if (!pubKey) {
	goto done;
    }
    if (CERT_FindSubjectKeyIDExtension(cert, &subjKeyID) != SECSuccess ||
        subjKeyID.data == NULL) {
	goto done;
    }
    retVal = NSS_CMSRecipientInfo_CreateWithSubjKeyID(cmsg, &subjKeyID, pubKey);
done:
    if (pubKey)
	SECKEY_DestroyPublicKey(pubKey);

    if (subjKeyID.data)
	SECITEM_FreeItem(&subjKeyID, PR_FALSE);

    return retVal;
}

void
NSS_CMSRecipientInfo_Destroy(NSSCMSRecipientInfo *ri)
{
    if (!ri) {
        return;
    }
    
    
    if (ri->cert != NULL)
	CERT_DestroyCertificate(ri->cert);

    if (nss_cmsrecipientinfo_usessubjectkeyid(ri)) {
	NSSCMSKeyTransRecipientInfoEx *extra;
	extra = &ri->ri.keyTransRecipientInfoEx;
	if (extra->pubKey)
	    SECKEY_DestroyPublicKey(extra->pubKey);
    }
    if (ri->cmsg && ri->cmsg->contentInfo.contentTypeTag == &fakeContent) {
	NSS_CMSMessage_Destroy(ri->cmsg);
    }

    
}

int
NSS_CMSRecipientInfo_GetVersion(NSSCMSRecipientInfo *ri)
{
    unsigned long version;
    SECItem *versionitem = NULL;

    switch (ri->recipientInfoType) {
    case NSSCMSRecipientInfoID_KeyTrans:
	
	versionitem = &(ri->ri.keyTransRecipientInfo.version);
	break;
    case NSSCMSRecipientInfoID_KEK:
	
	versionitem = &(ri->ri.kekRecipientInfo.version);
	break;
    case NSSCMSRecipientInfoID_KeyAgree:
	versionitem = &(ri->ri.keyAgreeRecipientInfo.version);
	break;
    }

    PORT_Assert(versionitem);
    if (versionitem == NULL) 
	return 0;

    
    if (SEC_ASN1DecodeInteger(versionitem, &version) != SECSuccess)
	return 0;
    else
	return (int)version;
}

SECItem *
NSS_CMSRecipientInfo_GetEncryptedKey(NSSCMSRecipientInfo *ri, int subIndex)
{
    SECItem *enckey = NULL;

    switch (ri->recipientInfoType) {
    case NSSCMSRecipientInfoID_KeyTrans:
	
	enckey = &(ri->ri.keyTransRecipientInfo.encKey);
	break;
    case NSSCMSRecipientInfoID_KEK:
	
	enckey = &(ri->ri.kekRecipientInfo.encKey);
	break;
    case NSSCMSRecipientInfoID_KeyAgree:
	enckey = &(ri->ri.keyAgreeRecipientInfo.recipientEncryptedKeys[subIndex]->encKey);
	break;
    }
    return enckey;
}


SECOidTag
NSS_CMSRecipientInfo_GetKeyEncryptionAlgorithmTag(NSSCMSRecipientInfo *ri)
{
    SECOidTag encalgtag = SEC_OID_UNKNOWN; 

    switch (ri->recipientInfoType) {
    case NSSCMSRecipientInfoID_KeyTrans:
	encalgtag = SECOID_GetAlgorithmTag(&(ri->ri.keyTransRecipientInfo.keyEncAlg));
	break;
    case NSSCMSRecipientInfoID_KeyAgree:
	encalgtag = SECOID_GetAlgorithmTag(&(ri->ri.keyAgreeRecipientInfo.keyEncAlg));
	break;
    case NSSCMSRecipientInfoID_KEK:
	encalgtag = SECOID_GetAlgorithmTag(&(ri->ri.kekRecipientInfo.keyEncAlg));
	break;
    }
    return encalgtag;
}

SECStatus
NSS_CMSRecipientInfo_WrapBulkKey(NSSCMSRecipientInfo *ri, PK11SymKey *bulkkey, 
                                 SECOidTag bulkalgtag)
{
    CERTCertificate *cert;
    SECOidTag certalgtag;
    SECStatus rv = SECSuccess;
    NSSCMSRecipientEncryptedKey *rek;
    NSSCMSOriginatorIdentifierOrKey *oiok;
    CERTSubjectPublicKeyInfo *spki, *freeSpki = NULL;
    PLArenaPool *poolp;
    NSSCMSKeyTransRecipientInfoEx *extra = NULL;
    PRBool usesSubjKeyID;

    poolp = ri->cmsg->poolp;
    cert = ri->cert;
    usesSubjKeyID = nss_cmsrecipientinfo_usessubjectkeyid(ri);
    if (cert) {
	spki = &cert->subjectPublicKeyInfo;
	certalgtag = SECOID_GetAlgorithmTag(&(spki->algorithm));
    } else if (usesSubjKeyID) {
	extra = &ri->ri.keyTransRecipientInfoEx;
	
	PORT_Assert(extra->pubKey);
	if (!extra->pubKey) {
	    PORT_SetError(SEC_ERROR_INVALID_ARGS);
	    return SECFailure;
	}
	spki = freeSpki = SECKEY_CreateSubjectPublicKeyInfo(extra->pubKey);
	certalgtag = SECOID_GetAlgorithmTag(&spki->algorithm);
    } else {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    
    

    certalgtag = SECOID_GetAlgorithmTag(&spki->algorithm);
    switch (certalgtag) {
    case SEC_OID_PKCS1_RSA_ENCRYPTION:
	
	if (cert) {
	    rv = NSS_CMSUtil_EncryptSymKey_RSA(poolp, cert, bulkkey, 
	                         &ri->ri.keyTransRecipientInfo.encKey);
 	    if (rv != SECSuccess)
		break;
	} else if (usesSubjKeyID) {
	    PORT_Assert(extra != NULL);
	    rv = NSS_CMSUtil_EncryptSymKey_RSAPubKey(poolp, extra->pubKey,
	                         bulkkey, &ri->ri.keyTransRecipientInfo.encKey);
 	    if (rv != SECSuccess)
		break;
	}

	rv = SECOID_SetAlgorithmID(poolp, &(ri->ri.keyTransRecipientInfo.keyEncAlg), certalgtag, NULL);
	break;
    case SEC_OID_X942_DIFFIE_HELMAN_KEY: 
	rek = ri->ri.keyAgreeRecipientInfo.recipientEncryptedKeys[0];
	if (rek == NULL) {
	    rv = SECFailure;
	    break;
	}

	oiok = &(ri->ri.keyAgreeRecipientInfo.originatorIdentifierOrKey);
	PORT_Assert(oiok->identifierType == NSSCMSOriginatorIDOrKey_OriginatorPublicKey);

	
	if (SECOID_SetAlgorithmID(poolp, &oiok->id.originatorPublicKey.algorithmIdentifier,
				    SEC_OID_X942_DIFFIE_HELMAN_KEY, NULL) != SECSuccess) {
	    rv = SECFailure;
	    break;
	}

	
	
	
	rv = NSS_CMSUtil_EncryptSymKey_ESDH(poolp, cert, bulkkey,
					&rek->encKey,
					&ri->ri.keyAgreeRecipientInfo.ukm,
					&ri->ri.keyAgreeRecipientInfo.keyEncAlg,
					&oiok->id.originatorPublicKey.publicKey);

	break;
    default:
	
	
	PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	rv = SECFailure;
	break;
    }
    if (freeSpki)
	SECKEY_DestroySubjectPublicKeyInfo(freeSpki);

    return rv;
}

PK11SymKey *
NSS_CMSRecipientInfo_UnwrapBulkKey(NSSCMSRecipientInfo *ri, int subIndex, 
	CERTCertificate *cert, SECKEYPrivateKey *privkey, SECOidTag bulkalgtag)
{
    PK11SymKey *bulkkey = NULL;
    SECAlgorithmID *encalg;
    SECOidTag encalgtag;
    SECItem *enckey;
    int error;

    ri->cert = CERT_DupCertificate(cert);
        	

    switch (ri->recipientInfoType) {
    case NSSCMSRecipientInfoID_KeyTrans:
	encalg = &(ri->ri.keyTransRecipientInfo.keyEncAlg);
	encalgtag = SECOID_GetAlgorithmTag(&(ri->ri.keyTransRecipientInfo.keyEncAlg));
	enckey = &(ri->ri.keyTransRecipientInfo.encKey); 
	switch (encalgtag) {
	case SEC_OID_PKCS1_RSA_ENCRYPTION:
	    
	    
	    bulkkey = NSS_CMSUtil_DecryptSymKey_RSA(privkey, enckey, bulkalgtag);
	    break;
	default:
	    error = SEC_ERROR_UNSUPPORTED_KEYALG;
	    goto loser;
	}
	break;
    case NSSCMSRecipientInfoID_KeyAgree:
	encalg = &(ri->ri.keyAgreeRecipientInfo.keyEncAlg);
	encalgtag = SECOID_GetAlgorithmTag(&(ri->ri.keyAgreeRecipientInfo.keyEncAlg));
	enckey = &(ri->ri.keyAgreeRecipientInfo.recipientEncryptedKeys[subIndex]->encKey);
	switch (encalgtag) {
	case SEC_OID_X942_DIFFIE_HELMAN_KEY:
	    
	    
	    
	    
	    
	    
	    
	    
	    error = SEC_ERROR_UNSUPPORTED_KEYALG;
	    goto loser;
	    break;
	default:
	    error = SEC_ERROR_UNSUPPORTED_KEYALG;
	    goto loser;
	}
	break;
    case NSSCMSRecipientInfoID_KEK:
	encalg = &(ri->ri.kekRecipientInfo.keyEncAlg);
	encalgtag = SECOID_GetAlgorithmTag(&(ri->ri.kekRecipientInfo.keyEncAlg));
	enckey = &(ri->ri.kekRecipientInfo.encKey);
	
	error = SEC_ERROR_UNSUPPORTED_KEYALG;
	goto loser;
	break;
    }
    
    return bulkkey;

loser:
    PORT_SetError(error);
    return NULL;
}

SECStatus NSS_CMSRecipientInfo_GetCertAndKey(NSSCMSRecipientInfo *ri,
                                             CERTCertificate** retcert,
                                             SECKEYPrivateKey** retkey)
{
    CERTCertificate* cert = NULL;
    NSSCMSRecipient** recipients = NULL;
    NSSCMSRecipientInfo* recipientInfos[2];
    SECStatus rv = SECSuccess;
    SECKEYPrivateKey* key = NULL;

    if (!ri)
        return SECFailure;
    
    if (!retcert && !retkey) {
        
        return SECSuccess;
    }

    if (retcert) {
        *retcert = NULL;
    }
    if (retkey) {
        *retkey = NULL;
    }

    if (ri->cert) {
        cert = CERT_DupCertificate(ri->cert);
        if (!cert) {
            rv = SECFailure;
        }
    }
    if (SECSuccess == rv && !cert) {
        
        
        recipientInfos[0] = ri;
        recipientInfos[1] = NULL;

        recipients = nss_cms_recipient_list_create(recipientInfos);
        if (recipients) {
            
            if (0 == PK11_FindCertAndKeyByRecipientListNew(recipients,
                ri->cmsg->pwfn_arg)) {
                cert = CERT_DupCertificate(recipients[0]->cert);
                key = SECKEY_CopyPrivateKey(recipients[0]->privkey);
            } else {
                rv = SECFailure;
            }

            nss_cms_recipient_list_destroy(recipients);
        }
        else {
            rv = SECFailure;
        }            
    } else if (SECSuccess == rv && cert && retkey) {
        
        key = PK11_FindPrivateKeyFromCert(cert->slot, cert, ri->cmsg->pwfn_arg);
    }
    if (retcert) {
        *retcert = cert;
    } else {
        if (cert) {
            CERT_DestroyCertificate(cert);
        }
    }
    if (retkey) {
        *retkey = key;
    } else {
        if (key) {
            SECKEY_DestroyPrivateKey(key);
        }
    }

    return rv;
}

SECStatus NSS_CMSRecipientInfo_Encode(PRArenaPool* poolp,
                                      const NSSCMSRecipientInfo *src,
                                      SECItem* returned)
{
    extern const SEC_ASN1Template NSSCMSRecipientInfoTemplate[];
    SECStatus rv = SECFailure;
    if (!src || !returned) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
    } else if (SEC_ASN1EncodeItem(poolp, returned, src,
        NSSCMSRecipientInfoTemplate))   {
        rv = SECSuccess;
    }
    return rv;
}
