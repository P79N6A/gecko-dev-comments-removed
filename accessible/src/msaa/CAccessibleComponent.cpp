







































#include "CAccessibleComponent.h"

#include "AccessibleComponent_i.c"

#include "nsIAccessNode.h"
#include "nsIAccessible.h"
#include "nsIAccessibleStates.h"

#include "nsCOMPtr.h"
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
    *ppv = NS_STATIC_CAST(IAccessibleComponent*, this);
    (NS_REINTERPRET_CAST(IUnknown*, *ppv))->AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}



STDMETHODIMP
CAccessibleComponent::get_locationInParent(long *aX, long *aY)
{
  *aX = 0;
  *aY = 0;

  nsCOMPtr<nsIAccessible> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  
  PRUint32 states = 0, extraStates = 0;
  nsresult rv = acc->GetFinalState(&states, &extraStates);
  if (NS_FAILED(rv))
    return E_FAIL;

  if (states & nsIAccessibleStates::STATE_INVISIBLE)
    return S_OK;

  PRInt32 x = 0, y = 0, width = 0, height = 0;
  rv = acc->GetBounds(&x, &y, &width, &height);
  if (NS_FAILED(rv))
    return E_FAIL;

  nsCOMPtr<nsIAccessible> parentAcc;
  rv = acc->GetParent(getter_AddRefs(parentAcc));
  if (NS_FAILED(rv))
    return E_FAIL;

  
  
  
  if (!parentAcc) {
    *aX = x;
    *aY = y;
    return NS_OK;
  }

  
  
  PRInt32 parentx = 0, parenty = 0;
  rv = acc->GetBounds(&parentx, &parenty, &width, &height);
  if (NS_FAILED(rv))
    return E_FAIL;

  *aX = x - parentx;
  *aY = y - parenty;

  return S_OK;
}

STDMETHODIMP
CAccessibleComponent::get_foreground(IA2Color *aForeground)
{
  return GetARGBValueFromCSSProperty(NS_LITERAL_STRING("color"), aForeground);
}

STDMETHODIMP
CAccessibleComponent::get_background(IA2Color *aBackground)
{
  return GetARGBValueFromCSSProperty(NS_LITERAL_STRING("background-color"),
                                     aBackground);
}

HRESULT
CAccessibleComponent::GetARGBValueFromCSSProperty(const nsAString& aPropName,
                                                  IA2Color *aColorValue)
{
  *aColorValue = 0;

  nsCOMPtr<nsIAccessNode> acc(do_QueryInterface(this));
  if (!acc)
    return E_FAIL;

  nsCOMPtr<nsIDOMCSSPrimitiveValue> cssValue;
  nsresult rv = acc->GetComputedStyleCSSValue(EmptyString(), aPropName,
                                              getter_AddRefs(cssValue));
  if (NS_FAILED(rv) || !cssValue)
    return E_FAIL;

  nsCOMPtr<nsIDOMRGBColor> rgbColor;
  rv = cssValue->GetRGBColorValue(getter_AddRefs(rgbColor));
  if (NS_FAILED(rv) || !rgbColor)
    return E_FAIL;

  nsCOMPtr<nsIDOMNSRGBAColor> rgbaColor(do_QueryInterface(rgbColor));
  if (!rgbaColor)
    return E_FAIL;

  
  nsCOMPtr<nsIDOMCSSPrimitiveValue> alphaValue;
  rv = rgbaColor->GetAlpha(getter_AddRefs(alphaValue));
  if (NS_FAILED(rv) || !alphaValue)
    return E_FAIL;

  float alpha = 0.0;
  rv = alphaValue->GetFloatValue(nsIDOMCSSPrimitiveValue::CSS_NUMBER, &alpha);
  if (NS_FAILED(rv))
    return E_FAIL;

  
  nsCOMPtr<nsIDOMCSSPrimitiveValue> redValue;
  rv = rgbaColor->GetRed(getter_AddRefs(redValue));
  if (NS_FAILED(rv) || !redValue)
    return E_FAIL;

  float red = 0.0;
  rv = redValue->GetFloatValue(nsIDOMCSSPrimitiveValue::CSS_NUMBER, &red);
  if (NS_FAILED(rv))
    return E_FAIL;

  
  nsCOMPtr<nsIDOMCSSPrimitiveValue> greenValue;
  rv = rgbaColor->GetGreen(getter_AddRefs(greenValue));
  if (NS_FAILED(rv) || !greenValue)
    return E_FAIL;

  float green = 0.0;
  rv = greenValue->GetFloatValue(nsIDOMCSSPrimitiveValue::CSS_NUMBER, &green);
  if (NS_FAILED(rv))
    return E_FAIL;

  
  nsCOMPtr<nsIDOMCSSPrimitiveValue> blueValue;
  rv = rgbaColor->GetBlue(getter_AddRefs(blueValue));
  if (NS_FAILED(rv) || !blueValue)
    return E_FAIL;

  float blue = 0.0;
  rv = blueValue->GetFloatValue(nsIDOMCSSPrimitiveValue::CSS_NUMBER, &blue);
  if (NS_FAILED(rv))
    return E_FAIL;

  
  *aColorValue = (((IA2Color) blue) << IA2BlueShift) |
                 (((IA2Color) green) << IA2GreenShift) |
                 (((IA2Color) red) << IA2RedShift) |
                 (((IA2Color) (alpha * 0xff)) << IA2AlphaShift);

  return S_OK;
}

