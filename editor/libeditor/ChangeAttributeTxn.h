




#ifndef ChangeAttributeTxn_h__
#define ChangeAttributeTxn_h__

#include "EditTxn.h"                      
#include "mozilla/Attributes.h"           
#include "nsCOMPtr.h"                     
#include "nsCycleCollectionParticipant.h" 
#include "nsISupportsImpl.h"              
#include "nsString.h"                     

class nsIAtom;

namespace mozilla {
namespace dom {

class Element;





class ChangeAttributeTxn : public EditTxn
{
public:
  



  ChangeAttributeTxn(Element& aElement, nsIAtom& aAttribute,
                     const nsAString* aValue);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ChangeAttributeTxn, EditTxn)

  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction() MOZ_OVERRIDE;

private:
  virtual ~ChangeAttributeTxn();

  
  nsCOMPtr<Element> mElement;

  
  nsCOMPtr<nsIAtom> mAttribute;

  
  nsString mValue;

  
  bool mRemoveAttribute;

  
  bool mAttributeWasSet;

  
  nsString mUndoValue;
};

}
}

#endif
