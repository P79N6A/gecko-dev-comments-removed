






































#include "nsDOMCSSValueList.h"
#include "nsCOMPtr.h"
#include "nsDOMError.h"
#include "prtypes.h"
#include "nsContentUtils.h"

nsDOMCSSValueList::nsDOMCSSValueList(PRBool aCommaDelimited, PRBool aReadonly)
  : mCommaDelimited(aCommaDelimited), mReadonly(aReadonly)
{
}

nsDOMCSSValueList::~nsDOMCSSValueList()
{
}

NS_IMPL_ADDREF(nsDOMCSSValueList)
NS_IMPL_RELEASE(nsDOMCSSValueList)


NS_INTERFACE_MAP_BEGIN(nsDOMCSSValueList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSValueList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMCSSValue)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(CSSValueList)
NS_INTERFACE_MAP_END

PRBool
nsDOMCSSValueList::AppendCSSValue(nsIDOMCSSValue* aValue)
{
  return mCSSValues.AppendObject(aValue);
}



NS_IMETHODIMP
nsDOMCSSValueList::GetLength(PRUint32* aLength)
{
  *aLength = mCSSValues.Count();

  return NS_OK;
}

NS_IMETHODIMP
nsDOMCSSValueList::Item(PRUint32 aIndex, nsIDOMCSSValue **aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);

  *aReturn = mCSSValues[aIndex];
  NS_IF_ADDREF(*aReturn);

  return NS_OK;
}



NS_IMETHODIMP
nsDOMCSSValueList::GetCssText(nsAString& aCssText)
{
  aCssText.Truncate();

  PRUint32 count = mCSSValues.Count();

  nsAutoString separator;
  if (mCommaDelimited) {
    separator.AssignLiteral(", ");
  }
  else {
    separator.Assign(PRUnichar(' '));
  }

  nsCOMPtr<nsIDOMCSSValue> cssValue;
  nsAutoString tmpStr;
  for (PRUint32 i = 0; i < count; ++i) {
    cssValue = mCSSValues[i];
    NS_ASSERTION(cssValue, "Eek!  Someone filled the value list with null CSSValues!");
    if (cssValue) {
      cssValue->GetCssText(tmpStr);

      if (tmpStr.IsEmpty()) {

#ifdef DEBUG_caillon
        NS_ERROR("Eek!  An empty CSSValue!  Bad!");
#endif

        continue;
      }

      
      
      if (!aCssText.IsEmpty()) {
        aCssText.Append(separator);
      }
      aCssText.Append(tmpStr);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMCSSValueList::SetCssText(const nsAString& aCssText)
{
  if (mReadonly) {
    return NS_ERROR_DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  NS_NOTYETIMPLEMENTED("Can't SetCssText yet: please write me!");
  return NS_OK;
}


NS_IMETHODIMP
nsDOMCSSValueList::GetCssValueType(PRUint16* aValueType)
{
  NS_ENSURE_ARG_POINTER(aValueType);
  *aValueType = nsIDOMCSSValue::CSS_VALUE_LIST;
  return NS_OK;
}

