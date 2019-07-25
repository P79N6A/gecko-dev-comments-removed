





































#include "PlaceholderTxn.h"
#include "nsEditor.h"
#include "IMETextTxn.h"
#include "nsGkAtoms.h"

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

NS_IMPL_CYCLE_COLLECTION_CLASS(PlaceholderTxn)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(PlaceholderTxn,
                                                EditAggregateTxn)
  tmp->mStartSel->DoUnlink();
  tmp->mEndSel.DoUnlink();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(PlaceholderTxn,
                                                  EditAggregateTxn)
  tmp->mStartSel->DoTraverse(cb);
  tmp->mEndSel.DoTraverse(cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(PlaceholderTxn)
  NS_INTERFACE_MAP_ENTRY(nsIAbsorbingTransaction)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END_INHERITING(EditAggregateTxn)

NS_IMPL_ADDREF_INHERITED(PlaceholderTxn, EditAggregateTxn)
NS_IMPL_RELEASE_INHERITED(PlaceholderTxn, EditAggregateTxn)

NS_IMETHODIMP PlaceholderTxn::Init(nsIAtom *aName, nsSelectionState *aSelState, nsIEditor *aEditor)
{
  NS_ENSURE_TRUE(aEditor && aSelState, NS_ERROR_NULL_POINTER);

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
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_TRUE(mStartSel, NS_ERROR_NULL_POINTER);

  
  nsCOMPtr<nsISelection> selection;
  res = mEditor->GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  return mStartSel->RestoreSelection(selection);
}


NS_IMETHODIMP PlaceholderTxn::RedoTransaction(void)
{
  
  nsresult res = EditAggregateTxn::RedoTransaction();
  NS_ENSURE_SUCCESS(res, res);
  
  
  nsCOMPtr<nsISelection> selection;
  res = mEditor->GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  return mEndSel.RestoreSelection(selection);
}


NS_IMETHODIMP PlaceholderTxn::Merge(nsITransaction *aTransaction, bool *aDidMerge)
{
  NS_ENSURE_TRUE(aDidMerge && aTransaction, NS_ERROR_NULL_POINTER);

  
  *aDidMerge=PR_FALSE;
    
  if (mForwarding) 
  {
    NS_NOTREACHED("tried to merge into a placeholder that was in forwarding mode!");
    return NS_ERROR_FAILURE;
  }

  
  
  
  

  nsCOMPtr<nsPIEditorTransaction> pTxn = do_QueryInterface(aTransaction);
  NS_ENSURE_TRUE(pTxn, NS_OK); 

  EditTxn *editTxn = (EditTxn*)aTransaction;  
  
  nsCOMPtr<nsIAbsorbingTransaction> plcTxn;
  
  
  editTxn->QueryInterface(NS_GET_IID(nsIAbsorbingTransaction), getter_AddRefs(plcTxn));

  
  if (mAbsorb)
  { 
    nsRefPtr<IMETextTxn> otherTxn;
    if (NS_SUCCEEDED(aTransaction->QueryInterface(IMETextTxn::GetCID(), getter_AddRefs(otherTxn))) && otherTxn)
    {
      
      
      if (!mIMETextTxn) 
      {
        
        mIMETextTxn =otherTxn;
        AppendChild(editTxn);
      }
      else  
      {
        bool didMerge;
        mIMETextTxn->Merge(otherTxn, &didMerge);
        if (!didMerge)
        {
          
          
          
          mIMETextTxn =otherTxn;
          AppendChild(editTxn);
        }
      }
    }
    else if (!plcTxn)  
    {                  
      AppendChild(editTxn);
    }
    *aDidMerge = PR_TRUE;




  }
  else
  { 
    if (((mName.get() == nsGkAtoms::TypingTxnName) ||
         (mName.get() == nsGkAtoms::IMETxnName)    ||
         (mName.get() == nsGkAtoms::DeleteTxnName)) 
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
          
          
          bool isSame;
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

NS_IMETHODIMP PlaceholderTxn::StartSelectionEquals(nsSelectionState *aSelState, bool *aResult)
{
  
  
  NS_ENSURE_TRUE(aResult && aSelState, NS_ERROR_NULL_POINTER);
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
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  return mEndSel.SaveSelection(selection);
}

