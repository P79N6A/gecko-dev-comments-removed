










































#include "cmslocal.h"

#include "cert.h"
#include "key.h"
#include "secasn1.h"
#include "secitem.h"
#include "secoid.h"
#include "pk11func.h"
#include "prtime.h"
#include "secerr.h"
#include "secder.h"
#include "cryptohi.h"

#include "smime.h"




NSSCMSSignerInfo *
nss_cmssignerinfo_create(NSSCMSMessage *cmsg, NSSCMSSignerIDSelector type, 
	CERTCertificate *cert, SECItem *subjKeyID, SECKEYPublicKey *pubKey, 
	SECKEYPrivateKey *signingKey, SECOidTag digestalgtag);

NSSCMSSignerInfo *
NSS_CMSSignerInfo_CreateWithSubjKeyID(NSSCMSMessage *cmsg, SECItem *subjKeyID, 
	SECKEYPublicKey *pubKey, SECKEYPrivateKey *signingKey, SECOidTag digestalgtag)
{
    return nss_cmssignerinfo_create(cmsg, NSSCMSSignerID_SubjectKeyID, NULL, subjKeyID, pubKey, signingKey, digestalgtag); 
}

NSSCMSSignerInfo *
NSS_CMSSignerInfo_Create(NSSCMSMessage *cmsg, CERTCertificate *cert, SECOidTag digestalgtag)
{
    return nss_cmssignerinfo_create(cmsg, NSSCMSSignerID_IssuerSN, cert, NULL, NULL, NULL, digestalgtag); 
}

NSSCMSSignerInfo *
nss_cmssignerinfo_create(NSSCMSMessage *cmsg, NSSCMSSignerIDSelector type, 
	CERTCertificate *cert, SECItem *subjKeyID, SECKEYPublicKey *pubKey, 
	SECKEYPrivateKey *signingKey, SECOidTag digestalgtag)
{
    void *mark;
    NSSCMSSignerInfo *signerinfo;
    int version;
    PLArenaPool *poolp;

    poolp = cmsg->poolp;

    mark = PORT_ArenaMark(poolp);

    signerinfo = (NSSCMSSignerInfo *)PORT_ArenaZAlloc(poolp, sizeof(NSSCMSSignerInfo));
    if (signerinfo == NULL) {
	PORT_ArenaRelease(poolp, mark);
	return NULL;
    }


    signerinfo->cmsg = cmsg;

    switch(type) {
    case NSSCMSSignerID_IssuerSN:
        signerinfo->signerIdentifier.identifierType = NSSCMSSignerID_IssuerSN;
        if ((signerinfo->cert = CERT_DupCertificate(cert)) == NULL)
	    goto loser;
        if ((signerinfo->signerIdentifier.id.issuerAndSN = CERT_GetCertIssuerAndSN(poolp, cert)) == NULL)
	    goto loser;
        break;
    case NSSCMSSignerID_SubjectKeyID:
        signerinfo->signerIdentifier.identifierType = NSSCMSSignerID_SubjectKeyID;
        PORT_Assert(subjKeyID);
        if (!subjKeyID)
            goto loser;

        signerinfo->signerIdentifier.id.subjectKeyID = PORT_ArenaNew(poolp, SECItem);
        SECITEM_CopyItem(poolp, signerinfo->signerIdentifier.id.subjectKeyID,
                         subjKeyID);
        signerinfo->signingKey = SECKEY_CopyPrivateKey(signingKey);
        if (!signerinfo->signingKey)
            goto loser;
        signerinfo->pubKey = SECKEY_CopyPublicKey(pubKey);
        if (!signerinfo->pubKey)
            goto loser;
        break;
    default:
        goto loser;
    }

    
    version = NSS_CMS_SIGNER_INFO_VERSION_ISSUERSN;
    
    if (signerinfo->signerIdentifier.identifierType == NSSCMSSignerID_SubjectKeyID)
	version = NSS_CMS_SIGNER_INFO_VERSION_SUBJKEY;
    (void)SEC_ASN1EncodeInteger(poolp, &(signerinfo->version), (long)version);

    if (SECOID_SetAlgorithmID(poolp, &signerinfo->digestAlg, digestalgtag, NULL) != SECSuccess)
	goto loser;

    PORT_ArenaUnmark(poolp, mark);
    return signerinfo;

loser:
    PORT_ArenaRelease(poolp, mark);
    return NULL;
}




