











































#include "nssilock.h"
#include "prmon.h"
#include "prtime.h"
#include "cert.h"
#include "certi.h"
#include "secder.h"
#include "secoid.h"
#include "secasn1.h"
#include "genname.h"
#include "keyhi.h"
#include "secitem.h"
#include "certdb.h"
#include "prprf.h"
#include "sechash.h"
#include "prlong.h"
#include "certxutl.h"
#include "portreg.h"
#include "secerr.h"
#include "sslerr.h"
#include "pk11func.h"
#include "xconst.h"   

#include "pki.h"
#include "pki3hack.h"

SEC_ASN1_MKSUB(CERT_TimeChoiceTemplate)
SEC_ASN1_MKSUB(SECOID_AlgorithmIDTemplate)
SEC_ASN1_MKSUB(SEC_BitStringTemplate)
SEC_ASN1_MKSUB(SEC_IntegerTemplate)
SEC_ASN1_MKSUB(SEC_SkipTemplate)






const SEC_ASN1Template CERT_CertExtensionTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(CERTCertExtension) },
    { SEC_ASN1_OBJECT_ID,
	  offsetof(CERTCertExtension,id) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_BOOLEAN,		
	  offsetof(CERTCertExtension,critical) },
    { SEC_ASN1_OCTET_STRING,
	  offsetof(CERTCertExtension,value) },
    { 0, }
};

const SEC_ASN1Template CERT_SequenceOfCertExtensionTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF, 0, CERT_CertExtensionTemplate }
};

const SEC_ASN1Template CERT_TimeChoiceTemplate[] = {
  { SEC_ASN1_CHOICE, offsetof(SECItem, type), 0, sizeof(SECItem) },
  { SEC_ASN1_UTC_TIME, 0, 0, siUTCTime },
  { SEC_ASN1_GENERALIZED_TIME, 0, 0, siGeneralizedTime },
  { 0 }
};

const SEC_ASN1Template CERT_ValidityTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(CERTValidity) },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
          offsetof(CERTValidity,notBefore),
          SEC_ASN1_SUB(CERT_TimeChoiceTemplate), 0 },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
          offsetof(CERTValidity,notAfter),
          SEC_ASN1_SUB(CERT_TimeChoiceTemplate), 0 },
    { 0 }
};

const SEC_ASN1Template CERT_CertificateTemplate[] = {
    { SEC_ASN1_SEQUENCE,
      0, NULL, sizeof(CERTCertificate) },
    { SEC_ASN1_EXPLICIT | SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | 
	  SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 0,  
	  offsetof(CERTCertificate,version),
	  SEC_ASN1_SUB(SEC_IntegerTemplate) },
    { SEC_ASN1_INTEGER,
	  offsetof(CERTCertificate,serialNumber) },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
	  offsetof(CERTCertificate,signature),
	  SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
    { SEC_ASN1_SAVE, 
	  offsetof(CERTCertificate,derIssuer) },
    { SEC_ASN1_INLINE,
	  offsetof(CERTCertificate,issuer),
	  CERT_NameTemplate },
    { SEC_ASN1_INLINE,
	  offsetof(CERTCertificate,validity),
	  CERT_ValidityTemplate },
    { SEC_ASN1_SAVE,
	  offsetof(CERTCertificate,derSubject) },
    { SEC_ASN1_INLINE,
	  offsetof(CERTCertificate,subject),
	  CERT_NameTemplate },
    { SEC_ASN1_SAVE,
	  offsetof(CERTCertificate,derPublicKey) },
    { SEC_ASN1_INLINE,
	  offsetof(CERTCertificate,subjectPublicKeyInfo),
	  CERT_SubjectPublicKeyInfoTemplate },
    { SEC_ASN1_OPTIONAL |  SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 1,
	  offsetof(CERTCertificate,issuerID),
	  SEC_ASN1_SUB(SEC_BitStringTemplate) },
    { SEC_ASN1_OPTIONAL |  SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 2,
	  offsetof(CERTCertificate,subjectID),
	  SEC_ASN1_SUB(SEC_BitStringTemplate) },
    { SEC_ASN1_EXPLICIT | SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | 
	  SEC_ASN1_CONTEXT_SPECIFIC | 3,
	  offsetof(CERTCertificate,extensions),
	  CERT_SequenceOfCertExtensionTemplate },
    { 0 }
};

const SEC_ASN1Template SEC_SignedCertificateTemplate[] =
{
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(CERTCertificate) },
    { SEC_ASN1_SAVE, 
	  offsetof(CERTCertificate,signatureWrap.data) },
    { SEC_ASN1_INLINE, 
	  0, CERT_CertificateTemplate },
    { SEC_ASN1_INLINE | SEC_ASN1_XTRN,
	  offsetof(CERTCertificate,signatureWrap.signatureAlgorithm),
	  SEC_ASN1_SUB(SECOID_AlgorithmIDTemplate) },
    { SEC_ASN1_BIT_STRING,
	  offsetof(CERTCertificate,signatureWrap.signature) },
    { 0 }
};




const SEC_ASN1Template SEC_CertSubjectTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(SECItem) },
    { SEC_ASN1_EXPLICIT | SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | 
	  SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 0,
	  0, SEC_ASN1_SUB(SEC_SkipTemplate) },	
    { SEC_ASN1_SKIP },		
    { SEC_ASN1_SKIP },		
    { SEC_ASN1_SKIP },		
    { SEC_ASN1_SKIP },		
    { SEC_ASN1_ANY, 0, NULL },		
    { SEC_ASN1_SKIP_REST },
    { 0 }
};




const SEC_ASN1Template SEC_CertIssuerTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(SECItem) },
    { SEC_ASN1_EXPLICIT | SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | 
	  SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 0,
	  0, SEC_ASN1_SUB(SEC_SkipTemplate) },	
    { SEC_ASN1_SKIP },		
    { SEC_ASN1_SKIP },		
    { SEC_ASN1_ANY, 0, NULL },		
    { SEC_ASN1_SKIP_REST },
    { 0 }
};



const SEC_ASN1Template SEC_CertSerialNumberTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(SECItem) },
    { SEC_ASN1_EXPLICIT | SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | 
	  SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 0,
	  0, SEC_ASN1_SUB(SEC_SkipTemplate) },	
    { SEC_ASN1_ANY, 0, NULL }, 
    { SEC_ASN1_SKIP_REST },
    { 0 }
};






const SEC_ASN1Template CERT_CertKeyTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(CERTCertKey) },
    { SEC_ASN1_EXPLICIT | SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | 
	  SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 0,
	  0, SEC_ASN1_SUB(SEC_SkipTemplate) },	 
    { SEC_ASN1_INTEGER,
	  offsetof(CERTCertKey,serialNumber) },
    { SEC_ASN1_SKIP },		
    { SEC_ASN1_ANY,
	  offsetof(CERTCertKey,derIssuer) },
    { SEC_ASN1_SKIP_REST },
    { 0 }
};

