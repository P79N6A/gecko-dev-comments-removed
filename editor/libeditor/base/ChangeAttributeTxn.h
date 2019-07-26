




#ifndef ChangeAttributeTxn_h__
#define ChangeAttributeTxn_h__

#include "EditTxn.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsISupportsImpl.h"
#include "nsString.h"
#include "nscore.h"

class nsEditor;

namespace mozilla {
namespace dom {
class Element;
}
}





class ChangeAttributeTxn : public EditTxn
{
public:
  






  NS_IMETHOD Init(nsEditor      *aEditor,
                  mozilla::dom::Element *aNode,
                  const nsAString& aAttribute,
                  const nsAString& aValue,
                  bool aRemoveAttribute);

  ChangeAttributeTxn();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ChangeAttributeTxn, EditTxn)

  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction();

protected:

  
  nsEditor*  mEditor;

  
  nsCOMPtr<mozilla::dom::Element> mElement;

  
  nsString mAttribute;

  
  nsString mValue;

  
  nsString mUndoValue;

  
  bool     mAttributeWasSet;

  
  bool     mRemoveAttribute;
};

#endif
