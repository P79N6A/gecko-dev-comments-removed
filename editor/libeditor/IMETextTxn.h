




#ifndef IMETextTxn_h__
#define IMETextTxn_h__

#include "EditTxn.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsID.h"
#include "nsIDOMCharacterData.h"
#include "nsString.h"
#include "nscore.h"
#include "mozilla/TextRange.h"

class nsITransaction;


#define IME_TEXT_TXN_CID							\
{0xd4d25721, 0x2813, 0x11d3,						\
{0x9e, 0xa3, 0x0, 0x60, 0x8, 0x9f, 0xe5, 0x9b }}


class nsIEditor;





class IMETextTxn : public EditTxn
{
public:
  static const nsIID& GetCID() { static const nsIID iid = IME_TEXT_TXN_CID; return iid; }

  







  NS_IMETHOD Init(nsIDOMCharacterData *aElement,
                  uint32_t aOffset,
                  uint32_t aReplaceLength,
                  mozilla::TextRangeArray* aTextRangeArray,
                  const nsAString& aString,
                  nsIEditor* aEditor);

  IMETextTxn();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IMETextTxn, EditTxn)

  NS_DECL_EDITTXN

  NS_IMETHOD Merge(nsITransaction *aTransaction, bool *aDidMerge);

  NS_IMETHOD MarkFixed(void);



  
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

protected:

  nsresult SetSelectionForRanges();

  
  nsCOMPtr<nsIDOMCharacterData> mElement;
  
  
  uint32_t mOffset;

  uint32_t mReplaceLength;

  
  nsString mStringToInsert;

  
  nsRefPtr<mozilla::TextRangeArray> mRanges;

  
  nsIEditor *mEditor;

  bool	mFixed;
};

#endif
