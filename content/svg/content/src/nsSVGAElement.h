




































#include "nsSVGGraphicElement.h"
#include "nsIDOMSVGAElement.h"
#include "nsIDOMSVGURIReference.h"
#include "nsILink.h"
#include "nsSVGString.h"

#include "Link.h"

typedef nsSVGGraphicElement nsSVGAElementBase;

class nsSVGAElement : public nsSVGAElementBase,
                      public nsIDOMSVGAElement,
                      public nsIDOMSVGURIReference,
                      public nsILink,
                      public mozilla::dom::Link
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

  
  virtual nsresult BindToTree(nsIDocument *aDocument, nsIContent *aParent,
                              nsIContent *aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  virtual PRBool IsFocusable(PRInt32 *aTabIndex = nsnull, PRBool aWithMouse = PR_FALSE);
  virtual PRBool IsLink(nsIURI** aURI) const;
  virtual void GetLinkTarget(nsAString& aTarget);
  virtual nsLinkState GetLinkState() const;
  virtual already_AddRefed<nsIURI> GetHrefURI() const;
  virtual PRInt32 IntrinsicState() const;
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify);
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify);

protected:

  virtual StringAttributesInfo GetStringInfo();

  enum { HREF, TARGET };
  nsSVGString mStringAttributes[2];
  static StringInfo sStringInfo[2];
};
