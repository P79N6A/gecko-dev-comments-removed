




































#include "EditAggregateTxn.h"
#include "nsCOMPtr.h"

EditAggregateTxn::EditAggregateTxn()
  : EditTxn()
{
  nsresult res = NS_NewISupportsArray(getter_AddRefs(mChildren));
  NS_POSTCONDITION(NS_SUCCEEDED(res), "EditAggregateTxn failed in constructor");
}

NS_IMETHODIMP EditAggregateTxn::DoTransaction(void)
{
  nsresult result=NS_OK;  
  if (mChildren)
  {
    PRInt32 i;
    PRUint32 count;
    mChildren->Count(&count);
    for (i=0; i<((PRInt32)count); i++)
    {
      nsCOMPtr<nsITransaction> txn (do_QueryElementAt(mChildren, i));
      if (!txn) { return NS_ERROR_NULL_POINTER; }
      result = txn->DoTransaction();
      if (NS_FAILED(result))
        break;
    }  
  }
  return result;
}

NS_IMETHODIMP EditAggregateTxn::UndoTransaction(void)
{
  nsresult result=NS_OK;  
  if (mChildren)
  {
    PRInt32 i;
    PRUint32 count;
    mChildren->Count(&count);
    
    for (i=count-1; i>=0; i--)
    {
      nsCOMPtr<nsITransaction> txn (do_QueryElementAt(mChildren, i));
      if (!txn) { return NS_ERROR_NULL_POINTER; }
      result = txn->UndoTransaction();
      if (NS_FAILED(result))
        break;
    }  
  }
  return result;
}

NS_IMETHODIMP EditAggregateTxn::RedoTransaction(void)
{
  nsresult result=NS_OK;  
  if (mChildren)
  {
    PRInt32 i;
    PRUint32 count;
    mChildren->Count(&count);
    for (i=0; i<((PRInt32)count); i++)
    {
      nsCOMPtr<nsITransaction> txn (do_QueryElementAt(mChildren, i));
      if (!txn) { return NS_ERROR_NULL_POINTER; }
      result = txn->RedoTransaction();
      if (NS_FAILED(result))
        break;
    }  
  }
  return result;
}

NS_IMETHODIMP EditAggregateTxn::Merge(nsITransaction *aTransaction, PRBool *aDidMerge)
{
  nsresult result=NS_OK;  
  if (aDidMerge)
    *aDidMerge = PR_FALSE;
  if (mChildren)
  {
    PRInt32 i=0;
    PRUint32 count;
    mChildren->Count(&count);
    NS_ASSERTION(count>0, "bad count");
    if (0<count)
    {
      nsCOMPtr<nsITransaction> txn (do_QueryElementAt(mChildren, i));
      if (!txn) { return NS_ERROR_NULL_POINTER; }
      result = txn->Merge(aTransaction, aDidMerge);
    }
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
  if (mChildren && aTxn)
  {
    
    nsCOMPtr<nsISupports> isupports;
    aTxn->QueryInterface(NS_GET_IID(nsISupports), getter_AddRefs(isupports));
    mChildren->AppendElement(isupports);
    return NS_OK;
  }
  return NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP EditAggregateTxn::GetCount(PRUint32 *aCount)
{
  if (!aCount) {
    return NS_ERROR_NULL_POINTER;
  }
  *aCount=0;
  if (mChildren) {
    mChildren->Count(aCount);
  }
  return NS_OK;
}

NS_IMETHODIMP EditAggregateTxn::GetTxnAt(PRInt32 aIndex, EditTxn **aTxn)
{
  
  NS_PRECONDITION(aTxn, "null out param");
  NS_PRECONDITION(mChildren, "bad internal state");

  if (!aTxn) {
    return NS_ERROR_NULL_POINTER;
  }
  *aTxn = nsnull; 
  if (!mChildren) {
    return NS_ERROR_UNEXPECTED;
  }

  
  PRUint32 txnCount;
  mChildren->Count(&txnCount);
  if (0>aIndex || ((PRInt32)txnCount)<=aIndex) {
    return NS_ERROR_UNEXPECTED;
  }
  
  mChildren->QueryElementAt(aIndex, EditTxn::GetCID(), (void**)aTxn);
  if (!*aTxn)
    return NS_ERROR_UNEXPECTED;
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

