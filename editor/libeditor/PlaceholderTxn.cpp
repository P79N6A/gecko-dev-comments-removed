




#include "PlaceholderTxn.h"
#include "nsEditor.h"
#include "IMETextTxn.h"
#include "nsGkAtoms.h"
#include "mozilla/dom/Selection.h"
#include "nsQueryObject.h"

using namespace mozilla;
using namespace mozilla::dom;

PlaceholderTxn::PlaceholderTxn() :  EditAggregateTxn(), 
                                    mAbsorb(true), 
                                    mForwarding(nullptr),
                                    mIMETextTxn(nullptr),
                                    mCommitted(false),
                                    mStartSel(nullptr),
                                    mEndSel(),
                                    mEditor(nullptr)
{
}

PlaceholderTxn::~PlaceholderTxn()
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

NS_IMETHODIMP
PlaceholderTxn::Init(nsIAtom* aName, nsSelectionState* aSelState,
                     nsEditor* aEditor)
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

  
  nsRefPtr<Selection> selection = mEditor->GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  return mStartSel->RestoreSelection(selection);
}


NS_IMETHODIMP PlaceholderTxn::RedoTransaction(void)
{
  
  nsresult res = EditAggregateTxn::RedoTransaction();
  NS_ENSURE_SUCCESS(res, res);
  
  
  nsRefPtr<Selection> selection = mEditor->GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  return mEndSel.RestoreSelection(selection);
}


NS_IMETHODIMP PlaceholderTxn::Merge(nsITransaction *aTransaction, bool *aDidMerge)
{
  NS_ENSURE_TRUE(aDidMerge && aTransaction, NS_ERROR_NULL_POINTER);

  
  *aDidMerge=false;
    
  if (mForwarding) 
  {
    NS_NOTREACHED("tried to merge into a placeholder that was in forwarding mode!");
    return NS_ERROR_FAILURE;
  }

  
  
  
  

  nsCOMPtr<nsPIEditorTransaction> pTxn = do_QueryInterface(aTransaction);
  NS_ENSURE_TRUE(pTxn, NS_OK); 

  EditTxn *editTxn = (EditTxn*)aTransaction;  
  
  nsCOMPtr<nsIAbsorbingTransaction> plcTxn = do_QueryObject(editTxn);

  
  if (mAbsorb)
  { 
    nsRefPtr<IMETextTxn> otherTxn = do_QueryObject(aTransaction);
    if (otherTxn) {
      
      
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
    *aDidMerge = true;




  }
  else
  { 
    if (((mName.get() == nsGkAtoms::TypingTxnName) ||
         (mName.get() == nsGkAtoms::IMETxnName)    ||
         (mName.get() == nsGkAtoms::DeleteTxnName)) 
         && !mCommitted ) 
    {
      nsCOMPtr<nsIAbsorbingTransaction> plcTxn = do_QueryObject(editTxn);
      if (plcTxn) {
        nsCOMPtr<nsIAtom> atom;
        plcTxn->GetTxnName(getter_AddRefs(atom));
        if (atom && (atom == mName))
        {
          
          
          bool isSame;
          plcTxn->StartSelectionEquals(&mEndSel, &isSame);
          if (isSame)
          {
            mAbsorb = true;  
            plcTxn->ForwardEndBatchTo(this);
            
            
            
            
            
            RememberEndingSelection();
            *aDidMerge = true;
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
    *aResult = false;
    return NS_OK;
  }
  *aResult = mStartSel->IsEqual(aSelState);
  return NS_OK;
}

NS_IMETHODIMP PlaceholderTxn::EndPlaceHolderBatch()
{
  mAbsorb = false;
  
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
  mCommitted = true;
  return NS_OK;
}

nsresult PlaceholderTxn::RememberEndingSelection()
{
  nsRefPtr<Selection> selection = mEditor->GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
  mEndSel.SaveSelection(selection);
  return NS_OK;
}

