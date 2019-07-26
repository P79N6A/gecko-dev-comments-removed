



#ifndef mozilla_dom_CanvasGradient_h
#define mozilla_dom_CanvasGradient_h

#include "nsIDOMCanvasRenderingContext2D.h"
#include "nsTArray.h"
#include "mozilla/RefPtr.h"
#include "mozilla/dom/CanvasRenderingContext2DBinding.h"
#include "mozilla/gfx/2D.h"

#define NS_CANVASGRADIENTAZURE_PRIVATE_IID \
    {0x28425a6a, 0x90e0, 0x4d42, {0x9c, 0x75, 0xff, 0x60, 0x09, 0xb3, 0x10, 0xa8}}

namespace mozilla {
namespace dom {

class CanvasGradient : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_CANVASGRADIENTAZURE_PRIVATE_IID)

  enum Type
  {
    LINEAR = 0,
    RADIAL
  };

  Type GetType()
  {
    return mType;
  }


  mozilla::gfx::GradientStops *
  GetGradientStopsForTarget(mozilla::gfx::DrawTarget *aRT)
  {
    if (mStops && mStops->GetBackendType() == aRT->GetType()) {
      return mStops;
    }

    mStops = aRT->CreateGradientStops(mRawStops.Elements(), mRawStops.Length());

    return mStops;
  }

  NS_DECL_ISUPPORTS

  
  void AddColorStop(float offset, const nsAString& colorstr, ErrorResult& rv);

  JSObject* WrapObject(JSContext* aCx, JSObject* aScope)
  {
    return CanvasGradientBinding::Wrap(aCx, aScope, this);
  }

protected:
  CanvasGradient(Type aType) : mType(aType)
  {}

  nsTArray<mozilla::gfx::GradientStop> mRawStops;
  mozilla::RefPtr<mozilla::gfx::GradientStops> mStops;
  Type mType;
  virtual ~CanvasGradient() {}
};

} 
} 

#endif 
