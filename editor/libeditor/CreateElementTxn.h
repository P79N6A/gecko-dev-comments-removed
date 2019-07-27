




#ifndef CreateElementTxn_h__
#define CreateElementTxn_h__

#include "EditTxn.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"
#include "nsISupportsImpl.h"

class nsEditor;
class nsIAtom;
class nsIContent;
class nsINode;




namespace mozilla {
namespace dom {

class Element;

class CreateElementTxn : public EditTxn
{
public:
  






  CreateElementTxn(nsEditor& aEditor,
                   nsIAtom& aTag,
                   nsINode& aParent,
                   int32_t aOffsetInParent);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(CreateElementTxn, EditTxn)

  NS_DECL_EDITTXN

  NS_IMETHOD RedoTransaction();

  already_AddRefed<Element> GetNewNode();

protected:
  virtual ~CreateElementTxn();

  
  nsEditor* mEditor;

  
  nsCOMPtr<nsIAtom> mTag;

  
  nsCOMPtr<nsINode> mParent;

  
  int32_t mOffsetInParent;

  
  nsCOMPtr<Element> mNewNode;

  
  nsCOMPtr<nsIContent> mRefNode;
};

}
}

#endif
