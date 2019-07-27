




#ifndef ChangeStyleTxn_h__
#define ChangeStyleTxn_h__

#include "EditTxn.h"                      
#include "nsCOMPtr.h"                     
#include "nsCycleCollectionParticipant.h" 
#include "nsString.h"                     

class nsAString;
class nsIAtom;

namespace mozilla {
namespace dom {
class Element;





class ChangeStyleTxn : public EditTxn
{
public:
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ChangeStyleTxn, EditTxn)

  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction() MOZ_OVERRIDE;

  enum EChangeType { eSet, eRemove };

  




  ChangeStyleTxn(Element& aElement, nsIAtom& aProperty,
                 const nsAString& aValue, EChangeType aChangeType);

  





  static bool ValueIncludes(const nsAString& aValueList,
                            const nsAString& aValue);

private:
  ~ChangeStyleTxn();

  




  void AddValueToMultivalueProperty(nsAString& aValues,
                                    const nsAString& aNewValue);

  




  bool AcceptsMoreThanOneValue(nsIAtom& aCSSProperty);

  



  void RemoveValueFromListOfValues(nsAString& aValues,
                                   const nsAString& aRemoveValue);

  




  nsresult SetStyle(bool aAttributeWasSet, nsAString& aValue);

  
  nsCOMPtr<Element> mElement;

  
  nsCOMPtr<nsIAtom> mProperty;

  
  nsString mValue;

  
  bool mRemoveProperty;

  
  nsString mUndoValue;
  
  nsString mRedoValue;
  
  bool mUndoAttributeWasSet;
  
  bool mRedoAttributeWasSet;
};

}
}

#endif
