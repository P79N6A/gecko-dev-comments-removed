




#ifndef mozilla_dom_HTMLLinkElement_h
#define mozilla_dom_HTMLLinkElement_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/Link.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLLinkElement.h"
#include "nsStyleLinkElement.h"

namespace mozilla {
class EventChainPostVisitor;
class EventChainPreVisitor;
namespace dom {

class HTMLLinkElement MOZ_FINAL : public nsGenericHTMLElement,
                                  public nsIDOMHTMLLinkElement,
                                  public nsStyleLinkElement,
                                  public Link
{
public:
  HTMLLinkElement(already_AddRefed<nsINodeInfo>& aNodeInfo);
  virtual ~HTMLLinkElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLLinkElement,
                                           nsGenericHTMLElement)

  
  NS_DECL_NSIDOMHTMLLINKELEMENT

  
  NS_DECL_SIZEOF_EXCLUDING_THIS

  void LinkAdded();
  void LinkRemoved();

  
  virtual nsresult PreHandleEvent(EventChainPreVisitor& aVisitor) MOZ_OVERRIDE;
  virtual nsresult PostHandleEvent(
                     EventChainPostVisitor& aVisitor) MOZ_OVERRIDE;

  
  virtual nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const MOZ_OVERRIDE;
  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) MOZ_OVERRIDE;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) MOZ_OVERRIDE;
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify) MOZ_OVERRIDE;
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify) MOZ_OVERRIDE;
  virtual bool IsLink(nsIURI** aURI) const MOZ_OVERRIDE;
  virtual already_AddRefed<nsIURI> GetHrefURI() const MOZ_OVERRIDE;

  
  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult) MOZ_OVERRIDE;
  virtual void GetLinkTarget(nsAString& aTarget) MOZ_OVERRIDE;
  virtual EventStates IntrinsicState() const MOZ_OVERRIDE;

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
  nsDOMTokenList* RelList();
  
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
  
  virtual already_AddRefed<nsIURI> GetStyleSheetURL(bool* aIsInline) MOZ_OVERRIDE;
  virtual void GetStyleSheetInfo(nsAString& aTitle,
                                 nsAString& aType,
                                 nsAString& aMedia,
                                 bool* aIsScoped,
                                 bool* aIsAlternate) MOZ_OVERRIDE;
  virtual CORSMode GetCORSMode() const;
protected:
  
  virtual void GetItemValueText(nsAString& text) MOZ_OVERRIDE;
  virtual void SetItemValueText(const nsAString& text) MOZ_OVERRIDE;
  nsRefPtr<nsDOMTokenList > mRelList;
};

} 
} 

#endif 
