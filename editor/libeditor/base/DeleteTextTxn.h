




































#ifndef DeleteTextTxn_h__
#define DeleteTextTxn_h__

#include "EditTxn.h"
#include "nsIEditor.h"
#include "nsIDOMCharacterData.h"
#include "nsCOMPtr.h"

class nsRangeUpdater;




class DeleteTextTxn : public EditTxn
{
public:
  





  NS_IMETHOD Init(nsIEditor *aEditor,
                  nsIDOMCharacterData *aElement,
                  PRUint32 aOffset,
                  PRUint32 aNumCharsToDelete,
                  nsRangeUpdater *aRangeUpdater);

  DeleteTextTxn();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(DeleteTextTxn, EditTxn)
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  NS_DECL_EDITTXN

  PRUint32 GetOffset() { return mOffset; }

  PRUint32 GetNumCharsToDelete() { return mNumCharsToDelete; }

protected:

  
  nsIEditor* mEditor;

  
  nsCOMPtr<nsIDOMCharacterData> mElement;
  
  
  PRUint32 mOffset;

  
  PRUint32 mNumCharsToDelete;

  
  nsString mDeletedText;

  
  nsRangeUpdater *mRangeUpdater;
};

#endif
