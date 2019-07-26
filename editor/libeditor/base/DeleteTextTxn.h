




#ifndef DeleteTextTxn_h__
#define DeleteTextTxn_h__

#include "EditTxn.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsID.h"
#include "nsIDOMCharacterData.h"
#include "nsString.h"
#include "nscore.h"

class nsEditor;
class nsRangeUpdater;




class DeleteTextTxn : public EditTxn
{
public:
  





  NS_IMETHOD Init(nsEditor* aEditor,
                  nsIDOMCharacterData* aCharData,
                  uint32_t aOffset,
                  uint32_t aNumCharsToDelete,
                  nsRangeUpdater* aRangeUpdater);

  DeleteTextTxn();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DeleteTextTxn, EditTxn)
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  NS_DECL_EDITTXN

  uint32_t GetOffset() { return mOffset; }

  uint32_t GetNumCharsToDelete() { return mNumCharsToDelete; }

protected:

  
  nsEditor* mEditor;

  
  nsCOMPtr<nsIDOMCharacterData> mCharData;

  
  uint32_t mOffset;

  
  uint32_t mNumCharsToDelete;

  
  nsString mDeletedText;

  
  nsRangeUpdater* mRangeUpdater;
};

#endif
