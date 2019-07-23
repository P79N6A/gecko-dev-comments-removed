




































#ifndef ChangeAttributeTxn_h__
#define ChangeAttributeTxn_h__

#include "EditTxn.h"
#include "nsCOMPtr.h"
#include "nsIDOMElement.h"
#include "nsIEditor.h"





class ChangeAttributeTxn : public EditTxn
{
public:
  






  NS_IMETHOD Init(nsIEditor      *aEditor,
                  nsIDOMElement  *aNode,
                  const nsAString& aAttribute,
                  const nsAString& aValue,
                  PRBool aRemoveAttribute);

  ChangeAttributeTxn();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ChangeAttributeTxn, EditTxn)
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

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
};

#endif
