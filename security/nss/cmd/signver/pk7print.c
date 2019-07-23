







































 

 
 
#include "secutil.h"
#include "secpkcs7.h"
#include "secoid.h"
#include <sys/stat.h>
#include <stdarg.h>
 
#ifdef XP_UNIX
#include <unistd.h>
#endif
 

#include "cert.h"
#include "prtypes.h"
#include "prtime.h"
 
#include "prlong.h"
#include "secmod.h"
#include "pk11func.h"
#include "prerror.h"
 








int
sv_PrintPKCS7ContentInfo(FILE *, SEC_PKCS7ContentInfo *, char *);


void
sv_PrintAsHex(FILE *out, SECItem *data, char *m)
{
    unsigned i;

    if (m) fprintf(out, m);
    
    for (i = 0; i < data->len; i++) {
        if (i < data->len - 1) {
            fprintf(out, "%02x:", data->data[i]);
        } else {
            fprintf(out, "%02x\n", data->data[i]);
            break;
        }
    }
}

void
sv_PrintInteger(FILE *out, SECItem *i, char *m)
{
    int iv;

    if (i->len > 4) {
        sv_PrintAsHex(out, i, m);
    } else {
        iv = DER_GetInteger(i);
        fprintf(out, "%s%d (0x%x)\n", m, iv, iv);
    }
}


int
sv_PrintTime(FILE *out, SECItem *t, char *m)
{
    PRExplodedTime printableTime; 
    int64 time;
    char *timeString;
    int rv;

    rv = DER_DecodeTimeChoice(&time, t);
    if (rv) return rv;

    
    PR_ExplodeTime(time, PR_LocalTimeParameters, &printableTime);

    timeString = (char *)PORT_Alloc(256);

    if ( timeString ) {
        if (PR_FormatTime( timeString, 256, "%a %b %d %H:%M:%S %Y", &printableTime )) {
            fprintf(out, "%s%s\n", m, timeString);
        }
        PORT_Free(timeString);
        return 0;
    }
    return SECFailure;
}

int
sv_PrintValidity(FILE *out, CERTValidity *v, char *m)
{
    int rv;

    fprintf(out, m);
    rv = sv_PrintTime(out, &v->notBefore, "notBefore=");
    if (rv) return rv;
    fprintf(out, m);
    sv_PrintTime(out, &v->notAfter, "notAfter=");
    return rv;
}

void
sv_PrintObjectID(FILE *out, SECItem *oid, char *m)
{
    const char *name;
    SECOidData *oiddata;
    
    oiddata = SECOID_FindOID(oid);
    if (oiddata == NULL) {
        sv_PrintAsHex(out, oid, m);
        return;
    }
    name = oiddata->desc;

    if (m != NULL)
        fprintf(out, "%s", m);
    fprintf(out, "%s\n", name);
}

void
sv_PrintAlgorithmID(FILE *out, SECAlgorithmID *a, char *m)
{
    sv_PrintObjectID(out, &a->algorithm, m);

    if ((a->parameters.len != 2) ||
        (PORT_Memcmp(a->parameters.data, "\005\000", 2) != 0)) {
        
        sv_PrintAsHex(out, &a->parameters, "Args=");
    }
}

void
sv_PrintAttribute(FILE *out, SEC_PKCS7Attribute *attr, char *m)
{
    SECItem *value;
    int i;
    char om[100];

    fprintf(out, m);

    



    sv_PrintObjectID(out, &(attr->type), "type=");
    if (attr->values != NULL) {
        i = 0;
        while ((value = attr->values[i]) != NULL) {
            sprintf(om, "%svalue[%d]=%s", m, i++, attr->encoded ? "(encoded)" : ""); 
            if (attr->encoded || attr->typeTag == NULL) {
                sv_PrintAsHex(out, value, om);
            } else {
                switch (attr->typeTag->offset) {
                    default:
                        sv_PrintAsHex(out, value, om);
                        break;
                    case SEC_OID_PKCS9_CONTENT_TYPE:
                        sv_PrintObjectID(out, value, om);
                        break;
                    case SEC_OID_PKCS9_SIGNING_TIME:
                        sv_PrintTime(out, value, om);
                        break;
                }
            }
        }
    }
}

void
sv_PrintName(FILE *out, CERTName *name, char *msg)
{
    char *str;

    str = CERT_NameToAscii(name);
    fprintf(out, "%s%s\n", msg, str);
    PORT_Free(str);
}


#if 0




