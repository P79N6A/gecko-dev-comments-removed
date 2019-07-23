




































#include "ChangeAttributeTxn.h"
#include "nsIDOMElement.h"

ChangeAttributeTxn::ChangeAttributeTxn()
  : EditTxn()
{
}

NS_IMETHODIMP ChangeAttributeTxn::Init(nsIEditor      *aEditor,
                                       nsIDOMElement  *aElement,
                                       const nsAString& aAttribute,
                                       const nsAString& aValue,
                                       PRBool aRemoveAttribute)
{
  NS_ASSERTION(aEditor && aElement, "bad arg");
  if (!aEditor || !aElement) { return NS_ERROR_NULL_POINTER; }

  mEditor = aEditor;
  mElement = do_QueryInterface(aElement);
  mAttribute = aAttribute;
  mValue = aValue;
  mRemoveAttribute = aRemoveAttribute;
  mAttributeWasSet=PR_FALSE;
  mUndoValue.Truncate();
  return NS_OK;
}

NS_IMETHODIMP ChangeAttributeTxn::DoTransaction(void)
{
  NS_ASSERTION(mEditor && mElement, "bad state");
  if (!mEditor || !mElement) { return NS_ERROR_NOT_INITIALIZED; }

  
  nsresult result = mEditor->GetAttributeValue(mElement, mAttribute, mUndoValue, &mAttributeWasSet);
  
  if (!mUndoValue.IsEmpty())
    mAttributeWasSet = PR_TRUE;
  
  
  
  if (!mRemoveAttribute)
    result = mElement->SetAttribute(mAttribute, mValue);
  else
    result = mElement->RemoveAttribute(mAttribute);

  return result;
}

NS_IMETHODIMP ChangeAttributeTxn::UndoTransaction(void)
{
  NS_ASSERTION(mEditor && mElement, "bad state");
  if (!mEditor || !mElement) { return NS_ERROR_NOT_INITIALIZED; }

  nsresult result;
  if (mAttributeWasSet)
    result = mElement->SetAttribute(mAttribute, mUndoValue);
  else
    result = mElement->RemoveAttribute(mAttribute);

  return result;
}

NS_IMETHODIMP ChangeAttributeTxn::RedoTransaction(void)
{
  NS_ASSERTION(mEditor && mElement, "bad state");
  if (!mEditor || !mElement) { return NS_ERROR_NOT_INITIALIZED; }

  nsresult result;
  if (!mRemoveAttribute)
    result = mElement->SetAttribute(mAttribute, mValue);
  else
    result = mElement->RemoveAttribute(mAttribute);

  return result;
}

NS_IMETHODIMP ChangeAttributeTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("ChangeAttributeTxn: [mRemoveAttribute == ");

  if (!mRemoveAttribute)
    aString.AppendLiteral("false] ");
  else
    aString.AppendLiteral("true] ");
  aString += mAttribute;
  return NS_OK;
}
