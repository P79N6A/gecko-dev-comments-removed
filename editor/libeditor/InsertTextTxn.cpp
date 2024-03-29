




#include "InsertTextTxn.h"

#include "mozilla/dom/Selection.h"      
#include "mozilla/dom/Text.h"           
#include "nsAString.h"                  
#include "nsDebug.h"                    
#include "nsEditor.h"                   
#include "nsError.h"                    
#include "nsQueryObject.h"              

using namespace mozilla;
using namespace mozilla::dom;

class nsITransaction;

InsertTextTxn::InsertTextTxn(Text& aTextNode, uint32_t aOffset,
                             const nsAString& aStringToInsert,
                             nsEditor& aEditor)
  : EditTxn()
  , mTextNode(&aTextNode)
  , mOffset(aOffset)
  , mStringToInsert(aStringToInsert)
  , mEditor(aEditor)
{
}

InsertTextTxn::~InsertTextTxn()
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(InsertTextTxn, EditTxn,
                                   mTextNode)

NS_IMPL_ADDREF_INHERITED(InsertTextTxn, EditTxn)
NS_IMPL_RELEASE_INHERITED(InsertTextTxn, EditTxn)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(InsertTextTxn)
  if (aIID.Equals(NS_GET_IID(InsertTextTxn))) {
    foundInterface = static_cast<nsITransaction*>(this);
  } else
NS_INTERFACE_MAP_END_INHERITING(EditTxn)


NS_IMETHODIMP
InsertTextTxn::DoTransaction()
{
  nsresult res = mTextNode->InsertData(mOffset, mStringToInsert);
  NS_ENSURE_SUCCESS(res, res);

  
  if (mEditor.GetShouldTxnSetSelection()) {
    nsRefPtr<Selection> selection = mEditor.GetSelection();
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
    res = selection->Collapse(mTextNode,
                              mOffset + mStringToInsert.Length());
    NS_ASSERTION(NS_SUCCEEDED(res),
                 "Selection could not be collapsed after insert");
  } else {
    
  }

  return NS_OK;
}

NS_IMETHODIMP
InsertTextTxn::UndoTransaction()
{
  return mTextNode->DeleteData(mOffset, mStringToInsert.Length());
}

NS_IMETHODIMP
InsertTextTxn::Merge(nsITransaction* aTransaction, bool* aDidMerge)
{
  if (!aTransaction || !aDidMerge) {
    return NS_OK;
  }
  
  *aDidMerge = false;

  
  
  nsRefPtr<InsertTextTxn> otherInsTxn = do_QueryObject(aTransaction);
  if (otherInsTxn && IsSequentialInsert(*otherInsTxn)) {
    nsAutoString otherData;
    otherInsTxn->GetData(otherData);
    mStringToInsert += otherData;
    *aDidMerge = true;
  }

  return NS_OK;
}

NS_IMETHODIMP
InsertTextTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("InsertTextTxn: ");
  aString += mStringToInsert;
  return NS_OK;
}



void
InsertTextTxn::GetData(nsString& aResult)
{
  aResult = mStringToInsert;
}

bool
InsertTextTxn::IsSequentialInsert(InsertTextTxn& aOtherTxn)
{
  return aOtherTxn.mTextNode == mTextNode &&
         aOtherTxn.mOffset == mOffset + mStringToInsert.Length();
}
