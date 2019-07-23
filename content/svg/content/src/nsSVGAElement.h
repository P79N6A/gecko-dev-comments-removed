




































#include "nsSVGGraphicElement.h"
#include "nsIDOMSVGAElement.h"
#include "nsIDOMSVGURIReference.h"
#include "nsILink.h"
#include "nsSVGString.h"

typedef nsSVGGraphicElement nsSVGAElementBase;

class nsSVGAElement : public nsSVGAElementBase,
                      public nsIDOMSVGAElement,
                      public nsIDOMSVGURIReference,
                      public nsILink
{
protected:
  friend nsresult NS_NewSVGAElement(nsIContent **aResult,
                                    nsINodeInfo *aNodeInfo);
  nsSVGAElement(nsINodeInfo *aNodeInfo);

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGAELEMENT
  NS_DECL_NSIDOMSVGURIREFERENCE

  
  NS_FORWARD_NSIDOMNODE(nsSVGAElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGAElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGAElementBase::)

  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  NS_IMETHOD LinkAdded() { return NS_OK; }
  NS_IMETHOD LinkRemoved() { return NS_OK; }

  
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  virtual nsresult SetAttr(PRInt32 aNamespaceID, nsIAtom *aName,
                           nsIAtom *aPrefix, const nsAString& aValue,
                           PRBool aNotify);
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom *aName,
                             PRBool aNotify);
  virtual PRBool IsFocusable(PRInt32 *aTabIndex = nsnull);
  virtual PRBool IsLink(nsIURI** aURI) const;
  virtual void GetLinkTarget(nsAString& aTarget);
  virtual nsLinkState GetLinkState() const;
  virtual void SetLinkState(nsLinkState aState);
  virtual already_AddRefed<nsIURI> GetHrefURI() const;

protected:

  virtual StringAttributesInfo GetStringInfo();

  enum { HREF, TARGET };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];

  
  nsLinkState mLinkState;
};
