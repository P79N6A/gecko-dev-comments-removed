




#include "mozilla/dom/SVGRect.h"
#include "nsSVGElement.h"

using namespace mozilla::gfx;

namespace mozilla {
namespace dom {




SVGRect::SVGRect(nsIContent* aParent, float x, float y, float w, float h)
  : SVGIRect(), mParent(aParent), mX(x), mY(y), mWidth(w), mHeight(h)
{
}




NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(SVGRect, mParent)

NS_IMPL_CYCLE_COLLECTING_ADDREF(SVGRect)
NS_IMPL_CYCLE_COLLECTING_RELEASE(SVGRect)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(SVGRect)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

} 
} 




already_AddRefed<mozilla::dom::SVGRect>
NS_NewSVGRect(nsIContent* aParent, float aX, float aY, float aWidth,
              float aHeight)
{
  nsRefPtr<mozilla::dom::SVGRect> rect =
    new mozilla::dom::SVGRect(aParent, aX, aY, aWidth, aHeight);

  return rect.forget();
}

already_AddRefed<mozilla::dom::SVGRect>
NS_NewSVGRect(nsIContent* aParent, const Rect& aRect)
{
  return NS_NewSVGRect(aParent, aRect.x, aRect.y,
                       aRect.width, aRect.height);
}