void
NSS_CMSSignerInfo_Destroy(NSSCMSSignerInfo *si)
{
    if (si->cert != NULL)
	CERT_DestroyCertificate(si->cert);

    if (si->certList != NULL) 
	CERT_DestroyCertificateList(si->certList);

    
}





SECStatus
NSS_CMSSignerInfo_Sign(NSSCMSSignerInfo *signerinfo, SECItem *digest, 
                       SECItem *contentType)
{
    CERTCertificate *cert;
    SECKEYPrivateKey *privkey = NULL;
    SECOidTag digestalgtag;
    SECOidTag pubkAlgTag;
    SECItem signature = { 0 };
    SECStatus rv;
    PLArenaPool *poolp, *tmppoolp = NULL;
    SECAlgorithmID *algID, freeAlgID;
    CERTSubjectPublicKeyInfo *spki;

    PORT_Assert (digest != NULL);

    poolp = signerinfo->cmsg->poolp;

    switch (signerinfo->signerIdentifier.identifierType) {
    case NSSCMSSignerID_IssuerSN:
        cert = signerinfo->cert;

        privkey = PK11_FindKeyByAnyCert(cert, signerinfo->cmsg->pwfn_arg);
        if (privkey == NULL)
	    goto loser;
        algID = &cert->subjectPublicKeyInfo.algorithm;
        break;
    case NSSCMSSignerID_SubjectKeyID:
        privkey = signerinfo->signingKey;
        signerinfo->signingKey = NULL;
        spki = SECKEY_CreateSubjectPublicKeyInfo(signerinfo->pubKey);
        SECKEY_DestroyPublicKey(signerinfo->pubKey);
        signerinfo->pubKey = NULL;
        SECOID_CopyAlgorithmID(NULL, &freeAlgID, &spki->algorithm);
        SECKEY_DestroySubjectPublicKeyInfo(spki); 
        algID = &freeAlgID;
        break;
    default:
        goto loser;
    }
    digestalgtag = NSS_CMSSignerInfo_GetDigestAlgTag(signerinfo);
    



    pubkAlgTag = SECOID_GetAlgorithmTag(algID);
    if (signerinfo->signerIdentifier.identifierType == NSSCMSSignerID_SubjectKeyID) {
      SECOID_DestroyAlgorithmID(&freeAlgID, PR_FALSE);
    }

    if (signerinfo->authAttr != NULL) {
	SECOidTag signAlgTag;
	SECItem encoded_attrs;

	
	rv = NSS_CMSAttributeArray_SetAttr(poolp, &(signerinfo->authAttr), 
	                       SEC_OID_PKCS9_MESSAGE_DIGEST, digest, PR_FALSE);
	if (rv != SECSuccess)
	    goto loser;

	if (contentType != NULL) {
	    
	    rv = NSS_CMSAttributeArray_SetAttr(poolp, &(signerinfo->authAttr), 
	                    SEC_OID_PKCS9_CONTENT_TYPE, contentType, PR_FALSE);
	    if (rv != SECSuccess)
		goto loser;
	}

	if ((tmppoolp = PORT_NewArena (1024)) == NULL) {
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    goto loser;
	}

	











	if (NSS_CMSAttributeArray_Reorder(signerinfo->authAttr) != SECSuccess)
	    goto loser;

	encoded_attrs.data = NULL;
	encoded_attrs.len = 0;
	if (NSS_CMSAttributeArray_Encode(tmppoolp, &(signerinfo->authAttr), 
	                &encoded_attrs) == NULL)
	    goto loser;

	signAlgTag = SEC_GetSignatureAlgorithmOidTag(privkey->keyType, 
                                                     digestalgtag);
	if (signAlgTag == SEC_OID_UNKNOWN) {
	    PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	    goto loser;
	}

	rv = SEC_SignData(&signature, encoded_attrs.data, encoded_attrs.len, 
	                  privkey, signAlgTag);
	PORT_FreeArena(tmppoolp, PR_FALSE); 
	tmppoolp = 0;
    } else {
	rv = SGN_Digest(privkey, digestalgtag, &signature, digest);
    }
    SECKEY_DestroyPrivateKey(privkey);
    privkey = NULL;

    if (rv != SECSuccess)
	goto loser;

    if (SECITEM_CopyItem(poolp, &(signerinfo->encDigest), &signature) 
          != SECSuccess)
	goto loser;

    SECITEM_FreeItem(&signature, PR_FALSE);

    if (SECOID_SetAlgorithmID(poolp, &(signerinfo->digestEncAlg), pubkAlgTag, 
                              NULL) != SECSuccess)
	goto loser;

    return SECSuccess;

loser:
    if (signature.len != 0)
	SECITEM_FreeItem (&signature, PR_FALSE);
    if (privkey)
	SECKEY_DestroyPrivateKey(privkey);
    if (tmppoolp)
	PORT_FreeArena(tmppoolp, PR_FALSE);
    return SECFailure;
}

