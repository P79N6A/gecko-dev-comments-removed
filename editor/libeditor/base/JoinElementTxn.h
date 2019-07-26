




#ifndef JoinElementTxn_h__
#define JoinElementTxn_h__

#include "EditTxn.h"                    
#include "nsCOMPtr.h"                   
#include "nsCycleCollectionParticipant.h"
#include "nsID.h"                       
#include "nscore.h"                     

class nsEditor;
class nsINode;








class JoinElementTxn : public EditTxn
{
public:
  




  NS_IMETHOD Init(nsEditor   *aEditor,
                  nsINode    *aLeftNode,
                  nsINode    *aRightNode);

  JoinElementTxn();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(JoinElementTxn, EditTxn)
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  NS_DECL_EDITTXN

protected:

  


  nsCOMPtr<nsINode> mLeftNode;
  nsCOMPtr<nsINode> mRightNode;

  



  uint32_t  mOffset;

  
  nsCOMPtr<nsINode> mParent;
  nsEditor*  mEditor;
};

#endif