void
secu_PrintPKCS7EncContent(FILE *out, SEC_PKCS7EncryptedContentInfo *src, 
			  char *m, int level)
{
    if (src->contentTypeTag == NULL)
	src->contentTypeTag = SECOID_FindOID(&(src->contentType));

    secu_Indent(out, level);
    fprintf(out, "%s:\n", m);
    secu_Indent(out, level + 1); 
    fprintf(out, "Content Type: %s\n",
	    (src->contentTypeTag != NULL) ? src->contentTypeTag->desc
					  : "Unknown");
    sv_PrintAlgorithmID(out, &(src->contentEncAlg),
			  "Content Encryption Algorithm");
    sv_PrintAsHex(out, &(src->encContent), 
		    "Encrypted Content", level+1);
}





void
secu_PrintRecipientInfo(FILE *out, SEC_PKCS7RecipientInfo *info, char *m, 
			int level)
{
    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    sv_PrintInteger(out, &(info->version), "Version");	

    sv_PrintName(out, &(info->issuerAndSN->issuer), "Issuer");
    sv_PrintInteger(out, &(info->issuerAndSN->serialNumber), 
		      "Serial Number");

    
    sv_PrintAlgorithmID(out, &(info->keyEncAlg), 
			"Key Encryption Algorithm");
    sv_PrintAsHex(out, &(info->encKey), "Encrypted Key", level + 1);
}
#endif





void
sv_PrintSignerInfo(FILE *out, SEC_PKCS7SignerInfo *info, char *m)
{
    SEC_PKCS7Attribute *attr;
    int iv;
    
    fprintf(out, m);
    sv_PrintInteger(out, &(info->version), "version=");

    fprintf(out, m);
    sv_PrintName(out, &(info->issuerAndSN->issuer), "issuerName=");
    fprintf(out, m);
    sv_PrintInteger(out, &(info->issuerAndSN->serialNumber), 
                        "serialNumber=");
  
    fprintf(out, m);
    sv_PrintAlgorithmID(out, &(info->digestAlg), "digestAlgorithm=");
    
    if (info->authAttr != NULL) {
        char mm[120];

        iv = 0;
        while (info->authAttr[iv] != NULL) iv++;
        fprintf(out, "%sauthenticatedAttributes=%d\n", m, iv);
        iv = 0;
        while ((attr = info->authAttr[iv]) != NULL) {
            sprintf(mm, "%sattribute[%d].", m, iv++); 
            sv_PrintAttribute(out, attr, mm);
        }
    }
    
    
    fprintf(out, m);
    sv_PrintAlgorithmID(out, &(info->digestEncAlg), "digestEncryptionAlgorithm=");
    fprintf(out, m);
    sv_PrintAsHex(out, &(info->encDigest), "encryptedDigest=");
    
    if (info->unAuthAttr != NULL) {
        char mm[120];

        iv = 0;
        while (info->unAuthAttr[iv] != NULL) iv++;
        fprintf(out, "%sunauthenticatedAttributes=%d\n", m, iv);
        iv = 0;
        while ((attr = info->unAuthAttr[iv]) != NULL) {
            sprintf(mm, "%sattribute[%d].", m, iv++); 
            sv_PrintAttribute(out, attr, mm);
        }
    }
}

void
sv_PrintRSAPublicKey(FILE *out, SECKEYPublicKey *pk, char *m)
{
    fprintf(out, m);
    sv_PrintInteger(out, &pk->u.rsa.modulus, "modulus=");
    fprintf(out, m);
    sv_PrintInteger(out, &pk->u.rsa.publicExponent, "exponent=");
}

void
sv_PrintDSAPublicKey(FILE *out, SECKEYPublicKey *pk, char *m)
{
    fprintf(out, m);
    sv_PrintInteger(out, &pk->u.dsa.params.prime, "prime=");
    fprintf(out, m);
    sv_PrintInteger(out, &pk->u.dsa.params.subPrime, "subprime=");
    fprintf(out, m);
    sv_PrintInteger(out, &pk->u.dsa.params.base, "base=");
    fprintf(out, m);
    sv_PrintInteger(out, &pk->u.dsa.publicValue, "publicValue=");
}

