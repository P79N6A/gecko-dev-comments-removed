



#ifndef mozilla_dom_CanvasPattern_h
#define mozilla_dom_CanvasPattern_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/CanvasRenderingContext2DBinding.h"
#include "mozilla/dom/CanvasRenderingContext2D.h"
#include "mozilla/RefPtr.h"
#include "nsISupports.h"
#include "nsWrapperCache.h"

class nsIPrincipal;

namespace mozilla {
namespace gfx {
class SourceSurface;
} 

namespace dom {
class SVGMatrix;

class CanvasPattern final : public nsWrapperCache
{
  ~CanvasPattern() {}
public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(CanvasPattern)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(CanvasPattern)

  enum class RepeatMode : uint8_t {
    REPEAT,
    REPEATX,
    REPEATY,
    NOREPEAT
  };

  CanvasPattern(CanvasRenderingContext2D* aContext,
                gfx::SourceSurface* aSurface,
                RepeatMode aRepeat,
                nsIPrincipal* principalForSecurityCheck,
                bool forceWriteOnly,
                bool CORSUsed)
    : mContext(aContext)
    , mSurface(aSurface)
    , mPrincipal(principalForSecurityCheck)
    , mTransform()
    , mForceWriteOnly(forceWriteOnly)
    , mCORSUsed(CORSUsed)
    , mRepeat(aRepeat)
  {
  }

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override
  {
    return CanvasPatternBinding::Wrap(aCx, this, aGivenProto);
  }

  CanvasRenderingContext2D* GetParentObject()
  {
    return mContext;
  }

  
  void SetTransform(SVGMatrix& matrix);

  nsRefPtr<CanvasRenderingContext2D> mContext;
  RefPtr<gfx::SourceSurface> mSurface;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  mozilla::gfx::Matrix mTransform;
  const bool mForceWriteOnly;
  const bool mCORSUsed;
  const RepeatMode mRepeat;
};

} 
} 

#endif 