SEC_ASN1_CHOOSER_IMPLEMENT(CERT_TimeChoiceTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(CERT_CertificateTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(SEC_SignedCertificateTemplate)
SEC_ASN1_CHOOSER_IMPLEMENT(CERT_SequenceOfCertExtensionTemplate)

SECStatus
CERT_KeyFromIssuerAndSN(PRArenaPool *arena, SECItem *issuer, SECItem *sn,
			SECItem *key)
{
    key->len = sn->len + issuer->len;

    if ((sn->data == NULL) || (issuer->data == NULL)) {
	goto loser;
    }
    
    key->data = (unsigned char*)PORT_ArenaAlloc(arena, key->len);
    if ( !key->data ) {
	goto loser;
    }

    
    PORT_Memcpy(key->data, sn->data, sn->len);

    
    PORT_Memcpy(&key->data[sn->len], issuer->data, issuer->len);

    return(SECSuccess);

loser:
    return(SECFailure);
}





SECStatus
CERT_NameFromDERCert(SECItem *derCert, SECItem *derName)
{
    int rv;
    PRArenaPool *arena;
    CERTSignedData sd;
    void *tmpptr;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( ! arena ) {
	return(SECFailure);
    }
   
    PORT_Memset(&sd, 0, sizeof(CERTSignedData));
    rv = SEC_QuickDERDecodeItem(arena, &sd, CERT_SignedDataTemplate, derCert);
    
    if ( rv ) {
	goto loser;
    }
    
    PORT_Memset(derName, 0, sizeof(SECItem));
    rv = SEC_QuickDERDecodeItem(arena, derName, SEC_CertSubjectTemplate, &sd.data);

    if ( rv ) {
	goto loser;
    }

    tmpptr = derName->data;
    derName->data = (unsigned char*)PORT_Alloc(derName->len);
    if ( derName->data == NULL ) {
	goto loser;
    }
    
    PORT_Memcpy(derName->data, tmpptr, derName->len);
    
    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    PORT_FreeArena(arena, PR_FALSE);
    return(SECFailure);
}

SECStatus
CERT_IssuerNameFromDERCert(SECItem *derCert, SECItem *derName)
{
    int rv;
    PRArenaPool *arena;
    CERTSignedData sd;
    void *tmpptr;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( ! arena ) {
	return(SECFailure);
    }
   
    PORT_Memset(&sd, 0, sizeof(CERTSignedData));
    rv = SEC_QuickDERDecodeItem(arena, &sd, CERT_SignedDataTemplate, derCert);
    
    if ( rv ) {
	goto loser;
    }
    
    PORT_Memset(derName, 0, sizeof(SECItem));
    rv = SEC_QuickDERDecodeItem(arena, derName, SEC_CertIssuerTemplate, &sd.data);

    if ( rv ) {
	goto loser;
    }

    tmpptr = derName->data;
    derName->data = (unsigned char*)PORT_Alloc(derName->len);
    if ( derName->data == NULL ) {
	goto loser;
    }
    
    PORT_Memcpy(derName->data, tmpptr, derName->len);
    
    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    PORT_FreeArena(arena, PR_FALSE);
    return(SECFailure);
}

SECStatus
CERT_SerialNumberFromDERCert(SECItem *derCert, SECItem *derName)
{
    int rv;
    PRArenaPool *arena;
    CERTSignedData sd;
    void *tmpptr;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( ! arena ) {
	return(SECFailure);
    }
   
    PORT_Memset(&sd, 0, sizeof(CERTSignedData));
    rv = SEC_QuickDERDecodeItem(arena, &sd, CERT_SignedDataTemplate, derCert);
    
    if ( rv ) {
	goto loser;
    }
    
    PORT_Memset(derName, 0, sizeof(SECItem));
    rv = SEC_QuickDERDecodeItem(arena, derName, SEC_CertSerialNumberTemplate, &sd.data);

    if ( rv ) {
	goto loser;
    }

    tmpptr = derName->data;
    derName->data = (unsigned char*)PORT_Alloc(derName->len);
    if ( derName->data == NULL ) {
	goto loser;
    }
    
    PORT_Memcpy(derName->data, tmpptr, derName->len);
    
    PORT_FreeArena(arena, PR_FALSE);
    return(SECSuccess);

loser:
    PORT_FreeArena(arena, PR_FALSE);
    return(SECFailure);
}





SECStatus
CERT_KeyFromDERCert(PRArenaPool *reqArena, SECItem *derCert, SECItem *key)
{
    int rv;
    CERTSignedData sd;
    CERTCertKey certkey;

    if (!reqArena) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    PORT_Memset(&sd, 0, sizeof(CERTSignedData));
    rv = SEC_QuickDERDecodeItem(reqArena, &sd, CERT_SignedDataTemplate,
                                derCert);
    
    if ( rv ) {
	goto loser;
    }
    
    PORT_Memset(&certkey, 0, sizeof(CERTCertKey));
    rv = SEC_QuickDERDecodeItem(reqArena, &certkey, CERT_CertKeyTemplate,
                                &sd.data);

    if ( rv ) {
	goto loser;
    }

    return(CERT_KeyFromIssuerAndSN(reqArena, &certkey.derIssuer,
				   &certkey.serialNumber, key));
loser:
    return(SECFailure);
}





static SECStatus
GetKeyUsage(CERTCertificate *cert)
{
    SECStatus rv;
    SECItem tmpitem;
    
    rv = CERT_FindKeyUsageExtension(cert, &tmpitem);
    if ( rv == SECSuccess ) {
	
	cert->rawKeyUsage = tmpitem.data[0];
	cert->keyUsagePresent = PR_TRUE;
	cert->keyUsage = tmpitem.data[0];

	PORT_Free(tmpitem.data);
	tmpitem.data = NULL;
	
    } else {
	
	cert->keyUsage = KU_ALL;
	cert->rawKeyUsage = KU_ALL;
	cert->keyUsagePresent = PR_FALSE;
    }

    if ( CERT_GovtApprovedBitSet(cert) ) {
	cert->keyUsage |= KU_NS_GOVT_APPROVED;
	cert->rawKeyUsage |= KU_NS_GOVT_APPROVED;
    }
    
    return(SECSuccess);
}


static SECStatus
findOIDinOIDSeqByTagNum(CERTOidSequence *seq, SECOidTag tagnum)
{
    SECItem **oids;
    SECItem *oid;
    SECStatus rv = SECFailure;
    
    if (seq != NULL) {
	oids = seq->oids;
	while (oids != NULL && *oids != NULL) {
	    oid = *oids;
	    if (SECOID_FindOIDTag(oid) == tagnum) {
		rv = SECSuccess;
		break;
	    }
	    oids++;
	}
    }
    return rv;
}




SECStatus
cert_GetCertType(CERTCertificate *cert)
{
    PRUint32 nsCertType;

    if (cert->nsCertType) {
        
        return SECSuccess;
    }
    nsCertType = cert_ComputeCertType(cert);

    
    PORT_Assert(sizeof(cert->nsCertType) == sizeof(PRInt32));
    PR_ATOMIC_SET((PRInt32 *)&cert->nsCertType, nsCertType);
    return SECSuccess;
}

PRUint32
cert_ComputeCertType(CERTCertificate *cert)
{
    SECStatus rv;
    SECItem tmpitem;
    SECItem encodedExtKeyUsage;
    CERTOidSequence *extKeyUsage = NULL;
    PRBool basicConstraintPresent = PR_FALSE;
    CERTBasicConstraints basicConstraint;
    PRUint32 nsCertType = 0;

    tmpitem.data = NULL;
    CERT_FindNSCertTypeExtension(cert, &tmpitem);
    encodedExtKeyUsage.data = NULL;
    rv = CERT_FindCertExtension(cert, SEC_OID_X509_EXT_KEY_USAGE, 
				&encodedExtKeyUsage);
    if (rv == SECSuccess) {
	extKeyUsage = CERT_DecodeOidSequence(&encodedExtKeyUsage);
    }
    rv = CERT_FindBasicConstraintExten(cert, &basicConstraint);
    if (rv == SECSuccess) {
	basicConstraintPresent = PR_TRUE;
    }
    if (tmpitem.data != NULL || extKeyUsage != NULL) {
	if (tmpitem.data == NULL) {
	    nsCertType = 0;
	} else {
	    nsCertType = tmpitem.data[0];
	}

	
	PORT_Free(tmpitem.data);
	tmpitem.data = NULL;
	
	



	if ( ( nsCertType & NS_CERT_TYPE_SSL_CLIENT ) &&
	    cert->emailAddr && cert->emailAddr[0]) {
	    nsCertType |= NS_CERT_TYPE_EMAIL;
	}
	



	if ( nsCertType & NS_CERT_TYPE_SSL_CA ) {
	    nsCertType |= NS_CERT_TYPE_EMAIL_CA;
	}
	




	if (findOIDinOIDSeqByTagNum(extKeyUsage, 
				    SEC_OID_EXT_KEY_USAGE_EMAIL_PROTECT) ==
	    SECSuccess) {
	    if (basicConstraintPresent == PR_TRUE &&
		(basicConstraint.isCA)) {
		nsCertType |= NS_CERT_TYPE_EMAIL_CA;
	    } else {
		nsCertType |= NS_CERT_TYPE_EMAIL;
	    }
	}
	if (findOIDinOIDSeqByTagNum(extKeyUsage, 
				    SEC_OID_EXT_KEY_USAGE_SERVER_AUTH) ==
	    SECSuccess){
	    if (basicConstraintPresent == PR_TRUE &&
		(basicConstraint.isCA)) {
		nsCertType |= NS_CERT_TYPE_SSL_CA;
	    } else {
		nsCertType |= NS_CERT_TYPE_SSL_SERVER;
	    }
	}
	if (findOIDinOIDSeqByTagNum(extKeyUsage,
				    SEC_OID_EXT_KEY_USAGE_CLIENT_AUTH) ==
	    SECSuccess){
	    if (basicConstraintPresent == PR_TRUE &&
		(basicConstraint.isCA)) {
		nsCertType |= NS_CERT_TYPE_SSL_CA;
	    } else {
		nsCertType |= NS_CERT_TYPE_SSL_CLIENT;
	    }
	}
	if (findOIDinOIDSeqByTagNum(extKeyUsage,
				    SEC_OID_EXT_KEY_USAGE_CODE_SIGN) ==
	    SECSuccess) {
	    if (basicConstraintPresent == PR_TRUE &&
		(basicConstraint.isCA)) {
		nsCertType |= NS_CERT_TYPE_OBJECT_SIGNING_CA;
	    } else {
		nsCertType |= NS_CERT_TYPE_OBJECT_SIGNING;
	    }
	}
	if (findOIDinOIDSeqByTagNum(extKeyUsage,
				    SEC_OID_EXT_KEY_USAGE_TIME_STAMP) ==
	    SECSuccess) {
	    nsCertType |= EXT_KEY_USAGE_TIME_STAMP;
	}
	if (findOIDinOIDSeqByTagNum(extKeyUsage,
				    SEC_OID_OCSP_RESPONDER) == 
	    SECSuccess) {
	    nsCertType |= EXT_KEY_USAGE_STATUS_RESPONDER;
	}
    } else {
	
	nsCertType = 0;
	if (CERT_IsCACert(cert, &nsCertType))
	    nsCertType |= EXT_KEY_USAGE_STATUS_RESPONDER;
	

	if (basicConstraintPresent && basicConstraint.isCA ) {
	    nsCertType |= (NS_CERT_TYPE_SSL_CA   |
		           NS_CERT_TYPE_EMAIL_CA |
		           EXT_KEY_USAGE_STATUS_RESPONDER);
	}
	
	nsCertType |= NS_CERT_TYPE_SSL_CLIENT | NS_CERT_TYPE_SSL_SERVER |
	              NS_CERT_TYPE_EMAIL;
    }

    if (encodedExtKeyUsage.data != NULL) {
	PORT_Free(encodedExtKeyUsage.data);
    }
    if (extKeyUsage != NULL) {
	CERT_DestroyOidSequence(extKeyUsage);
    }
    return nsCertType;
}




SECStatus
cert_GetKeyID(CERTCertificate *cert)
{
    SECItem tmpitem;
    SECStatus rv;
    
    cert->subjectKeyID.len = 0;

    
    rv = CERT_FindSubjectKeyIDExtension(cert, &tmpitem);
    if ( rv == SECSuccess ) {
	cert->subjectKeyID.data = (unsigned char*) PORT_ArenaAlloc(cert->arena, tmpitem.len);
	if ( cert->subjectKeyID.data != NULL ) {
	    PORT_Memcpy(cert->subjectKeyID.data, tmpitem.data, tmpitem.len);
	    cert->subjectKeyID.len = tmpitem.len;
	    cert->keyIDGenerated = PR_FALSE;
	}
	
	PORT_Free(tmpitem.data);
    }
    
    
    if ( cert->subjectKeyID.len == 0 ) {
	



	cert->subjectKeyID.data = (unsigned char *)PORT_ArenaAlloc(cert->arena, SHA1_LENGTH);
	if ( cert->subjectKeyID.data != NULL ) {
	    rv = PK11_HashBuf(SEC_OID_SHA1,cert->subjectKeyID.data,
			      cert->derPublicKey.data,
			      cert->derPublicKey.len);
	    if ( rv == SECSuccess ) {
		cert->subjectKeyID.len = SHA1_LENGTH;
	    }
	}
    }

    if ( cert->subjectKeyID.len == 0 ) {
	return(SECFailure);
    }
    return(SECSuccess);

}

static PRBool
cert_IsRootCert(CERTCertificate *cert)
{
    SECStatus rv;
    SECItem tmpitem;

    
    cert->authKeyID = CERT_FindAuthKeyIDExten(cert->arena, cert);

    
    if (cert->derIssuer.len == 0 ||
        !SECITEM_ItemsAreEqual(&cert->derIssuer, &cert->derSubject))
    {
	return PR_FALSE;
    }

    
    if (cert->authKeyID) {
	
	if (cert->authKeyID->keyID.len > 0) {
	    
	    rv = CERT_FindSubjectKeyIDExtension(cert, &tmpitem);
	    if (rv == SECSuccess) {
		PRBool match;
		
		match = SECITEM_ItemsAreEqual(&cert->authKeyID->keyID,
		                              &tmpitem);
		PORT_Free(tmpitem.data);
		if (!match) return PR_FALSE; 
	    } else {
		
		return PR_FALSE;
	    }
	}
	if (cert->authKeyID->authCertIssuer) {
	    SECItem *caName;
	    caName = (SECItem *)CERT_GetGeneralNameByType(
	                                  cert->authKeyID->authCertIssuer,
	                                  certDirectoryName, PR_TRUE);
	    if (caName) {
		if (!SECITEM_ItemsAreEqual(&cert->derIssuer, caName)) {
		    return PR_FALSE;
		} 
	    } 
	}
	if (cert->authKeyID->authCertSerialNumber.len > 0) {
	    if (!SECITEM_ItemsAreEqual(&cert->serialNumber,
	                         &cert->authKeyID->authCertSerialNumber)) {
		return PR_FALSE;
	    } 
	}
	
	return PR_TRUE;
    }
    
    return PR_TRUE;
}




CERTCertificate *
CERT_DecodeDERCertificate(SECItem *derSignedCert, PRBool copyDER,
			 char *nickname)
{
    CERTCertificate *cert;
    PRArenaPool *arena;
    void *data;
    int rv;
    int len;
    char *tmpname;
    
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( !arena ) {
	return 0;
    }

    
    cert = (CERTCertificate *)PORT_ArenaZAlloc(arena, sizeof(CERTCertificate));
    
    if ( !cert ) {
	goto loser;
    }
    
    cert->arena = arena;
    
    if ( copyDER ) {
	
	data = (void *)PORT_ArenaAlloc(arena, derSignedCert->len);
	if ( !data ) {
	    goto loser;
	}
	cert->derCert.data = (unsigned char *)data;
	cert->derCert.len = derSignedCert->len;
	PORT_Memcpy(data, derSignedCert->data, derSignedCert->len);
    } else {
	
	cert->derCert = *derSignedCert;
    }

    
    rv = SEC_QuickDERDecodeItem(arena, cert, SEC_SignedCertificateTemplate,
		    &cert->derCert);

    if ( rv ) {
	goto loser;
    }

    if (cert_HasUnknownCriticalExten (cert->extensions) == PR_TRUE) {
        cert->options.bits.hasUnsupportedCriticalExt = PR_TRUE;
    }

    
    rv = CERT_KeyFromIssuerAndSN(arena, &cert->derIssuer, &cert->serialNumber,
			&cert->certKey);
    if ( rv ) {
	goto loser;
    }

    
    if ( nickname == NULL ) {
	cert->nickname = NULL;
    } else {
	
	len = PORT_Strlen(nickname) + 1;
	cert->nickname = (char*)PORT_ArenaAlloc(arena, len);
	if ( cert->nickname == NULL ) {
	    goto loser;
	}

	PORT_Memcpy(cert->nickname, nickname, len);
    }

    
    cert->emailAddr = cert_GetCertificateEmailAddresses(cert);
    
    
    rv = cert_GetKeyID(cert);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    
    rv = GetKeyUsage(cert);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    
    cert->isRoot = cert_IsRootCert(cert);

    
    rv = cert_GetCertType(cert);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    tmpname = CERT_NameToAscii(&cert->subject);
    if ( tmpname != NULL ) {
	cert->subjectName = PORT_ArenaStrdup(cert->arena, tmpname);
	PORT_Free(tmpname);
    }
    
    tmpname = CERT_NameToAscii(&cert->issuer);
    if ( tmpname != NULL ) {
	cert->issuerName = PORT_ArenaStrdup(cert->arena, tmpname);
	PORT_Free(tmpname);
    }
    
    cert->referenceCount = 1;
    cert->slot = NULL;
    cert->pkcs11ID = CK_INVALID_HANDLE;
    cert->dbnickname = NULL;
    
    return(cert);
    
loser:

    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(0);
}

CERTCertificate *
__CERT_DecodeDERCertificate(SECItem *derSignedCert, PRBool copyDER,
			 char *nickname)
{
    return CERT_DecodeDERCertificate(derSignedCert, copyDER, nickname);
}


CERTValidity *
CERT_CreateValidity(int64 notBefore, int64 notAfter)
{
    CERTValidity *v;
    int rv;
    PRArenaPool *arena;

    if (notBefore > notAfter) {
       PORT_SetError(SEC_ERROR_INVALID_ARGS);
       return NULL;
    }
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( !arena ) {
	return(0);
    }
    
    v = (CERTValidity*) PORT_ArenaZAlloc(arena, sizeof(CERTValidity));
    if (v) {
	v->arena = arena;
	rv = DER_EncodeTimeChoice(arena, &v->notBefore, notBefore);
	if (rv) goto loser;
	rv = DER_EncodeTimeChoice(arena, &v->notAfter, notAfter);
	if (rv) goto loser;
    }
    return v;

  loser:
    CERT_DestroyValidity(v);
    return 0;
}

SECStatus
CERT_CopyValidity(PRArenaPool *arena, CERTValidity *to, CERTValidity *from)
{
    SECStatus rv;

    CERT_DestroyValidity(to);
    to->arena = arena;
    
    rv = SECITEM_CopyItem(arena, &to->notBefore, &from->notBefore);
    if (rv) return rv;
    rv = SECITEM_CopyItem(arena, &to->notAfter, &from->notAfter);
    return rv;
}

void
CERT_DestroyValidity(CERTValidity *v)
{
    if (v && v->arena) {
	PORT_FreeArena(v->arena, PR_FALSE);
    }
    return;
}







#define PENDING_SLOP (24L*60L*60L)		/* seconds per day */
static PRInt32 pendingSlop = PENDING_SLOP;	

PRInt32
CERT_GetSlopTime(void)
{
    return pendingSlop;			
}

SECStatus
CERT_SetSlopTime(PRInt32 slop)		
{
    if (slop < 0)
	return SECFailure;
    pendingSlop = slop;
    return SECSuccess;
}

SECStatus
CERT_GetCertTimes(CERTCertificate *c, PRTime *notBefore, PRTime *notAfter)
{
    SECStatus rv;

    if (!c || !notBefore || !notAfter) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    
    
    rv = DER_DecodeTimeChoice(notBefore, &c->validity.notBefore);
    if (rv) {
	return(SECFailure);
    }
    
    
    rv = DER_DecodeTimeChoice(notAfter, &c->validity.notAfter);
    if (rv) {
	return(SECFailure);
    }

    return(SECSuccess);
}




SECCertTimeValidity
CERT_CheckCertValidTimes(CERTCertificate *c, PRTime t, PRBool allowOverride)
{
    PRTime notBefore, notAfter, llPendingSlop, tmp1;
    SECStatus rv;

    if (!c) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return(secCertTimeUndetermined);
    }
    
    if ( allowOverride && c->timeOK ) {
	return(secCertTimeValid);
    }

    rv = CERT_GetCertTimes(c, &notBefore, &notAfter);
    
    if (rv) {
	return(secCertTimeExpired); 
    }
    
    LL_I2L(llPendingSlop, pendingSlop);
    
    LL_UI2L(tmp1, PR_USEC_PER_SEC);
    LL_MUL(llPendingSlop, llPendingSlop, tmp1);
    LL_SUB(notBefore, notBefore, llPendingSlop);
    if ( LL_CMP( t, <, notBefore ) ) {
	PORT_SetError(SEC_ERROR_EXPIRED_CERTIFICATE);
	return(secCertTimeNotValidYet);
    }
    if ( LL_CMP( t, >, notAfter) ) {
	PORT_SetError(SEC_ERROR_EXPIRED_CERTIFICATE);
	return(secCertTimeExpired);
    }

    return(secCertTimeValid);
}

SECStatus
SEC_GetCrlTimes(CERTCrl *date, PRTime *notBefore, PRTime *notAfter)
{
    int rv;
    
    
    rv = DER_DecodeTimeChoice(notBefore, &date->lastUpdate);
    if (rv) {
	return(SECFailure);
    }
    
    
    if (date->nextUpdate.data) {
	rv = DER_DecodeTimeChoice(notAfter, &date->nextUpdate);
	if (rv) {
	    return(SECFailure);
	}
    }
    else {
	LL_I2L(*notAfter, 0L);
    }
    return(SECSuccess);
}




SECCertTimeValidity
SEC_CheckCrlTimes(CERTCrl *crl, PRTime t) {
    PRTime notBefore, notAfter, llPendingSlop, tmp1;
    SECStatus rv;

    rv = SEC_GetCrlTimes(crl, &notBefore, &notAfter);
    
    if (rv) {
	return(secCertTimeExpired); 
    }

    LL_I2L(llPendingSlop, pendingSlop);
    
    LL_I2L(tmp1, PR_USEC_PER_SEC);
    LL_MUL(llPendingSlop, llPendingSlop, tmp1);
    LL_SUB(notBefore, notBefore, llPendingSlop);
    if ( LL_CMP( t, <, notBefore ) ) {
	return(secCertTimeNotValidYet);
    }

    


    if ( LL_IS_ZERO(notAfter) ) {
	return(secCertTimeValid);
    }

    if ( LL_CMP( t, >, notAfter) ) {
	return(secCertTimeExpired);
    }

    return(secCertTimeValid);
}

PRBool
SEC_CrlIsNewer(CERTCrl *inNew, CERTCrl *old) {
    PRTime newNotBefore, newNotAfter;
    PRTime oldNotBefore, oldNotAfter;
    SECStatus rv;

    
    rv = SEC_GetCrlTimes(inNew, &newNotBefore, &newNotAfter);
    if (rv) return PR_FALSE;

    
    rv = SEC_GetCrlTimes(old, &oldNotBefore, &oldNotAfter);
    if (rv) return PR_TRUE;

    
    return ((PRBool)LL_CMP(oldNotBefore, <, newNotBefore));
}
   



SECStatus
CERT_KeyUsageAndTypeForCertUsage(SECCertUsage usage,
				 PRBool ca,
				 unsigned int *retKeyUsage,
				 unsigned int *retCertType)
{
    unsigned int requiredKeyUsage = 0;
    unsigned int requiredCertType = 0;
    
    if ( ca ) {
	switch ( usage ) {
	  case certUsageSSLServerWithStepUp:
	    requiredKeyUsage = KU_NS_GOVT_APPROVED | KU_KEY_CERT_SIGN;
	    requiredCertType = NS_CERT_TYPE_SSL_CA;
	    break;
	  case certUsageSSLClient:
	    requiredKeyUsage = KU_KEY_CERT_SIGN;
	    requiredCertType = NS_CERT_TYPE_SSL_CA;
	    break;
	  case certUsageSSLServer:
	    requiredKeyUsage = KU_KEY_CERT_SIGN;
	    requiredCertType = NS_CERT_TYPE_SSL_CA;
	    break;
	  case certUsageSSLCA:
	    requiredKeyUsage = KU_KEY_CERT_SIGN;
	    requiredCertType = NS_CERT_TYPE_SSL_CA;
	    break;
	  case certUsageEmailSigner:
	    requiredKeyUsage = KU_KEY_CERT_SIGN;
	    requiredCertType = NS_CERT_TYPE_EMAIL_CA;
	    break;
	  case certUsageEmailRecipient:
	    requiredKeyUsage = KU_KEY_CERT_SIGN;
	    requiredCertType = NS_CERT_TYPE_EMAIL_CA;
	    break;
	  case certUsageObjectSigner:
	    requiredKeyUsage = KU_KEY_CERT_SIGN;
	    requiredCertType = NS_CERT_TYPE_OBJECT_SIGNING_CA;
	    break;
	  case certUsageAnyCA:
	  case certUsageVerifyCA:
	  case certUsageStatusResponder:
	    requiredKeyUsage = KU_KEY_CERT_SIGN;
	    requiredCertType = NS_CERT_TYPE_OBJECT_SIGNING_CA |
		NS_CERT_TYPE_EMAIL_CA |
		    NS_CERT_TYPE_SSL_CA;
	    break;
	  default:
	    PORT_Assert(0);
	    goto loser;
	}
    } else {
	switch ( usage ) {
	  case certUsageSSLClient:
	    




	    requiredKeyUsage = KU_DIGITAL_SIGNATURE;
	    requiredCertType = NS_CERT_TYPE_SSL_CLIENT;
	    break;
	  case certUsageSSLServer:
	    requiredKeyUsage = KU_KEY_AGREEMENT_OR_ENCIPHERMENT;
	    requiredCertType = NS_CERT_TYPE_SSL_SERVER;
	    break;
	  case certUsageSSLServerWithStepUp:
	    requiredKeyUsage = KU_KEY_AGREEMENT_OR_ENCIPHERMENT |
		KU_NS_GOVT_APPROVED;
	    requiredCertType = NS_CERT_TYPE_SSL_SERVER;
	    break;
	  case certUsageSSLCA:
	    requiredKeyUsage = KU_KEY_CERT_SIGN;
	    requiredCertType = NS_CERT_TYPE_SSL_CA;
	    break;
	  case certUsageEmailSigner:
	    requiredKeyUsage = KU_DIGITAL_SIGNATURE_OR_NON_REPUDIATION;
	    requiredCertType = NS_CERT_TYPE_EMAIL;
	    break;
	  case certUsageEmailRecipient:
	    requiredKeyUsage = KU_KEY_AGREEMENT_OR_ENCIPHERMENT;
	    requiredCertType = NS_CERT_TYPE_EMAIL;
	    break;
	  case certUsageObjectSigner:
	    
	    requiredKeyUsage = KU_DIGITAL_SIGNATURE;
	    requiredCertType = NS_CERT_TYPE_OBJECT_SIGNING;
	    break;
	  case certUsageStatusResponder:
	    requiredKeyUsage = KU_DIGITAL_SIGNATURE_OR_NON_REPUDIATION;
	    requiredCertType = EXT_KEY_USAGE_STATUS_RESPONDER;
	    break;
	  default:
	    PORT_Assert(0);
	    goto loser;
	}
    }

    if ( retKeyUsage != NULL ) {
	*retKeyUsage = requiredKeyUsage;
    }
    if ( retCertType != NULL ) {
	*retCertType = requiredCertType;
    }

    return(SECSuccess);
loser:
    return(SECFailure);
}




SECStatus
CERT_CheckKeyUsage(CERTCertificate *cert, unsigned int requiredUsage)
{
    if (!cert) {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    


    if ( requiredUsage & KU_KEY_AGREEMENT_OR_ENCIPHERMENT ) {
	KeyType keyType = CERT_GetCertKeyType(&cert->subjectPublicKeyInfo);
	
	requiredUsage &= (~KU_KEY_AGREEMENT_OR_ENCIPHERMENT);

	switch (keyType) {
	case rsaKey:
	    requiredUsage |= KU_KEY_ENCIPHERMENT;
	    break;
	case dsaKey:
	    requiredUsage |= KU_DIGITAL_SIGNATURE;
	    break;
	case dhKey:
	    requiredUsage |= KU_KEY_AGREEMENT;
	    break;
	case ecKey:
	    
	    if (!(cert->keyUsage & (KU_DIGITAL_SIGNATURE | KU_KEY_AGREEMENT)))
		 goto loser;
	    break;
	default:
	    goto loser;
	}
    }

    
    if ( requiredUsage & KU_DIGITAL_SIGNATURE_OR_NON_REPUDIATION ) {
	
	requiredUsage &= (~KU_DIGITAL_SIGNATURE_OR_NON_REPUDIATION);

        if (!(cert->keyUsage & (KU_DIGITAL_SIGNATURE | KU_NON_REPUDIATION)))
             goto loser;
     }
    
    if ( (cert->keyUsage & requiredUsage) == requiredUsage ) 
    	return SECSuccess;

loser:
    PORT_SetError(SEC_ERROR_INADEQUATE_KEY_USAGE);
    return SECFailure;
}


CERTCertificate *
CERT_DupCertificate(CERTCertificate *c)
{
    if (c) {
	NSSCertificate *tmp = STAN_GetNSSCertificate(c);
	nssCertificate_AddRef(tmp);
    }
    return c;
}





static CERTCertDBHandle *default_cert_db_handle = 0;

void
CERT_SetDefaultCertDB(CERTCertDBHandle *handle)
{
    default_cert_db_handle = handle;
    
    return;
}

CERTCertDBHandle *
CERT_GetDefaultCertDB(void)
{
    return(default_cert_db_handle);
}


static void
sec_lower_string(char *s)
{
    if ( s == NULL ) {
	return;
    }
    
    while ( *s ) {
	*s = PORT_Tolower(*s);
	s++;
    }
    
    return;
}

static PRBool
cert_IsIPAddr(const char *hn)
{
    PRBool            isIPaddr       = PR_FALSE;
    PRNetAddr         netAddr;
    isIPaddr = (PR_SUCCESS == PR_StringToNetAddr(hn, &netAddr));
    return isIPaddr;
}





SECStatus
CERT_AddOKDomainName(CERTCertificate *cert, const char *hn)
{
    CERTOKDomainName *domainOK;
    int	       newNameLen;

    if (!hn || !(newNameLen = strlen(hn))) {
    	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    domainOK = (CERTOKDomainName *)PORT_ArenaZAlloc(cert->arena, 
				  (sizeof *domainOK) + newNameLen);
    if (!domainOK) 
    	return SECFailure;	

    PORT_Strcpy(domainOK->name, hn);
    sec_lower_string(domainOK->name);

    
    domainOK->next = cert->domainOK;
    cert->domainOK = domainOK;
    return SECSuccess;
}







static SECStatus
cert_TestHostName(char * cn, const char * hn)
{
    static int useShellExp = -1;

    if (useShellExp < 0) {
        useShellExp = (NULL != PR_GetEnv("NSS_USE_SHEXP_IN_CERT_NAME"));
    }
    if (useShellExp) {
    	
	int regvalid = PORT_RegExpValid(cn);
	if (regvalid != NON_SXP) {
	    SECStatus rv;
	    
	    int match = PORT_RegExpCaseSearch(hn, cn);

	    if ( match == 0 ) {
		rv = SECSuccess;
	    } else {
		PORT_SetError(SSL_ERROR_BAD_CERT_DOMAIN);
		rv = SECFailure;
	    }
	    return rv;
	}
    } else {
	
	char *wildcard    = PORT_Strchr(cn, '*');
	char *firstcndot  = PORT_Strchr(cn, '.');
	char *secondcndot = firstcndot ? PORT_Strchr(firstcndot+1, '.') : NULL;
	char *firsthndot  = PORT_Strchr(hn, '.');

	




	if (wildcard && secondcndot && secondcndot[1] && firsthndot 
	    && firstcndot  - wildcard  == 1
	    && secondcndot - firstcndot > 1
	    && PORT_Strrchr(cn, '*') == wildcard
	    && !PORT_Strncasecmp(cn, hn, wildcard - cn)
	    && !PORT_Strcasecmp(firstcndot, firsthndot)) {
	    
	    return SECSuccess;
	}
    }
    


    if (PORT_Strcasecmp(hn, cn) == 0) {
	return SECSuccess;
    }

    PORT_SetError(SSL_ERROR_BAD_CERT_DOMAIN);
    return SECFailure;
}


SECStatus
cert_VerifySubjectAltName(CERTCertificate *cert, const char *hn)
{
    PRArenaPool *     arena          = NULL;
    CERTGeneralName * nameList       = NULL;
    CERTGeneralName * current;
    char *            cn;
    int               cnBufLen;
    unsigned int      hnLen;
    int               DNSextCount    = 0;
    int               IPextCount     = 0;
    PRBool            isIPaddr       = PR_FALSE;
    SECStatus         rv             = SECFailure;
    SECItem           subAltName;
    PRNetAddr         netAddr;
    char              cnbuf[128];

    subAltName.data = NULL;
    hnLen    = strlen(hn);
    cn       = cnbuf;
    cnBufLen = sizeof cnbuf;

    rv = CERT_FindCertExtension(cert, SEC_OID_X509_SUBJECT_ALT_NAME, 
				&subAltName);
    if (rv != SECSuccess) {
	goto fail;
    }
    isIPaddr = (PR_SUCCESS == PR_StringToNetAddr(hn, &netAddr));
    rv = SECFailure;
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!arena) 
	goto fail;

    nameList = current = CERT_DecodeAltNameExtension(arena, &subAltName);
    if (!current)
    	goto fail;

    do {
	switch (current->type) {
	case certDNSName:
	    if (!isIPaddr) {
		


		int cnLen = current->name.other.len;
		rv = CERT_RFC1485_EscapeAndQuote(cn, cnBufLen, 
					    (char *)current->name.other.data,
					    cnLen);
		if (rv != SECSuccess && PORT_GetError() == SEC_ERROR_OUTPUT_LEN) {
		    cnBufLen = cnLen * 3 + 3; 
		    cn = (char *)PORT_ArenaAlloc(arena, cnBufLen);
		    if (!cn)
			goto fail;
		    rv = CERT_RFC1485_EscapeAndQuote(cn, cnBufLen, 
					    (char *)current->name.other.data,
					    cnLen);
		}
		if (rv == SECSuccess)
		    rv = cert_TestHostName(cn ,hn);
		if (rv == SECSuccess)
		    goto finish;
	    }
	    DNSextCount++;
	    break;
	case certIPAddress:
	    if (isIPaddr) {
		int match = 0;
		PRIPv6Addr v6Addr;
		if (current->name.other.len == 4 &&         
		    netAddr.inet.family == PR_AF_INET) {
		    match = !memcmp(&netAddr.inet.ip, 
		                    current->name.other.data, 4);
		} else if (current->name.other.len == 16 && 
		    netAddr.ipv6.family == PR_AF_INET6) {
		    match = !memcmp(&netAddr.ipv6.ip,
		                     current->name.other.data, 16);
		} else if (current->name.other.len == 16 && 
		    netAddr.inet.family == PR_AF_INET) {
		    
		    
		    PR_ConvertIPv4AddrToIPv6(netAddr.inet.ip, &v6Addr);
		    match = !memcmp(&v6Addr, current->name.other.data, 16);
		} else if (current->name.other.len == 4 &&  
		    netAddr.inet.family == PR_AF_INET6) {
		    
		    PRUint32 ipv4 = (current->name.other.data[0] << 24) |
		                    (current->name.other.data[1] << 16) |
				    (current->name.other.data[2] <<  8) |
				     current->name.other.data[3];
		    
		    PR_ConvertIPv4AddrToIPv6(PR_htonl(ipv4), &v6Addr);
		    match = !memcmp(&netAddr.ipv6.ip, &v6Addr, 16);
		} 
		if (match) {
		    rv = SECSuccess;
		    goto finish;
		}
	    }
	    IPextCount++;
	    break;
	default:
	    break;
	}
	current = CERT_GetNextGeneralName(current);
    } while (current != nameList);

fail:

    if (!(isIPaddr ? IPextCount : DNSextCount)) {
	
	PORT_SetError(SEC_ERROR_EXTENSION_NOT_FOUND);
    } else {
	PORT_SetError(SSL_ERROR_BAD_CERT_DOMAIN);
    }
    rv = SECFailure;

finish:

    
    if (arena) {
	PORT_FreeArena(arena, PR_FALSE);
    }

    if (subAltName.data) {
	SECITEM_FreeItem(&subAltName, PR_FALSE);
    }

    return rv;
}








CERTGeneralName *
cert_GetSubjectAltNameList(CERTCertificate *cert, PRArenaPool *arena)
{
    CERTGeneralName * nameList       = NULL;
    SECStatus         rv             = SECFailure;
    SECItem           subAltName;

    if (!cert || !arena)
      return NULL;

    subAltName.data = NULL;

    rv = CERT_FindCertExtension(cert, SEC_OID_X509_SUBJECT_ALT_NAME, 
                                &subAltName);
    if (rv != SECSuccess)
      return NULL;

    nameList = CERT_DecodeAltNameExtension(arena, &subAltName);
    SECITEM_FreeItem(&subAltName, PR_FALSE);
    return nameList;
}

PRUint32
cert_CountDNSPatterns(CERTGeneralName *firstName)
{
    CERTGeneralName * current;
    PRUint32 count = 0;

    if (!firstName)
      return 0;

    current = firstName;
    do {
        switch (current->type) {
        case certDNSName:
        case certIPAddress:
            ++count;
            break;
        default:
            break;
        }
        current = CERT_GetNextGeneralName(current);
    } while (current != firstName);

    return count;
}

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif






SECStatus
cert_GetDNSPatternsFromGeneralNames(CERTGeneralName *firstName,
                                    PRUint32 numberOfGeneralNames, 
                                    CERTCertNicknames *nickNames)
{
    CERTGeneralName *currentInput;
    char **currentOutput;

    if (!firstName || !nickNames || !numberOfGeneralNames)
      return SECFailure;

    nickNames->numnicknames = numberOfGeneralNames;
    nickNames->nicknames = PORT_ArenaAlloc(nickNames->arena,
                                       sizeof(char *) * numberOfGeneralNames);
    if (!nickNames->nicknames)
      return SECFailure;

    currentInput = firstName;
    currentOutput = nickNames->nicknames;
    do {
        char *cn = NULL;
        char ipbuf[INET6_ADDRSTRLEN];
        PRNetAddr addr;

        if (numberOfGeneralNames < 1) {
          
          return SECFailure;
        }

        switch (currentInput->type) {
        case certDNSName:
            


            cn = (char *)PORT_ArenaAlloc(nickNames->arena, 
                                         currentInput->name.other.len + 1);
            if (!cn)
              return SECFailure;
            PORT_Memcpy(cn, currentInput->name.other.data, 
                            currentInput->name.other.len);
            cn[currentInput->name.other.len] = 0;
            break;
        case certIPAddress:
            if (currentInput->name.other.len == 4) {
              addr.inet.family = PR_AF_INET;
              memcpy(&addr.inet.ip, currentInput->name.other.data, 
                                    currentInput->name.other.len);
            } else if (currentInput->name.other.len == 16) {
              addr.ipv6.family = PR_AF_INET6;
              memcpy(&addr.ipv6.ip, currentInput->name.other.data, 
                                    currentInput->name.other.len);
            }
            if (PR_NetAddrToString(&addr, ipbuf, sizeof(ipbuf)) == PR_FAILURE)
              return SECFailure;
            cn = PORT_ArenaStrdup(nickNames->arena, ipbuf);
            if (!cn)
              return SECFailure;
            break;
        default:
            break;
        }
        if (cn) {
          *currentOutput = cn;
          nickNames->totallen += PORT_Strlen(cn);
          ++currentOutput;
          --numberOfGeneralNames;
        }
        currentInput = CERT_GetNextGeneralName(currentInput);
    } while (currentInput != firstName);

    return (numberOfGeneralNames == 0) ? SECSuccess : SECFailure;
}







CERTCertNicknames *
CERT_GetValidDNSPatternsFromCert(CERTCertificate *cert)
{
    CERTGeneralName *generalNames;
    CERTCertNicknames *nickNames;
    PRArenaPool *arena;
    char *singleName;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!arena) {
        return NULL;
    }
    
    nickNames = PORT_ArenaAlloc(arena, sizeof(CERTCertNicknames));
    if (!nickNames) {
      PORT_FreeArena(arena, PR_FALSE);
      return NULL;
    }

    
    nickNames->arena = arena;
    nickNames->head = NULL;
    nickNames->numnicknames = 0;
    nickNames->nicknames = NULL;
    nickNames->totallen = 0;

    generalNames = cert_GetSubjectAltNameList(cert, arena);
    if (generalNames) {
      SECStatus rv_getnames = SECFailure; 
      PRUint32 numNames = cert_CountDNSPatterns(generalNames);

      if (numNames) {
        rv_getnames = cert_GetDNSPatternsFromGeneralNames(generalNames, 
                                                          numNames, nickNames);
      }

      
      if (numNames) {
        if (rv_getnames == SECSuccess) {
          return nickNames;
        }

        
        PORT_FreeArena(arena, PR_FALSE);
        return NULL;
      }
    }

    
    singleName = CERT_GetCommonName(&cert->subject);
    if (singleName) {
      nickNames->numnicknames = 1;
      nickNames->nicknames = PORT_ArenaAlloc(arena, sizeof(char *));
      if (nickNames->nicknames) {
        *nickNames->nicknames = PORT_ArenaStrdup(arena, singleName);
      }
      PORT_Free(singleName);

      
      if (nickNames->nicknames && *nickNames->nicknames) {
        return nickNames;
      }
    }

    PORT_FreeArena(arena, PR_FALSE);
    return NULL;
}





SECStatus
CERT_VerifyCertName(CERTCertificate *cert, const char *hn)
{
    char *    cn;
    SECStatus rv;
    CERTOKDomainName *domainOK;

    if (!hn || !strlen(hn)) {
    	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    
    for (domainOK = cert->domainOK; domainOK; domainOK = domainOK->next) {
	if (0 == PORT_Strcasecmp(hn, domainOK->name)) {
	    return SECSuccess;
    	}
    }

    


    rv = cert_VerifySubjectAltName(cert, hn);
    if (rv == SECSuccess || PORT_GetError() != SEC_ERROR_EXTENSION_NOT_FOUND)
    	return rv;

    cn = CERT_GetCommonName(&cert->subject);
    if ( cn ) {
        PRBool isIPaddr = cert_IsIPAddr(hn);
        if (isIPaddr) {
            if (PORT_Strcasecmp(hn, cn) == 0) {
                rv =  SECSuccess;
            } else {
                PORT_SetError(SSL_ERROR_BAD_CERT_DOMAIN);
                rv = SECFailure;
            }
        } else {
            rv = cert_TestHostName(cn, hn);
        }
	PORT_Free(cn);
    } else 
	PORT_SetError(SSL_ERROR_BAD_CERT_DOMAIN);
    return rv;
}

PRBool
CERT_CompareCerts(CERTCertificate *c1, CERTCertificate *c2)
{
    SECComparison comp;
    
    comp = SECITEM_CompareItem(&c1->derCert, &c2->derCert);
    if ( comp == SECEqual ) { 
	return(PR_TRUE);
    } else {
	return(PR_FALSE);
    }
}

static SECStatus
StringsEqual(char *s1, char *s2) {
    if ( ( s1 == NULL ) || ( s2 == NULL ) ) {
	if ( s1 != s2 ) { 
	    return(SECFailure);
	}
	return(SECSuccess); 
    }
	
    if ( PORT_Strcmp( s1, s2 ) != 0 ) {
	return(SECFailure); 
    }

    return(SECSuccess); 
}


PRBool
CERT_CompareCertsForRedirection(CERTCertificate *c1, CERTCertificate *c2)
{
    SECComparison comp;
    char *c1str, *c2str;
    SECStatus eq;
    
    comp = SECITEM_CompareItem(&c1->derCert, &c2->derCert);
    if ( comp == SECEqual ) { 
	return(PR_TRUE);
    }
	
    
    comp = SECITEM_CompareItem(&c1->derIssuer, &c2->derIssuer);
    if ( comp != SECEqual ) { 
	return(PR_FALSE);
    }

    
    c1str = CERT_GetCountryName(&c1->subject);
    c2str = CERT_GetCountryName(&c2->subject);
    eq = StringsEqual(c1str, c2str);
    PORT_Free(c1str);
    PORT_Free(c2str);
    if ( eq != SECSuccess ) {
	return(PR_FALSE);
    }

    
    c1str = CERT_GetLocalityName(&c1->subject);
    c2str = CERT_GetLocalityName(&c2->subject);
    eq = StringsEqual(c1str, c2str);
    PORT_Free(c1str);
    PORT_Free(c2str);
    if ( eq != SECSuccess ) {
	return(PR_FALSE);
    }
	
    
    c1str = CERT_GetStateName(&c1->subject);
    c2str = CERT_GetStateName(&c2->subject);
    eq = StringsEqual(c1str, c2str);
    PORT_Free(c1str);
    PORT_Free(c2str);
    if ( eq != SECSuccess ) {
	return(PR_FALSE);
    }

    
    c1str = CERT_GetOrgName(&c1->subject);
    c2str = CERT_GetOrgName(&c2->subject);
    eq = StringsEqual(c1str, c2str);
    PORT_Free(c1str);
    PORT_Free(c2str);
    if ( eq != SECSuccess ) {
	return(PR_FALSE);
    }

#ifdef NOTDEF	
    
    



    c1str = CERT_GetOrgUnitName(&c1->subject);
    c2str = CERT_GetOrgUnitName(&c2->subject);
    eq = StringsEqual(c1str, c2str);
    PORT_Free(c1str);
    PORT_Free(c2str);
    if ( eq != SECSuccess ) {
	return(PR_FALSE);
    }
#endif

    return(PR_TRUE); 
}






CERTIssuerAndSN *
CERT_GetCertIssuerAndSN(PRArenaPool *arena, CERTCertificate *cert)
{
    CERTIssuerAndSN *result;
    SECStatus rv;

    if ( arena == NULL ) {
	arena = cert->arena;
    }
    
    result = (CERTIssuerAndSN*)PORT_ArenaZAlloc(arena, sizeof(*result));
    if (result == NULL) {
	PORT_SetError (SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    rv = SECITEM_CopyItem(arena, &result->derIssuer, &cert->derIssuer);
    if (rv != SECSuccess)
	return NULL;

    rv = CERT_CopyName(arena, &result->issuer, &cert->issuer);
    if (rv != SECSuccess)
	return NULL;

    rv = SECITEM_CopyItem(arena, &result->serialNumber, &cert->serialNumber);
    if (rv != SECSuccess)
	return NULL;

    return result;
}

char *
CERT_MakeCANickname(CERTCertificate *cert)
{
    char *firstname = NULL;
    char *org = NULL;
    char *nickname = NULL;
    int count;
    CERTCertificate *dummycert;
    
    firstname = CERT_GetCommonName(&cert->subject);
    if ( firstname == NULL ) {
	firstname = CERT_GetOrgUnitName(&cert->subject);
    }

    org = CERT_GetOrgName(&cert->issuer);
    if (org == NULL) {
	org = CERT_GetDomainComponentName(&cert->issuer);
	if (org == NULL) {
	    if (firstname) {
		org = firstname;
		firstname = NULL;
	    } else {
		org = PORT_Strdup("Unknown CA");
	    }
	}
    }

    

    if (org == NULL) {
	goto done;
    }

    
    count = 1;
    while ( 1 ) {

	if ( firstname ) {
	    if ( count == 1 ) {
		nickname = PR_smprintf("%s - %s", firstname, org);
	    } else {
		nickname = PR_smprintf("%s - %s #%d", firstname, org, count);
	    }
	} else {
	    if ( count == 1 ) {
		nickname = PR_smprintf("%s", org);
	    } else {
		nickname = PR_smprintf("%s #%d", org, count);
	    }
	}
	if ( nickname == NULL ) {
	    goto done;
	}

	
	dummycert = CERT_FindCertByNickname(cert->dbhandle, nickname);

	if ( dummycert == NULL ) {
	    goto done;
	}
	
	
	CERT_DestroyCertificate(dummycert);

	
	PORT_Free(nickname);

	count++;
    }

done:
    if ( firstname ) {
	PORT_Free(firstname);
    }
    if ( org ) {
	PORT_Free(org);
    }
    
    return(nickname);
}



void
CERT_DestroyCrl (CERTSignedCrl *crl)
{
    SEC_DestroyCrl (crl);
}

static int
cert_Version(CERTCertificate *cert)
{
    int version = 0;
    if (cert && cert->version.data && cert->version.len) {
	version = DER_GetInteger(&cert->version);
	if (version < 0)
	    version = 0;
    }
    return version;
}

static unsigned int
cert_ComputeTrustOverrides(CERTCertificate *cert, unsigned int cType)
{
    CERTCertTrust *trust = cert->trust;

    if (trust && (trust->sslFlags |
		  trust->emailFlags |
		  trust->objectSigningFlags)) {

	if (trust->sslFlags & (CERTDB_TERMINAL_RECORD|CERTDB_TRUSTED)) 
	    cType |= NS_CERT_TYPE_SSL_SERVER|NS_CERT_TYPE_SSL_CLIENT;
	if (trust->sslFlags & (CERTDB_VALID_CA|CERTDB_TRUSTED_CA)) 
	    cType |= NS_CERT_TYPE_SSL_CA;
#if defined(CERTDB_NOT_TRUSTED)
	if (trust->sslFlags & CERTDB_NOT_TRUSTED) 
	    cType &= ~(NS_CERT_TYPE_SSL_SERVER|NS_CERT_TYPE_SSL_CLIENT|
	               NS_CERT_TYPE_SSL_CA);
#endif
	if (trust->emailFlags & (CERTDB_TERMINAL_RECORD|CERTDB_TRUSTED)) 
	    cType |= NS_CERT_TYPE_EMAIL;
	if (trust->emailFlags & (CERTDB_VALID_CA|CERTDB_TRUSTED_CA)) 
	    cType |= NS_CERT_TYPE_EMAIL_CA;
#if defined(CERTDB_NOT_TRUSTED)
	if (trust->emailFlags & CERTDB_NOT_TRUSTED) 
	    cType &= ~(NS_CERT_TYPE_EMAIL|NS_CERT_TYPE_EMAIL_CA);
#endif
	if (trust->objectSigningFlags & (CERTDB_TERMINAL_RECORD|CERTDB_TRUSTED)) 
	    cType |= NS_CERT_TYPE_OBJECT_SIGNING;
	if (trust->objectSigningFlags & (CERTDB_VALID_CA|CERTDB_TRUSTED_CA)) 
	    cType |= NS_CERT_TYPE_OBJECT_SIGNING_CA;
#if defined(CERTDB_NOT_TRUSTED)
	if (trust->objectSigningFlags & CERTDB_NOT_TRUSTED) 
	    cType &= ~(NS_CERT_TYPE_OBJECT_SIGNING|
	               NS_CERT_TYPE_OBJECT_SIGNING_CA);
#endif
    }
    return cType;
}





PRBool
CERT_IsCACert(CERTCertificate *cert, unsigned int *rettype)
{
    unsigned int cType = cert->nsCertType;
    PRBool ret = PR_FALSE;

    if (cType & (NS_CERT_TYPE_SSL_CA | NS_CERT_TYPE_EMAIL_CA | 
                NS_CERT_TYPE_OBJECT_SIGNING_CA)) {
        ret = PR_TRUE;
    } else {
	SECStatus rv;
	CERTBasicConstraints constraints;

	rv = CERT_FindBasicConstraintExten(cert, &constraints);
	if (rv == SECSuccess && constraints.isCA) {
	    ret = PR_TRUE;
	    cType |= (NS_CERT_TYPE_SSL_CA | NS_CERT_TYPE_EMAIL_CA);
	} 
    }

    
    if (!ret && 
        (cert->isRoot && cert_Version(cert) < SEC_CERTIFICATE_VERSION_3)) {
	ret = PR_TRUE;
	cType |= (NS_CERT_TYPE_SSL_CA | NS_CERT_TYPE_EMAIL_CA);
    }
    
    cType = cert_ComputeTrustOverrides(cert, cType);
    ret = (cType & (NS_CERT_TYPE_SSL_CA | NS_CERT_TYPE_EMAIL_CA |
                    NS_CERT_TYPE_OBJECT_SIGNING_CA)) ? PR_TRUE : PR_FALSE;

    if (rettype != NULL) {
	*rettype = cType;
    }
    return ret;
}

PRBool
CERT_IsCADERCert(SECItem *derCert, unsigned int *type) {
    CERTCertificate *cert;
    PRBool isCA;

    
    cert = CERT_DecodeDERCertificate(derCert, PR_FALSE, NULL);
    if (cert == NULL) return PR_FALSE;

    isCA = CERT_IsCACert(cert,type);
    CERT_DestroyCertificate (cert);
    return isCA;
}

PRBool
CERT_IsRootDERCert(SECItem *derCert)
{
    CERTCertificate *cert;
    PRBool isRoot;

    
    cert = CERT_DecodeDERCertificate(derCert, PR_FALSE, NULL);
    if (cert == NULL) return PR_FALSE;

    isRoot = cert->isRoot;
    CERT_DestroyCertificate (cert);
    return isRoot;
}

CERTCompareValidityStatus
CERT_CompareValidityTimes(CERTValidity* val_a, CERTValidity* val_b)
{
    PRTime notBeforeA, notBeforeB, notAfterA, notAfterB;

    if (!val_a || !val_b)
    {
        PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return certValidityUndetermined;
    }

    if ( SECSuccess != DER_DecodeTimeChoice(&notBeforeA, &val_a->notBefore) ||
         SECSuccess != DER_DecodeTimeChoice(&notBeforeB, &val_b->notBefore) ||
         SECSuccess != DER_DecodeTimeChoice(&notAfterA, &val_a->notAfter) ||
         SECSuccess != DER_DecodeTimeChoice(&notAfterB, &val_b->notAfter) ) {
        return certValidityUndetermined;
    }

    
    if (LL_CMP(notBeforeA,>,notAfterA) || LL_CMP(notBeforeB,>,notAfterB)) {
        PORT_SetError(SEC_ERROR_INVALID_TIME);
        return certValidityUndetermined;
    }

    if (LL_CMP(notAfterA,!=,notAfterB)) {
        
        return LL_CMP(notAfterA,<,notAfterB) ?
            certValidityChooseB : certValidityChooseA;
    }
    
    PORT_Assert(LL_CMP(notAfterA, == , notAfterB));
    
    if (LL_CMP(notBeforeA,==,notBeforeB)) {
	return certValidityEqual;
    }
    
    return LL_CMP(notBeforeA,<,notBeforeB) ?
        certValidityChooseB : certValidityChooseA;
}




PRBool
CERT_IsNewer(CERTCertificate *certa, CERTCertificate *certb)
{
    PRTime notBeforeA, notAfterA, notBeforeB, notAfterB, now;
    SECStatus rv;
    PRBool newerbefore, newerafter;
    
    rv = CERT_GetCertTimes(certa, &notBeforeA, &notAfterA);
    if ( rv != SECSuccess ) {
	return(PR_FALSE);
    }
    
    rv = CERT_GetCertTimes(certb, &notBeforeB, &notAfterB);
    if ( rv != SECSuccess ) {
	return(PR_TRUE);
    }

    newerbefore = PR_FALSE;
    if ( LL_CMP(notBeforeA, >, notBeforeB) ) {
	newerbefore = PR_TRUE;
    }

    newerafter = PR_FALSE;
    if ( LL_CMP(notAfterA, >, notAfterB) ) {
	newerafter = PR_TRUE;
    }
    
    if ( newerbefore && newerafter ) {
	return(PR_TRUE);
    }
    
    if ( ( !newerbefore ) && ( !newerafter ) ) {
	return(PR_FALSE);
    }

    
    now = PR_Now();

    if ( newerbefore ) {
	
	
	if ( LL_CMP(notAfterA, <, now ) ) {
	    return(PR_FALSE);
	}
	return(PR_TRUE);
    } else {
	
	
	if ( LL_CMP(notAfterB, <, now ) ) {
	    return(PR_TRUE);
	}
	return(PR_FALSE);
    }
}

void
CERT_DestroyCertArray(CERTCertificate **certs, unsigned int ncerts)
{
    unsigned int i;
    
    if ( certs ) {
	for ( i = 0; i < ncerts; i++ ) {
	    if ( certs[i] ) {
		CERT_DestroyCertificate(certs[i]);
	    }
	}

	PORT_Free(certs);
    }
    
    return;
}

char *
CERT_FixupEmailAddr(const char *emailAddr)
{
    char *retaddr;
    char *str;

    if ( emailAddr == NULL ) {
	return(NULL);
    }
    
    
    str = retaddr = PORT_Strdup(emailAddr);
    if ( str == NULL ) {
	return(NULL);
    }
    
    
    while ( *str ) {
	*str = tolower( *str );
	str++;
    }
    
    return(retaddr);
}




SECStatus
CERT_DecodeTrustString(CERTCertTrust *trust, const char *trusts)
{
    unsigned int i;
    unsigned int *pflags;
    
    if (!trust) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }
    trust->sslFlags = 0;
    trust->emailFlags = 0;
    trust->objectSigningFlags = 0;
    if (!trusts) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return SECFailure;
    }

    pflags = &trust->sslFlags;
    
    for (i=0; i < PORT_Strlen(trusts); i++) {
	switch (trusts[i]) {
	  case 'p':
	      *pflags = *pflags | CERTDB_TERMINAL_RECORD;
	      break;

	  case 'P':
	      *pflags = *pflags | CERTDB_TRUSTED | CERTDB_TERMINAL_RECORD;
	      break;

	  case 'w':
	      *pflags = *pflags | CERTDB_SEND_WARN;
	      break;

	  case 'c':
	      *pflags = *pflags | CERTDB_VALID_CA;
	      break;

	  case 'T':
	      *pflags = *pflags | CERTDB_TRUSTED_CLIENT_CA | CERTDB_VALID_CA;
	      break;

	  case 'C' :
	      *pflags = *pflags | CERTDB_TRUSTED_CA | CERTDB_VALID_CA;
	      break;

	  case 'u':
	      *pflags = *pflags | CERTDB_USER;
	      break;

	  case 'i':
	      *pflags = *pflags | CERTDB_INVISIBLE_CA;
	      break;
	  case 'g':
	      *pflags = *pflags | CERTDB_GOVT_APPROVED_CA;
	      break;

	  case ',':
	      if ( pflags == &trust->sslFlags ) {
		  pflags = &trust->emailFlags;
	      } else {
		  pflags = &trust->objectSigningFlags;
	      }
	      break;
	  default:
	      return SECFailure;
	}
    }

    return SECSuccess;
}

static void
EncodeFlags(char *trusts, unsigned int flags)
{
    if (flags & CERTDB_VALID_CA)
	if (!(flags & CERTDB_TRUSTED_CA) &&
	    !(flags & CERTDB_TRUSTED_CLIENT_CA))
	    PORT_Strcat(trusts, "c");
    if (flags & CERTDB_TERMINAL_RECORD)
	if (!(flags & CERTDB_TRUSTED))
	    PORT_Strcat(trusts, "p");
    if (flags & CERTDB_TRUSTED_CA)
	PORT_Strcat(trusts, "C");
    if (flags & CERTDB_TRUSTED_CLIENT_CA)
	PORT_Strcat(trusts, "T");
    if (flags & CERTDB_TRUSTED)
	PORT_Strcat(trusts, "P");
    if (flags & CERTDB_USER)
	PORT_Strcat(trusts, "u");
    if (flags & CERTDB_SEND_WARN)
	PORT_Strcat(trusts, "w");
    if (flags & CERTDB_INVISIBLE_CA)
	PORT_Strcat(trusts, "I");
    if (flags & CERTDB_GOVT_APPROVED_CA)
	PORT_Strcat(trusts, "G");
    return;
}

char *
CERT_EncodeTrustString(CERTCertTrust *trust)
{
    char tmpTrustSSL[32];
    char tmpTrustEmail[32];
    char tmpTrustSigning[32];
    char *retstr = NULL;

    if ( trust ) {
	tmpTrustSSL[0] = '\0';
	tmpTrustEmail[0] = '\0';
	tmpTrustSigning[0] = '\0';
    
	EncodeFlags(tmpTrustSSL, trust->sslFlags);
	EncodeFlags(tmpTrustEmail, trust->emailFlags);
	EncodeFlags(tmpTrustSigning, trust->objectSigningFlags);
    
	retstr = PR_smprintf("%s,%s,%s", tmpTrustSSL, tmpTrustEmail,
			     tmpTrustSigning);
    }
    
    return(retstr);
}

SECStatus
CERT_ImportCerts(CERTCertDBHandle *certdb, SECCertUsage usage,
		 unsigned int ncerts, SECItem **derCerts,
		 CERTCertificate ***retCerts, PRBool keepCerts,
		 PRBool caOnly, char *nickname)
{
    unsigned int i;
    CERTCertificate **certs = NULL;
    SECStatus rv;
    unsigned int fcerts = 0;

    if ( ncerts ) {
	certs = PORT_ZNewArray(CERTCertificate*, ncerts);
	if ( certs == NULL ) {
	    return(SECFailure);
	}
    
	
	for ( i = 0, fcerts= 0; i < ncerts; i++) {
	    certs[fcerts] = CERT_NewTempCertificate(certdb,
	                                            derCerts[i],
	                                            NULL,
	                                            PR_FALSE,
	                                            PR_TRUE);
	    if (certs[fcerts]) {
		SECItem subjKeyID = {siBuffer, NULL, 0};
		if (CERT_FindSubjectKeyIDExtension(certs[fcerts],
		                                   &subjKeyID) == SECSuccess) {
		    if (subjKeyID.data) {
			cert_AddSubjectKeyIDMapping(&subjKeyID, certs[fcerts]);
		    }
		    SECITEM_FreeItem(&subjKeyID, PR_FALSE);
		}
		fcerts++;
	    }
	}

	if ( keepCerts ) {
	    for ( i = 0; i < fcerts; i++ ) {
                char* canickname = NULL;
                PRBool isCA;

		SECKEY_UpdateCertPQG(certs[i]);
                
                isCA = CERT_IsCACert(certs[i], NULL);
                if ( isCA ) {
                    canickname = CERT_MakeCANickname(certs[i]);
                }

		if(isCA && (fcerts > 1)) {
		    





		    rv = CERT_AddTempCertToPerm(certs[i], canickname, NULL);
		} else {
		    rv = CERT_AddTempCertToPerm(certs[i],
                                                nickname?nickname:canickname, NULL);
		}

                PORT_Free(canickname);
		
	    }
	}
    }

    if ( retCerts ) {
	*retCerts = certs;
    } else {
	if (certs) {
	    CERT_DestroyCertArray(certs, fcerts);
	}
    }

    return ((fcerts || !ncerts) ? SECSuccess : SECFailure);
}





CERTCertList *
CERT_NewCertList(void)
{
    PRArenaPool *arena = NULL;
    CERTCertList *ret = NULL;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	goto loser;
    }
    
    ret = (CERTCertList *)PORT_ArenaZAlloc(arena, sizeof(CERTCertList));
    if ( ret == NULL ) {
	goto loser;
    }
    
    ret->arena = arena;
    
    PR_INIT_CLIST(&ret->list);
    
    return(ret);

loser:
    if ( arena != NULL ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(NULL);
}

void
CERT_DestroyCertList(CERTCertList *certs)
{
    PRCList *node;

    while( !PR_CLIST_IS_EMPTY(&certs->list) ) {
	node = PR_LIST_HEAD(&certs->list);
	CERT_DestroyCertificate(((CERTCertListNode *)node)->cert);
	PR_REMOVE_LINK(node);
    }
    
    PORT_FreeArena(certs->arena, PR_FALSE);
    
    return;
}

void
CERT_RemoveCertListNode(CERTCertListNode *node)
{
    CERT_DestroyCertificate(node->cert);
    PR_REMOVE_LINK(&node->links);
    return;
}


SECStatus
CERT_AddCertToListTailWithData(CERTCertList *certs, 
				CERTCertificate *cert, void *appData)
{
    CERTCertListNode *node;
    
    node = (CERTCertListNode *)PORT_ArenaZAlloc(certs->arena,
						sizeof(CERTCertListNode));
    if ( node == NULL ) {
	goto loser;
    }
    
    PR_INSERT_BEFORE(&node->links, &certs->list);
    
    node->cert = cert;
    node->appData = appData;
    return(SECSuccess);
    
loser:
    return(SECFailure);
}

SECStatus
CERT_AddCertToListTail(CERTCertList *certs, CERTCertificate *cert)
{
    return CERT_AddCertToListTailWithData(certs, cert, NULL);
}

SECStatus
CERT_AddCertToListHeadWithData(CERTCertList *certs, 
					CERTCertificate *cert, void *appData)
{
    CERTCertListNode *node;
    CERTCertListNode *head;
    
    head = CERT_LIST_HEAD(certs);

    if (head == NULL) return CERT_AddCertToListTail(certs,cert);

    node = (CERTCertListNode *)PORT_ArenaZAlloc(certs->arena,
						sizeof(CERTCertListNode));
    if ( node == NULL ) {
	goto loser;
    }
    
    PR_INSERT_BEFORE(&node->links, &head->links);
    
    node->cert = cert;
    node->appData = appData;
    return(SECSuccess);
    
loser:
    return(SECFailure);
}

SECStatus
CERT_AddCertToListHead(CERTCertList *certs, CERTCertificate *cert)
{
    return CERT_AddCertToListHeadWithData(certs, cert, NULL);
}





PRBool
CERT_SortCBValidity(CERTCertificate *certa,
		    CERTCertificate *certb,
		    void *arg)
{
    PRTime sorttime;
    PRTime notBeforeA, notAfterA, notBeforeB, notAfterB;
    SECStatus rv;
    PRBool newerbefore, newerafter;
    PRBool aNotValid = PR_FALSE, bNotValid = PR_FALSE;

    sorttime = *(PRTime *)arg;
    
    rv = CERT_GetCertTimes(certa, &notBeforeA, &notAfterA);
    if ( rv != SECSuccess ) {
	return(PR_FALSE);
    }
    
    rv = CERT_GetCertTimes(certb, &notBeforeB, &notAfterB);
    if ( rv != SECSuccess ) {
	return(PR_TRUE);
    }
    newerbefore = PR_FALSE;
    if ( LL_CMP(notBeforeA, >, notBeforeB) ) {
	newerbefore = PR_TRUE;
    }
    newerafter = PR_FALSE;
    if ( LL_CMP(notAfterA, >, notAfterB) ) {
	newerafter = PR_TRUE;
    }

    
    if ( CERT_CheckCertValidTimes(certa, sorttime, PR_FALSE)
	!= secCertTimeValid ) {
	aNotValid = PR_TRUE;
    }

    
    if ( CERT_CheckCertValidTimes(certb, sorttime, PR_FALSE)
	!= secCertTimeValid ) {
	bNotValid = PR_TRUE;
    }

    
    if ( bNotValid && ( ! aNotValid ) ) {
	return(PR_TRUE);
    }

    
    if ( aNotValid && ( ! bNotValid ) ) {
	return(PR_FALSE);
    }
    
    
    if ( newerbefore && newerafter ) {
	return(PR_TRUE);
    }
    
    if ( ( !newerbefore ) && ( !newerafter ) ) {
	return(PR_FALSE);
    }

    if ( newerbefore ) {
	
	return(PR_TRUE);
    } else {
	
	return(PR_FALSE);
    }
}


SECStatus
CERT_AddCertToListSorted(CERTCertList *certs,
			 CERTCertificate *cert,
			 CERTSortCallback f,
			 void *arg)
{
    CERTCertListNode *node;
    CERTCertListNode *head;
    PRBool ret;
    
    node = (CERTCertListNode *)PORT_ArenaZAlloc(certs->arena,
						sizeof(CERTCertListNode));
    if ( node == NULL ) {
	goto loser;
    }
    
    head = CERT_LIST_HEAD(certs);
    
    while ( !CERT_LIST_END(head, certs) ) {

	
	if ( cert == head->cert ) {
	    
	    
	    CERT_DestroyCertificate(cert);
	    goto done;
	}
	
	ret = (* f)(cert, head->cert, arg);
	
	if ( ret ) {
	    PR_INSERT_BEFORE(&node->links, &head->links);
	    goto done;
	}

	head = CERT_LIST_NEXT(head);
    }
    
    PR_INSERT_BEFORE(&node->links, &certs->list);

done:    
    
    node->cert = cert;
    return(SECSuccess);
    
loser:
    return(SECFailure);
}









SECStatus
CERT_FilterCertListByUsage(CERTCertList *certList, SECCertUsage usage,
			   PRBool ca)
{
    unsigned int requiredKeyUsage;
    unsigned int requiredCertType;
    CERTCertListNode *node, *savenode;
    SECStatus rv;
    
    if (certList == NULL) goto loser;

    rv = CERT_KeyUsageAndTypeForCertUsage(usage, ca, &requiredKeyUsage,
					  &requiredCertType);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    node = CERT_LIST_HEAD(certList);
	
    while ( !CERT_LIST_END(node, certList) ) {

	PRBool bad = (PRBool)(!node->cert);

	
	if ( !bad && 
	     CERT_CheckKeyUsage(node->cert, requiredKeyUsage) != SECSuccess ) {
	    bad = PR_TRUE;
	}
	
	if ( !bad ) {
	    unsigned int certType = 0;
	    if ( ca ) {
		



		(void)CERT_IsCACert(node->cert, &certType);
	    } else {
		certType = node->cert->nsCertType;
	    }
	    if ( !( certType & requiredCertType ) ) {
		bad = PR_TRUE;
	    }
	}

	if ( bad ) {
	    
	    savenode = CERT_LIST_NEXT(node);
	    CERT_RemoveCertListNode(node);
	    node = savenode;
	} else {
	    node = CERT_LIST_NEXT(node);
	}
    }
    return(SECSuccess);
    
loser:
    return(SECFailure);
}

PRBool CERT_IsUserCert(CERTCertificate* cert)
{
    if ( cert->trust &&
        ((cert->trust->sslFlags & CERTDB_USER ) ||
         (cert->trust->emailFlags & CERTDB_USER ) ||
         (cert->trust->objectSigningFlags & CERTDB_USER )) ) {
        return PR_TRUE;
    } else {
        return PR_FALSE;
    }
}

SECStatus
CERT_FilterCertListForUserCerts(CERTCertList *certList)
{
    CERTCertListNode *node, *freenode;
    CERTCertificate *cert;

    if (!certList) {
        return SECFailure;
    }

    node = CERT_LIST_HEAD(certList);
    
    while ( ! CERT_LIST_END(node, certList) ) {
	cert = node->cert;
	if ( PR_TRUE != CERT_IsUserCert(cert) ) {
	    
	    freenode = node;
	    node = CERT_LIST_NEXT(node);
	    CERT_RemoveCertListNode(freenode);
	} else {
	    
	    node = CERT_LIST_NEXT(node);
	}
    }

    return(SECSuccess);
}

static PZLock *certRefCountLock = NULL;







void
CERT_LockCertRefCount(CERTCertificate *cert)
{
    PORT_Assert(certRefCountLock != NULL);
    PZ_Lock(certRefCountLock);
    return;
}




void
CERT_UnlockCertRefCount(CERTCertificate *cert)
{
    PRStatus prstat;

    PORT_Assert(certRefCountLock != NULL);
    
    prstat = PZ_Unlock(certRefCountLock);
    
    PORT_Assert(prstat == PR_SUCCESS);

    return;
}

static PZLock *certTrustLock = NULL;







void
CERT_LockCertTrust(CERTCertificate *cert)
{
    PORT_Assert(certTrustLock != NULL);
    PZ_Lock(certTrustLock);
    return;
}

SECStatus
cert_InitLocks(void)
{
    if ( certRefCountLock == NULL ) {
        certRefCountLock = PZ_NewLock(nssILockRefLock);
        PORT_Assert(certRefCountLock != NULL);
        if (!certRefCountLock) {
            return SECFailure;
        }
    }

    if ( certTrustLock == NULL ) {
        certTrustLock = PZ_NewLock(nssILockCertDB);
        PORT_Assert(certTrustLock != NULL);
        if (!certTrustLock) {
            PZ_DestroyLock(certRefCountLock);
            return SECFailure;
        }
    }    

    return SECSuccess;
}

SECStatus
cert_DestroyLocks(void)
{
    SECStatus rv = SECSuccess;

    PORT_Assert(certRefCountLock != NULL);
    if (certRefCountLock) {
        PZ_DestroyLock(certRefCountLock);
        certRefCountLock = NULL;
    } else {
        rv = SECFailure;
    }

    PORT_Assert(certTrustLock != NULL);
    if (certTrustLock) {
        PZ_DestroyLock(certTrustLock);
        certTrustLock = NULL;
    } else {
        rv = SECFailure;
    }
    return rv;
}




void
CERT_UnlockCertTrust(CERTCertificate *cert)
{
    PRStatus prstat;

    PORT_Assert(certTrustLock != NULL);
    
    prstat = PZ_Unlock(certTrustLock);
    
    PORT_Assert(prstat == PR_SUCCESS);

    return;
}





CERTStatusConfig *
CERT_GetStatusConfig(CERTCertDBHandle *handle)
{
  return handle->statusConfig;
}





void
CERT_SetStatusConfig(CERTCertDBHandle *handle, CERTStatusConfig *statusConfig)
{
  PORT_Assert(handle->statusConfig == NULL);
  handle->statusConfig = statusConfig;
}





static PLHashTable *gSubjKeyIDHash = NULL;
static PRLock      *gSubjKeyIDLock = NULL;
static PLHashTable *gSubjKeyIDSlotCheckHash = NULL;
static PRLock      *gSubjKeyIDSlotCheckLock = NULL;

static void *cert_AllocTable(void *pool, PRSize size)
{
    return PORT_Alloc(size);
}

static void cert_FreeTable(void *pool, void *item)
{
    PORT_Free(item);
}

static PLHashEntry* cert_AllocEntry(void *pool, const void *key)
{
    return PORT_New(PLHashEntry);
}

static void cert_FreeEntry(void *pool, PLHashEntry *he, PRUintn flag)
{
    SECITEM_FreeItem((SECItem*)(he->value), PR_TRUE);
    if (flag == HT_FREE_ENTRY) {
        SECITEM_FreeItem((SECItem*)(he->key), PR_TRUE);
        PORT_Free(he);
    }
}

static PLHashAllocOps cert_AllocOps = {
    cert_AllocTable, cert_FreeTable, cert_AllocEntry, cert_FreeEntry
};

SECStatus
cert_CreateSubjectKeyIDSlotCheckHash(void)
{
    



    gSubjKeyIDSlotCheckHash = PL_NewHashTable(0, SECITEM_Hash,
                                             SECITEM_HashCompare,
                                             SECITEM_HashCompare,
                                             &cert_AllocOps, NULL);
    if (!gSubjKeyIDSlotCheckHash) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }
    gSubjKeyIDSlotCheckLock = PR_NewLock();
    if (!gSubjKeyIDSlotCheckLock) {
        PL_HashTableDestroy(gSubjKeyIDSlotCheckHash);
        gSubjKeyIDSlotCheckHash = NULL;
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }
    return SECSuccess;
}

SECStatus
cert_CreateSubjectKeyIDHashTable(void)
{
    gSubjKeyIDHash = PL_NewHashTable(0, SECITEM_Hash, SECITEM_HashCompare,
                                    SECITEM_HashCompare,
                                    &cert_AllocOps, NULL);
    if (!gSubjKeyIDHash) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }
    gSubjKeyIDLock = PR_NewLock();
    if (!gSubjKeyIDLock) {
        PL_HashTableDestroy(gSubjKeyIDHash);
        gSubjKeyIDHash = NULL;
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        return SECFailure;
    }
    
    if (cert_CreateSubjectKeyIDSlotCheckHash() != SECSuccess) {
	cert_DestroySubjectKeyIDHashTable();
	return SECFailure;
    }
    return SECSuccess;
}

