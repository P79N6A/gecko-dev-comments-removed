




#ifndef InsertElementTxn_h__
#define InsertElementTxn_h__

#include "EditTxn.h"                    
#include "nsCOMPtr.h"                   
#include "nsCycleCollectionParticipant.h"
#include "nsISupportsImpl.h"            
#include "nscore.h"                     

class nsEditor;
class nsINode;




class InsertElementTxn : public EditTxn
{
public:
  




  NS_IMETHOD Init(nsINode *aNode,
                  nsINode *aParent,
                  int32_t     aOffset,
                  nsEditor  *aEditor);

  InsertElementTxn();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(InsertElementTxn, EditTxn)

  NS_DECL_EDITTXN

protected:

  
  nsCOMPtr<nsINode> mNode;

  
  nsCOMPtr<nsINode> mParent;

  
  nsEditor*           mEditor;

  
  int32_t mOffset;
};

#endif
