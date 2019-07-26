



#ifndef nsHTMLUnknownElement_h___
#define nsHTMLUnknownElement_h___

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLUnknownElement.h"

class nsHTMLUnknownElement : public nsGenericHTMLElement
                           , public nsIDOMHTMLUnknownElement
{
public:
  nsHTMLUnknownElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
    if (NodeInfo()->Equals(nsGkAtoms::bdi)) {
      SetHasDirAuto();
    }
  }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
};

#endif 
