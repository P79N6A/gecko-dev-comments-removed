




#ifndef mozilla_dom_HTMLFrameElement_h
#define mozilla_dom_HTMLFrameElement_h

#include "mozilla/Util.h"
#include "nsIDOMHTMLFrameElement.h"
#include "nsGenericHTMLFrameElement.h"
#include "nsGkAtoms.h"

class nsIDOMDocument;

namespace mozilla {
namespace dom {

class HTMLFrameElement : public nsGenericHTMLFrameElement,
                         public nsIDOMHTMLFrameElement
{
public:
  HTMLFrameElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                   FromParser aFromParser = NOT_FROM_PARSER);
  virtual ~HTMLFrameElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLFRAMEELEMENT

  
  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult);
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsXPCClassInfo* GetClassInfo();
  virtual nsIDOMNode* AsDOMNode() { return this; }
};

} 
} 

#endif 
