




#ifndef SplitNodeTxn_h__
#define SplitNodeTxn_h__

#include "EditTxn.h"                    
#include "nsCOMPtr.h"                   
#include "nsCycleCollectionParticipant.h"
#include "nsISupportsImpl.h"            
#include "nscore.h"                     

class nsEditor;
class nsIContent;

namespace mozilla {
namespace dom {





class SplitNodeTxn : public EditTxn
{
public:
  





  SplitNodeTxn(nsEditor& aEditor, nsIContent& aNode, int32_t aOffset);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(SplitNodeTxn, EditTxn)

  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction();

  nsIContent* GetNewNode();

protected:
  virtual ~SplitNodeTxn();

  nsEditor& mEditor;

  
  nsCOMPtr<nsIContent> mExistingRightNode;

  



  int32_t mOffset;

  
  nsCOMPtr<nsIContent> mNewLeftNode;

  
  nsCOMPtr<nsINode> mParent;
};

}
}

#endif
