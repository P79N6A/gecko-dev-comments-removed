






































#include "nsROCSSPrimitiveValue.h"

#include "nsCOMPtr.h"
#include "nsDOMError.h"
#include "prprf.h"
#include "nsContentUtils.h"
#include "nsXPIDLString.h"
#include "nsCRT.h"
#include "nsPresContext.h"
#include "nsStyleUtil.h"

nsROCSSPrimitiveValue::nsROCSSPrimitiveValue(PRInt32 aAppUnitsPerInch)
  : mType(CSS_PX), mAppUnitsPerInch(aAppUnitsPerInch)
{
  mValue.mAppUnits = 0;
}


nsROCSSPrimitiveValue::~nsROCSSPrimitiveValue()
{
  Reset();
}

NS_IMPL_ADDREF(nsROCSSPrimitiveValue)
NS_IMPL_RELEASE(nsROCSSPrimitiveValue)



NS_INTERFACE_MAP_BEGIN(nsROCSSPrimitiveValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSPrimitiveValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSValue)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMCSSPrimitiveValue)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(ROCSSPrimitiveValue)
NS_INTERFACE_MAP_END





NS_IMETHODIMP
nsROCSSPrimitiveValue::GetCssText(nsAString& aCssText)
{
  nsAutoString tmpStr;
  aCssText.Truncate();
  nsresult result = NS_OK;

  switch (mType) {
    case CSS_PX :
      {
        float val = nsPresContext::AppUnitsToFloatCSSPixels(mValue.mAppUnits);
        tmpStr.AppendFloat(val);
        tmpStr.AppendLiteral("px");
        break;
      }
    case CSS_IDENT :
      {
        AppendUTF8toUTF16(nsCSSKeywords::GetStringValue(mValue.mKeyword),
                          tmpStr);
        break;
      }
    case CSS_STRING :
    case CSS_COUNTER : 
      {
        tmpStr.Append(mValue.mString);
        break;
      }
    case CSS_URI :
      {
        if (mValue.mURI) {
          nsCAutoString specUTF8;
          mValue.mURI->GetSpec(specUTF8);

          tmpStr.AssignLiteral("url(");
          nsStyleUtil::AppendEscapedCSSString(NS_ConvertUTF8toUTF16(specUTF8),
                                              tmpStr);
          tmpStr.AppendLiteral(")");
        } else {
          
          
          tmpStr.Assign(NS_LITERAL_STRING("url(invalid-url:)"));
        }
        break;
      }
    case CSS_ATTR :
      {
        tmpStr.AppendLiteral("attr(");
        tmpStr.Append(mValue.mString);
        tmpStr.Append(PRUnichar(')'));
        break;
      }
    case CSS_PERCENTAGE :
      {
        tmpStr.AppendFloat(mValue.mFloat * 100);
        tmpStr.Append(PRUnichar('%'));
        break;
      }
    case CSS_NUMBER :
      {
        tmpStr.AppendFloat(mValue.mFloat);
        break;
      }
    case CSS_RECT :
      {
        NS_ASSERTION(mValue.mRect, "mValue.mRect should never be null");
        NS_NAMED_LITERAL_STRING(comma, ", ");
        nsCOMPtr<nsIDOMCSSPrimitiveValue> sideCSSValue;
        nsAutoString sideValue;
        tmpStr.AssignLiteral("rect(");
        
        result = mValue.mRect->GetTop(getter_AddRefs(sideCSSValue));
        if (NS_FAILED(result))
          break;
        result = sideCSSValue->GetCssText(sideValue);
        if (NS_FAILED(result))
          break;
        tmpStr.Append(sideValue + comma);
        
        result = mValue.mRect->GetRight(getter_AddRefs(sideCSSValue));
        if (NS_FAILED(result))
          break;
        result = sideCSSValue->GetCssText(sideValue);
        if (NS_FAILED(result))
          break;
        tmpStr.Append(sideValue + comma);
        
        result = mValue.mRect->GetBottom(getter_AddRefs(sideCSSValue));
        if (NS_FAILED(result))
          break;
        result = sideCSSValue->GetCssText(sideValue);
        if (NS_FAILED(result))
          break;
        tmpStr.Append(sideValue + comma);
        
        result = mValue.mRect->GetLeft(getter_AddRefs(sideCSSValue));
        if (NS_FAILED(result))
          break;
        result = sideCSSValue->GetCssText(sideValue);
        if (NS_FAILED(result))
          break;
        tmpStr.Append(sideValue + NS_LITERAL_STRING(")"));
        break;
      }
    case CSS_RGBCOLOR :
      {
        NS_ASSERTION(mValue.mColor, "mValue.mColor should never be null");
        NS_NAMED_LITERAL_STRING(comma, ", ");
        nsCOMPtr<nsIDOMCSSPrimitiveValue> colorCSSValue;
        nsAutoString colorValue;
        if (mValue.mColor->HasAlpha())
          tmpStr.AssignLiteral("rgba(");
        else
          tmpStr.AssignLiteral("rgb(");

        
        result = mValue.mColor->GetRed(getter_AddRefs(colorCSSValue));
        if (NS_FAILED(result))
          break;
        result = colorCSSValue->GetCssText(colorValue);
        if (NS_FAILED(result))
          break;
        tmpStr.Append(colorValue + comma);

        
        result = mValue.mColor->GetGreen(getter_AddRefs(colorCSSValue));
        if (NS_FAILED(result))
          break;
        result = colorCSSValue->GetCssText(colorValue);
        if (NS_FAILED(result))
          break;
        tmpStr.Append(colorValue + comma);

        
        result = mValue.mColor->GetBlue(getter_AddRefs(colorCSSValue));
        if (NS_FAILED(result))
          break;
        result = colorCSSValue->GetCssText(colorValue);
        if (NS_FAILED(result))
          break;
        tmpStr.Append(colorValue);

        if (mValue.mColor->HasAlpha()) {
          
          result = mValue.mColor->GetAlpha(getter_AddRefs(colorCSSValue));
          if (NS_FAILED(result))
            break;
          result = colorCSSValue->GetCssText(colorValue);
          if (NS_FAILED(result))
            break;
          tmpStr.Append(comma + colorValue);
        }

        tmpStr.Append(NS_LITERAL_STRING(")"));

        break;
      }
    case CSS_S :
      {
        tmpStr.AppendFloat(mValue.mFloat);
        tmpStr.AppendLiteral("s");
        break;
      }
    case CSS_CM :
    case CSS_MM :
    case CSS_IN :
    case CSS_PT :
    case CSS_PC :
    case CSS_UNKNOWN :
    case CSS_EMS :
    case CSS_EXS :
    case CSS_DEG :
    case CSS_RAD :
    case CSS_GRAD :
    case CSS_MS :
    case CSS_HZ :
    case CSS_KHZ :
    case CSS_DIMENSION :
      NS_ERROR("We have a bogus value set.  This should not happen");
      return NS_ERROR_DOM_INVALID_ACCESS_ERR;
  }

  if (NS_SUCCEEDED(result)) {
    aCssText.Assign(tmpStr);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsROCSSPrimitiveValue::SetCssText(const nsAString& aCssText)
{
  return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
}


NS_IMETHODIMP
nsROCSSPrimitiveValue::GetCssValueType(PRUint16* aValueType)
{
  NS_ENSURE_ARG_POINTER(aValueType);
  *aValueType = nsIDOMCSSValue::CSS_PRIMITIVE_VALUE;
  return NS_OK;
}




NS_IMETHODIMP
nsROCSSPrimitiveValue::GetPrimitiveType(PRUint16* aPrimitiveType)
{
  NS_ENSURE_ARG_POINTER(aPrimitiveType);
  *aPrimitiveType = mType;

  return NS_OK;
}


NS_IMETHODIMP
nsROCSSPrimitiveValue::SetFloatValue(PRUint16 aUnitType, float aFloatValue)
{
  return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
}


NS_IMETHODIMP
nsROCSSPrimitiveValue::GetFloatValue(PRUint16 aUnitType, float* aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = 0;

  switch(aUnitType) {
    case CSS_PX :
      if (mType != CSS_PX)
        return NS_ERROR_DOM_INVALID_ACCESS_ERR;
      *aReturn = nsPresContext::AppUnitsToFloatCSSPixels(mValue.mAppUnits);
      break;
    case CSS_CM :
      if (mType != CSS_PX)
        return NS_ERROR_DOM_INVALID_ACCESS_ERR;
      *aReturn = mValue.mAppUnits * 2.54f / float(mAppUnitsPerInch);
      break;
    case CSS_MM :
      if (mType != CSS_PX)
        return NS_ERROR_DOM_INVALID_ACCESS_ERR;
      *aReturn = mValue.mAppUnits * 25.4f / float(mAppUnitsPerInch);
      break;
    case CSS_IN :
      if (mType != CSS_PX)
        return NS_ERROR_DOM_INVALID_ACCESS_ERR;
      *aReturn = mValue.mAppUnits / float(mAppUnitsPerInch);
      break;
    case CSS_PT :
      if (mType != CSS_PX)
        return NS_ERROR_DOM_INVALID_ACCESS_ERR;
      *aReturn = mValue.mAppUnits * POINTS_PER_INCH_FLOAT / 
        float(mAppUnitsPerInch);
      break;
    case CSS_PC :
      if (mType != CSS_PX)
        return NS_ERROR_DOM_INVALID_ACCESS_ERR;
      *aReturn = mValue.mAppUnits * 6.0f / float(mAppUnitsPerInch);
      break;
    case CSS_PERCENTAGE :
      if (mType != CSS_PERCENTAGE)
        return NS_ERROR_DOM_INVALID_ACCESS_ERR;
      *aReturn = mValue.mFloat * 100;
      break;
    case CSS_NUMBER :
      if (mType != CSS_NUMBER)
        return NS_ERROR_DOM_INVALID_ACCESS_ERR;
      *aReturn = mValue.mFloat;
      break;
    case CSS_UNKNOWN :
    case CSS_EMS :
    case CSS_EXS :
    case CSS_DEG :
    case CSS_RAD :
    case CSS_GRAD :
    case CSS_MS :
    case CSS_S :
    case CSS_HZ :
    case CSS_KHZ :
    case CSS_DIMENSION :
    case CSS_STRING :
    case CSS_URI :
    case CSS_IDENT :
    case CSS_ATTR :
    case CSS_COUNTER :
    case CSS_RECT :
    case CSS_RGBCOLOR :
      return NS_ERROR_DOM_INVALID_ACCESS_ERR;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsROCSSPrimitiveValue::SetStringValue(PRUint16 aStringType,
                                      const nsAString& aStringValue)
{
  return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
}


NS_IMETHODIMP
nsROCSSPrimitiveValue::GetStringValue(nsAString& aReturn)
{
  switch (mType) {
    case CSS_IDENT:
      CopyUTF8toUTF16(nsCSSKeywords::GetStringValue(mValue.mKeyword), aReturn);
      break;
    case CSS_STRING:
    case CSS_ATTR:
      aReturn.Assign(mValue.mString);
      break;
    case CSS_URI: {
      nsCAutoString spec;
      if (mValue.mURI)
        mValue.mURI->GetSpec(spec);
      CopyUTF8toUTF16(spec, aReturn);
      } break;
    default:
      aReturn.Truncate();
      return NS_ERROR_DOM_INVALID_ACCESS_ERR;
  }
  return NS_OK;
}


NS_IMETHODIMP
nsROCSSPrimitiveValue::GetCounterValue(nsIDOMCounter** aReturn)
{
  return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
}


NS_IMETHODIMP
nsROCSSPrimitiveValue::GetRectValue(nsIDOMRect** aReturn)
{
  if (mType != CSS_RECT) {
    *aReturn = nsnull;
    return NS_ERROR_DOM_INVALID_ACCESS_ERR;
  }
  NS_ASSERTION(mValue.mRect, "mValue.mRect should never be null");
  return CallQueryInterface(mValue.mRect, aReturn);
}


NS_IMETHODIMP 
nsROCSSPrimitiveValue::GetRGBColorValue(nsIDOMRGBColor** aReturn)
{
  if (mType != CSS_RGBCOLOR) {
    *aReturn = nsnull;
    return NS_ERROR_DOM_INVALID_ACCESS_ERR;
  }
  NS_ASSERTION(mValue.mColor, "mValue.mColor should never be null");
  return CallQueryInterface(mValue.mColor, aReturn);
}

