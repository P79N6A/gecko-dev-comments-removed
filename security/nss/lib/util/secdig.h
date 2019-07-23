







































#ifndef _SECDIG_H_
#define _SECDIG_H_

#include "utilrename.h"
#include "secdigt.h"

#include "seccomon.h"
#include "secasn1t.h" 
#include "secdert.h"

SEC_BEGIN_PROTOS


extern const SEC_ASN1Template sgn_DigestInfoTemplate[];

SEC_ASN1_CHOOSER_DECLARE(sgn_DigestInfoTemplate)


















extern SGNDigestInfo *SGN_CreateDigestInfo(SECOidTag algorithm,
					   unsigned char *sig,
					   unsigned int sigLen);




extern void SGN_DestroyDigestInfo(SGNDigestInfo *info);














extern SECItem *SGN_EncodeDigestInfo(PLArenaPool *poolp, SECItem *dest,
				     SGNDigestInfo *diginfo);









extern SGNDigestInfo *SGN_DecodeDigestInfo(SECItem *didata);














extern SECStatus  SGN_CopyDigestInfo(PLArenaPool *poolp,
					SGNDigestInfo *a, 
					SGNDigestInfo *b);





extern SECComparison SGN_CompareDigestInfo(SGNDigestInfo *a, SGNDigestInfo *b);


SEC_END_PROTOS

#endif 
