







































#include "prtypes.h"
#include <limits.h>     
#include "seccomon.h"
#include "secdert.h"
#include "secoidt.h"
#include "secasn1t.h"
#include "secasn1.h"
#include "certt.h"
#include "secder.h"
#include "prprf.h"
#include "secerr.h"

typedef struct EncodedContext{
    SECItem isCA;
    SECItem pathLenConstraint;
    SECItem encodedValue;
    PRArenaPool *arena;
}EncodedContext;

static const SEC_ASN1Template CERTBasicConstraintsTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(EncodedContext) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_BOOLEAN,		
	  offsetof(EncodedContext,isCA)},
    { SEC_ASN1_OPTIONAL | SEC_ASN1_INTEGER,
	  offsetof(EncodedContext,pathLenConstraint) },
    { 0, }
};

static unsigned char hexTrue = 0xff;
static unsigned char hexFalse = 0x00;

#define GEN_BREAK(status) rv = status; break;

SECStatus CERT_EncodeBasicConstraintValue
   (PRArenaPool *arena, CERTBasicConstraints *value, SECItem *encodedValue)
{
    EncodedContext encodeContext;
    PRArenaPool *our_pool = NULL;   
    SECStatus rv = SECSuccess;

    do {
	PORT_Memset (&encodeContext, 0, sizeof (encodeContext));
	if (!value->isCA && value->pathLenConstraint >= 0) {
	    PORT_SetError (SEC_ERROR_EXTENSION_VALUE_INVALID);
	    GEN_BREAK (SECFailure);
	}

        encodeContext.arena = arena;
	if (value->isCA == PR_TRUE) {
	    encodeContext.isCA.data =  &hexTrue ;
	    encodeContext.isCA.len = 1;
	}

	


	if (value->isCA && value->pathLenConstraint >= 0) {
	    our_pool = PORT_NewArena (SEC_ASN1_DEFAULT_ARENA_SIZE);
	    if (our_pool == NULL) {
		PORT_SetError (SEC_ERROR_NO_MEMORY);
		GEN_BREAK (SECFailure);
	    }
	    if (SEC_ASN1EncodeUnsignedInteger
		(our_pool, &encodeContext.pathLenConstraint,
		 (unsigned long)value->pathLenConstraint) == NULL) {
		PORT_SetError (SEC_ERROR_NO_MEMORY);
		GEN_BREAK (SECFailure);
	    }
	}
	if (SEC_ASN1EncodeItem (arena, encodedValue, &encodeContext,
				CERTBasicConstraintsTemplate) == NULL) {
	    GEN_BREAK (SECFailure);
	}
    } while (0);
    if (our_pool)
	PORT_FreeArena (our_pool, PR_FALSE);
    return(rv);

}

SECStatus CERT_DecodeBasicConstraintValue
   (CERTBasicConstraints *value, SECItem *encodedValue)
{
    EncodedContext decodeContext;
    PRArenaPool *our_pool;
    SECStatus rv = SECSuccess;

    do {
	PORT_Memset (&decodeContext, 0, sizeof (decodeContext));
	


	decodeContext.isCA.data =&hexFalse;
	decodeContext.isCA.len = 1;
	
	our_pool = PORT_NewArena (SEC_ASN1_DEFAULT_ARENA_SIZE);
	if (our_pool == NULL) {
	    PORT_SetError (SEC_ERROR_NO_MEMORY);
	    GEN_BREAK (SECFailure);
	}

        rv = SEC_QuickDERDecodeItem
	     (our_pool, &decodeContext, CERTBasicConstraintsTemplate, encodedValue);
	if (rv == SECFailure)
	    break;
	
	value->isCA = decodeContext.isCA.data 
	              ? (PRBool)(decodeContext.isCA.data[0] != 0)
		      : PR_FALSE;
	if (decodeContext.pathLenConstraint.data == NULL) {
	    



	    if (value->isCA)
		value->pathLenConstraint = CERT_UNLIMITED_PATH_CONSTRAINT;
	} else if (value->isCA) {
	    long len = DER_GetInteger (&decodeContext.pathLenConstraint);
	    if (len < 0 || len == LONG_MAX) {
		PORT_SetError (SEC_ERROR_BAD_DER);
		GEN_BREAK (SECFailure);
	    }
	    value->pathLenConstraint = len;
	} else {
	    

	    PORT_SetError (SEC_ERROR_BAD_DER);
	    GEN_BREAK (SECFailure);
	    break;
	}
	 
    } while (0);
    PORT_FreeArena (our_pool, PR_FALSE);
    return (rv);

}