int
sv_PrintSubjectPublicKeyInfo(FILE *out, PRArenaPool *arena,
                             CERTSubjectPublicKeyInfo *i,  char *msg)
{
    SECKEYPublicKey *pk;
    int rv;
    char mm[200];

    sprintf(mm, "%s.publicKeyAlgorithm=", msg);
    sv_PrintAlgorithmID(out, &i->algorithm, mm);

    pk = (SECKEYPublicKey*) PORT_ZAlloc(sizeof(SECKEYPublicKey));
    if (!pk) return PORT_GetError();

    DER_ConvertBitString(&i->subjectPublicKey);
    switch(SECOID_FindOIDTag(&i->algorithm.algorithm)) {
        case SEC_OID_PKCS1_RSA_ENCRYPTION:
            rv = SEC_ASN1DecodeItem(arena, pk,
                                    SEC_ASN1_GET(SECKEY_RSAPublicKeyTemplate),
                                    &i->subjectPublicKey);
            if (rv) return rv;
            sprintf(mm, "%s.rsaPublicKey.", msg);
            sv_PrintRSAPublicKey(out, pk, mm);
            break;
        case SEC_OID_ANSIX9_DSA_SIGNATURE:
            rv = SEC_ASN1DecodeItem(arena, pk,
                                    SEC_ASN1_GET(SECKEY_DSAPublicKeyTemplate),
                                    &i->subjectPublicKey);
            if (rv) return rv;
            sprintf(mm, "%s.dsaPublicKey.", msg);
            sv_PrintDSAPublicKey(out, pk, mm);
            break;
        default:
            fprintf(out, "%s=bad SPKI algorithm type\n", msg);
            return 0;
    }

    return 0;
}

SECStatus
sv_PrintInvalidDateExten  (FILE *out, SECItem *value, char *msg)
{
    SECItem decodedValue;
    SECStatus rv;
    int64 invalidTime;
    char *formattedTime = NULL;

    decodedValue.data = NULL;
    rv = SEC_ASN1DecodeItem (NULL, &decodedValue,
                             SEC_ASN1_GET(SEC_GeneralizedTimeTemplate),
                             value);
    if (rv == SECSuccess) {
        rv = DER_GeneralizedTimeToTime(&invalidTime, &decodedValue);
        if (rv == SECSuccess) {
            formattedTime = CERT_GenTime2FormattedAscii(invalidTime, "%a %b %d %H:%M:%S %Y");
            fprintf (out, "%s: %s\n", msg, formattedTime);
            PORT_Free (formattedTime);
        }
    }
    PORT_Free (decodedValue.data);

    return (rv);
}

int
sv_PrintExtensions(FILE *out, CERTCertExtension **extensions, char *msg)
{
    SECOidTag oidTag;

    if (extensions) {

        while ( *extensions ) {
            SECItem *tmpitem;

            fprintf(out, "%sname=", msg);

            tmpitem = &(*extensions)->id;
            sv_PrintObjectID(out, tmpitem, NULL);

            tmpitem = &(*extensions)->critical;
            if ( tmpitem->len )
                fprintf(out, "%scritical=%s\n", msg,
                        (tmpitem->data && tmpitem->data[0])? "True": "False");

            oidTag = SECOID_FindOIDTag (&((*extensions)->id));

            fprintf(out, msg);
            tmpitem = &((*extensions)->value);
            if (oidTag == SEC_OID_X509_INVALID_DATE) 
                sv_PrintInvalidDateExten (out, tmpitem,"invalidExt");
            else			    
                sv_PrintAsHex(out,tmpitem, "data=");

            
            extensions++;
        }
    }

    return 0;
}





void
sv_PrintCRLInfo(FILE *out, CERTCrl *crl, char *m)
{
    CERTCrlEntry *entry;
    int iv;
    char om[100];
    
    fprintf(out, m);
    sv_PrintAlgorithmID(out, &(crl->signatureAlg), "signatureAlgorithm=");
    fprintf(out, m);
    sv_PrintName(out, &(crl->name), "name=");
    fprintf(out, m);
    sv_PrintTime(out, &(crl->lastUpdate), "lastUpdate=");
    fprintf(out, m);
    sv_PrintTime(out, &(crl->nextUpdate), "nextUpdate=");
    
    if (crl->entries != NULL) {
        iv = 0;
        while ((entry = crl->entries[iv]) != NULL) {
            fprintf(out, "%sentry[%d].", m, iv); 
            sv_PrintInteger(out, &(entry->serialNumber), "serialNumber=");
            fprintf(out, "%sentry[%d].", m, iv); 
            sv_PrintTime(out, &(entry->revocationDate), "revocationDate=");
            sprintf(om, "%sentry[%d].signedCRLEntriesExtensions.", m, iv++); 
            sv_PrintExtensions(out, entry->extensions, om);
        }
    }
    sprintf(om, "%ssignedCRLEntriesExtensions.", m); 
    sv_PrintExtensions(out, crl->extensions, om);
}


