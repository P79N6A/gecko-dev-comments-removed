



































#ifndef _SECITEM_H_
#define _SECITEM_H_

#include "utilrename.h"








#include "plarena.h"
#include "plhash.h"
#include "seccomon.h"

SEC_BEGIN_PROTOS















extern SECItem *SECITEM_AllocItem(PLArenaPool *arena, SECItem *item,
				  unsigned int len);









extern SECStatus SECITEM_ReallocItem(PLArenaPool *arena, SECItem *item,
				     unsigned int oldlen, unsigned int newlen);




extern SECComparison SECITEM_CompareItem(const SECItem *a, const SECItem *b);




extern PRBool SECITEM_ItemsAreEqual(const SECItem *a, const SECItem *b);




extern SECStatus SECITEM_CopyItem(PLArenaPool *arena, SECItem *to, 
                                  const SECItem *from);




extern SECItem *SECITEM_DupItem(const SECItem *from);






extern SECItem *SECITEM_ArenaDupItem(PLArenaPool *arena, const SECItem *from);




extern void SECITEM_FreeItem(SECItem *zap, PRBool freeit);




extern void SECITEM_ZfreeItem(SECItem *zap, PRBool freeit);

PLHashNumber PR_CALLBACK SECITEM_Hash ( const void *key);

PRIntn PR_CALLBACK SECITEM_HashCompare ( const void *k1, const void *k2);


SEC_END_PROTOS

#endif 
