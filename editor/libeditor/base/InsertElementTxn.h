




#ifndef InsertElementTxn_h__
#define InsertElementTxn_h__

#include "EditTxn.h"                    
#include "nsCOMPtr.h"                   
#include "nsCycleCollectionParticipant.h"
#include "nsIDOMNode.h"                 
#include "nsISupportsImpl.h"            
#include "nscore.h"                     

class nsIEditor;




class InsertElementTxn : public EditTxn
{
public:
  




  NS_IMETHOD Init(nsIDOMNode *aNode,
                  nsIDOMNode *aParent,
                  int32_t     aOffset,
                  nsIEditor  *aEditor);

  InsertElementTxn();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(InsertElementTxn, EditTxn)

  NS_DECL_EDITTXN

protected:
  virtual ~InsertElementTxn();

  
  nsCOMPtr<nsIDOMNode> mNode;

  
  nsCOMPtr<nsIDOMNode> mParent;

  
  nsIEditor*           mEditor;

  
  int32_t mOffset;
};

#endif
