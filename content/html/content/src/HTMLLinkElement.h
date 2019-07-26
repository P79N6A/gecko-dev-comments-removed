




#ifndef mozilla_dom_HTMLLinkElement_h
#define mozilla_dom_HTMLLinkElement_h

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLLinkElement.h"
#include "nsILink.h"
#include "nsStyleLinkElement.h"
#include "mozilla/dom/Link.h"

namespace mozilla {
namespace dom {

class HTMLLinkElement : public nsGenericHTMLElement,
                        public nsIDOMHTMLLinkElement,
                        public nsILink,
                        public nsStyleLinkElement,
                        public Link
{
public:
  HTMLLinkElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLLinkElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLLinkElement,
                                           nsGenericHTMLElement)

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLLINKELEMENT

  
  NS_DECL_SIZEOF_EXCLUDING_THIS

  
  NS_IMETHOD    LinkAdded();
  NS_IMETHOD    LinkRemoved();

  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

  
  virtual nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const;
  virtual nsIDOMNode* AsDOMNode() { return this; }
  virtual JSObject*
  WrapNode(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify);
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify);
  virtual bool IsLink(nsIURI** aURI) const;
  virtual nsLinkState GetLinkState() const;
  virtual already_AddRefed<nsIURI> GetHrefURI() const;

  
  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult);
  virtual void GetLinkTarget(nsAString& aTarget);
  virtual nsEventStates IntrinsicState() const;

  void CreateAndDispatchEvent(nsIDocument* aDoc, const nsAString& aEventName);

  
  bool Disabled();
  void SetDisabled(bool aDisabled);
  
  void SetHref(const nsAString& aHref, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::href, aHref, aRv);
  }
  
  void SetCrossOrigin(const nsAString& aCrossOrigin, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::crossorigin, aCrossOrigin, aRv);
  }
  
  void SetRel(const nsAString& aRel, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::rel, aRel, aRv);
  }
  
  void SetMedia(const nsAString& aMedia, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::media, aMedia, aRv);
  }
  
  void SetHreflang(const nsAString& aHreflang, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::hreflang, aHreflang, aRv);
  }
  
  void SetType(const nsAString& aType, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::type, aType, aRv);
  }
  
  void SetCharset(const nsAString& aCharset, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::charset, aCharset, aRv);
  }
  
  void SetRev(const nsAString& aRev, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::rev, aRev, aRv);
  }
  
  void SetTarget(const nsAString& aTarget, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::target, aTarget, aRv);
  }

protected:
  
  virtual already_AddRefed<nsIURI> GetStyleSheetURL(bool* aIsInline);
  virtual void GetStyleSheetInfo(nsAString& aTitle,
                                 nsAString& aType,
                                 nsAString& aMedia,
                                 bool* aIsScoped,
                                 bool* aIsAlternate);
  virtual CORSMode GetCORSMode() const;
protected:
  
  virtual void GetItemValueText(nsAString& text);
  virtual void SetItemValueText(const nsAString& text);
};

} 
} 

#endif 