SECStatus
NSS_CMSSignerInfo_VerifyCertificate(NSSCMSSignerInfo *signerinfo, CERTCertDBHandle *certdb,
			    SECCertUsage certusage)
{
    CERTCertificate *cert;
    int64 stime;

    if ((cert = NSS_CMSSignerInfo_GetSigningCertificate(signerinfo, certdb)) == NULL) {
	signerinfo->verificationStatus = NSSCMSVS_SigningCertNotFound;
	return SECFailure;
    }

    




    if (NSS_CMSSignerInfo_GetSigningTime (signerinfo, &stime) != SECSuccess)
	stime = PR_Now(); 
    
    







    if (CERT_VerifyCert(certdb, cert, PR_TRUE, certusage, stime, 
                        signerinfo->cmsg->pwfn_arg, NULL) != SECSuccess) {
	signerinfo->verificationStatus = NSSCMSVS_SigningCertNotTrusted;
	return SECFailure;
    }
    return SECSuccess;
}







SECStatus
NSS_CMSSignerInfo_Verify(NSSCMSSignerInfo *signerinfo, 
                         SECItem *digest,               
                         SECItem *contentType)          
{
    SECKEYPublicKey *publickey = NULL;
    NSSCMSAttribute *attr;
    SECItem encoded_attrs;
    CERTCertificate *cert;
    NSSCMSVerificationStatus vs = NSSCMSVS_Unverified;
    PLArenaPool *poolp;
    SECOidTag    digestalgtag;
    SECOidTag    pubkAlgTag;

    if (signerinfo == NULL)
	return SECFailure;

    


    cert = NSS_CMSSignerInfo_GetSigningCertificate(signerinfo, NULL);
    if (cert == NULL) {
	vs = NSSCMSVS_SigningCertNotFound;
	goto loser;
    }

    if ((publickey = CERT_ExtractPublicKey(cert)) == NULL) {
	vs = NSSCMSVS_ProcessingError;
	goto loser;
    }

    digestalgtag = NSS_CMSSignerInfo_GetDigestAlgTag(signerinfo);
    pubkAlgTag = SECOID_GetAlgorithmTag(&(signerinfo->digestEncAlg));
    if ((pubkAlgTag == SEC_OID_UNKNOWN) || (digestalgtag == SEC_OID_UNKNOWN)) {
	vs = NSSCMSVS_SignatureAlgorithmUnknown;
	goto loser;
    }

#ifndef NSS_ECC_MORE_THAN_SUITE_B
    if (pubkAlgTag == SEC_OID_ANSIX962_EC_PUBLIC_KEY) {
	vs = NSSCMSVS_SignatureAlgorithmUnknown;
	goto loser;
    }
#endif

    if (!NSS_CMSArray_IsEmpty((void **)signerinfo->authAttr)) {
	if (contentType) {
	    








	    attr = NSS_CMSAttributeArray_FindAttrByOidTag(signerinfo->authAttr,
					SEC_OID_PKCS9_CONTENT_TYPE, PR_TRUE);
	    if (attr == NULL) {
		vs = NSSCMSVS_MalformedSignature;
		goto loser;
	    }
		
	    if (NSS_CMSAttribute_CompareValue(attr, contentType) == PR_FALSE) {
		vs = NSSCMSVS_MalformedSignature;
		goto loser;
	    }
	}

	


	attr = NSS_CMSAttributeArray_FindAttrByOidTag(signerinfo->authAttr, 
	                              SEC_OID_PKCS9_MESSAGE_DIGEST, PR_TRUE);
	if (attr == NULL) {
	    vs = NSSCMSVS_MalformedSignature;
	    goto loser;
	}
	if (!digest || 
	    NSS_CMSAttribute_CompareValue(attr, digest) == PR_FALSE) {
	    vs = NSSCMSVS_DigestMismatch;
	    goto loser;
	}

	if ((poolp = PORT_NewArena (1024)) == NULL) {
	    vs = NSSCMSVS_ProcessingError;
	    goto loser;
	}

	







	encoded_attrs.data = NULL;
	encoded_attrs.len = 0;

	if (NSS_CMSAttributeArray_Encode(poolp, &(signerinfo->authAttr), 
	                                 &encoded_attrs) == NULL ||
		encoded_attrs.data == NULL || encoded_attrs.len == 0) {
	    vs = NSSCMSVS_ProcessingError;
	    goto loser;
	}

	vs = (VFY_VerifyDataDirect(encoded_attrs.data, encoded_attrs.len,
		publickey, &(signerinfo->encDigest), pubkAlgTag,
		digestalgtag, NULL, signerinfo->cmsg->pwfn_arg) != SECSuccess) 
		? NSSCMSVS_BadSignature : NSSCMSVS_GoodSignature;

	PORT_FreeArena(poolp, PR_FALSE);  

    } else {
	SECItem *sig;

	


	sig = &(signerinfo->encDigest);
	if (sig->len == 0)
	    goto loser;

	vs = (!digest || 
	      VFY_VerifyDigestDirect(digest, publickey, sig, pubkAlgTag,
		digestalgtag, signerinfo->cmsg->pwfn_arg) != SECSuccess) 
		? NSSCMSVS_BadSignature : NSSCMSVS_GoodSignature;
    }

    if (vs == NSSCMSVS_BadSignature) {
	int error = PORT_GetError();
	















	if (error == SEC_ERROR_BAD_SIGNATURE)
	    PORT_SetError(SEC_ERROR_PKCS7_BAD_SIGNATURE);
	


	if ((error == SEC_ERROR_PKCS7_KEYALG_MISMATCH) ||
	    (error == SEC_ERROR_INVALID_ALGORITHM)) {
	    
	    PORT_SetError(SEC_ERROR_PKCS7_BAD_SIGNATURE);
	    vs = NSSCMSVS_SignatureAlgorithmUnsupported;
	}
    }

    if (publickey != NULL)
	SECKEY_DestroyPublicKey (publickey);

    signerinfo->verificationStatus = vs;

    return (vs == NSSCMSVS_GoodSignature) ? SECSuccess : SECFailure;

loser:
    if (publickey != NULL)
	SECKEY_DestroyPublicKey (publickey);

    signerinfo->verificationStatus = vs;

    PORT_SetError (SEC_ERROR_PKCS7_BAD_SIGNATURE);
    return SECFailure;
}

