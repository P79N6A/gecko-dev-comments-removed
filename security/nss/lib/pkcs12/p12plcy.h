


#ifndef _P12PLCY_H_
#define _P12PLCY_H_

#include "secoid.h"
#include "ciferfam.h"

SEC_BEGIN_PROTOS


extern PRBool SEC_PKCS12DecryptionAllowed(SECAlgorithmID *algid);


extern PRBool SEC_PKCS12IsEncryptionAllowed(void);


extern SECStatus SEC_PKCS12EnableCipher(long which, int on);


extern SECStatus SEC_PKCS12SetPreferredCipher(long which, int on);

SEC_END_PROTOS
#endif
