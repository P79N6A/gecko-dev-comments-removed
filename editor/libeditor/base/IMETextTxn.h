




































#ifndef IMETextTxn_h__
#define IMETextTxn_h__

#include "EditTxn.h"
#include "nsIDOMCharacterData.h"
#include "nsIPrivateTextRange.h"
#include "nsCOMPtr.h"
#include "nsWeakPtr.h"
#include "nsIAtom.h"


#define IME_TEXT_TXN_CID							\
{0xd4d25721, 0x2813, 0x11d3,						\
{0x9e, 0xa3, 0x0, 0x60, 0x8, 0x9f, 0xe5, 0x9b }}




class nsIPresShell;




class IMETextTxn : public EditTxn
{
public:
  static const nsIID& GetCID() { static const nsIID iid = IME_TEXT_TXN_CID; return iid; }

  






  NS_IMETHOD Init(nsIDOMCharacterData *aElement,
                  PRUint32 aOffset,
                  PRUint32 aReplaceLength,
                  nsIPrivateTextRangeList* aTextRangeList,
                  const nsAString& aString,
                  nsWeakPtr aSelCon);

private:
	
	IMETextTxn();

public:
  NS_DECL_EDITTXN

  NS_IMETHOD Merge(nsITransaction *aTransaction, PRBool *aDidMerge);

  NS_IMETHOD MarkFixed(void);



  
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

  
  NS_IMETHOD GetData(nsString& aResult, nsIPrivateTextRangeList** aTextRangeList);

protected:

  NS_IMETHOD CollapseTextSelection(void);

  
  nsCOMPtr<nsIDOMCharacterData> mElement;
  
  
  PRUint32 mOffset;

  PRUint32 mReplaceLength;

  
  nsString mStringToInsert;

  
  nsCOMPtr<nsIPrivateTextRangeList>	mRangeList;

  
  nsWeakPtr mSelConWeak;  

  PRBool	mFixed;

  friend class TransactionFactory;

  friend class nsDerivedSafe<IMETextTxn>; 

};

#endif
