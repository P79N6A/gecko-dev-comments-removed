




#include "mozilla/dom/SVGRect.h"
#include "nsContentUtils.h"
#include "nsDOMClassInfoID.h"

DOMCI_DATA(SVGRect, mozilla::dom::SVGRect)

namespace mozilla {
namespace dom {




SVGRect::SVGRect(float x, float y, float w, float h)
    : mX(x), mY(y), mWidth(w), mHeight(h)
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
NS_NewSVGRect(float x, float y, float width, float height)
{
  nsRefPtr<mozilla::dom::SVGRect> rect =
    new mozilla::dom::SVGRect(x, y, width, height);

  return rect.forget();
}

already_AddRefed<mozilla::dom::SVGRect>
NS_NewSVGRect(const gfxRect& rect)
{
  return NS_NewSVGRect(rect.X(), rect.Y(),
                       rect.Width(), rect.Height());
}