NSSCMSVerificationStatus
NSS_CMSSignerInfo_GetVerificationStatus(NSSCMSSignerInfo *signerinfo)
{
    return signerinfo->verificationStatus;
}

SECOidData *
NSS_CMSSignerInfo_GetDigestAlg(NSSCMSSignerInfo *signerinfo)
{
    SECOidData *algdata;
    SECOidTag   algtag;

    algdata = SECOID_FindOID (&(signerinfo->digestAlg.algorithm));
    if (algdata == NULL) {
	return algdata;
    }
    




    algtag = NSS_CMSUtil_MapSignAlgs(algdata->offset);
    if (algtag != algdata->offset) {
	


	algdata = SECOID_FindOIDByTag(algtag);
    }

    return algdata;

}

SECOidTag
NSS_CMSSignerInfo_GetDigestAlgTag(NSSCMSSignerInfo *signerinfo)
{
    SECOidData *algdata;

    if (!signerinfo) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SEC_OID_UNKNOWN;
    }

    algdata = NSS_CMSSignerInfo_GetDigestAlg(signerinfo);
    if (algdata != NULL)
	return algdata->offset;
    else
	return SEC_OID_UNKNOWN;
}

CERTCertificateList *
NSS_CMSSignerInfo_GetCertList(NSSCMSSignerInfo *signerinfo)
{
    return signerinfo->certList;
}

