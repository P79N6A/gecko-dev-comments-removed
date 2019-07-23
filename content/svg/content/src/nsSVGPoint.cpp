






































#include "nsSVGPoint.h"
#include "nsIDOMSVGMatrix.h"
#include "nsSVGValue.h"
#include "nsContentUtils.h"
#include "nsDOMError.h"

class nsSVGPoint : public nsIDOMSVGPoint,
                   public nsSVGValue
{
public:
  nsSVGPoint(float x, float y);

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGPOINT

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);
  
protected:
  float mX;
  float mY;
};





nsresult
NS_NewSVGPoint(nsIDOMSVGPoint** result, float x, float y)
{
  *result = new nsSVGPoint(x, y);
  if (!*result)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*result);
  return NS_OK;
}

nsresult
NS_NewSVGPoint(nsIDOMSVGPoint** result, const gfxPoint& point)
{
  return NS_NewSVGPoint(result, float(point.x), float(point.y));
}

nsSVGPoint::nsSVGPoint(float x, float y)
    : mX(x), mY(y)
{
}




NS_IMPL_ADDREF(nsSVGPoint)
NS_IMPL_RELEASE(nsSVGPoint)

NS_INTERFACE_MAP_BEGIN(nsSVGPoint)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGPoint)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGPoint)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END





NS_IMETHODIMP nsSVGPoint::GetX(float *aX)
{
  *aX = mX;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPoint::SetX(float aX)
{
  WillModify();
  mX = aX;
  DidModify();
  
  return NS_OK;
}


NS_IMETHODIMP nsSVGPoint::GetY(float *aY)
{
  *aY = mY;
  return NS_OK;
}
NS_IMETHODIMP nsSVGPoint::SetY(float aY)
{
  WillModify();
  mY = aY;
  DidModify();
  
  return NS_OK;
}


NS_IMETHODIMP nsSVGPoint::MatrixTransform(nsIDOMSVGMatrix *matrix,
                                          nsIDOMSVGPoint **_retval)
{
  if (!matrix)
    return NS_ERROR_DOM_SVG_WRONG_TYPE_ERR;

  float a, b, c, d, e, f;
  matrix->GetA(&a);
  matrix->GetB(&b);
  matrix->GetC(&c);
  matrix->GetD(&d);
  matrix->GetE(&e);
  matrix->GetF(&f);
  
  return NS_NewSVGPoint(_retval, a*mX + c*mY + e, b*mX + d*mY + f);
}



NS_IMETHODIMP
nsSVGPoint::SetValueString(const nsAString& aValue)
{
  NS_NOTYETIMPLEMENTED("nsSVGPoint::SetValueString");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsSVGPoint::GetValueString(nsAString& aValue)
{
  NS_NOTYETIMPLEMENTED("nsSVGPoint::GetValueString");
  return NS_ERROR_NOT_IMPLEMENTED;
}









class nsSVGReadonlyPoint : public nsSVGPoint
{
public:
  nsSVGReadonlyPoint(float x, float y)
    : nsSVGPoint(x, y)
  {
  }

  
  NS_IMETHODIMP SetX(float) { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
  NS_IMETHODIMP SetY(float) { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
  NS_IMETHODIMP SetValueString(const nsAString&) { return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR; }
};

nsresult
NS_NewSVGReadonlyPoint(nsIDOMSVGPoint** result, float x, float y)
{
  *result = new nsSVGReadonlyPoint(x, y);
  if (!*result)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*result);
  return NS_OK;
}

