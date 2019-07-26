













#include "utilrename.h"
#include "secasn1.h"
#include "secder.h"
#include "secoid.h"
#include "secdig.h"

const SEC_ASN1Template SECOID_AlgorithmIDTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(SECAlgorithmID) },
    { SEC_ASN1_OBJECT_ID,
	  offsetof(SECAlgorithmID,algorithm), },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_ANY,
	  offsetof(SECAlgorithmID,parameters), },
    { 0, }
};

SEC_ASN1_CHOOSER_IMPLEMENT(SECOID_AlgorithmIDTemplate)

const SEC_ASN1Template SEC_AnyTemplate[] = {
    { SEC_ASN1_ANY | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem) }
};

SEC_ASN1_CHOOSER_IMPLEMENT(SEC_AnyTemplate)

const SEC_ASN1Template SEC_BMPStringTemplate[] = {
    { SEC_ASN1_BMP_STRING | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem) }
};

SEC_ASN1_CHOOSER_IMPLEMENT(SEC_BMPStringTemplate)

const SEC_ASN1Template SEC_BitStringTemplate[] = {
    { SEC_ASN1_BIT_STRING | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem) }
};

SEC_ASN1_CHOOSER_IMPLEMENT(SEC_BitStringTemplate)

const SEC_ASN1Template SEC_BooleanTemplate[] = {
    { SEC_ASN1_BOOLEAN, 0, NULL, sizeof(SECItem) }
};

SEC_ASN1_CHOOSER_IMPLEMENT(SEC_BooleanTemplate)

const SEC_ASN1Template SEC_GeneralizedTimeTemplate[] = {
    { SEC_ASN1_GENERALIZED_TIME | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem)}
};

SEC_ASN1_CHOOSER_IMPLEMENT(SEC_GeneralizedTimeTemplate)

const SEC_ASN1Template SEC_IA5StringTemplate[] = {
    { SEC_ASN1_IA5_STRING | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem) }
};

SEC_ASN1_CHOOSER_IMPLEMENT(SEC_IA5StringTemplate)

const SEC_ASN1Template SEC_IntegerTemplate[] = {
    { SEC_ASN1_INTEGER, 0, NULL, sizeof(SECItem) }
};

SEC_ASN1_CHOOSER_IMPLEMENT(SEC_IntegerTemplate)

const SEC_ASN1Template SEC_NullTemplate[] = {
    { SEC_ASN1_NULL, 0, NULL, sizeof(SECItem) }
};

SEC_ASN1_CHOOSER_IMPLEMENT(SEC_NullTemplate)

const SEC_ASN1Template SEC_ObjectIDTemplate[] = {
    { SEC_ASN1_OBJECT_ID, 0, NULL, sizeof(SECItem) }
};

SEC_ASN1_CHOOSER_IMPLEMENT(SEC_ObjectIDTemplate)

const SEC_ASN1Template SEC_OctetStringTemplate[] = {
    { SEC_ASN1_OCTET_STRING | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem) }
};

SEC_ASN1_CHOOSER_IMPLEMENT(SEC_OctetStringTemplate)

const SEC_ASN1Template SEC_PointerToAnyTemplate[] = {
    { SEC_ASN1_POINTER, 0, SEC_AnyTemplate }
};

SEC_ASN1_CHOOSER_IMPLEMENT(SEC_PointerToAnyTemplate)

const SEC_ASN1Template SEC_PointerToOctetStringTemplate[] = {
    { SEC_ASN1_POINTER | SEC_ASN1_MAY_STREAM, 0, SEC_OctetStringTemplate }
};

SEC_ASN1_CHOOSER_IMPLEMENT(SEC_PointerToOctetStringTemplate)

const SEC_ASN1Template SEC_SetOfAnyTemplate[] = {
    { SEC_ASN1_SET_OF, 0, SEC_AnyTemplate }
};

SEC_ASN1_CHOOSER_IMPLEMENT(SEC_SetOfAnyTemplate)

const SEC_ASN1Template SEC_UTCTimeTemplate[] = {
    { SEC_ASN1_UTC_TIME | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem) }
};

SEC_ASN1_CHOOSER_IMPLEMENT(SEC_UTCTimeTemplate)

const SEC_ASN1Template SEC_UTF8StringTemplate[] = {
    { SEC_ASN1_UTF8_STRING | SEC_ASN1_MAY_STREAM, 0, NULL, sizeof(SECItem)}
};

SEC_ASN1_CHOOSER_IMPLEMENT(SEC_UTF8StringTemplate)



const SEC_ASN1Template sgn_DigestInfoTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(SGNDigestInfo) },
    { SEC_ASN1_INLINE,
	  offsetof(SGNDigestInfo,digestAlgorithm),
	  SECOID_AlgorithmIDTemplate },
    { SEC_ASN1_OCTET_STRING,
	  offsetof(SGNDigestInfo,digest) },
    { 0 }
};

SEC_ASN1_CHOOSER_IMPLEMENT(sgn_DigestInfoTemplate)
