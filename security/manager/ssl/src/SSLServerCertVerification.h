




































#ifndef _SSLSERVERCERTVERIFICATION_H
#define _SSLSERVERCERTVERIFICATION_H

#include "seccomon.h"
#include "prio.h"

typedef struct PRFileDesc PRFileDesc;
typedef struct CERTCertificateStr CERTCertificate;
class nsNSSSocketInfo;
class nsNSSShutDownPreventionLock;

namespace mozilla { namespace psm {

SECStatus AuthCertificateHook(void *arg, PRFileDesc *fd, 
                              PRBool checkSig, PRBool isServer);

} } 

#endif
