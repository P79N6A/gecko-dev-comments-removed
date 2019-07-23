



































#ifndef _SECOID_H_
#define _SECOID_H_

#include "utilrename.h"







#include "plarena.h"

#include "seccomon.h"
#include "secoidt.h"
#include "secasn1t.h"

SEC_BEGIN_PROTOS

extern const SEC_ASN1Template SECOID_AlgorithmIDTemplate[];


SEC_ASN1_CHOOSER_DECLARE(SECOID_AlgorithmIDTemplate)




extern SECOidData *SECOID_FindOID( const SECItem *oid);
extern SECOidTag SECOID_FindOIDTag(const SECItem *oid);
extern SECOidData *SECOID_FindOIDByTag(SECOidTag tagnum);
extern SECOidData *SECOID_FindOIDByMechanism(unsigned long mechanism);













extern SECStatus SECOID_SetAlgorithmID(PLArenaPool *arena, SECAlgorithmID *aid,
				   SECOidTag tag, SECItem *params);







extern SECStatus SECOID_CopyAlgorithmID(PLArenaPool *arena, SECAlgorithmID *dest,
				    SECAlgorithmID *src);




extern SECOidTag SECOID_GetAlgorithmTag(SECAlgorithmID *aid);






extern void SECOID_DestroyAlgorithmID(SECAlgorithmID *aid, PRBool freeit);





extern SECComparison SECOID_CompareAlgorithmID(SECAlgorithmID *a,
					   SECAlgorithmID *b);

extern PRBool SECOID_KnownCertExtenOID (SECItem *extenOid);



extern const char *SECOID_FindOIDTagDescription(SECOidTag tagnum);





extern SECOidTag SECOID_AddEntry(const SECOidData * src);




extern SECStatus SECOID_Init(void);




extern SECStatus SECOID_Shutdown(void);














extern SECStatus SEC_StringToOID(PLArenaPool *pool, SECItem *to, 
                                 const char *from, PRUint32 len);

extern void UTIL_SetForkState(PRBool forked);












extern SECStatus NSS_GetAlgorithmPolicy(SECOidTag tag, PRUint32 *pValue);





extern SECStatus
NSS_SetAlgorithmPolicy(SECOidTag tag, PRUint32 setBits, PRUint32 clearBits);


SEC_END_PROTOS

#endif 
