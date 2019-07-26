




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




nsresult
NS_NewSVGRect(mozilla::dom::SVGRect** result, float x, float y,
              float width, float height)
{
  *result = new mozilla::dom::SVGRect(x, y, width, height);
  if (!*result) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*result);
  return NS_OK;
}

nsresult
NS_NewSVGRect(mozilla::dom::SVGRect** result, const gfxRect& rect)
{
  return NS_NewSVGRect(result,
                       rect.X(), rect.Y(),
                       rect.Width(), rect.Height());
}

