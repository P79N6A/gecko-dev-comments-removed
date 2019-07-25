



































#include "cert.h"
#include "certt.h"
#include "secder.h"
#include "key.h"
#include "secitem.h"
#include "secasn1.h"
#include "secerr.h"

SEC_ASN1_MKSUB(SEC_AnyTemplate)

const SEC_ASN1Template CERT_AttributeTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	0, NULL, sizeof(CERTAttribute) },
    { SEC_ASN1_OBJECT_ID, offsetof(CERTAttribute, attrType) },
    { SEC_ASN1_SET_OF | SEC_ASN1_XTRN, offsetof(CERTAttribute, attrValue),
	SEC_ASN1_SUB(SEC_AnyTemplate) },
    { 0 }
};

const SEC_ASN1Template CERT_SetOfAttributeTemplate[] = {
    { SEC_ASN1_SET_OF, 0, CERT_AttributeTemplate },
};

const SEC_ASN1Template CERT_CertificateRequestTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(CERTCertificateRequest) },
    { SEC_ASN1_INTEGER,
	  offsetof(CERTCertificateRequest,version) },
    { SEC_ASN1_INLINE,
	  offsetof(CERTCertificateRequest,subject),
	  CERT_NameTemplate },
    { SEC_ASN1_INLINE,
	  offsetof(CERTCertificateRequest,subjectPublicKeyInfo),
	  CERT_SubjectPublicKeyInfoTemplate },
    { SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 0,
	  offsetof(CERTCertificateRequest,attributes), 
	  CERT_SetOfAttributeTemplate },
    { 0 }
};

SEC_ASN1_CHOOSER_IMPLEMENT(CERT_CertificateRequestTemplate)

CERTCertificate *
CERT_CreateCertificate(unsigned long serialNumber,
		      CERTName *issuer,
		      CERTValidity *validity,
		      CERTCertificateRequest *req)
{
    CERTCertificate *c;
    int rv;
    PRArenaPool *arena;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    if ( !arena ) {
	return(0);
    }

    c = (CERTCertificate *)PORT_ArenaZAlloc(arena, sizeof(CERTCertificate));
    
    if (!c) {
	PORT_FreeArena(arena, PR_FALSE);
	return 0;
    }

    c->referenceCount = 1;
    c->arena = arena;

    



    rv = DER_SetUInteger(arena, &c->version, SEC_CERTIFICATE_VERSION_1);
    if (rv) goto loser;

    rv = DER_SetUInteger(arena, &c->serialNumber, serialNumber);
    if (rv) goto loser;

    rv = CERT_CopyName(arena, &c->issuer, issuer);
    if (rv) goto loser;

    rv = CERT_CopyValidity(arena, &c->validity, validity);
    if (rv) goto loser;

    rv = CERT_CopyName(arena, &c->subject, &req->subject);
    if (rv) goto loser;
    rv = SECKEY_CopySubjectPublicKeyInfo(arena, &c->subjectPublicKeyInfo,
					 &req->subjectPublicKeyInfo);
    if (rv) goto loser;

    return c;

 loser:
    CERT_DestroyCertificate(c);
    return 0;
}




















