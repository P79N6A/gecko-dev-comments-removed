



#ifndef PublicKeyPinningService_h
#define PublicKeyPinningService_h

#include "cert.h"
#include "nsString.h"
#include "nsTArray.h"
#include "pkix/Time.h"

namespace mozilla {
namespace psm {

class PublicKeyPinningService
{
public:
  










  static bool ChainHasValidPins(const CERTCertList* certList,
                                const char* hostname,
                                mozilla::pkix::Time time,
                                bool enforceTestMode);
  




  static bool ChainMatchesPinset(const CERTCertList* certList,
                                 const nsTArray<nsCString>& aSHA256keys);

  




  static nsAutoCString CanonicalizeHostname(const char* hostname);
};

}} 

#endif 
