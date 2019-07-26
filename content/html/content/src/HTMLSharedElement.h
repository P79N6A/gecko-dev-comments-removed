




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

#include "mozilla/Attributes.h"
#include "mozilla/Assertions.h"

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
  {
    SetIsDOMBinding();
  }
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

  virtual nsIDOMNode* AsDOMNode()
  {
    return static_cast<nsIDOMHTMLParamElement*>(this);
  }

  
  
  void GetName(DOMString& aValue)
  {
    MOZ_ASSERT(mNodeInfo->Equals(nsGkAtoms::param));
    GetHTMLAttr(nsGkAtoms::name, aValue);
  }
  void SetName(const nsAString& aValue, ErrorResult& aResult)
  {
    MOZ_ASSERT(mNodeInfo->Equals(nsGkAtoms::param));
    SetHTMLAttr(nsGkAtoms::name, aValue, aResult);
  }
  void GetValue(DOMString& aValue)
  {
    MOZ_ASSERT(mNodeInfo->Equals(nsGkAtoms::param));
    GetHTMLAttr(nsGkAtoms::value, aValue);
  }
  void SetValue(const nsAString& aValue, ErrorResult& aResult)
  {
    MOZ_ASSERT(mNodeInfo->Equals(nsGkAtoms::param));
    SetHTMLAttr(nsGkAtoms::value, aValue, aResult);
  }
  void GetType(DOMString& aValue)
  {
    MOZ_ASSERT(mNodeInfo->Equals(nsGkAtoms::param));
    GetHTMLAttr(nsGkAtoms::type, aValue);
  }
  void SetType(const nsAString& aValue, ErrorResult& aResult)
  {
    MOZ_ASSERT(mNodeInfo->Equals(nsGkAtoms::param));
    SetHTMLAttr(nsGkAtoms::type, aValue, aResult);
  }
  void GetValueType(DOMString& aValue)
  {
    MOZ_ASSERT(mNodeInfo->Equals(nsGkAtoms::param));
    GetHTMLAttr(nsGkAtoms::valuetype, aValue);
  }
  void SetValueType(const nsAString& aValue, ErrorResult& aResult)
  {
    MOZ_ASSERT(mNodeInfo->Equals(nsGkAtoms::param));
    SetHTMLAttr(nsGkAtoms::valuetype, aValue, aResult);
  }

  
  void GetTarget(DOMString& aValue)
  {
    MOZ_ASSERT(mNodeInfo->Equals(nsGkAtoms::base));
    GetHTMLAttr(nsGkAtoms::target, aValue);
  }
  void SetTarget(const nsAString& aValue, ErrorResult& aResult)
  {
    MOZ_ASSERT(mNodeInfo->Equals(nsGkAtoms::base));
    SetHTMLAttr(nsGkAtoms::target, aValue, aResult);
  }
  
  void SetHref(const nsAString& aValue, ErrorResult& aResult)
  {
    MOZ_ASSERT(mNodeInfo->Equals(nsGkAtoms::base));
    SetHTMLAttr(nsGkAtoms::href, aValue, aResult);
  }

  
  bool Compact() const
  {
    MOZ_ASSERT(mNodeInfo->Equals(nsGkAtoms::dir));
    return GetBoolAttr(nsGkAtoms::compact);
  }
  void SetCompact(bool aCompact, ErrorResult& aResult)
  {
    MOZ_ASSERT(mNodeInfo->Equals(nsGkAtoms::dir));
    SetHTMLBoolAttr(nsGkAtoms::compact, aCompact, aResult);
  }

  
  
  void SetCite(const nsAString& aValue, ErrorResult& aResult)
  {
    MOZ_ASSERT(mNodeInfo->Equals(nsGkAtoms::q) ||
               mNodeInfo->Equals(nsGkAtoms::blockquote));
    SetHTMLAttr(nsGkAtoms::cite, aValue, aResult);
  }

  
  void GetVersion(DOMString& aValue)
  {
    MOZ_ASSERT(mNodeInfo->Equals(nsGkAtoms::html));
    GetHTMLAttr(nsGkAtoms::version, aValue);
  }
  void SetVersion(const nsAString& aValue, ErrorResult& aResult)
  {
    MOZ_ASSERT(mNodeInfo->Equals(nsGkAtoms::html));
    SetHTMLAttr(nsGkAtoms::version, aValue, aResult);
  }

protected:
  virtual JSObject* WrapNode(JSContext* aCx, JSObject* aScope,
                             bool* aTriedToWrap) MOZ_OVERRIDE;
};

} 
} 

#endif 
