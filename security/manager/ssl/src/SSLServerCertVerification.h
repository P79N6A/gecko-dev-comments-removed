




#ifndef _SSLSERVERCERTVERIFICATION_H
#define _SSLSERVERCERTVERIFICATION_H

#include "seccomon.h"
#include "prio.h"

namespace mozilla { namespace psm {

SECStatus AuthCertificateHook(void* arg, PRFileDesc* fd,
                              PRBool checkSig, PRBool isServer);




void EnsureServerVerificationInitialized();

} } 

#endif
