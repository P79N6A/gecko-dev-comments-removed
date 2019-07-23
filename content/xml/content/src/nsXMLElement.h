





































#ifndef nsXMLElement_h___
#define nsXMLElement_h___

#include "nsIDOMElement.h"
#include "nsGenericElement.h"

class nsXMLElement : public nsGenericElement,
                     public nsIDOMElement
{
public:
  nsXMLElement(nsINodeInfo *aNodeInfo)
    : nsGenericElement(aNodeInfo)
  {
  }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericElement::)

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};

#endif 
