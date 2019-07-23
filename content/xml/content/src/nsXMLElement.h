





































#ifndef nsXMLElement_h___
#define nsXMLElement_h___

#include "nsIDOMElement.h"
#include "nsGenericElement.h"

class nsIDocShell;

class nsXMLElement : public nsGenericElement,
                     public nsIDOMElement
{
public:
  nsXMLElement(nsINodeInfo *aNodeInfo);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericElement::)

  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  virtual PRBool IsLink(nsIURI** aURI) const;
  virtual nsresult MaybeTriggerAutoLink(nsIDocShell *aShell);
  virtual PRBool IsFocusable(PRInt32 *aTabIndex = nsnull);

  
  virtual void GetLinkTarget(nsAString& aTarget);

  
  nsresult GetLinkTargetAndAutoType(nsAString& aTarget);
};

#endif 
