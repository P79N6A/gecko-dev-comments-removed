



#ifndef mozilla_dom_CanvasPattern_h
#define mozilla_dom_CanvasPattern_h

#include "mozilla/dom/CanvasRenderingContext2DBinding.h"
#include "mozilla/RefPtr.h"
#include "nsISupports.h"

#define NS_CANVASPATTERNAZURE_PRIVATE_IID \
    {0xc9bacc25, 0x28da, 0x421e, {0x9a, 0x4b, 0xbb, 0xd6, 0x93, 0x05, 0x12, 0xbc}}
class nsIPrincipal;

namespace mozilla {
namespace gfx {
class SourceSurface;
}

namespace dom {

class CanvasPattern MOZ_FINAL : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_CANVASPATTERNAZURE_PRIVATE_IID)
  NS_DECL_ISUPPORTS

  enum RepeatMode
  {
    REPEAT,
    REPEATX,
    REPEATY,
    NOREPEAT
  };

  CanvasPattern(mozilla::gfx::SourceSurface* aSurface,
                RepeatMode aRepeat,
                nsIPrincipal* principalForSecurityCheck,
                bool forceWriteOnly,
                bool CORSUsed)
    : mSurface(aSurface)
    , mRepeat(aRepeat)
    , mPrincipal(principalForSecurityCheck)
    , mForceWriteOnly(forceWriteOnly)
    , mCORSUsed(CORSUsed)
  {
  }

  JSObject* WrapObject(JSContext* aCx, JSObject* aScope)
  {
    return CanvasPatternBinding::Wrap(aCx, aScope, this);
  }

  mozilla::RefPtr<mozilla::gfx::SourceSurface> mSurface;
  const RepeatMode mRepeat;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  const bool mForceWriteOnly;
  const bool mCORSUsed;
};

}
}

#endif
