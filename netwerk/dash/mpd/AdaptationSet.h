










































#ifndef ADAPTATIONSET_H_
#define ADAPTATIONSET_H_

#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsTArray.h"
#include "Representation.h"

namespace mozilla {
namespace net {

class AdaptationSet
{
public:
  AdaptationSet() :
    mWidth(0),
    mHeight(0),
    mIsBitstreamSwitching(false)
  {
    MOZ_COUNT_CTOR(AdaptationSet);
  }
  virtual ~AdaptationSet() {
    MOZ_COUNT_DTOR(AdaptationSet);
  }

  
  int32_t  GetWidth() const;
  void     SetWidth(int32_t const aWidth);
  int32_t  GetHeight() const;
  void     SetHeight(int32_t const aHeight);
  void     GetMIMEType(nsAString& aMIMEType) const;
  void     SetMIMEType(nsAString const &aMIMEType);

  
  Representation const * GetRepresentation(uint32_t) const;

  
  
  void     AddRepresentation(Representation* aRep);

  
  uint16_t GetNumRepresentations() const;

  
  void EnableBitstreamSwitching(bool const aEnable);
  bool IsBitstreamSwitchingEnabled() const;

private:
  
  nsTArray<nsAutoPtr<Representation> >  mRepresentations;

  
  int32_t                    mWidth;
  int32_t                    mHeight;
  nsString                   mMIMEType;

  
  bool                       mIsBitstreamSwitching;
};
}
}

#endif 