int
NSS_CMSSignerInfo_GetVersion(NSSCMSSignerInfo *signerinfo)
{
    unsigned long version;

    
    if (SEC_ASN1DecodeInteger(&(signerinfo->version), &version) != SECSuccess)
	return 0;
    else
	return (int)version;
}











SECStatus
NSS_CMSSignerInfo_GetSigningTime(NSSCMSSignerInfo *sinfo, PRTime *stime)
{
    NSSCMSAttribute *attr;
    SECItem *value;

    if (sinfo == NULL)
	return SECFailure;

    if (sinfo->signingTime != 0) {
	*stime = sinfo->signingTime;	
	return SECSuccess;
    }

    attr = NSS_CMSAttributeArray_FindAttrByOidTag(sinfo->authAttr, SEC_OID_PKCS9_SIGNING_TIME, PR_TRUE);
    
    if (attr == NULL || (value = NSS_CMSAttribute_GetValue(attr)) == NULL)
	return SECFailure;
    if (DER_DecodeTimeChoice(stime, value) != SECSuccess)
	return SECFailure;
    sinfo->signingTime = *stime;	
    return SECSuccess;
}






CERTCertificate *
NSS_CMSSignerInfo_GetSigningCertificate(NSSCMSSignerInfo *signerinfo, CERTCertDBHandle *certdb)
{
    CERTCertificate *cert;
    NSSCMSSignerIdentifier *sid;

    if (signerinfo->cert != NULL)
	return signerinfo->cert;

    
    if (certdb == NULL)
	return NULL;

    





    sid = &signerinfo->signerIdentifier;
    switch (sid->identifierType) {
    case NSSCMSSignerID_IssuerSN:
	cert = CERT_FindCertByIssuerAndSN(certdb, sid->id.issuerAndSN);
	break;
    case NSSCMSSignerID_SubjectKeyID:
	cert = CERT_FindCertBySubjectKeyID(certdb, sid->id.subjectKeyID);
	break;
    default:
	cert = NULL;
	break;
    }

    
    signerinfo->cert = cert;	

    return cert;
}









char *
NSS_CMSSignerInfo_GetSignerCommonName(NSSCMSSignerInfo *sinfo)
{
    CERTCertificate *signercert;

    
    if ((signercert = NSS_CMSSignerInfo_GetSigningCertificate(sinfo, NULL)) == NULL)
	return NULL;

    return (CERT_GetCommonName(&signercert->subject));
}









char *
NSS_CMSSignerInfo_GetSignerEmailAddress(NSSCMSSignerInfo *sinfo)
{
    CERTCertificate *signercert;

    if ((signercert = NSS_CMSSignerInfo_GetSigningCertificate(sinfo, NULL)) == NULL)
	return NULL;

    if (!signercert->emailAddr || !signercert->emailAddr[0])
	return NULL;

    return (PORT_Strdup(signercert->emailAddr));
}





SECStatus
NSS_CMSSignerInfo_AddAuthAttr(NSSCMSSignerInfo *signerinfo, NSSCMSAttribute *attr)
{
    return NSS_CMSAttributeArray_AddAttr(signerinfo->cmsg->poolp, &(signerinfo->authAttr), attr);
}





SECStatus
NSS_CMSSignerInfo_AddUnauthAttr(NSSCMSSignerInfo *signerinfo, NSSCMSAttribute *attr)
{
    return NSS_CMSAttributeArray_AddAttr(signerinfo->cmsg->poolp, &(signerinfo->unAuthAttr), attr);
}














SECStatus
NSS_CMSSignerInfo_AddSigningTime(NSSCMSSignerInfo *signerinfo, PRTime t)
{
    NSSCMSAttribute *attr;
    SECItem stime;
    void *mark;
    PLArenaPool *poolp;

    poolp = signerinfo->cmsg->poolp;

    mark = PORT_ArenaMark(poolp);

    
    if (DER_EncodeTimeChoice(NULL, &stime, t) != SECSuccess)
	goto loser;

    if ((attr = NSS_CMSAttribute_Create(poolp, SEC_OID_PKCS9_SIGNING_TIME, &stime, PR_FALSE)) == NULL) {
	SECITEM_FreeItem (&stime, PR_FALSE);
	goto loser;
    }

    SECITEM_FreeItem (&stime, PR_FALSE);

    if (NSS_CMSSignerInfo_AddAuthAttr(signerinfo, attr) != SECSuccess)
	goto loser;

    PORT_ArenaUnmark (poolp, mark);

    return SECSuccess;

loser:
    PORT_ArenaRelease (poolp, mark);
    return SECFailure;
}








