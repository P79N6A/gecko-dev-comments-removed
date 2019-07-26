



#include "cert.h"
#include "base64.h"
#include "secitem.h"
#include "secder.h"
#include "secasn1.h"
#include "secoid.h"
#include "secerr.h"

SEC_ASN1_MKSUB(SEC_AnyTemplate)
SEC_ASN1_MKSUB(SEC_SetOfAnyTemplate)

typedef struct ContentInfoStr ContentInfo;
typedef struct DegenerateSignedDataStr DegenerateSignedData;

struct ContentInfoStr {
    SECOidTag contentTypeTag;   
    SECItem contentType;
    union {
        SECItem *data;
        DegenerateSignedData *signedData;
    } content;
};

struct DegenerateSignedDataStr {
    SECItem version;
    SECItem **digestAlgorithms;
    ContentInfo contentInfo;
    SECItem **certificates;
    SECItem **crls;
    SECItem **signerInfos;
};

static const SEC_ASN1Template *
choose_content_template(void *src_or_dest, PRBool encoding);

static const SEC_ASN1TemplateChooserPtr template_chooser
        = choose_content_template;

static const SEC_ASN1Template ContentInfoTemplate[] = {
    { SEC_ASN1_SEQUENCE,
          0, NULL, sizeof(ContentInfo) },
    { SEC_ASN1_OBJECT_ID,
          offsetof(ContentInfo,contentType) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_DYNAMIC |
      SEC_ASN1_EXPLICIT | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC | 0,
          offsetof(ContentInfo,content),
          &template_chooser },
    { 0 }
};

static const SEC_ASN1Template DegenerateSignedDataTemplate[] = {
    { SEC_ASN1_SEQUENCE,
          0, NULL, sizeof(DegenerateSignedData) },
    { SEC_ASN1_INTEGER,
          offsetof(DegenerateSignedData,version) },
    { SEC_ASN1_SET_OF | SEC_ASN1_XTRN,
          offsetof(DegenerateSignedData,digestAlgorithms),
          SEC_ASN1_SUB(SEC_AnyTemplate) },
    { SEC_ASN1_INLINE,
          offsetof(DegenerateSignedData,contentInfo),
          ContentInfoTemplate },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
      SEC_ASN1_XTRN | 0,
          offsetof(DegenerateSignedData,certificates),
          SEC_ASN1_SUB(SEC_SetOfAnyTemplate) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC |
      SEC_ASN1_XTRN | 1,
          offsetof(DegenerateSignedData,crls),
          SEC_ASN1_SUB(SEC_SetOfAnyTemplate) },
    { SEC_ASN1_SET_OF | SEC_ASN1_XTRN,
          offsetof(DegenerateSignedData,signerInfos),
          SEC_ASN1_SUB(SEC_AnyTemplate) },
    { 0 }
};

static const SEC_ASN1Template PointerToDegenerateSignedDataTemplate[] = {
    { SEC_ASN1_POINTER, 0, DegenerateSignedDataTemplate }
};

static SECOidTag
GetContentTypeTag(ContentInfo *cinfo)
{
    if (cinfo->contentTypeTag == SEC_OID_UNKNOWN)
        cinfo->contentTypeTag = SECOID_FindOIDTag(&cinfo->contentType);
    return cinfo->contentTypeTag;
}

static const SEC_ASN1Template *
choose_content_template(void *src_or_dest, PRBool encoding)
{
    const SEC_ASN1Template *theTemplate;
    ContentInfo *cinfo;
    SECOidTag kind;

    PORT_Assert(src_or_dest != NULL);
    if (src_or_dest == NULL)
        return NULL;

    cinfo = (ContentInfo*)src_or_dest;
    kind = GetContentTypeTag(cinfo);
    switch (kind) {
      default:
        theTemplate = SEC_ASN1_GET(SEC_PointerToAnyTemplate);
        break;
      case SEC_OID_PKCS7_DATA:
        theTemplate = SEC_ASN1_GET(SEC_PointerToOctetStringTemplate);
        break;
      case SEC_OID_PKCS7_SIGNED_DATA:
        theTemplate = PointerToDegenerateSignedDataTemplate;
        break;
    }
    return theTemplate;
}

static SECStatus
SEC_ReadPKCS7Certs(SECItem *pkcs7Item, CERTImportCertificateFunc f, void *arg)
{
    ContentInfo contentInfo;
    SECStatus rv;
    SECItem **certs;
    int count;
    PLArenaPool *arena;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	return SECFailure;
    }

    PORT_Memset(&contentInfo, 0, sizeof(contentInfo));
    rv = SEC_ASN1DecodeItem(arena, &contentInfo, ContentInfoTemplate,
			    pkcs7Item);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    if ( GetContentTypeTag(&contentInfo) != SEC_OID_PKCS7_SIGNED_DATA ) {
	goto loser;
    }

    certs = contentInfo.content.signedData->certificates;
    if ( certs ) {
	count = 0;
	
	while ( *certs ) {
	    count++;
	    certs++;
	}
	rv = (* f)(arg, contentInfo.content.signedData->certificates, count);
    }
    
    rv = SECSuccess;
    
    goto done;
