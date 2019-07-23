#ifndef _HASH_H_
#define _HASH_H_





































#include "seccomon.h"
#include "hasht.h"
#include "secoidt.h"

SEC_BEGIN_PROTOS





extern unsigned int  HASH_ResultLen(HASH_HashType type);

extern unsigned int  HASH_ResultLenContext(HASHContext *context);

extern unsigned int  HASH_ResultLenByOidTag(SECOidTag hashOid);

extern SECStatus     HASH_HashBuf(HASH_HashType type,
				 unsigned char *dest,
				 unsigned char *src,
				 PRUint32 src_len);

extern HASHContext * HASH_Create(HASH_HashType type);

extern HASHContext * HASH_Clone(HASHContext *context);

extern void          HASH_Destroy(HASHContext *context);

extern void          HASH_Begin(HASHContext *context);

extern void          HASH_Update(HASHContext *context,
				const unsigned char *src,
				unsigned int len);

extern void          HASH_End(HASHContext *context,
			     unsigned char *result,
			     unsigned int *result_len,
			     unsigned int max_result_len);
			     
extern HASH_HashType HASH_GetType(HASHContext *context);

extern const SECHashObject * HASH_GetHashObject(HASH_HashType type);

extern const SECHashObject * HASH_GetHashObjectByOidTag(SECOidTag hashOid);

extern HASH_HashType HASH_GetHashTypeByOidTag(SECOidTag hashOid);
extern SECOidTag HASH_GetHashOidTagByHMACOidTag(SECOidTag hmacOid);
extern SECOidTag HASH_GetHMACOidTagByHashOidTag(SECOidTag hashOid);

SEC_END_PROTOS

#endif 