SECStatus
cert_AddSubjectKeyIDMapping(SECItem *subjKeyID, CERTCertificate *cert)
{
    SECItem *newKeyID, *oldVal, *newVal;
    SECStatus rv = SECFailure;

    if (!gSubjKeyIDLock) {
	
	return SECFailure;
    }

    newVal = SECITEM_DupItem(&cert->derCert);
    if (!newVal) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        goto done;
    }
    newKeyID = SECITEM_DupItem(subjKeyID);
    if (!newKeyID) {
        SECITEM_FreeItem(newVal, PR_TRUE);
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        goto done;
    }

    PR_Lock(gSubjKeyIDLock);
    




    oldVal = (SECItem*)PL_HashTableLookup(gSubjKeyIDHash, subjKeyID);
    if (oldVal) {
        PL_HashTableRemove(gSubjKeyIDHash, subjKeyID);
    }

    rv = (PL_HashTableAdd(gSubjKeyIDHash, newKeyID, newVal)) ? SECSuccess :
                                                               SECFailure;
    PR_Unlock(gSubjKeyIDLock);
done:
    return rv;
}

SECStatus
cert_RemoveSubjectKeyIDMapping(SECItem *subjKeyID)
{
    SECStatus rv;
    if (!gSubjKeyIDLock)
        return SECFailure;

    PR_Lock(gSubjKeyIDLock);
    rv = (PL_HashTableRemove(gSubjKeyIDHash, subjKeyID)) ? SECSuccess :
                                                           SECFailure;
    PR_Unlock(gSubjKeyIDLock);
    return rv;
}

