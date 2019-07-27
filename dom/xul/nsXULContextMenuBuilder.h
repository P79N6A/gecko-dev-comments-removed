




#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIMenuBuilder.h"
#include "nsIXULContextMenuBuilder.h"
#include "nsCycleCollectionParticipant.h"

class nsIAtom;
class nsIContent;
class nsIDocument;
class nsIDOMHTMLElement;

namespace mozilla {
namespace dom {
class Element;
} 
} 

class nsXULContextMenuBuilder : public nsIMenuBuilder,
                                public nsIXULContextMenuBuilder
{
public:
  nsXULContextMenuBuilder();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXULContextMenuBuilder,
                                           nsIMenuBuilder)
  NS_DECL_NSIMENUBUILDER

  NS_DECL_NSIXULCONTEXTMENUBUILDER

protected:
  virtual ~nsXULContextMenuBuilder();

  nsresult CreateElement(nsIAtom* aTag,
                         nsIDOMHTMLElement* aHTMLElement,
                         mozilla::dom::Element** aResult);

  nsCOMPtr<nsIContent>          mFragment;
  nsCOMPtr<nsIDocument>         mDocument;
  nsCOMPtr<nsIAtom>             mGeneratedItemIdAttr;

  nsCOMPtr<nsIContent>          mCurrentNode;
  int32_t                       mCurrentGeneratedItemId;

  nsCOMArray<nsIDOMHTMLElement> mElements;
};
