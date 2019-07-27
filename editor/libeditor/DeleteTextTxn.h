




#ifndef DeleteTextTxn_h__
#define DeleteTextTxn_h__

#include "EditTxn.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsGenericDOMDataNode.h"
#include "nsID.h"
#include "nsString.h"
#include "nscore.h"

class nsEditor;
class nsRangeUpdater;

namespace mozilla {
namespace dom {




class DeleteTextTxn : public EditTxn
{
public:
  





  DeleteTextTxn(nsEditor& aEditor,
                nsGenericDOMDataNode& aCharData,
                uint32_t aOffset,
                uint32_t aNumCharsToDelete,
                nsRangeUpdater* aRangeUpdater);

  nsresult Init();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DeleteTextTxn, EditTxn)
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  NS_DECL_EDITTXN

  uint32_t GetOffset() { return mOffset; }

  uint32_t GetNumCharsToDelete() { return mNumCharsToDelete; }

protected:

  
  nsEditor& mEditor;

  
  nsRefPtr<nsGenericDOMDataNode> mCharData;

  
  uint32_t mOffset;

  
  uint32_t mNumCharsToDelete;

  
  nsString mDeletedText;

  
  nsRangeUpdater* mRangeUpdater;
};

}
}

#endif