SECStatus
NSS_CMSSignerInfo_AddSMIMECaps(NSSCMSSignerInfo *signerinfo)
{
    NSSCMSAttribute *attr;
    SECItem *smimecaps = NULL;
    void *mark;
    PLArenaPool *poolp;

    poolp = signerinfo->cmsg->poolp;

    mark = PORT_ArenaMark(poolp);

    smimecaps = SECITEM_AllocItem(poolp, NULL, 0);
    if (smimecaps == NULL)
	goto loser;

    
    if (NSS_SMIMEUtil_CreateSMIMECapabilities(poolp, smimecaps) != SECSuccess)
	goto loser;

    if ((attr = NSS_CMSAttribute_Create(poolp, SEC_OID_PKCS9_SMIME_CAPABILITIES, smimecaps, PR_TRUE)) == NULL)
	goto loser;

    if (NSS_CMSSignerInfo_AddAuthAttr(signerinfo, attr) != SECSuccess)
	goto loser;

    PORT_ArenaUnmark (poolp, mark);
    return SECSuccess;

loser:
    PORT_ArenaRelease (poolp, mark);
    return SECFailure;
}







SECStatus
NSS_CMSSignerInfo_AddSMIMEEncKeyPrefs(NSSCMSSignerInfo *signerinfo, CERTCertificate *cert, CERTCertDBHandle *certdb)
{
    NSSCMSAttribute *attr;
    SECItem *smimeekp = NULL;
    void *mark;
    PLArenaPool *poolp;

    
    if (CERT_VerifyCert(certdb, cert, PR_TRUE, certUsageEmailRecipient, PR_Now(), signerinfo->cmsg->pwfn_arg, NULL) != SECSuccess) {
	return SECFailure;
    }

    poolp = signerinfo->cmsg->poolp;
    mark = PORT_ArenaMark(poolp);

    smimeekp = SECITEM_AllocItem(poolp, NULL, 0);
    if (smimeekp == NULL)
	goto loser;

    
    if (NSS_SMIMEUtil_CreateSMIMEEncKeyPrefs(poolp, smimeekp, cert) != SECSuccess)
	goto loser;

    if ((attr = NSS_CMSAttribute_Create(poolp, SEC_OID_SMIME_ENCRYPTION_KEY_PREFERENCE, smimeekp, PR_TRUE)) == NULL)
	goto loser;

    if (NSS_CMSSignerInfo_AddAuthAttr(signerinfo, attr) != SECSuccess)
	goto loser;

    PORT_ArenaUnmark (poolp, mark);
    return SECSuccess;

loser:
    PORT_ArenaRelease (poolp, mark);
    return SECFailure;
}








SECStatus
NSS_CMSSignerInfo_AddMSSMIMEEncKeyPrefs(NSSCMSSignerInfo *signerinfo, CERTCertificate *cert, CERTCertDBHandle *certdb)
{
    NSSCMSAttribute *attr;
    SECItem *smimeekp = NULL;
    void *mark;
    PLArenaPool *poolp;

    
    if (CERT_VerifyCert(certdb, cert, PR_TRUE, certUsageEmailRecipient, PR_Now(), signerinfo->cmsg->pwfn_arg, NULL) != SECSuccess) {
	return SECFailure;
    }

    poolp = signerinfo->cmsg->poolp;
    mark = PORT_ArenaMark(poolp);

    smimeekp = SECITEM_AllocItem(poolp, NULL, 0);
    if (smimeekp == NULL)
	goto loser;

    
    if (NSS_SMIMEUtil_CreateMSSMIMEEncKeyPrefs(poolp, smimeekp, cert) != SECSuccess)
	goto loser;

    if ((attr = NSS_CMSAttribute_Create(poolp, SEC_OID_MS_SMIME_ENCRYPTION_KEY_PREFERENCE, smimeekp, PR_TRUE)) == NULL)
	goto loser;

    if (NSS_CMSSignerInfo_AddAuthAttr(signerinfo, attr) != SECSuccess)
	goto loser;

    PORT_ArenaUnmark (poolp, mark);
    return SECSuccess;

loser:
    PORT_ArenaRelease (poolp, mark);
    return SECFailure;
}














