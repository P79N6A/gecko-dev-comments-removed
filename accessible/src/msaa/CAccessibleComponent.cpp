







































#include "CAccessibleComponent.h"

#include "AccessibleComponent_i.c"

#include "nsAccessible.h"
#include "States.h"

#include "nsString.h"

#include "nsIDOMCSSPrimitiveValue.h"
#include "nsIDOMNSRGBAColor.h"

enum {
  IA2AlphaShift = 24,
  IA2RedShift = 16,
  IA2GreenShift = 8,
  IA2BlueShift = 0
};



STDMETHODIMP
CAccessibleComponent::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = NULL;

  if (IID_IAccessibleComponent == iid) {
    *ppv = static_cast<IAccessibleComponent*>(this);
    (reinterpret_cast<IUnknown*>(*ppv))->AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}



STDMETHODIMP
CAccessibleComponent::get_locationInParent(long *aX, long *aY)
{
__try {
  *aX = 0;
  *aY = 0;

  nsRefPtr<nsAccessible> acc(do_QueryObject(this));
  if (!acc)
    return E_FAIL;

  
  PRUint64 state = acc->State();
  if (state & states::INVISIBLE)
    return S_OK;

  PRInt32 x = 0, y = 0, width = 0, height = 0;
  nsresult rv = acc->GetBounds(&x, &y, &width, &height);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  nsCOMPtr<nsIAccessible> parentAcc;
  rv = acc->GetParent(getter_AddRefs(parentAcc));
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  
  
  
  if (!parentAcc) {
    *aX = x;
    *aY = y;
    return S_OK;
  }

  
  
  PRInt32 parentx = 0, parenty = 0;
  rv = acc->GetBounds(&parentx, &parenty, &width, &height);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  *aX = x - parentx;
  *aY = y - parenty;
  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

STDMETHODIMP
CAccessibleComponent::get_foreground(IA2Color *aForeground)
{
__try {
  return GetARGBValueFromCSSProperty(NS_LITERAL_STRING("color"), aForeground);
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

STDMETHODIMP
CAccessibleComponent::get_background(IA2Color *aBackground)
{
__try {
  return GetARGBValueFromCSSProperty(NS_LITERAL_STRING("background-color"),
                                     aBackground);
} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return E_FAIL;
}

HRESULT
CAccessibleComponent::GetARGBValueFromCSSProperty(const nsAString& aPropName,
                                                  IA2Color *aColorValue)
{
__try {
  *aColorValue = 0;

  nsCOMPtr<nsIAccessNode> acc(do_QueryObject(this));
  if (!acc)
    return E_FAIL;

  nsCOMPtr<nsIDOMCSSPrimitiveValue> cssValue;
  nsresult rv = acc->GetComputedStyleCSSValue(EmptyString(), aPropName,
                                              getter_AddRefs(cssValue));
  if (NS_FAILED(rv) || !cssValue)
    return GetHRESULT(rv);

  nsCOMPtr<nsIDOMRGBColor> rgbColor;
  rv = cssValue->GetRGBColorValue(getter_AddRefs(rgbColor));
  if (NS_FAILED(rv) || !rgbColor)
    return GetHRESULT(rv);

  nsCOMPtr<nsIDOMNSRGBAColor> rgbaColor(do_QueryInterface(rgbColor));
  if (!rgbaColor)
    return GetHRESULT(rv);

  
  nsCOMPtr<nsIDOMCSSPrimitiveValue> alphaValue;
  rv = rgbaColor->GetAlpha(getter_AddRefs(alphaValue));
  if (NS_FAILED(rv) || !alphaValue)
    return GetHRESULT(rv);

  float alpha = 0.0;
  rv = alphaValue->GetFloatValue(nsIDOMCSSPrimitiveValue::CSS_NUMBER, &alpha);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  
  nsCOMPtr<nsIDOMCSSPrimitiveValue> redValue;
  rv = rgbaColor->GetRed(getter_AddRefs(redValue));
  if (NS_FAILED(rv) || !redValue)
    return GetHRESULT(rv);

  float red = 0.0;
  rv = redValue->GetFloatValue(nsIDOMCSSPrimitiveValue::CSS_NUMBER, &red);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  
  nsCOMPtr<nsIDOMCSSPrimitiveValue> greenValue;
  rv = rgbaColor->GetGreen(getter_AddRefs(greenValue));
  if (NS_FAILED(rv) || !greenValue)
    return GetHRESULT(rv);

  float green = 0.0;
  rv = greenValue->GetFloatValue(nsIDOMCSSPrimitiveValue::CSS_NUMBER, &green);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  
  nsCOMPtr<nsIDOMCSSPrimitiveValue> blueValue;
  rv = rgbaColor->GetBlue(getter_AddRefs(blueValue));
  if (NS_FAILED(rv) || !blueValue)
    return GetHRESULT(rv);

  float blue = 0.0;
  rv = blueValue->GetFloatValue(nsIDOMCSSPrimitiveValue::CSS_NUMBER, &blue);
  if (NS_FAILED(rv))
    return GetHRESULT(rv);

  
  *aColorValue = (((IA2Color) blue) << IA2BlueShift) |
                 (((IA2Color) green) << IA2GreenShift) |
                 (((IA2Color) red) << IA2RedShift) |
                 (((IA2Color) (alpha * 0xff)) << IA2AlphaShift);
  return S_OK;

} __except(nsAccessNodeWrap::FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return E_FAIL;
}

