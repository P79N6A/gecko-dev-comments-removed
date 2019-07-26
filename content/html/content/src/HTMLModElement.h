




#ifndef mozilla_dom_HTMLModElement_h
#define mozilla_dom_HTMLModElement_h

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLModElement.h"

namespace mozilla {
namespace dom {

class HTMLModElement : public nsGenericHTMLElement,
                       public nsIDOMHTMLModElement
{
public:
  HTMLModElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLModElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLMODELEMENT

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }
};

} 
} 

#endif 
