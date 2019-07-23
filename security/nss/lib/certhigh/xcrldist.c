






































#include "genname.h"
#include "certt.h"
#include "secerr.h"

SEC_ASN1_MKSUB(SEC_AnyTemplate)
SEC_ASN1_MKSUB(SEC_BitStringTemplate)

extern void PrepareBitStringForEncoding (SECItem *bitMap, SECItem *value);

static const SEC_ASN1Template FullNameTemplate[] = {
    {SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_CONSTRUCTED | 0,
	offsetof (CRLDistributionPoint,derFullName), CERT_GeneralNamesTemplate}
};

static const SEC_ASN1Template RelativeNameTemplate[] = {
    {SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_CONSTRUCTED | 1, 
	offsetof (CRLDistributionPoint,distPoint.relativeName), CERT_RDNTemplate}
};
	 
static const SEC_ASN1Template CRLDistributionPointTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(CRLDistributionPoint) },
	{ SEC_ASN1_OPTIONAL | SEC_ASN1_CONTEXT_SPECIFIC |
	    SEC_ASN1_CONSTRUCTED | SEC_ASN1_EXPLICIT | SEC_ASN1_XTRN | 0,
	    offsetof(CRLDistributionPoint,derDistPoint),
            SEC_ASN1_SUB(SEC_AnyTemplate)},
	{ SEC_ASN1_OPTIONAL | SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 1,
	    offsetof(CRLDistributionPoint,bitsmap),
            SEC_ASN1_SUB(SEC_BitStringTemplate) },
	{ SEC_ASN1_OPTIONAL | SEC_ASN1_CONTEXT_SPECIFIC |
	    SEC_ASN1_CONSTRUCTED | 2,
	    offsetof(CRLDistributionPoint, derCrlIssuer), CERT_GeneralNamesTemplate},
    { 0 }
};

const SEC_ASN1Template CERTCRLDistributionPointsTemplate[] = {
    {SEC_ASN1_SEQUENCE_OF, 0, CRLDistributionPointTemplate}
};

SECStatus
CERT_EncodeCRLDistributionPoints (PRArenaPool *arena, CERTCrlDistributionPoints *value,
				  SECItem *derValue)
{
    CRLDistributionPoint **pointList, *point;
    PRArenaPool *ourPool = NULL;
    SECStatus rv = SECSuccess;

    PORT_Assert (derValue);
    PORT_Assert (value && value->distPoints);

    do {
	ourPool = PORT_NewArena (SEC_ASN1_DEFAULT_ARENA_SIZE);
	if (ourPool == NULL) {
	    rv = SECFailure;
	    break;
	}    
	
	pointList = value->distPoints;
	while (*pointList) {
	    point = *pointList;
	    point->derFullName = NULL;
	    point->derDistPoint.data = NULL;

	    if (point->distPointType == generalName) {
		point->derFullName = cert_EncodeGeneralNames
		    (ourPool, point->distPoint.fullName);
		
		if (point->derFullName) {
		    rv = (SEC_ASN1EncodeItem (ourPool, &point->derDistPoint,
			  point, FullNameTemplate) == NULL) ? SECFailure : SECSuccess;
		} else {
		    rv = SECFailure;
		}
	    }
	    else if (point->distPointType == relativeDistinguishedName) {
		if (SEC_ASN1EncodeItem
		     (ourPool, &point->derDistPoint, 
		      point, RelativeNameTemplate) == NULL) 
		    rv = SECFailure;
	    }
	    
	    else if (point->distPointType != 0) {
		PORT_SetError (SEC_ERROR_EXTENSION_VALUE_INVALID);
		rv = SECFailure;
	    }
	    if (rv != SECSuccess)
		break;

	    if (point->reasons.data)
		PrepareBitStringForEncoding (&point->bitsmap, &point->reasons);

	    if (point->crlIssuer) {
		point->derCrlIssuer = cert_EncodeGeneralNames
		    (ourPool, point->crlIssuer);
		if (!point->crlIssuer)
		    break;
	    }
	    
	    ++pointList;
	}
	if (rv != SECSuccess)
	    break;
	if (SEC_ASN1EncodeItem
	     (arena, derValue, value, CERTCRLDistributionPointsTemplate) == NULL) {
	    rv = SECFailure;
	    break;
	}
    } while (0);
    PORT_FreeArena (ourPool, PR_FALSE);
    return (rv);
}

CERTCrlDistributionPoints *
CERT_DecodeCRLDistributionPoints (PRArenaPool *arena, SECItem *encodedValue)
{
   CERTCrlDistributionPoints *value = NULL;    
   CRLDistributionPoint **pointList, *point;    
   SECStatus rv;
   SECItem newEncodedValue;

   PORT_Assert (arena);
   do {
	value = (CERTCrlDistributionPoints*)PORT_ArenaZAlloc (arena, sizeof (*value));
	if (value == NULL) {
	    rv = SECFailure;
	    break;
	}

        

        rv = SECITEM_CopyItem(arena, &newEncodedValue, encodedValue);
        if ( rv != SECSuccess ) {
	    break;
        }

	rv = SEC_QuickDERDecodeItem
	     (arena, &value->distPoints, CERTCRLDistributionPointsTemplate,
	      &newEncodedValue);
	if (rv != SECSuccess)
	    break;

	pointList = value->distPoints;
	while (*pointList) {
	    point = *pointList;

	    
	    if (point->derDistPoint.data != NULL) {
		point->distPointType = (DistributionPointTypes)
					((point->derDistPoint.data[0] & 0x1f) +1);
		if (point->distPointType == generalName) {
		    SECItem innerDER;
		
		    innerDER.data = NULL;
		    rv = SEC_QuickDERDecodeItem
			 (arena, point, FullNameTemplate, &(point->derDistPoint));
		    if (rv != SECSuccess)
			break;
		    point->distPoint.fullName = cert_DecodeGeneralNames
			(arena, point->derFullName);

		    if (!point->distPoint.fullName)
			break;
		}
		else if ( relativeDistinguishedName) {
		    rv = SEC_QuickDERDecodeItem
			 (arena, point, RelativeNameTemplate, &(point->derDistPoint));
		    if (rv != SECSuccess)
			break;
		}
		else {
		    PORT_SetError (SEC_ERROR_EXTENSION_VALUE_INVALID);
		    break;
		}
	    }

	    
	    if (point->bitsmap.data != NULL) {
		point->reasons.data = (unsigned char*) PORT_ArenaAlloc
				      (arena, (point->bitsmap.len + 7) >> 3);
		if (!point->reasons.data) {
		    rv = SECFailure;
		    break;
		}
		PORT_Memcpy (point->reasons.data, point->bitsmap.data,
			     point->reasons.len = ((point->bitsmap.len + 7) >> 3));
	    }

	    
	    if (point->derCrlIssuer != NULL) {
		point->crlIssuer = cert_DecodeGeneralNames
		    (arena, point->derCrlIssuer);

		if (!point->crlIssuer)
		    break;
	    }
	    ++pointList;
	}
   } while (0);
   return (rv == SECSuccess ? value : NULL);
}
