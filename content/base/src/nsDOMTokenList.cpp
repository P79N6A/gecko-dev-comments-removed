







































#include "nsDOMTokenList.h"

#include "nsAttrValue.h"
#include "nsContentUtils.h"
#include "nsDOMError.h"
#include "nsGenericElement.h"
#include "nsHashSets.h"


nsDOMTokenList::nsDOMTokenList(nsGenericElement *aElement, nsIAtom* aAttrAtom)
  : mElement(aElement),
    mAttrAtom(aAttrAtom)
{
  
  
}

nsDOMTokenList::~nsDOMTokenList() { }

NS_INTERFACE_TABLE_HEAD(nsDOMTokenList)
  NS_INTERFACE_TABLE1(nsDOMTokenList,
                      nsIDOMDOMTokenList)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(DOMTokenList)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsDOMTokenList)
NS_IMPL_RELEASE(nsDOMTokenList)

void
nsDOMTokenList::DropReference()
{
  mElement = nsnull;
}

NS_IMETHODIMP
nsDOMTokenList::GetLength(PRUint32 *aLength)
{
  const nsAttrValue* attr = GetParsedAttr();
  if (!attr) {
    *aLength = 0;
    return NS_OK;
  }

  *aLength = attr->GetAtomCount();

  return NS_OK;
}

NS_IMETHODIMP
nsDOMTokenList::Item(PRUint32 aIndex, nsAString& aResult)
{
  const nsAttrValue* attr = GetParsedAttr();

  if (!attr || aIndex >= static_cast<PRUint32>(attr->GetAtomCount())) {
    SetDOMStringToNull(aResult);
    return NS_OK;
  }
  attr->AtomAt(aIndex)->ToString(aResult);

  return NS_OK;
}

nsresult
nsDOMTokenList::CheckToken(const nsAString& aStr)
{
  if (aStr.IsEmpty()) {
    return NS_ERROR_DOM_SYNTAX_ERR;
  }

  nsAString::const_iterator iter, end;
  aStr.BeginReading(iter);
  aStr.EndReading(end);

  while (iter != end) {
    if (nsContentUtils::IsHTMLWhitespace(*iter))
      return NS_ERROR_DOM_INVALID_CHARACTER_ERR;
    ++iter;
  }

  return NS_OK;
}

PRBool
nsDOMTokenList::ContainsInternal(const nsAttrValue* aAttr,
                                 const nsAString& aToken)
{
  NS_ABORT_IF_FALSE(aAttr, "Need an attribute");

  nsCOMPtr<nsIAtom> atom = do_GetAtom(aToken);
  return aAttr->Contains(atom, eCaseMatters);
}

NS_IMETHODIMP
nsDOMTokenList::Contains(const nsAString& aToken, PRBool* aResult)
{
  nsresult rv = CheckToken(aToken);
  NS_ENSURE_SUCCESS(rv, rv);

  const nsAttrValue* attr = GetParsedAttr();
  if (!attr) {
    *aResult = PR_FALSE;
    return NS_OK;
  }

  *aResult = ContainsInternal(attr, aToken);

  return NS_OK;
}

void
nsDOMTokenList::AddInternal(const nsAttrValue* aAttr,
                            const nsAString& aToken)
{
  nsAutoString resultStr;

  if (aAttr) {
    aAttr->ToString(resultStr);
  }

  if (!resultStr.IsEmpty() &&
      !nsContentUtils::IsHTMLWhitespace(
          resultStr.CharAt(resultStr.Length() - 1))) {
    resultStr.Append(NS_LITERAL_STRING(" ") + aToken);
  } else {
    resultStr.Append(aToken);
  }
  mElement->SetAttr(kNameSpaceID_None, mAttrAtom, resultStr, PR_TRUE);
}

NS_IMETHODIMP
nsDOMTokenList::Add(const nsAString& aToken)
{
  nsresult rv = CheckToken(aToken);
  NS_ENSURE_SUCCESS(rv, rv);

  const nsAttrValue* attr = GetParsedAttr();

  if (attr && ContainsInternal(attr, aToken)) {
    return NS_OK;
  }

  AddInternal(attr, aToken);

  return NS_OK;
}

void
nsDOMTokenList::RemoveInternal(const nsAttrValue* aAttr,
                               const nsAString& aToken)
{
  NS_ABORT_IF_FALSE(aAttr, "Need an attribute");

  nsAutoString input;
  aAttr->ToString(input);

  nsAString::const_iterator copyStart, tokenStart, iter, end;
  input.BeginReading(iter);
  input.EndReading(end);
  copyStart = iter;

  nsAutoString output;
  PRBool lastTokenRemoved = PR_FALSE;

  while (iter != end) {
    
    while (iter != end && nsContentUtils::IsHTMLWhitespace(*iter)) {
      ++iter;
    }

    if (iter == end) {
      
      
      NS_ABORT_IF_FALSE(!lastTokenRemoved, "How did this happen?");

      output.Append(Substring(copyStart, end));
      break;
    }

    tokenStart = iter;
    do {
      ++iter;
    } while (iter != end && !nsContentUtils::IsHTMLWhitespace(*iter));

    if (Substring(tokenStart, iter).Equals(aToken)) {

      
      while (iter != end && nsContentUtils::IsHTMLWhitespace(*iter)) {
        ++iter;
      }
      copyStart = iter;
      lastTokenRemoved = PR_TRUE;

    } else {

      if (lastTokenRemoved && !output.IsEmpty()) {
        NS_ABORT_IF_FALSE(!nsContentUtils::IsHTMLWhitespace(
          output.CharAt(output.Length() - 1)), "Invalid last output token");
        output.Append(PRUnichar(' '));
      }
      lastTokenRemoved = PR_FALSE;
      output.Append(Substring(copyStart, iter));
      copyStart = iter;
    }
  }

  mElement->SetAttr(kNameSpaceID_None, mAttrAtom, output, PR_TRUE);
}

NS_IMETHODIMP
nsDOMTokenList::Remove(const nsAString& aToken)
{
  nsresult rv = CheckToken(aToken);
  NS_ENSURE_SUCCESS(rv, rv);

  const nsAttrValue* attr = GetParsedAttr();
  if (!attr) {
    return NS_OK;
  }

  if (!ContainsInternal(attr, aToken)) {
    return NS_OK;
  }

  RemoveInternal(attr, aToken);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMTokenList::Toggle(const nsAString& aToken, PRBool* aResult)
{
  nsresult rv = CheckToken(aToken);
  NS_ENSURE_SUCCESS(rv, rv);

  const nsAttrValue* attr = GetParsedAttr();

  if (attr && ContainsInternal(attr, aToken)) {
    RemoveInternal(attr, aToken);
    *aResult = PR_FALSE;
  } else {
    AddInternal(attr, aToken);
    *aResult = PR_TRUE;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMTokenList::ToString(nsAString& aResult)
{
  if (!mElement) {
    aResult.Truncate();
    return NS_OK;
  }

  mElement->GetAttr(kNameSpaceID_None, mAttrAtom, aResult);

  return NS_OK;
}