SECStatus
cert_UpdateSubjectKeyIDSlotCheck(SECItem *slotid, int series)
{
    SECItem *oldSeries, *newSlotid, *newSeries;
    SECStatus rv = SECFailure;

    if (!gSubjKeyIDSlotCheckLock) {
	return rv;
    }

    newSlotid = SECITEM_DupItem(slotid);
    newSeries = SECITEM_AllocItem(NULL, NULL, sizeof(int));
    if (!newSlotid || !newSeries ) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
        goto loser;
    }
    PORT_Memcpy(newSeries->data, &series, sizeof(int));

    PR_Lock(gSubjKeyIDSlotCheckLock);
    oldSeries = (SECItem *)PL_HashTableLookup(gSubjKeyIDSlotCheckHash, slotid);
    if (oldSeries) {
	



        PL_HashTableRemove(gSubjKeyIDSlotCheckHash, slotid);
    }
    rv = (PL_HashTableAdd(gSubjKeyIDSlotCheckHash, newSlotid, newSeries)) ?
         SECSuccess : SECFailure;
    PR_Unlock(gSubjKeyIDSlotCheckLock);
    if (rv == SECSuccess) {
	return rv;
    }

loser:
    if (newSlotid) {
        SECITEM_FreeItem(newSlotid, PR_TRUE);
    }
    if (newSeries) {
        SECITEM_FreeItem(newSeries, PR_TRUE);
    }
    return rv;
}

