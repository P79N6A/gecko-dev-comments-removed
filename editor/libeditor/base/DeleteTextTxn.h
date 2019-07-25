




#ifndef DeleteTextTxn_h__
#define DeleteTextTxn_h__

#include "EditTxn.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsID.h"
#include "nsIDOMCharacterData.h"
#include "nsString.h"
#include "nscore.h"
#include "prtypes.h"

class nsEditor;
class nsRangeUpdater;




class DeleteTextTxn : public EditTxn
{
public:
  





  NS_IMETHOD Init(nsEditor* aEditor,
                  nsIDOMCharacterData* aCharData,
                  PRUint32 aOffset,
                  PRUint32 aNumCharsToDelete,
                  nsRangeUpdater* aRangeUpdater);

  DeleteTextTxn();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DeleteTextTxn, EditTxn)
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  NS_DECL_EDITTXN

  PRUint32 GetOffset() { return mOffset; }

  PRUint32 GetNumCharsToDelete() { return mNumCharsToDelete; }

protected:

  
  nsEditor* mEditor;

  
  nsCOMPtr<nsIDOMCharacterData> mCharData;

  
  PRUint32 mOffset;

  
  PRUint32 mNumCharsToDelete;

  
  nsString mDeletedText;

  
  nsRangeUpdater* mRangeUpdater;
};

#endif
