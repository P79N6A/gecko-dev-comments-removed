



#ifndef HTMLUnknownElement_h___
#define HTMLUnknownElement_h___

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLUnknownElement.h"

namespace mozilla {
namespace dom {

class HTMLUnknownElement MOZ_FINAL : public nsGenericHTMLElement
                                   , public nsIDOMHTMLUnknownElement
{
public:
  HTMLUnknownElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
    SetIsDOMBinding();
    if (NodeInfo()->Equals(nsGkAtoms::bdi)) {
      SetHasDirAuto();
    }
  }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }

protected:
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;
};

} 
} 

#endif 
