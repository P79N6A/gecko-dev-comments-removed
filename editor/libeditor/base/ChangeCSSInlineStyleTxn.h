





































#ifndef ChangeCSSInlineStyleTxn_h__
#define ChangeCSSInlineStyleTxn_h__

#include "EditTxn.h"
#include "nsCOMPtr.h"
#include "nsIDOMElement.h"
#include "nsIEditor.h"

#define CHANGE_CSSINLINESTYLE_TXN_CID \
{/* a2185c9e-1dd1-11b2-88d6-d89704bf7a5a */ \
0xa2185c9e, 0x1dd1, 0x11b2, \
{0x88, 0xd6, 0xd8, 0x97, 0x04, 0xbf, 0x7a, 0x5a} }





class ChangeCSSInlineStyleTxn : public EditTxn
{
public:

  static const nsIID& GetCID() { static const nsIID iid = CHANGE_CSSINLINESTYLE_TXN_CID; return iid; }

  






  NS_IMETHOD Init(nsIEditor      * aEditor,
                  nsIDOMElement  * aElement,
                  nsIAtom        * aProperty,
                  const nsAString & aValue,
                  PRBool aRemoveProperty);

  






  static PRBool ValueIncludes(const nsAString & aValueList, const nsAString & aValue, PRBool aCaseSensitive);

  




  NS_IMETHOD AddValueToMultivalueProperty(nsAString & aValues, const nsAString  & aNewValue);

private:
  ChangeCSSInlineStyleTxn();

  




  PRBool AcceptsMoreThanOneValue(nsIAtom * aCSSProperty);

  



  void   RemoveValueFromListOfValues(nsAString & aValues, const nsAString  & aRemoveValue);

  




  nsresult SetStyle(PRBool aAttributeWasSet, nsAString & aValue);

public:
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

  friend class TransactionFactory;
};

#endif
