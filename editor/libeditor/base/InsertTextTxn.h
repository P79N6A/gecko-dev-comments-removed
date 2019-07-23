




































#ifndef InsertTextTxn_h__
#define InsertTextTxn_h__

#include "EditTxn.h"
#include "nsIDOMCharacterData.h"
#include "nsIEditor.h"
#include "nsCOMPtr.h"

#define INSERT_TEXT_TXN_CID \
{/* 93276f00-ab2c-11d2-8f4b-006008159b0c*/ \
0x93276f00, 0xab2c, 0x11d2, \
{0x8f, 0xb4, 0x0, 0x60, 0x8, 0x15, 0x9b, 0xc} }




class InsertTextTxn : public EditTxn
{
public:

  static const nsIID& GetCID() { static const nsIID iid = INSERT_TEXT_TXN_CID; return iid; }

  





  NS_IMETHOD Init(nsIDOMCharacterData *aElement,
                  PRUint32 aOffset,
                  const nsAString& aString,
                  nsIEditor *aEditor);

private:
	
	InsertTextTxn();

public:
	
  NS_DECL_EDITTXN

  NS_IMETHOD Merge(nsITransaction *aTransaction, PRBool *aDidMerge);



  
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

  
  NS_IMETHOD GetData(nsString& aResult);

protected:

  
  virtual PRBool IsSequentialInsert(InsertTextTxn *aOtherTxn);
  
  
  nsCOMPtr<nsIDOMCharacterData> mElement;
  
  
  PRUint32 mOffset;

  
  nsString mStringToInsert;

  
  nsIEditor *mEditor;   

  friend class TransactionFactory;

  friend class nsDerivedSafe<InsertTextTxn>; 

};

#endif
