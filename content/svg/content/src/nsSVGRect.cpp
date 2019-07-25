






































#include "nsSVGRect.h"
#include "prdtoa.h"
#include "nsTextFormatter.h"
#include "nsCRT.h"
#include "nsIDOMSVGLength.h"
#include "nsContentUtils.h"
#include "nsDOMError.h"




nsSVGRect::nsSVGRect(float x, float y, float w, float h)
    : mX(x), mY(y), mWidth(w), mHeight(h)
{
}




NS_IMPL_ADDREF(nsSVGRect)
NS_IMPL_RELEASE(nsSVGRect)

DOMCI_DATA(SVGRect, nsSVGRect)

NS_INTERFACE_MAP_BEGIN(nsSVGRect)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGRect)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGRect)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END





NS_IMETHODIMP nsSVGRect::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGRect::SetX(float aX)
{
  NS_ENSURE_FINITE(aX, NS_ERROR_ILLEGAL_VALUE);
  mX = aX;
  return NS_OK;
}


NS_IMETHODIMP nsSVGRect::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGRect::SetY(float aY)
{
  NS_ENSURE_FINITE(aY, NS_ERROR_ILLEGAL_VALUE);
  mY = aY;
  return NS_OK;
}


NS_IMETHODIMP nsSVGRect::GetWidth(float *aWidth)
{
  *aWidth = mWidth;
  return NS_OK;
}
NS_IMETHODIMP nsSVGRect::SetWidth(float aWidth)
{
  NS_ENSURE_FINITE(aWidth, NS_ERROR_ILLEGAL_VALUE);
  mWidth = aWidth;
  return NS_OK;
}


NS_IMETHODIMP nsSVGRect::GetHeight(float *aHeight)
{
  *aHeight = mHeight;
  return NS_OK;
}
NS_IMETHODIMP nsSVGRect::SetHeight(float aHeight)
{
  NS_ENSURE_FINITE(aHeight, NS_ERROR_ILLEGAL_VALUE);
  mHeight = aHeight;
  return NS_OK;
}






nsresult
NS_NewSVGRect(nsIDOMSVGRect** result, float x, float y,
              float width, float height)
{
  *result = new nsSVGRect(x, y, width, height);
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

