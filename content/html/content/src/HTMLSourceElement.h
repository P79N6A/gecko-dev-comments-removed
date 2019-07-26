





#ifndef mozilla_dom_HTMLSourceElement_h
#define mozilla_dom_HTMLSourceElement_h

#include "nsIDOMHTMLSourceElement.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLMediaElement.h"

namespace mozilla {
namespace dom {

class HTMLSourceElement : public nsGenericHTMLElement,
                          public nsIDOMHTMLSourceElement
{
public:
  HTMLSourceElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLSourceElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLSOURCEELEMENT

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  
  virtual nsresult BindToTree(nsIDocument *aDocument, nsIContent *aParent,
                              nsIContent *aBindingParent,
                              bool aCompileEventHandlers);

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

protected:
  virtual void GetItemValueText(nsAString& text);
  virtual void SetItemValueText(const nsAString& text);
};

} 
} 

#endif 