int
sv_PrintCertificate(FILE *out, SECItem *der, char *m, int level)
{
    PRArenaPool *arena = NULL;
    CERTCertificate *c;
    int rv;
    int iv;
    char mm[200];
    
    
    c = (CERTCertificate*) PORT_ZAlloc(sizeof(CERTCertificate));
    if (!c) return PORT_GetError();

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!arena) return SEC_ERROR_NO_MEMORY;

    rv = SEC_ASN1DecodeItem(arena, c, SEC_ASN1_GET(CERT_CertificateTemplate),
                            der);
    if (rv) {
        PORT_FreeArena(arena, PR_FALSE);
        return rv;
    }

    
    iv = DER_GetInteger(&c->version);
    fprintf(out, "%sversion=%d (0x%x)\n", m, iv + 1, iv);
    sprintf(mm, "%sserialNumber=", m);
    sv_PrintInteger(out, &c->serialNumber, mm);
    sprintf(mm, "%ssignatureAlgorithm=", m);
    sv_PrintAlgorithmID(out, &c->signature, mm);
    sprintf(mm, "%sissuerName=", m);
    sv_PrintName(out, &c->issuer, mm);
    sprintf(mm, "%svalidity.", m);
    sv_PrintValidity(out, &c->validity, mm);
    sprintf(mm, "%ssubject=", m);
    sv_PrintName(out, &c->subject, mm);
    sprintf(mm, "%ssubjectPublicKeyInfo", m);
    rv = sv_PrintSubjectPublicKeyInfo(out, arena, &c->subjectPublicKeyInfo, mm);
    if (rv) {
        PORT_FreeArena(arena, PR_FALSE);
        return rv;
    }
    sprintf(mm, "%ssignedExtensions.", m);
    sv_PrintExtensions(out, c->extensions, mm);
    
    PORT_FreeArena(arena, PR_FALSE);
    return 0;
}

int
sv_PrintSignedData(FILE *out, SECItem *der, char *m, SECU_PPFunc inner)
{
    PRArenaPool *arena = NULL;
    CERTSignedData *sd;
    int rv;

    
    sd = (CERTSignedData*) PORT_ZAlloc(sizeof(CERTSignedData));
    if (!sd) return PORT_GetError();

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!arena) return SEC_ERROR_NO_MEMORY;

    rv = SEC_ASN1DecodeItem(arena, sd, SEC_ASN1_GET(CERT_SignedDataTemplate),
                            der);
    if (rv) {
        PORT_FreeArena(arena, PR_FALSE);
        return rv;
    }


    PORT_Strcat(m, "data.");

    rv = (*inner)(out, &sd->data, m, 0);
    if (rv) {
        PORT_FreeArena(arena, PR_FALSE);
        return rv;
    }

    m[PORT_Strlen(m) - 5] = 0;
    fprintf(out, m);
    sv_PrintAlgorithmID(out, &sd->signatureAlgorithm, "signatureAlgorithm=");
    DER_ConvertBitString(&sd->signature);
    fprintf(out, m);
    sv_PrintAsHex(out, &sd->signature, "signature=");

    PORT_FreeArena(arena, PR_FALSE);
    return 0;

}






