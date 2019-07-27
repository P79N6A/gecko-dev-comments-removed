




#ifndef SplitElementTxn_h__
#define SplitElementTxn_h__

#include "EditTxn.h"                    
#include "nsCOMPtr.h"                   
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMNode.h"                 
#include "nsISupportsImpl.h"            
#include "nscore.h"                     

class nsEditor;





class SplitElementTxn : public EditTxn
{
public:
  






  NS_IMETHOD Init (nsEditor   *aEditor,
                   nsIDOMNode *aNode,
                   int32_t     aOffset);

  SplitElementTxn();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SplitElementTxn, EditTxn)

  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction(void);

  NS_IMETHOD GetNewNode(nsIDOMNode **aNewNode);

protected:
  virtual ~SplitElementTxn();

  
  nsCOMPtr<nsIDOMNode> mExistingRightNode;

  



  int32_t  mOffset;

  
  nsCOMPtr<nsIDOMNode> mNewLeftNode;

  
  nsCOMPtr<nsIDOMNode> mParent;
  nsEditor*  mEditor;
};

#endif
