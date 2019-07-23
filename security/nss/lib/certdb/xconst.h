


































#ifndef _XCONST_H_
#define _XCONST_H_

#include "certt.h"

typedef struct CERTAltNameEncodedContextStr {
    SECItem **encodedGenName;
} CERTAltNameEncodedContext;



SEC_BEGIN_PROTOS

extern SECStatus
CERT_EncodePrivateKeyUsagePeriod(PRArenaPool *arena, 
                                CERTPrivKeyUsagePeriod *pkup,
				SECItem *encodedValue);

extern SECStatus
CERT_EncodeNameConstraintsExtension(PRArenaPool *arena, 
                                    CERTNameConstraints  *value,
			            SECItem *encodedValue);

extern SECStatus 
CERT_EncodeIA5TypeExtension(PRArenaPool *arena, char *value, 
                            SECItem *encodedValue);

SECStatus
cert_EncodeAuthInfoAccessExtension(PRArenaPool *arena,
				   CERTAuthInfoAccess **info,
				   SECItem *dest);
SEC_END_PROTOS
#endif