int
cert_SubjectKeyIDSlotCheckSeries(SECItem *slotid)
{
    SECItem *seriesItem = NULL;
    int series;

    if (!gSubjKeyIDSlotCheckLock) {
	PORT_SetError(SEC_ERROR_NOT_INITIALIZED);
	return -1;
    }

    PR_Lock(gSubjKeyIDSlotCheckLock);
    seriesItem = (SECItem *)PL_HashTableLookup(gSubjKeyIDSlotCheckHash, slotid);
    PR_Unlock(gSubjKeyIDSlotCheckLock);
     

    if (seriesItem == NULL) {
	return 0;
    }
    
    PORT_Assert(seriesItem->len == sizeof(int));
    if (seriesItem->len != sizeof(int)) {
	PORT_SetError(SEC_ERROR_LIBRARY_FAILURE);
	return -1;
    }
    PORT_Memcpy(&series, seriesItem->data, sizeof(int));
    return series;
}

SECStatus
cert_DestroySubjectKeyIDSlotCheckHash(void)
{
    if (gSubjKeyIDSlotCheckHash) {
        PR_Lock(gSubjKeyIDSlotCheckLock);
        PL_HashTableDestroy(gSubjKeyIDSlotCheckHash);
        gSubjKeyIDSlotCheckHash = NULL;
        PR_Unlock(gSubjKeyIDSlotCheckLock);
        PR_DestroyLock(gSubjKeyIDSlotCheckLock);
        gSubjKeyIDSlotCheckLock = NULL;
    }
    return SECSuccess;
}

