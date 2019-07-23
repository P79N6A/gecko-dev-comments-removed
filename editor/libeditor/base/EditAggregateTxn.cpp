




































#include "EditAggregateTxn.h"
#include "nsCOMPtr.h"

EditAggregateTxn::EditAggregateTxn()
  : EditTxn()
{
}

NS_IMETHODIMP EditAggregateTxn::DoTransaction(void)
{
  nsresult result=NS_OK;  
  for (PRUint32 i = 0, length = mChildren.Length(); i < length; ++i)
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
  
  for (PRUint32 i = mChildren.Length(); i-- != 0; )
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
  for (PRUint32 i = 0, length = mChildren.Length(); i < length; ++i)
  {
    nsITransaction *txn = mChildren[i];
    if (!txn) { return NS_ERROR_NULL_POINTER; }
    result = txn->RedoTransaction();
    if (NS_FAILED(result))
      break;
  }
  return result;
}

NS_IMETHODIMP EditAggregateTxn::Merge(nsITransaction *aTransaction, PRBool *aDidMerge)
{
  nsresult result=NS_OK;  
  if (aDidMerge)
    *aDidMerge = PR_FALSE;
  
  
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

NS_IMETHODIMP EditAggregateTxn::GetCount(PRUint32 *aCount)
{
  if (!aCount) {
    return NS_ERROR_NULL_POINTER;
  }
  *aCount = mChildren.Length();
  return NS_OK;
}

NS_IMETHODIMP EditAggregateTxn::GetTxnAt(PRInt32 aIndex, EditTxn **aTxn)
{
  
  NS_PRECONDITION(aTxn, "null out param");

  if (!aTxn) {
    return NS_ERROR_NULL_POINTER;
  }
  *aTxn = nsnull; 
  
  PRUint32 txnCount = mChildren.Length();
  if (0>aIndex || ((PRInt32)txnCount)<=aIndex) {
    return NS_ERROR_UNEXPECTED;
  }
  
  *aTxn = mChildren[aIndex];
  if (!*aTxn)
    return NS_ERROR_UNEXPECTED;
  NS_ADDREF(*aTxn);
  return NS_OK;
}


NS_IMETHODIMP EditAggregateTxn::SetName(nsIAtom *aName)
{
  mName = do_QueryInterface(aName);
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

NS_IMETHODIMP EditAggregateTxn::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  if (!aInstancePtr) return NS_ERROR_NULL_POINTER;
 
  if (aIID.Equals(EditAggregateTxn::GetCID())) {
    *aInstancePtr = static_cast<EditAggregateTxn*>(this);
    NS_ADDREF_THIS();
    return NS_OK;
  }
  return EditTxn::QueryInterface(aIID, aInstancePtr);
}

