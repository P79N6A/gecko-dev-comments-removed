




#ifndef mozilla_dom_HTMLSharedElement_h
#define mozilla_dom_HTMLSharedElement_h

#include "nsIDOMHTMLParamElement.h"
#include "nsIDOMHTMLBaseElement.h"
#include "nsIDOMHTMLDirectoryElement.h"
#include "nsIDOMHTMLQuoteElement.h"
#include "nsIDOMHTMLHeadElement.h"
#include "nsIDOMHTMLHtmlElement.h"
#include "nsGenericHTMLElement.h"

#include "nsGkAtoms.h"

namespace mozilla {
namespace dom {

class HTMLSharedElement : public nsGenericHTMLElement,
                          public nsIDOMHTMLParamElement,
                          public nsIDOMHTMLBaseElement,
                          public nsIDOMHTMLDirectoryElement,
                          public nsIDOMHTMLQuoteElement,
                          public nsIDOMHTMLHeadElement,
                          public nsIDOMHTMLHtmlElement
{
public:
  HTMLSharedElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {}
  virtual ~HTMLSharedElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLPARAMELEMENT

  
  NS_DECL_NSIDOMHTMLBASEELEMENT

  
  NS_DECL_NSIDOMHTMLDIRECTORYELEMENT

  
  NS_DECL_NSIDOMHTMLQUOTEELEMENT

  
  NS_DECL_NSIDOMHTMLHEADELEMENT

  
  NS_DECL_NSIDOMHTMLHTMLELEMENT

  
  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify);

  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                             bool aNotify);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);

  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);

  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo()
  {
    return static_cast<nsXPCClassInfo*>(GetClassInfoInternal());
  }
  nsIClassInfo* GetClassInfoInternal();

  virtual nsIDOMNode* AsDOMNode()
  {
    return static_cast<nsIDOMHTMLParamElement*>(this);
  }
};

} 
} 

#endif 