loser:
    rv = SECFailure;
    
done:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }

    return(rv);
}

const SEC_ASN1Template SEC_CertSequenceTemplate[] = {
    { SEC_ASN1_SEQUENCE_OF | SEC_ASN1_XTRN, 0, SEC_ASN1_SUB(SEC_AnyTemplate) }
};

static SECStatus
SEC_ReadCertSequence(SECItem *certsItem, CERTImportCertificateFunc f, void *arg)
{
    SECStatus rv;
    SECItem **certs;
    int count;
    SECItem **rawCerts = NULL;
    PLArenaPool *arena;
    ContentInfo contentInfo;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( arena == NULL ) {
	return SECFailure;
    }

    PORT_Memset(&contentInfo, 0, sizeof(contentInfo));
    rv = SEC_ASN1DecodeItem(arena, &contentInfo, ContentInfoTemplate,
			    certsItem);
    if ( rv != SECSuccess ) {
	goto loser;
    }

    if ( GetContentTypeTag(&contentInfo) != SEC_OID_NS_TYPE_CERT_SEQUENCE ) {
	goto loser;
    }

    rv = SEC_QuickDERDecodeItem(arena, &rawCerts, SEC_CertSequenceTemplate,
		    contentInfo.content.data);

    if (rv != SECSuccess) {
	goto loser;
    }

    certs = rawCerts;
    if ( certs ) {
	count = 0;
	
	while ( *certs ) {
	    count++;
	    certs++;
	}
	rv = (* f)(arg, rawCerts, count);
    }
    
    rv = SECSuccess;
    
    goto done;
loser:
    rv = SECFailure;
    
done:
    if ( arena ) {
	PORT_FreeArena(arena, PR_FALSE);
    }
    
    return(rv);
}

CERTCertificate *
CERT_ConvertAndDecodeCertificate(char *certstr)
{
    CERTCertificate *cert;
    SECStatus rv;
    SECItem der;

    rv = ATOB_ConvertAsciiToItem(&der, certstr);
    if (rv != SECSuccess)
	return NULL;

    cert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(), 
                                   &der, NULL, PR_FALSE, PR_TRUE);

    PORT_Free(der.data);
    return cert;
}

static const char NS_CERT_HEADER[]  = "-----BEGIN CERTIFICATE-----";
static const char NS_CERT_TRAILER[] = "-----END CERTIFICATE-----";
#define NS_CERT_HEADER_LEN  ((sizeof NS_CERT_HEADER) - 1)
#define NS_CERT_TRAILER_LEN ((sizeof NS_CERT_TRAILER) - 1)




