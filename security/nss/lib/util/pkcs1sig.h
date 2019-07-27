




#ifndef _PKCS1SIG_H_
#define _PKCS1SIG_H_

#include "hasht.h"
#include "seccomon.h"
#include "secoidt.h"













SECStatus _SGN_VerifyPKCS1DigestInfo(SECOidTag digestAlg,
                                     const SECItem* digest,
                                     const SECItem* dataRecoveredFromSignature,
                                     PRBool unsafeAllowMissingParameters);

#endif 