CERTCertificateRequest *
CERT_CreateCertificateRequest(CERTName *subject,
			     CERTSubjectPublicKeyInfo *spki,
			     SECItem **attributes)
{
    CERTCertificateRequest *certreq;
    PRArenaPool *arena;
    CERTAttribute * attribute;
    SECOidData * oidData;
    SECStatus rv;
    int i = 0;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	return NULL;
    }
    
    certreq = PORT_ArenaZNew(arena, CERTCertificateRequest);
    if (!certreq) {
	PORT_FreeArena(arena, PR_FALSE);
	return NULL;
    }
    

    certreq->arena = arena;
    
    rv = DER_SetUInteger(arena, &certreq->version,
			 SEC_CERTIFICATE_REQUEST_VERSION);
    if (rv != SECSuccess)
	goto loser;

    rv = CERT_CopyName(arena, &certreq->subject, subject);
    if (rv != SECSuccess)
	goto loser;

    rv = SECKEY_CopySubjectPublicKeyInfo(arena,
				      &certreq->subjectPublicKeyInfo,
				      spki);
    if (rv != SECSuccess)
	goto loser;

    certreq->attributes = PORT_ArenaZNewArray(arena, CERTAttribute*, 2);
    if(!certreq->attributes) 
	goto loser;

    
    if (!attributes || !attributes[0]) {
	








	certreq->attributes[0] = NULL;
	return certreq;
    }	

    
    attribute = PORT_ArenaZNew(arena, CERTAttribute);
    if (!attribute) 
	goto loser;

    oidData = SECOID_FindOIDByTag( SEC_OID_PKCS9_EXTENSION_REQUEST );
    PORT_Assert(oidData);
    if (!oidData)
	goto loser;
    rv = SECITEM_CopyItem(arena, &attribute->attrType, &oidData->oid);
    if (rv != SECSuccess)
	goto loser;

    for (i = 0; attributes[i] != NULL ; i++) 
	;
    attribute->attrValue = PORT_ArenaZNewArray(arena, SECItem *, i+1);
    if (!attribute->attrValue) 
	goto loser;

    
    for (i = 0; attributes[i]; i++) {
	






	attribute->attrValue[i] = SECITEM_ArenaDupItem(arena, attributes[i]);
	if(!attribute->attrValue[i]) 
	    goto loser;
    }

    certreq->attributes[0] = attribute;

    return certreq;

loser:
    CERT_DestroyCertificateRequest(certreq);
    return NULL;
}

void
CERT_DestroyCertificateRequest(CERTCertificateRequest *req)
{
    if (req && req->arena) {
	PORT_FreeArena(req->arena, PR_FALSE);
    }
    return;
}

static void
setCRExt(void *o, CERTCertExtension **exts)
{
    ((CERTCertificateRequest *)o)->attributes = (struct CERTAttributeStr **)exts;
}






extern void *cert_StartExtensions(void *owner, PRArenaPool *ownerArena,
                       void (*setExts)(void *object, CERTCertExtension **exts));
void *
CERT_StartCertificateRequestAttributes(CERTCertificateRequest *req)
{
    return (cert_StartExtensions ((void *)req, req->arena, setCRExt));
}








SECStatus
CERT_FinishCertificateRequestAttributes(CERTCertificateRequest *req)
{   SECItem *extlist;
    SECOidData *oidrec;
    CERTAttribute *attribute;
   
    if (!req || !req->arena) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    if (req->attributes == NULL || req->attributes[0] == NULL)
        return SECSuccess;

    extlist = SEC_ASN1EncodeItem(req->arena, NULL, &req->attributes,
                            SEC_ASN1_GET(CERT_SequenceOfCertExtensionTemplate));
    if (extlist == NULL)
        return(SECFailure);

    oidrec = SECOID_FindOIDByTag(SEC_OID_PKCS9_EXTENSION_REQUEST);
    if (oidrec == NULL)
	return SECFailure;

    

    req->attributes = PORT_ArenaZNewArray(req->arena, CERTAttribute*, 2);

    attribute = PORT_ArenaZNew(req->arena, CERTAttribute);
    
    if (req->attributes == NULL || attribute == NULL ||
        SECITEM_CopyItem(req->arena, &attribute->attrType, &oidrec->oid) != 0) {
        PORT_SetError(SEC_ERROR_NO_MEMORY);
	return SECFailure;
    }
    attribute->attrValue = PORT_ArenaZNewArray(req->arena, SECItem*, 2);

    if (attribute->attrValue == NULL)
        return SECFailure;

    attribute->attrValue[0] = extlist;
    attribute->attrValue[1] = NULL;
    req->attributes[0] = attribute;
    req->attributes[1] = NULL;

    return SECSuccess;
}

SECStatus
CERT_GetCertificateRequestExtensions(CERTCertificateRequest *req,
                                        CERTCertExtension ***exts)
{
    if (req == NULL || exts == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }
    
    if (req->attributes == NULL || *req->attributes == NULL)
        return SECSuccess;
    
    if ((*req->attributes)->attrValue == NULL) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
        return SECFailure;
    }

    return(SEC_ASN1DecodeItem(req->arena, exts, 
            SEC_ASN1_GET(CERT_SequenceOfCertExtensionTemplate),
            (*req->attributes)->attrValue[0]));
}
