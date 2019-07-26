



#ifndef _NS_DATASIGNATUREVERIFIER_H_
#define _NS_DATASIGNATUREVERIFIER_H_

#include "nsIDataSignatureVerifier.h"
#include "mozilla/Attributes.h"

#include "keythi.h"

typedef struct CERTCertificateStr CERTCertificate;


#define NS_DATASIGNATUREVERIFIER_CID \
    { 0x296d76aa, 0x275b, 0x4f3c, \
    { 0xaf, 0x8a, 0x30, 0xa4, 0x02, 0x6c, 0x18, 0xfc } }
#define NS_DATASIGNATUREVERIFIER_CONTRACTID \
    "@mozilla.org/security/datasignatureverifier;1"

class nsDataSignatureVerifier MOZ_FINAL : public nsIDataSignatureVerifier
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDATASIGNATUREVERIFIER

  nsDataSignatureVerifier()
  {
  }

private:
  ~nsDataSignatureVerifier()
  {
  }
};

namespace mozilla {

nsresult VerifyCMSDetachedSignatureIncludingCertificate(
  const SECItem& buffer, const SECItem& detachedDigest,
  nsresult (*verifyCertificate)(CERTCertificate* cert, void* context,
                                void* pinArg),
  void* verifyCertificateContext, void* pinArg);

} 

#endif 