SECStatus
NSS_CMSSignerInfo_AddCounterSignature(NSSCMSSignerInfo *signerinfo,
				    SECOidTag digestalg, CERTCertificate signingcert)
{
    
    return SECFailure;
}





SECStatus
NSS_SMIMESignerInfo_SaveSMIMEProfile(NSSCMSSignerInfo *signerinfo)
{
    CERTCertificate *cert = NULL;
    SECItem *profile = NULL;
    NSSCMSAttribute *attr;
    SECItem *stime = NULL;
    SECItem *ekp;
    CERTCertDBHandle *certdb;
    int save_error;
    SECStatus rv;
    PRBool must_free_cert = PR_FALSE;

    certdb = CERT_GetDefaultCertDB();

    
    if (signerinfo->verificationStatus != NSSCMSVS_GoodSignature)
	return SECFailure;

    
    if (!NSS_CMSArray_IsEmpty((void **)signerinfo->authAttr) &&
	(attr = NSS_CMSAttributeArray_FindAttrByOidTag(signerinfo->authAttr,
			       SEC_OID_SMIME_ENCRYPTION_KEY_PREFERENCE, PR_TRUE)) != NULL)
    { 
	ekp = NSS_CMSAttribute_GetValue(attr);
	if (ekp == NULL)
	    return SECFailure;

	
	
	cert = NSS_SMIMEUtil_GetCertFromEncryptionKeyPreference(certdb, ekp);
	if (cert == NULL)
	    return SECFailure;
	must_free_cert = PR_TRUE;
    }

    if (cert == NULL) {
	

	cert = NSS_CMSSignerInfo_GetSigningCertificate(signerinfo, certdb);
	if (cert == NULL || cert->emailAddr == NULL || !cert->emailAddr[0])
	    return SECFailure;
    }

    
    


#ifdef notdef
    if (CERT_VerifyCert(certdb, cert, PR_TRUE, certUsageEmailRecipient, PR_Now(), signerinfo->cmsg->pwfn_arg, NULL) != SECSuccess) {
	if (must_free_cert)
	    CERT_DestroyCertificate(cert);
	return SECFailure;
    }
#endif

    

    



    save_error = PORT_GetError();

    if (!NSS_CMSArray_IsEmpty((void **)signerinfo->authAttr)) {
	attr = NSS_CMSAttributeArray_FindAttrByOidTag(signerinfo->authAttr,
				       SEC_OID_PKCS9_SMIME_CAPABILITIES,
				       PR_TRUE);
	profile = NSS_CMSAttribute_GetValue(attr);
	attr = NSS_CMSAttributeArray_FindAttrByOidTag(signerinfo->authAttr,
				       SEC_OID_PKCS9_SIGNING_TIME,
				       PR_TRUE);
	stime = NSS_CMSAttribute_GetValue(attr);
    }

    rv = CERT_SaveSMimeProfile (cert, profile, stime);
    if (must_free_cert)
	CERT_DestroyCertificate(cert);

    



    PORT_SetError (save_error);

    return rv;
}




SECStatus
NSS_CMSSignerInfo_IncludeCerts(NSSCMSSignerInfo *signerinfo, NSSCMSCertChainMode cm, SECCertUsage usage)
{
    if (signerinfo->cert == NULL)
	return SECFailure;

    
    if (signerinfo->certList != NULL) {
	CERT_DestroyCertificateList(signerinfo->certList);
	signerinfo->certList = NULL;
    }

    switch (cm) {
    case NSSCMSCM_None:
	signerinfo->certList = NULL;
	break;
    case NSSCMSCM_CertOnly:
	signerinfo->certList = CERT_CertListFromCert(signerinfo->cert);
	break;
    case NSSCMSCM_CertChain:
	signerinfo->certList = CERT_CertChainFromCert(signerinfo->cert, usage, PR_FALSE);
	break;
    case NSSCMSCM_CertChainWithRoot:
	signerinfo->certList = CERT_CertChainFromCert(signerinfo->cert, usage, PR_TRUE);
	break;
    }

    if (cm != NSSCMSCM_None && signerinfo->certList == NULL)
	return SECFailure;
    
    return SECSuccess;
}
