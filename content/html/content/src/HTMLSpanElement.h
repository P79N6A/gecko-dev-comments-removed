




#ifndef mozilla_dom_HTMLSpanElement_h
#define mozilla_dom_HTMLSpanElement_h

#include "mozilla/Attributes.h"
#include "nsIDOMHTMLElement.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsIAtom.h"
#include "nsRuleData.h"

namespace mozilla {
namespace dom {

class HTMLSpanElement MOZ_FINAL : public nsGenericHTMLElement,
                                  public nsIDOMHTMLElement
{
public:
  HTMLSpanElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
  }
  virtual ~HTMLSpanElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  virtual nsIDOMNode* AsDOMNode() MOZ_OVERRIDE { return this; }

protected:
  virtual JSObject* WrapNode(JSContext *aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;
};

} 
} 

#endif 
