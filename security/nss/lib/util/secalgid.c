



































#include "secoid.h"
#include "secder.h"	
#include "secasn1.h"
#include "secitem.h"
#include "secerr.h"

SECOidTag
SECOID_GetAlgorithmTag(SECAlgorithmID *id)
{
    if (id == NULL || id->algorithm.data == NULL)
	return SEC_OID_UNKNOWN;

    return SECOID_FindOIDTag (&(id->algorithm));
}

SECStatus
SECOID_SetAlgorithmID(PRArenaPool *arena, SECAlgorithmID *id, SECOidTag which,
		      SECItem *params)
{
    SECOidData *oiddata;
    PRBool add_null_param;

    oiddata = SECOID_FindOIDByTag(which);
    if ( !oiddata ) {
	PORT_SetError(SEC_ERROR_INVALID_ALGORITHM);
	return SECFailure;
    }

    if (SECITEM_CopyItem(arena, &id->algorithm, &oiddata->oid))
	return SECFailure;

    switch (which) {
      case SEC_OID_MD2:
      case SEC_OID_MD4:
      case SEC_OID_MD5:
      case SEC_OID_SHA1:
      case SEC_OID_SHA224:
      case SEC_OID_SHA256:
      case SEC_OID_SHA384:
      case SEC_OID_SHA512:
      case SEC_OID_PKCS1_RSA_ENCRYPTION:
      case SEC_OID_PKCS1_MD2_WITH_RSA_ENCRYPTION:
      case SEC_OID_PKCS1_MD4_WITH_RSA_ENCRYPTION:
      case SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION:
      case SEC_OID_PKCS1_SHA1_WITH_RSA_ENCRYPTION:
      case SEC_OID_PKCS1_SHA224_WITH_RSA_ENCRYPTION:
      case SEC_OID_PKCS1_SHA256_WITH_RSA_ENCRYPTION:
      case SEC_OID_PKCS1_SHA384_WITH_RSA_ENCRYPTION:
      case SEC_OID_PKCS1_SHA512_WITH_RSA_ENCRYPTION:
	add_null_param = PR_TRUE;
	break;
      default:
	add_null_param = PR_FALSE;
	break;
    }

    if (params) {
	







	PORT_Assert(!add_null_param || (params->len == 2
					&& params->data[0] == SEC_ASN1_NULL
					&& params->data[1] == 0)); 
	if (SECITEM_CopyItem(arena, &id->parameters, params)) {
	    return SECFailure;
	}
    } else {
	






	PORT_Assert(id->parameters.data == NULL);

	if (add_null_param) {
	    (void) SECITEM_AllocItem(arena, &id->parameters, 2);
	    if (id->parameters.data == NULL) {
		return SECFailure;
	    }
	    id->parameters.data[0] = SEC_ASN1_NULL;
	    id->parameters.data[1] = 0;
	}
    }

    return SECSuccess;
}

SECStatus
SECOID_CopyAlgorithmID(PRArenaPool *arena, SECAlgorithmID *to, SECAlgorithmID *from)
{
    SECStatus rv;

    rv = SECITEM_CopyItem(arena, &to->algorithm, &from->algorithm);
    if (rv) return rv;
    rv = SECITEM_CopyItem(arena, &to->parameters, &from->parameters);
    return rv;
}

void SECOID_DestroyAlgorithmID(SECAlgorithmID *algid, PRBool freeit)
{
    SECITEM_FreeItem(&algid->parameters, PR_FALSE);
    SECITEM_FreeItem(&algid->algorithm, PR_FALSE);
    if(freeit == PR_TRUE)
        PORT_Free(algid);
}

SECComparison
SECOID_CompareAlgorithmID(SECAlgorithmID *a, SECAlgorithmID *b)
{
    SECComparison rv;

    rv = SECITEM_CompareItem(&a->algorithm, &b->algorithm);
    if (rv) return rv;
    rv = SECITEM_CompareItem(&a->parameters, &b->parameters);
    return rv;
}
