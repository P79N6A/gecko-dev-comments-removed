




































#ifndef _SSLSERVERCERTVERIFICATION_H
#define _SSLSERVERCERTVERIFICATION_H

#include "seccomon.h"
#include "nsAutoPtr.h"
#include "nsThreadUtils.h"
#include "nsIRunnable.h"
#include "prerror.h"
#include "nsNSSIOLayer.h"

typedef struct PRFileDesc PRFileDesc;
typedef struct CERTCertificateStr CERTCertificate;
class nsNSSSocketInfo;
class nsNSSShutDownPreventionLock;

namespace mozilla { namespace psm {

SECStatus AuthCertificateHook(void *arg, PRFileDesc *fd, 
                              PRBool checkSig, PRBool isServer);

SECStatus HandleBadCertificate(PRErrorCode defaultErrorCodeToReport,
                               nsNSSSocketInfo * socketInfo,
                               CERTCertificate & cert,
                               const void * fdForLogging,
                               const nsNSSShutDownPreventionLock &);







class SSLServerCertVerificationResult : public nsRunnable
{
public:
  NS_DECL_NSIRUNNABLE

  SSLServerCertVerificationResult(nsNSSSocketInfo & socketInfo,
                                  PRErrorCode errorCode,
                                  SSLErrorMessageType errorMessageType = 
                                      PlainErrorMessage);

  void Dispatch();
private:
  const nsRefPtr<nsNSSSocketInfo> mSocketInfo;
  const PRErrorCode mErrorCode;
  const SSLErrorMessageType mErrorMessageType;
};

} } 

#endif
