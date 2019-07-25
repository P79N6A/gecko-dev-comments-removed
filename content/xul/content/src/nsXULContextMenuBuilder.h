



































#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIContent.h"
#include "nsIMenuBuilder.h"
#include "nsIXULContextMenuBuilder.h"
#include "nsIDOMDocumentFragment.h"
#include "nsCycleCollectionParticipant.h"

class nsXULContextMenuBuilder : public nsIMenuBuilder,
                                public nsIXULContextMenuBuilder
{
public:
  nsXULContextMenuBuilder();
  virtual ~nsXULContextMenuBuilder();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXULContextMenuBuilder,
                                           nsIMenuBuilder)
  NS_DECL_NSIMENUBUILDER

  NS_DECL_NSIXULCONTEXTMENUBUILDER

protected:
  nsresult CreateElement(nsIAtom* aTag, nsIContent** aResult);

  nsCOMPtr<nsIContent>          mFragment;
  nsCOMPtr<nsIDocument>         mDocument;
  nsCOMPtr<nsIAtom>             mGeneratedAttr;
  nsCOMPtr<nsIAtom>             mIdentAttr;

  nsCOMPtr<nsIContent>          mCurrentNode;
  PRInt32                       mCurrentIdent;

  nsCOMArray<nsIDOMHTMLElement> mElements;
};
