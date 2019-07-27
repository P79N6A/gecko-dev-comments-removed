





#ifndef mozilla_dom_BasicSymmetricKeyAlgorithm_h
#define mozilla_dom_BasicSymmetricKeyAlgorithm_h

#include "mozilla/dom/KeyAlgorithm.h"

namespace mozilla {
namespace dom {

class BasicSymmetricKeyAlgorithm : public KeyAlgorithm
{
public:
  BasicSymmetricKeyAlgorithm(nsIGlobalObject* aGlobal, const nsString& aName, uint16_t aLength)
    : KeyAlgorithm(aGlobal, aName)
    , mLength(aLength)
  {}

  ~BasicSymmetricKeyAlgorithm()
  {}

  uint16_t Length() const
  {
    return mLength;
  }

protected:
  uint16_t mLength;
};

} 
} 

#endif 
