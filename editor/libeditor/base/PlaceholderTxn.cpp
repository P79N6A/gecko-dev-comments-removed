





































#include "PlaceholderTxn.h"
#include "nsEditor.h"
#include "IMETextTxn.h"

PlaceholderTxn::PlaceholderTxn() :  EditAggregateTxn(), 
                                    mAbsorb(PR_TRUE), 
                                    mForwarding(nsnull),
                                    mIMETextTxn(nsnull),
                                    mCommitted(PR_FALSE),
                                    mStartSel(nsnull),
                                    mEndSel(),
                                    mEditor(nsnull)
{
}


NS_IMPL_ISUPPORTS_INHERITED2(PlaceholderTxn, EditAggregateTxn,
                             nsIAbsorbingTransaction, nsISupportsWeakReference)

NS_IMETHODIMP PlaceholderTxn::Init(nsIAtom *aName, nsSelectionState *aSelState, nsIEditor *aEditor)
{
  if (!aEditor || !aSelState) return NS_ERROR_NULL_POINTER;

  mName = aName;
  mStartSel = aSelState;
  mEditor = aEditor;
  return NS_OK;
}

NS_IMETHODIMP PlaceholderTxn::DoTransaction(void)
{
  return NS_OK;
}

NS_IMETHODIMP PlaceholderTxn::UndoTransaction(void)
{
  
  nsresult res = EditAggregateTxn::UndoTransaction();
  if (NS_FAILED(res)) return res;
  
  if (!mStartSel) return NS_ERROR_NULL_POINTER;

  
  nsCOMPtr<nsISelection> selection;
  res = mEditor->GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  if (!selection) return NS_ERROR_NULL_POINTER;
  return mStartSel->RestoreSelection(selection);
}


NS_IMETHODIMP PlaceholderTxn::RedoTransaction(void)
{
  
  nsresult res = EditAggregateTxn::RedoTransaction();
  if (NS_FAILED(res)) return res;
  
  
  nsCOMPtr<nsISelection> selection;
  res = mEditor->GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  if (!selection) return NS_ERROR_NULL_POINTER;
  return mEndSel.RestoreSelection(selection);
}


NS_IMETHODIMP PlaceholderTxn::Merge(nsITransaction *aTransaction, PRBool *aDidMerge)
{
  if (!aDidMerge || !aTransaction) return NS_ERROR_NULL_POINTER;

  
  *aDidMerge=PR_FALSE;
    
  if (mForwarding) 
  {
    NS_NOTREACHED("tried to merge into a placeholder that was in forwarding mode!");
    return NS_ERROR_FAILURE;
  }

  
  
  
  

  nsCOMPtr<nsPIEditorTransaction> pTxn = do_QueryInterface(aTransaction);
  if (!pTxn) return NS_OK; 

  EditTxn *editTxn = (EditTxn*)aTransaction;  
  
  nsCOMPtr<nsIAbsorbingTransaction> plcTxn;
  
  
  editTxn->QueryInterface(NS_GET_IID(nsIAbsorbingTransaction), getter_AddRefs(plcTxn));

  
  if (mAbsorb)
  { 
    IMETextTxn*  otherTxn = nsnull;
    if (NS_SUCCEEDED(aTransaction->QueryInterface(IMETextTxn::GetCID(),(void**)&otherTxn)) && otherTxn)
    {
      
      
      if (!mIMETextTxn) 
      {
        
        mIMETextTxn =otherTxn;
        AppendChild(editTxn);
      }
      else  
      {
        PRBool didMerge;
        mIMETextTxn->Merge(otherTxn, &didMerge);
        if (!didMerge)
        {
          
          
          
          mIMETextTxn =otherTxn;
          AppendChild(editTxn);
        }
      }
      NS_IF_RELEASE(otherTxn);
    }
    else if (!plcTxn)  
    {                  
      AppendChild(editTxn);
    }
    *aDidMerge = PR_TRUE;




  }
  else
  { 
    if (((mName.get() == nsEditor::gTypingTxnName) ||
         (mName.get() == nsEditor::gIMETxnName)    ||
         (mName.get() == nsEditor::gDeleteTxnName)) 
         && !mCommitted ) 
    {
      nsCOMPtr<nsIAbsorbingTransaction> plcTxn;
      
      
      editTxn->QueryInterface(NS_GET_IID(nsIAbsorbingTransaction), getter_AddRefs(plcTxn));
      if (plcTxn)
      {
        nsCOMPtr<nsIAtom> atom;
        plcTxn->GetTxnName(getter_AddRefs(atom));
        if (atom && (atom == mName))
        {
          
          
          PRBool isSame;
          plcTxn->StartSelectionEquals(&mEndSel, &isSame);
          if (isSame)
          {
            mAbsorb = PR_TRUE;  
            plcTxn->ForwardEndBatchTo(this);
            
            
            
            
            
            RememberEndingSelection();
            *aDidMerge = PR_TRUE;
          }
        }
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP PlaceholderTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("PlaceholderTxn: ");

  if (mName)
  {
    nsAutoString name;
    mName->ToString(name);
    aString += name;
  }

  return NS_OK;
}

NS_IMETHODIMP PlaceholderTxn::GetTxnName(nsIAtom **aName)
{
  return GetName(aName);
}

NS_IMETHODIMP PlaceholderTxn::StartSelectionEquals(nsSelectionState *aSelState, PRBool *aResult)
{
  
  
  if (!aResult || !aSelState) return NS_ERROR_NULL_POINTER;
  if (!mStartSel->IsCollapsed() || !aSelState->IsCollapsed())
  {
    *aResult = PR_FALSE;
    return NS_OK;
  }
  *aResult = mStartSel->IsEqual(aSelState);
  return NS_OK;
}

NS_IMETHODIMP PlaceholderTxn::EndPlaceHolderBatch()
{
  mAbsorb = PR_FALSE;
  
  if (mForwarding) 
  {
    nsCOMPtr<nsIAbsorbingTransaction> plcTxn = do_QueryReferent(mForwarding);
    if (plcTxn) plcTxn->EndPlaceHolderBatch();
  }
  
  
  return RememberEndingSelection();
}

NS_IMETHODIMP PlaceholderTxn::ForwardEndBatchTo(nsIAbsorbingTransaction *aForwardingAddress)
{   
  mForwarding = do_GetWeakReference(aForwardingAddress);
  return NS_OK;
}

NS_IMETHODIMP PlaceholderTxn::Commit()
{
  mCommitted = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP PlaceholderTxn::RememberEndingSelection()
{
  nsCOMPtr<nsISelection> selection;
  nsresult res = mEditor->GetSelection(getter_AddRefs(selection));
  if (NS_FAILED(res)) return res;
  if (!selection) return NS_ERROR_NULL_POINTER;
  return mEndSel.SaveSelection(selection);
}

