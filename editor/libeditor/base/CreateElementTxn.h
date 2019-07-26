




#ifndef CreateElementTxn_h__
#define CreateElementTxn_h__

#include "EditTxn.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsISupportsImpl.h"
#include "nsString.h"
#include "nscore.h"

class nsEditor;
class nsINode;




class CreateElementTxn : public EditTxn
{
public:
  enum { eAppend=-1 };

  






  NS_IMETHOD Init(nsEditor *aEditor,
                  const nsAString& aTag,
                  nsINode *aParent,
                  uint32_t aOffsetInParent);

  CreateElementTxn();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(CreateElementTxn, EditTxn)

  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction();

  NS_IMETHOD GetNewNode(nsINode **aNewNode);

protected:

  
  nsEditor* mEditor;

  
  nsString mTag;

  
  nsCOMPtr<nsINode> mParent;

  
  uint32_t mOffsetInParent;

  
  nsCOMPtr<nsINode> mNewNode;

  
  nsCOMPtr<nsINode> mRefNode;
};

#endif
