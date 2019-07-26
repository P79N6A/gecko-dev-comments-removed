



#ifndef mozilla_dom_CanvasGradient_h
#define mozilla_dom_CanvasGradient_h

#include "mozilla/Attributes.h"
#include "nsTArray.h"
#include "mozilla/RefPtr.h"
#include "mozilla/dom/CanvasRenderingContext2DBinding.h"
#include "mozilla/dom/CanvasRenderingContext2D.h"
#include "mozilla/gfx/2D.h"
#include "nsWrapperCache.h"
#include "gfxGradientCache.h"

namespace mozilla {
namespace dom {

class CanvasGradient : public nsWrapperCache
{
public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(CanvasGradient)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(CanvasGradient)

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

    mStops =
      gfx::gfxGradientCache::GetOrCreateGradientStops(aRT,
                                                      mRawStops,
                                                      gfx::ExtendMode::CLAMP);

    return mStops;
  }

  
  void AddColorStop(float offset, const nsAString& colorstr, ErrorResult& rv);

  JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE
  {
    return CanvasGradientBinding::Wrap(aCx, this);
  }

  CanvasRenderingContext2D* GetParentObject()
  {
    return mContext;
  }

protected:
  CanvasGradient(CanvasRenderingContext2D* aContext, Type aType)
    : mContext(aContext)
    , mType(aType)
  {
    SetIsDOMBinding();
  }

  nsRefPtr<CanvasRenderingContext2D> mContext;
  nsTArray<mozilla::gfx::GradientStop> mRawStops;
  mozilla::RefPtr<mozilla::gfx::GradientStops> mStops;
  Type mType;
  virtual ~CanvasGradient() {}
};

} 
} 

#endif 