int
sv_PrintPKCS7Signed(FILE *out, SEC_PKCS7SignedData *src)
{
    SECAlgorithmID *digAlg;		
    SECItem *aCert;			
    CERTSignedCrl *aCrl;		
    SEC_PKCS7SignerInfo *sigInfo;	
    int rv, iv;
    char om[120];

    sv_PrintInteger(out, &(src->version), "pkcs7.version=");

    
    if (src->digestAlgorithms != NULL) {
        iv = 0;
        while (src->digestAlgorithms[iv] != NULL)
            iv++;
        fprintf(out, "pkcs7.digestAlgorithmListLength=%d\n", iv);
        iv = 0;
        while ((digAlg = src->digestAlgorithms[iv]) != NULL) {
            sprintf(om, "pkcs7.digestAlgorithm[%d]=", iv++);
            sv_PrintAlgorithmID(out, digAlg, om);
        }
    }

    
    rv = sv_PrintPKCS7ContentInfo(out, &(src->contentInfo), 
				                    "pkcs7.contentInformation=");
    if (rv != 0) return rv;

    
    if (src->rawCerts != NULL) {
        iv = 0;
        while (src->rawCerts[iv] != NULL)
            iv++;
        fprintf(out, "pkcs7.certificateListLength=%d\n", iv);

        iv = 0;
        while ((aCert = src->rawCerts[iv]) != NULL) {
            sprintf(om, "certificate[%d].", iv++);
            rv = sv_PrintSignedData(out, aCert, om, sv_PrintCertificate);
            if (rv) return rv;
        }
    }

    
    if (src->crls != NULL) {
        iv = 0;
        while (src->crls[iv] != NULL) iv++;
        fprintf(out, "pkcs7.signedRevocationLists=%d\n", iv);
        iv = 0;
        while ((aCrl = src->crls[iv]) != NULL) {
            sprintf(om, "signedRevocationList[%d].", iv);
            fprintf(out, om);
            sv_PrintAlgorithmID(out, &aCrl->signatureWrap.signatureAlgorithm, 
                                "signatureAlgorithm=");
            DER_ConvertBitString(&aCrl->signatureWrap.signature);
            fprintf(out, om);
            sv_PrintAsHex(out, &aCrl->signatureWrap.signature, "signature=");
            sprintf(om, "certificateRevocationList[%d].", iv);
            sv_PrintCRLInfo(out, &aCrl->crl, om);
            iv++;
        }
    }

    
    if (src->signerInfos != NULL) {
        iv = 0;
        while (src->signerInfos[iv] != NULL)
            iv++;
        fprintf(out, "pkcs7.signerInformationListLength=%d\n", iv);
        iv = 0;
        while ((sigInfo = src->signerInfos[iv]) != NULL) {
            sprintf(om, "signerInformation[%d].", iv++);
            sv_PrintSignerInfo(out, sigInfo, om);
        }
    }  

    return 0;
}

#if 0




void
secu_PrintPKCS7Enveloped(FILE *out, SEC_PKCS7EnvelopedData *src,
			 char *m, int level)
{
    SEC_PKCS7RecipientInfo *recInfo;   
    int iv;
    char om[100];

    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    sv_PrintInteger(out, &(src->version), "Version", level + 1);

    
    if (src->recipientInfos != NULL) {
	secu_Indent(out, level + 1);
	fprintf(out, "Recipient Information List:\n");
	iv = 0;
	while ((recInfo = src->recipientInfos[iv++]) != NULL) {
	    sprintf(om, "Recipient Information (%x)", iv);
	    secu_PrintRecipientInfo(out, recInfo, om, level + 2);
	}
    }  

    secu_PrintPKCS7EncContent(out, &src->encContentInfo, 
			      "Encrypted Content Information", level + 1);
}





int
secu_PrintPKCS7SignedAndEnveloped(FILE *out,
				  SEC_PKCS7SignedAndEnvelopedData *src,
				  char *m, int level)
{
    SECAlgorithmID *digAlg;  
    SECItem *aCert;           
    CERTSignedCrl *aCrl;        
    SEC_PKCS7SignerInfo *sigInfo;   
    SEC_PKCS7RecipientInfo *recInfo; 
    int rv, iv;
    char om[100];

    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    sv_PrintInteger(out, &(src->version), "Version", level + 1);

    
    if (src->recipientInfos != NULL) {
	secu_Indent(out, level + 1);
	fprintf(out, "Recipient Information List:\n");
	iv = 0;
	while ((recInfo = src->recipientInfos[iv++]) != NULL) {
	    sprintf(om, "Recipient Information (%x)", iv);
	    secu_PrintRecipientInfo(out, recInfo, om, level + 2);
	}
    }  

    
    if (src->digestAlgorithms != NULL) {
	secu_Indent(out, level + 1);  fprintf(out, "Digest Algorithm List:\n");
	iv = 0;
	while ((digAlg = src->digestAlgorithms[iv++]) != NULL) {
	    sprintf(om, "Digest Algorithm (%x)", iv);
	    sv_PrintAlgorithmID(out, digAlg, om);
	}
    }

    secu_PrintPKCS7EncContent(out, &src->encContentInfo, 
			      "Encrypted Content Information", level + 1);

    
    if (src->rawCerts != NULL) {
	secu_Indent(out, level + 1);  fprintf(out, "Certificate List:\n");
	iv = 0;
	while ((aCert = src->rawCerts[iv++]) != NULL) {
	    sprintf(om, "Certificate (%x)", iv);
	    rv = SECU_PrintSignedData(out, aCert, om, level + 2, 
				      SECU_PrintCertificate);
	    if (rv)
		return rv;
	}
    }

    
    if (src->crls != NULL) {
	secu_Indent(out, level + 1);  
	fprintf(out, "Signed Revocation Lists:\n");
	iv = 0;
	while ((aCrl = src->crls[iv++]) != NULL) {
	    sprintf(om, "Signed Revocation List (%x)", iv);
	    secu_Indent(out, level + 2);  fprintf(out, "%s:\n", om);
	    sv_PrintAlgorithmID(out, &aCrl->signatureWrap.signatureAlgorithm, 
				  "Signature Algorithm");
	    DER_ConvertBitString(&aCrl->signatureWrap.signature);
	    sv_PrintAsHex(out, &aCrl->signatureWrap.signature, "Signature",
			    level+3);
	    SECU_PrintCRLInfo(out, &aCrl->crl, "Certificate Revocation List", 
			  level + 3); 
	}
    }

    
    if (src->signerInfos != NULL) {
	secu_Indent(out, level + 1);
	fprintf(out, "Signer Information List:\n");
	iv = 0;
	while ((sigInfo = src->signerInfos[iv++]) != NULL) {
	    sprintf(om, "Signer Information (%x)", iv);
	    secu_PrintSignerInfo(out, sigInfo, om, level + 2);
	}
    }  

    return 0;
}





