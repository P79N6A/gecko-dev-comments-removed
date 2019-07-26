




#ifndef SplitElementTxn_h__
#define SplitElementTxn_h__

#include "EditTxn.h"                    
#include "nsCOMPtr.h"                   
#include "nsCycleCollectionParticipant.h"
#include "nsISupportsImpl.h"            
#include "nscore.h"                     

class nsEditor;
class nsINode;





class SplitElementTxn : public EditTxn
{
public:
  






  NS_IMETHOD Init (nsEditor   *aEditor,
                   nsINode *aNode,
                   int32_t     aOffset);

  SplitElementTxn();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SplitElementTxn, EditTxn)

  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction(void);

  NS_IMETHOD GetNewNode(nsINode **aNewNode);

protected:

  
  nsCOMPtr<nsINode> mExistingRightNode;

  



  int32_t  mOffset;

  
  nsCOMPtr<nsINode> mNewLeftNode;

  
  nsCOMPtr<nsINode> mParent;
  nsEditor*  mEditor;
};

#endif
