



#ifndef PublicKeyPinningService_h
#define PublicKeyPinningService_h

#include "cert.h"

namespace mozilla {
namespace psm {

class PublicKeyPinningService
{
public:
  










  static bool ChainHasValidPins(const CERTCertList* certList,
                                const char* hostname,
                                const PRTime time,
                                bool enforceTestMode);
};

}} 

#endif 
