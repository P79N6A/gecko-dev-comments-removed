




































#ifndef ChangeAttributeTxn_h__
#define ChangeAttributeTxn_h__

#include "EditTxn.h"
#include "nsCOMPtr.h"
#include "nsIDOMElement.h"
#include "nsIEditor.h"

#define CHANGE_ATTRIBUTE_TXN_CID \
{/* 97818860-ac48-11d2-86d8-000064657374 */ \
0x97818860, 0xac48, 0x11d2, \
{0x86, 0xd8, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74} }





class ChangeAttributeTxn : public EditTxn
{
public:

  static const nsIID& GetCID() { static const nsIID iid = CHANGE_ATTRIBUTE_TXN_CID; return iid; }

  






  NS_IMETHOD Init(nsIEditor      *aEditor,
                  nsIDOMElement  *aNode,
                  const nsAString& aAttribute,
                  const nsAString& aValue,
                  PRBool aRemoveAttribute);

private:
  ChangeAttributeTxn();

public:

  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction();

protected:

  
  nsIEditor*  mEditor;
  
  
  nsCOMPtr<nsIDOMElement> mElement;
  
  
  nsString mAttribute;

  
  nsString mValue;

  
  nsString mUndoValue;

  
  PRBool   mAttributeWasSet;

  
  PRBool   mRemoveAttribute;

  friend class TransactionFactory;
};

#endif
