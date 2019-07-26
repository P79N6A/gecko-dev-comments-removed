




#include "mozilla/dom/SVGRect.h"
#include "nsContentUtils.h"
#include "nsDOMClassInfoID.h"
#include "nsSVGElement.h"

DOMCI_DATA(SVGRect, mozilla::dom::SVGRect)

namespace mozilla {
namespace dom {




SVGRect::SVGRect(nsIContent* aParent, float x, float y, float w, float h)
  : SVGIRect(aParent), mX(x), mY(y), mWidth(w), mHeight(h)
{
}




NS_IMPL_ADDREF(SVGRect)
NS_IMPL_RELEASE(SVGRect)

NS_INTERFACE_MAP_BEGIN(SVGRect)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGRect)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGRect)
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
NS_NewSVGRect(nsIContent* aParent, const gfxRect& aRect)
{
  return NS_NewSVGRect(aParent, aRect.X(), aRect.Y(),
                       aRect.Width(), aRect.Height());
}