void
secu_PrintPKCS7Encrypted(FILE *out, SEC_PKCS7EncryptedData *src,
			 char *m, int level)
{
    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    sv_PrintInteger(out, &(src->version), "Version", level + 1);

    secu_PrintPKCS7EncContent(out, &src->encContentInfo, 
			      "Encrypted Content Information", level + 1);
}





void
sv_PrintPKCS7Digested(FILE *out, SEC_PKCS7DigestedData *src)
{
    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    sv_PrintInteger(out, &(src->version), "Version", level + 1);
    
    sv_PrintAlgorithmID(out, &src->digestAlg, "Digest Algorithm");
    sv_PrintPKCS7ContentInfo(out, &src->contentInfo, "Content Information",
			       level + 1);
    sv_PrintAsHex(out, &src->digest, "Digest", level + 1);  
}

#endif






int
sv_PrintPKCS7ContentInfo(FILE *out, SEC_PKCS7ContentInfo *src, char *m)
{
    const char *desc;
    SECOidTag kind;
    int rv;

    if (src->contentTypeTag == NULL)
        src->contentTypeTag = SECOID_FindOID(&(src->contentType));

    if (src->contentTypeTag == NULL) {
        desc = "Unknown";
        kind = SEC_OID_PKCS7_DATA;
    } else {
        desc = src->contentTypeTag->desc;
        kind = src->contentTypeTag->offset;
    }

    fprintf(out, "%s%s\n", m, desc);

    if (src->content.data == NULL) {
        fprintf(out, "pkcs7.data=<no content>\n");
        return 0;
    }

    rv = 0;
    switch (kind) {
        case SEC_OID_PKCS7_SIGNED_DATA:  
            rv = sv_PrintPKCS7Signed(out, src->content.signedData);
            break;

        case SEC_OID_PKCS7_ENVELOPED_DATA:  
            fprintf(out, "pkcs7EnvelopedData=<unsupported>\n");
            
            break;

        case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:  
            fprintf(out, "pkcs7SignedEnvelopedData=<unsupported>\n");
            

            break;

        case SEC_OID_PKCS7_DIGESTED_DATA:  
            fprintf(out, "pkcs7DigestedData=<unsupported>\n");
            
            break;

        case SEC_OID_PKCS7_ENCRYPTED_DATA:  
            fprintf(out, "pkcs7EncryptedData=<unsupported>\n");
            
            break;

        default:
            fprintf(out, "pkcs7UnknownData=<unsupported>\n");
            
            break;
    }

    return rv;
}


int
SV_PrintPKCS7ContentInfo(FILE *out, SECItem *der)
{
    SEC_PKCS7ContentInfo *cinfo;
    int rv = -1;

    cinfo = SEC_PKCS7DecodeItem(der, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    if (cinfo != NULL) {
        rv = sv_PrintPKCS7ContentInfo(out, cinfo, "pkcs7.contentInfo=");
        SEC_PKCS7DestroyContentInfo(cinfo);
    }

    return rv;
}



