








































#include "prtypes.h"
#include "mcom_db.h"
#include "seccomon.h"
#include "secdert.h"
#include "secoidt.h"
#include "secasn1t.h"
#include "secasn1.h"
#include "secport.h"
#include "certt.h"  
#include "genname.h"
#include "secerr.h"

SEC_ASN1_MKSUB(SEC_IntegerTemplate)
SEC_ASN1_MKSUB(SEC_OctetStringTemplate)

const SEC_ASN1Template CERTAuthKeyIDTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(CERTAuthKeyID) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 0,
	  offsetof(CERTAuthKeyID,keyID), SEC_ASN1_SUB(SEC_OctetStringTemplate)},
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONSTRUCTED | SEC_ASN1_CONTEXT_SPECIFIC  | 1,
          offsetof(CERTAuthKeyID, DERAuthCertIssuer), CERT_GeneralNamesTemplate},
    { SEC_ASN1_OPTIONAL | SEC_ASN1_CONTEXT_SPECIFIC | SEC_ASN1_XTRN | 2,
	  offsetof(CERTAuthKeyID,authCertSerialNumber),
          SEC_ASN1_SUB(SEC_IntegerTemplate) },
    { 0 }
};



SECStatus CERT_EncodeAuthKeyID (PRArenaPool *arena, CERTAuthKeyID *value, SECItem *encodedValue)
{
    SECStatus rv = SECFailure;
 
    PORT_Assert (value);
    PORT_Assert (arena);
    PORT_Assert (value->DERAuthCertIssuer == NULL);
    PORT_Assert (encodedValue);

    do {
	
	



	if (value->authCertIssuer) {
	    if (!value->authCertSerialNumber.data) {
		PORT_SetError (SEC_ERROR_EXTENSION_VALUE_INVALID);
		break;
	    }

	    value->DERAuthCertIssuer = cert_EncodeGeneralNames
		(arena, value->authCertIssuer);
	    if (!value->DERAuthCertIssuer) {
		PORT_SetError (SEC_ERROR_EXTENSION_VALUE_INVALID);
		break;
	    }
	}
	else if (value->authCertSerialNumber.data) {
		PORT_SetError (SEC_ERROR_EXTENSION_VALUE_INVALID);
		break;
	}

	if (SEC_ASN1EncodeItem (arena, encodedValue, value,
				CERTAuthKeyIDTemplate) == NULL)
	    break;
	rv = SECSuccess;

    } while (0);
     return(rv);
}

CERTAuthKeyID *
CERT_DecodeAuthKeyID (PRArenaPool *arena, SECItem *encodedValue)
{
    CERTAuthKeyID * value = NULL;
    SECStatus       rv    = SECFailure;
    void *          mark;
    SECItem         newEncodedValue;

    PORT_Assert (arena);
   
    do {
	mark = PORT_ArenaMark (arena);
	value = (CERTAuthKeyID*)PORT_ArenaZAlloc (arena, sizeof (*value));
	if (value == NULL)
	    break;
	value->DERAuthCertIssuer = NULL;
        

        rv = SECITEM_CopyItem(arena, &newEncodedValue, encodedValue);
        if ( rv != SECSuccess ) {
	    break;
        }

        rv = SEC_QuickDERDecodeItem
	     (arena, value, CERTAuthKeyIDTemplate, &newEncodedValue);
	if (rv != SECSuccess)
	    break;

        value->authCertIssuer = cert_DecodeGeneralNames (arena, value->DERAuthCertIssuer);
	if (value->authCertIssuer == NULL)
	    break;
	
	


	if ((value->authCertSerialNumber.data && !value->authCertIssuer) ||
	    (!value->authCertSerialNumber.data && value->authCertIssuer)){
	    PORT_SetError (SEC_ERROR_EXTENSION_VALUE_INVALID);
	    break;
	}
    } while (0);

    if (rv != SECSuccess) {
	PORT_ArenaRelease (arena, mark);
	return ((CERTAuthKeyID *)NULL);	    
    } 
    PORT_ArenaUnmark(arena, mark);
    return (value);
}
