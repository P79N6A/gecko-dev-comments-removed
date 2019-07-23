





































#ifndef ChangeCSSInlineStyleTxn_h__
#define ChangeCSSInlineStyleTxn_h__

#include "EditTxn.h"
#include "nsCOMPtr.h"
#include "nsIDOMElement.h"
#include "nsIEditor.h"





class ChangeCSSInlineStyleTxn : public EditTxn
{
public:
  






  NS_IMETHOD Init(nsIEditor      * aEditor,
                  nsIDOMElement  * aElement,
                  nsIAtom        * aProperty,
                  const nsAString & aValue,
                  PRBool aRemoveProperty);

  






  static PRBool ValueIncludes(const nsAString & aValueList, const nsAString & aValue, PRBool aCaseSensitive);

  




  NS_IMETHOD AddValueToMultivalueProperty(nsAString & aValues, const nsAString  & aNewValue);

  ChangeCSSInlineStyleTxn();

private:
  




  PRBool AcceptsMoreThanOneValue(nsIAtom * aCSSProperty);

  



  void   RemoveValueFromListOfValues(nsAString & aValues, const nsAString  & aRemoveValue);

  




  nsresult SetStyle(PRBool aAttributeWasSet, nsAString & aValue);

public:
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ChangeCSSInlineStyleTxn, EditTxn)
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction();

protected:

  
  nsIEditor *mEditor;
  
  
  nsCOMPtr<nsIDOMElement> mElement;
  
  
  nsIAtom *mProperty;

  
  nsString mValue;

  
  nsString mUndoValue;
  
  nsString mRedoValue;
  
  PRBool   mUndoAttributeWasSet;
  
  PRBool   mRedoAttributeWasSet;

  
  PRBool   mRemoveProperty;
};

#endif
