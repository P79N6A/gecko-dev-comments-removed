




































#ifndef DeleteTextTxn_h__
#define DeleteTextTxn_h__

#include "EditTxn.h"
#include "nsIEditor.h"
#include "nsIDOMCharacterData.h"
#include "nsCOMPtr.h"

#define DELETE_TEXT_TXN_CID \
{/* 4d3a2720-ac49-11d2-86d8-000064657374 */ \
0x4d3a2720, 0xac49, 0x11d2, \
{0x86, 0xd8, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74} }

class nsRangeUpdater;




class DeleteTextTxn : public EditTxn
{
public:

  static const nsIID& GetCID() { static const nsIID iid = DELETE_TEXT_TXN_CID; return iid; }

  





  NS_IMETHOD Init(nsIEditor *aEditor,
                  nsIDOMCharacterData *aElement,
                  PRUint32 aOffset,
                  PRUint32 aNumCharsToDelete,
                  nsRangeUpdater *aRangeUpdater);

private:
  DeleteTextTxn();

public:
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
  
  friend class TransactionFactory;

};

#endif
