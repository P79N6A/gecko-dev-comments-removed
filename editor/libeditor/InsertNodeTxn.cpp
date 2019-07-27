




#include "InsertNodeTxn.h"

#include "mozilla/dom/Selection.h"      

#include "nsAString.h"
#include "nsDebug.h"                    
#include "nsEditor.h"                   
#include "nsError.h"                    
#include "nsIContent.h"                 
#include "nsMemory.h"                   
#include "nsReadableUtils.h"            
#include "nsString.h"                   

using namespace mozilla;
using namespace mozilla::dom;

InsertNodeTxn::InsertNodeTxn(nsIContent& aNode, nsINode& aParent,
                             int32_t aOffset, nsEditor& aEditor)
  : EditTxn()
  , mNode(&aNode)
  , mParent(&aParent)
  , mOffset(aOffset)
  , mEditor(aEditor)
{
}

InsertNodeTxn::~InsertNodeTxn()
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(InsertNodeTxn, EditTxn,
                                   mNode,
                                   mParent)

NS_IMPL_ADDREF_INHERITED(InsertNodeTxn, EditTxn)
NS_IMPL_RELEASE_INHERITED(InsertNodeTxn, EditTxn)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(InsertNodeTxn)
NS_INTERFACE_MAP_END_INHERITING(EditTxn)

NS_IMETHODIMP
InsertNodeTxn::DoTransaction()
{
  MOZ_ASSERT(mNode && mParent);

  uint32_t count = mParent->GetChildCount();
  if (mOffset > static_cast<int32_t>(count) || mOffset == -1) {
    
    mOffset = count;
  }

  
  nsCOMPtr<nsIContent> ref = mParent->GetChildAt(mOffset);

  mEditor.MarkNodeDirty(GetAsDOMNode(mNode));

  ErrorResult rv;
  mParent->InsertBefore(*mNode, ref, rv);
  NS_ENSURE_TRUE(!rv.Failed(), rv.StealNSResult());

  
  if (mEditor.GetShouldTxnSetSelection()) {
    nsRefPtr<Selection> selection = mEditor.GetSelection();
    NS_ENSURE_TRUE(selection, NS_ERROR_NULL_POINTER);
    
    selection->Collapse(mParent, mOffset + 1);
  } else {
    
  }
  return NS_OK;
}

NS_IMETHODIMP
InsertNodeTxn::UndoTransaction()
{
  MOZ_ASSERT(mNode && mParent);

  ErrorResult rv;
  mParent->RemoveChild(*mNode, rv);
  return rv.StealNSResult();
}

NS_IMETHODIMP
InsertNodeTxn::GetTxnDescription(nsAString& aString)
{
  aString.AssignLiteral("InsertNodeTxn");
  return NS_OK;
}