SECStatus
CERT_DecodeCertPackage(char *certbuf,
		       int certlen,
		       CERTImportCertificateFunc f,
		       void *arg)
{
    unsigned char *cp;
    unsigned char *bincert = NULL;
    char *         ascCert = NULL;
    SECStatus      rv;
    
    if ( certbuf == NULL ) {
	PORT_SetError(SEC_ERROR_INVALID_ARGS);
	return(SECFailure);
    }
    











    if (certlen < 17) {
	PORT_SetError(SEC_ERROR_INPUT_LEN);
	return(SECFailure);
    }
    
    cp = (unsigned char *)certbuf;

    
    if ( ( *cp  & 0x1f ) == SEC_ASN1_SEQUENCE ) {
	SECItem certitem;
	SECItem *pcertitem = &certitem;
	int seqLen, seqLenLen;

	cp++;
	
	if ( *cp & 0x80) {
	    
	    seqLenLen = cp[0] & 0x7f;
	    
	    switch (seqLenLen) {
	      case 4:
		seqLen = ((unsigned long)cp[1]<<24) |
		    ((unsigned long)cp[2]<<16) | (cp[3]<<8) | cp[4];
		break;
	      case 3:
		seqLen = ((unsigned long)cp[1]<<16) | (cp[2]<<8) | cp[3];
		break;
	      case 2:
		seqLen = (cp[1]<<8) | cp[2];
		break;
	      case 1:
		seqLen = cp[1];
		break;
	      case 0:
		
		seqLen = 0;
		break;
	      default:
		goto notder;
	    }
	    cp += ( seqLenLen + 1 );

	} else {
	    seqLenLen = 0;
	    seqLen = *cp;
	    cp++;
	}

	
	if ( seqLen || seqLenLen ) {
	    if ( certlen != ( seqLen + seqLenLen + 2 ) ) {
		if (certlen > ( seqLen + seqLenLen + 2 ))
		    PORT_SetError(SEC_ERROR_EXTRA_INPUT);
		else 
		    PORT_SetError(SEC_ERROR_INPUT_LEN);
		goto notder;
	    }
	}
	
	
	if ( cp[0] == SEC_ASN1_OBJECT_ID ) {
	    SECOidData *oiddata;
	    SECItem oiditem;
	    
	    oiditem.len = cp[1];
	    



	    if ( oiditem.len > 9 ) {
		PORT_SetError(SEC_ERROR_UNRECOGNIZED_OID);
		return(SECFailure);
	    }
	    oiditem.data = (unsigned char *)&cp[2];
	    oiddata = SECOID_FindOID(&oiditem);
	    if ( oiddata == NULL ) {
		return(SECFailure);
	    }

	    certitem.data = (unsigned char*)certbuf;
	    certitem.len = certlen;
	    
	    switch ( oiddata->offset ) {
	      case SEC_OID_PKCS7_SIGNED_DATA:
		
		return(SEC_ReadPKCS7Certs(&certitem, f, arg));
		break;
	      case SEC_OID_NS_TYPE_CERT_SEQUENCE:
		
		return(SEC_ReadCertSequence(&certitem, f, arg));
		break;
	      default:
		break;
	    }
	    
	} else {
	    
	    certitem.data = (unsigned char*)certbuf;
	    certitem.len = certlen;
	    
	    rv = (* f)(arg, &pcertitem, 1);
	    return(rv);
	}
    }

    
notder:
  {
    unsigned char *certbegin = NULL; 
    unsigned char *certend   = NULL;
    char          *pc;
    int cl;

    
    ascCert = (char *)PORT_Alloc(certlen + 1);
    if (!ascCert) {
        rv = SECFailure;
	goto loser;
    }

    PORT_Memcpy(ascCert, certbuf, certlen);
    ascCert[certlen] = '\0';

    pc = PORT_Strchr(ascCert, '\n');  
    if (!pc) { 
	pc = ascCert;
	while (*pc && NULL != (pc = PORT_Strchr(pc, '\r'))) {
	    *pc++ = '\n';
	}
    }

    cp = (unsigned char *)ascCert;
    cl = certlen;

    
    while ( cl > NS_CERT_HEADER_LEN ) {
	int found = 0;
	if ( !PORT_Strncasecmp((char *)cp, NS_CERT_HEADER,
			        NS_CERT_HEADER_LEN) ) {
	    cl -= NS_CERT_HEADER_LEN;
	    cp += NS_CERT_HEADER_LEN;
	    found = 1;
	}
	
	
	while ( cl && ( *cp != '\n' )) {
	    cp++;
	    cl--;
	} 

	
	while ( cl && ( *cp == '\n' || *cp == '\r' )) {
	    cp++;
	    cl--;
	}
	if (cl && found) {
	    certbegin = cp;
	    break;
    	}
    }

    if ( certbegin ) {
	
	while ( cl >= NS_CERT_TRAILER_LEN ) {
	    if ( !PORT_Strncasecmp((char *)cp, NS_CERT_TRAILER,
				   NS_CERT_TRAILER_LEN) ) {
		certend = cp;
		break;
	    }

	    
	    while ( cl && ( *cp != '\n' )) {
		cp++;
		cl--;
	    }

	    
	    while ( cl && ( *cp == '\n' || *cp == '\r' )) {
		cp++;
		cl--;
	    }
	}
    }

    if ( certbegin && certend ) {
	unsigned int binLen;

	*certend = 0;
	
	bincert = ATOB_AsciiToData((char *)certbegin, &binLen);
	if (!bincert) {
	    rv = SECFailure;
	    goto loser;
	}

	
	rv = CERT_DecodeCertPackage((char *)bincert, binLen, f, arg);
	
    } else {
	PORT_SetError(SEC_ERROR_BAD_DER);
	rv = SECFailure;
    }
  }

loser:

    if ( bincert ) {
	PORT_Free(bincert);
    }

    if ( ascCert ) {
	PORT_Free(ascCert);
    }

    return(rv);
}

typedef struct {
    PLArenaPool *arena;
    SECItem cert;
} collect_args;

static SECStatus
collect_certs(void *arg, SECItem **certs, int numcerts)
{
    SECStatus rv;
    collect_args *collectArgs;
    
    collectArgs = (collect_args *)arg;
    
    rv = SECITEM_CopyItem(collectArgs->arena, &collectArgs->cert, *certs);

    return(rv);
}





CERTCertificate *
CERT_DecodeCertFromPackage(char *certbuf, int certlen)
{
    collect_args collectArgs;
    SECStatus rv;
    CERTCertificate *cert = NULL;
    
    collectArgs.arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    
    rv = CERT_DecodeCertPackage(certbuf, certlen, collect_certs,
				(void *)&collectArgs);
    if ( rv == SECSuccess ) {
	cert = CERT_NewTempCertificate(CERT_GetDefaultCertDB(),
	                               &collectArgs.cert, NULL, 
	                               PR_FALSE, PR_TRUE);
    }
    
    PORT_FreeArena(collectArgs.arena, PR_FALSE);
    
    return(cert);
}
