




#ifndef JoinNodeTxn_h__
#define JoinNodeTxn_h__

#include "EditTxn.h"                    
#include "nsCOMPtr.h"                   
#include "nsCycleCollectionParticipant.h"
#include "nsID.h"                       
#include "nscore.h"                     

class nsEditor;
class nsINode;

namespace mozilla {
namespace dom {







class JoinNodeTxn : public EditTxn
{
public:
  



  JoinNodeTxn(nsEditor& aEditor, nsINode& aLeftNode, nsINode& aRightNode);

  
  nsresult CheckValidity();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(JoinNodeTxn, EditTxn)
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  NS_DECL_EDITTXN

protected:
  nsEditor&  mEditor;

  


  nsCOMPtr<nsINode> mLeftNode;
  nsCOMPtr<nsINode> mRightNode;

  



  uint32_t  mOffset;

  
  nsCOMPtr<nsINode> mParent;
};

}
}

#endif
