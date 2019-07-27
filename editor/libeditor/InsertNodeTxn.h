




#ifndef InsertNodeTxn_h__
#define InsertNodeTxn_h__

#include "EditTxn.h"                    
#include "nsCOMPtr.h"                   
#include "nsCycleCollectionParticipant.h"
#include "nsIContent.h"                 
#include "nsISupportsImpl.h"            

class nsEditor;

namespace mozilla {
namespace dom {




class InsertNodeTxn : public EditTxn
{
public:
  




  InsertNodeTxn(nsIContent& aNode, nsINode& aParent, int32_t aOffset,
                nsEditor& aEditor);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(InsertNodeTxn, EditTxn)

  NS_DECL_EDITTXN

protected:
  virtual ~InsertNodeTxn();

  
  nsCOMPtr<nsIContent> mNode;

  
  nsCOMPtr<nsINode> mParent;

  
  int32_t mOffset;

  
  nsEditor& mEditor;
};

}
}

#endif