SECStatus
cert_DestroySubjectKeyIDHashTable(void)
{
    if (gSubjKeyIDHash) {
        PR_Lock(gSubjKeyIDLock);
        PL_HashTableDestroy(gSubjKeyIDHash);
        gSubjKeyIDHash = NULL;
        PR_Unlock(gSubjKeyIDLock);
        PR_DestroyLock(gSubjKeyIDLock);
        gSubjKeyIDLock = NULL;
    }
    cert_DestroySubjectKeyIDSlotCheckHash();
    return SECSuccess;
}

SECItem*
cert_FindDERCertBySubjectKeyID(SECItem *subjKeyID)
{
    SECItem   *val;
 
    if (!gSubjKeyIDLock)
        return NULL;

    PR_Lock(gSubjKeyIDLock);
    val = (SECItem*)PL_HashTableLookup(gSubjKeyIDHash, subjKeyID);
    if (val) {
        val = SECITEM_DupItem(val);
    }
    PR_Unlock(gSubjKeyIDLock);
    return val;
}

CERTCertificate*
CERT_FindCertBySubjectKeyID(CERTCertDBHandle *handle, SECItem *subjKeyID)
{
    CERTCertificate *cert = NULL;
    SECItem *derCert;

    derCert = cert_FindDERCertBySubjectKeyID(subjKeyID);
    if (derCert) {
        cert = CERT_FindCertByDERCert(handle, derCert);
        SECITEM_FreeItem(derCert, PR_TRUE);
    }
    return cert;
}
