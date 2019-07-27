




#ifndef DeleteNodeTxn_h__
#define DeleteNodeTxn_h__

#include "EditTxn.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIContent.h"
#include "nsINode.h"
#include "nsISupportsImpl.h"
#include "nscore.h"

class nsEditor;
class nsRangeUpdater;




class DeleteNodeTxn : public EditTxn
{
public:
  


  nsresult Init(nsEditor* aEditor, nsINode* aNode,
                nsRangeUpdater* aRangeUpdater);

  DeleteNodeTxn();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DeleteNodeTxn, EditTxn)

  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction();

protected:
  virtual ~DeleteNodeTxn();

  
  nsCOMPtr<nsINode> mNode;

  
  nsCOMPtr<nsINode> mParent;

  
  nsCOMPtr<nsIContent> mRefNode;

  
  nsEditor* mEditor;

  
  nsRangeUpdater* mRangeUpdater;
};

#endif
