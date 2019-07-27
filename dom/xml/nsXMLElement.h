




#ifndef nsXMLElement_h___
#define nsXMLElement_h___

#include "mozilla/Attributes.h"
#include "nsIDOMElement.h"
#include "mozilla/dom/ElementInlines.h"
#include "mozilla/dom/DOMRect.h"

class nsXMLElement : public mozilla::dom::Element,
                     public nsIDOMElement
{
public:
  nsXMLElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : mozilla::dom::Element(aNodeInfo)
  {
  }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  virtual nsIDOMNode* AsDOMNode() MOZ_OVERRIDE { return this; }

protected:
  virtual ~nsXMLElement() {}

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;
};

#endif 
