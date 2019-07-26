




#include "ChangeAttributeTxn.h"
#include "nsAString.h"
#include "nsDebug.h"                    
#include "nsError.h"                    
#include "nsEditor.h"                   
#include "nsString.h"                   
#include "mozilla/dom/Element.h"

using namespace mozilla;

ChangeAttributeTxn::ChangeAttributeTxn()
  : EditTxn()
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(ChangeAttributeTxn, EditTxn,
                                   mElement)

NS_IMPL_ADDREF_INHERITED(ChangeAttributeTxn, EditTxn)
NS_IMPL_RELEASE_INHERITED(ChangeAttributeTxn, EditTxn)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(ChangeAttributeTxn)
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

NS_IMETHODIMP ChangeAttributeTxn::Init(nsEditor      *aEditor,
                                       dom::Element *aElement,
                                       const nsAString& aAttribute,
                                       const nsAString& aValue,
                                       bool aRemoveAttribute)
{
  NS_ASSERTION(aEditor && aElement, "bad arg");
  if (!aEditor || !aElement) { return NS_ERROR_NULL_POINTER; }

  mEditor = aEditor;
  mElement = aElement;
  mAttribute = aAttribute;
  mValue = aValue;
  mRemoveAttribute = aRemoveAttribute;
  mAttributeWasSet=false;
  mUndoValue.Truncate();
  return NS_OK;
}

NS_IMETHODIMP ChangeAttributeTxn::DoTransaction(void)
{
  NS_ASSERTION(mEditor && mElement, "bad state");
  if (!mEditor || !mElement) { return NS_ERROR_NOT_INITIALIZED; }

  nsCOMPtr<nsIDOMElement> element = do_QueryInterface(mElement);
  
  nsresult result = mEditor->GetAttributeValue(element, mAttribute, mUndoValue, &mAttributeWasSet);
  NS_ENSURE_SUCCESS(result, result);

  
  if (!mUndoValue.IsEmpty())
    mAttributeWasSet = true;
  

  ErrorResult rv;
  
  if (!mRemoveAttribute)
    mElement->SetAttribute(mAttribute, mValue, rv);
  else
    mElement->RemoveAttribute(mAttribute, rv);

  return rv.ErrorCode();
}

NS_IMETHODIMP ChangeAttributeTxn::UndoTransaction(void)
{
  NS_ASSERTION(mEditor && mElement, "bad state");
  if (!mEditor || !mElement) { return NS_ERROR_NOT_INITIALIZED; }

  ErrorResult rv;
  if (mAttributeWasSet)
    mElement->SetAttribute(mAttribute, mUndoValue, rv);
  else
    mElement->RemoveAttribute(mAttribute, rv);

  return rv.ErrorCode();
}

NS_IMETHODIMP ChangeAttributeTxn::RedoTransaction(void)
{
  NS_ASSERTION(mEditor && mElement, "bad state");
  if (!mEditor || !mElement) { return NS_ERROR_NOT_INITIALIZED; }

  ErrorResult rv;
  if (!mRemoveAttribute)
    mElement->SetAttribute(mAttribute, mValue, rv);
  else
    mElement->RemoveAttribute(mAttribute, rv);

  return rv.ErrorCode();
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
