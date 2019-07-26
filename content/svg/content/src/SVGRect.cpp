




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





NS_IMETHODIMP SVGRect::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP SVGRect::SetX(float aX)
{
  NS_ENSURE_FINITE(aX, NS_ERROR_ILLEGAL_VALUE);
  mX = aX;
  return NS_OK;
}


NS_IMETHODIMP SVGRect::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP SVGRect::SetY(float aY)
{
  NS_ENSURE_FINITE(aY, NS_ERROR_ILLEGAL_VALUE);
  mY = aY;
  return NS_OK;
}


NS_IMETHODIMP SVGRect::GetWidth(float *aWidth)
{
  *aWidth = mWidth;
  return NS_OK;
}
NS_IMETHODIMP SVGRect::SetWidth(float aWidth)
{
  NS_ENSURE_FINITE(aWidth, NS_ERROR_ILLEGAL_VALUE);
  mWidth = aWidth;
  return NS_OK;
}


NS_IMETHODIMP SVGRect::GetHeight(float *aHeight)
{
  *aHeight = mHeight;
  return NS_OK;
}
NS_IMETHODIMP SVGRect::SetHeight(float aHeight)
{
  NS_ENSURE_FINITE(aHeight, NS_ERROR_ILLEGAL_VALUE);
  mHeight = aHeight;
  return NS_OK;
}

} 
} 




nsresult
NS_NewSVGRect(nsIDOMSVGRect** result, float x, float y,
              float width, float height)
{
  *result = new mozilla::dom::SVGRect(x, y, width, height);
  if (!*result) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*result);
  return NS_OK;
}

nsresult
NS_NewSVGRect(nsIDOMSVGRect** result, const gfxRect& rect)
{
  return NS_NewSVGRect(result,
                       rect.X(), rect.Y(),
                       rect.Width(), rect.Height());
}

