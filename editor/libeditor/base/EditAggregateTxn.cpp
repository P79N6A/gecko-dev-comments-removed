




#include "EditAggregateTxn.h"
#include "nsAString.h"
#include "nsCOMPtr.h"                   
#include "nsError.h"                    
#include "nsISupportsUtils.h"           
#include "nsITransaction.h"             
#include "nsString.h"                   

EditAggregateTxn::EditAggregateTxn()
  : EditTxn()
{
}

EditAggregateTxn::~EditAggregateTxn()
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(EditAggregateTxn, EditTxn,
                                   mChildren)

NS_IMPL_ADDREF_INHERITED(EditAggregateTxn, EditTxn)
NS_IMPL_RELEASE_INHERITED(EditAggregateTxn, EditTxn)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(EditAggregateTxn)
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

NS_IMETHODIMP EditAggregateTxn::DoTransaction(void)
{
  nsresult result=NS_OK;  
  for (uint32_t i = 0, length = mChildren.Length(); i < length; ++i)
  {
    nsITransaction *txn = mChildren[i];
    if (!txn) { return NS_ERROR_NULL_POINTER; }
    result = txn->DoTransaction();
    if (NS_FAILED(result))
      break;
  }
  return result;
}

NS_IMETHODIMP EditAggregateTxn::UndoTransaction(void)
{
  nsresult result=NS_OK;  
  
  for (uint32_t i = mChildren.Length(); i-- != 0; )
  {
    nsITransaction *txn = mChildren[i];
    if (!txn) { return NS_ERROR_NULL_POINTER; }
    result = txn->UndoTransaction();
    if (NS_FAILED(result))
      break;
  }
  return result;
}

NS_IMETHODIMP EditAggregateTxn::RedoTransaction(void)
{
  nsresult result=NS_OK;  
  for (uint32_t i = 0, length = mChildren.Length(); i < length; ++i)
  {
    nsITransaction *txn = mChildren[i];
    if (!txn) { return NS_ERROR_NULL_POINTER; }
    result = txn->RedoTransaction();
    if (NS_FAILED(result))
      break;
  }
  return result;
}

NS_IMETHODIMP EditAggregateTxn::Merge(nsITransaction *aTransaction, bool *aDidMerge)
{
  nsresult result=NS_OK;  
  if (aDidMerge)
    *aDidMerge = false;
  
  
  if (mChildren.Length() > 0)
  {
    nsITransaction *txn = mChildren[0];
    if (!txn) { return NS_ERROR_NULL_POINTER; }
    result = txn->Merge(aTransaction, aDidMerge);
  }
  return result;
}

NS_IMETHODIMP EditAggregateTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("EditAggregateTxn: ");

  if (mName)
  {
    nsAutoString name;
    mName->ToString(name);
    aString += name;
  }

  return NS_OK;
}

NS_IMETHODIMP EditAggregateTxn::AppendChild(EditTxn *aTxn)
{
  if (!aTxn) {
    return NS_ERROR_NULL_POINTER;
  }

  nsRefPtr<EditTxn> *slot = mChildren.AppendElement();
  if (!slot) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  *slot = aTxn;
  return NS_OK;
}

NS_IMETHODIMP EditAggregateTxn::GetName(nsIAtom **aName)
{
  if (aName && mName)
  {
    *aName = mName;
    NS_ADDREF(*aName);
    return NS_OK;
  }
  return NS_ERROR_NULL_POINTER;
}
