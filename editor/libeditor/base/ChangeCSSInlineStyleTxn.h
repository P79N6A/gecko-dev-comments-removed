




#ifndef ChangeCSSInlineStyleTxn_h__
#define ChangeCSSInlineStyleTxn_h__

#include "EditTxn.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsID.h"
#include "nsIDOMElement.h"
#include "nsString.h"
#include "nscore.h"

class nsIAtom;
class nsIEditor;





class ChangeCSSInlineStyleTxn : public EditTxn
{
public:
  






  NS_IMETHOD Init(nsIEditor      * aEditor,
                  nsIDOMElement  * aElement,
                  nsIAtom        * aProperty,
                  const nsAString & aValue,
                  bool aRemoveProperty);

  






  static bool ValueIncludes(const nsAString & aValueList, const nsAString & aValue, bool aCaseSensitive);

  




  NS_IMETHOD AddValueToMultivalueProperty(nsAString & aValues, const nsAString  & aNewValue);

  ChangeCSSInlineStyleTxn();

private:
  




  bool AcceptsMoreThanOneValue(nsIAtom * aCSSProperty);

  



  void   RemoveValueFromListOfValues(nsAString & aValues, const nsAString  & aRemoveValue);

  




  nsresult SetStyle(bool aAttributeWasSet, nsAString & aValue);

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
  
  bool     mUndoAttributeWasSet;
  
  bool     mRedoAttributeWasSet;

  
  bool     mRemoveProperty;
};

#endif
