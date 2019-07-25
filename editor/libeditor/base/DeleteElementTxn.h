




#ifndef DeleteElementTxn_h__
#define DeleteElementTxn_h__

#include "EditTxn.h"

#include "nsIContent.h"
#include "nsCOMPtr.h"

class nsRangeUpdater;
class nsEditor;




class DeleteElementTxn : public EditTxn
{
public:
  


  nsresult Init(nsEditor* aEditor, nsINode* aNode,
                nsRangeUpdater* aRangeUpdater);

  DeleteElementTxn();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DeleteElementTxn, EditTxn)

  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction();

protected:

  
  nsCOMPtr<nsINode> mNode;

  
  nsCOMPtr<nsINode> mParent;

  
  nsCOMPtr<nsIContent> mRefNode;

  
  nsEditor* mEditor;

  
  nsRangeUpdater* mRangeUpdater;
};

#endif
