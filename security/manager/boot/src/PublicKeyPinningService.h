



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
  










  static nsresult ChainHasValidPins(const CERTCertList* certList,
                                    const char* hostname,
                                    mozilla::pkix::Time time,
                                    bool enforceTestMode,
                             bool& chainHasValidPins);
  




  static nsresult ChainMatchesPinset(const CERTCertList* certList,
                                     const nsTArray<nsCString>& aSHA256keys,
                              bool& chainMatchesPinset);

  




  static nsresult HostHasPins(const char* hostname,
                              mozilla::pkix::Time time,
                              bool enforceTestMode,
                       bool& hostHasPins);

  




  static nsAutoCString CanonicalizeHostname(const char* hostname);
};

}} 

#endif 
