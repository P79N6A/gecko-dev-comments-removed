




#ifndef mozilla_dom_HTMLLinkElement_h
#define mozilla_dom_HTMLLinkElement_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/Link.h"
#include "ImportManager.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLLinkElement.h"
#include "nsStyleLinkElement.h"

namespace mozilla {
class EventChainPostVisitor;
class EventChainPreVisitor;
namespace dom {

class HTMLLinkElement final : public nsGenericHTMLElement,
                              public nsIDOMHTMLLinkElement,
                              public nsStyleLinkElement,
                              public Link
{
public:
  explicit HTMLLinkElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLLinkElement,
                                           nsGenericHTMLElement)

  
  NS_DECL_NSIDOMHTMLLINKELEMENT

  
  NS_DECL_SIZEOF_EXCLUDING_THIS

  void LinkAdded();
  void LinkRemoved();

  void UpdateImport();
  void UpdatePreconnect();

  
  virtual nsresult PreHandleEvent(EventChainPreVisitor& aVisitor) override;
  virtual nsresult PostHandleEvent(
                     EventChainPostVisitor& aVisitor) override;

  
  virtual nsresult Clone(mozilla::dom::NodeInfo* aNodeInfo, nsINode** aResult) const override;
  virtual JSObject* WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) override;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) override;
  nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  virtual nsresult SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify) override;
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify) override;
  virtual bool IsLink(nsIURI** aURI) const override;
  virtual already_AddRefed<nsIURI> GetHrefURI() const override;

  
  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult) override;
  virtual void GetLinkTarget(nsAString& aTarget) override;
  virtual EventStates IntrinsicState() const override;

  void CreateAndDispatchEvent(nsIDocument* aDoc, const nsAString& aEventName);

  
  bool Disabled();
  void SetDisabled(bool aDisabled);
  
  void SetHref(const nsAString& aHref, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::href, aHref, aRv);
  }
  void GetCrossOrigin(nsAString& aResult)
  {
    
    
    
    GetEnumAttr(nsGkAtoms::crossorigin, nullptr, aResult);
  }
  void SetCrossOrigin(const nsAString& aCrossOrigin, ErrorResult& aError)
  {
    SetOrRemoveNullableStringAttr(nsGkAtoms::crossorigin, aCrossOrigin, aError);
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
  nsDOMSettableTokenList* Sizes()
  {
    return GetTokenList(nsGkAtoms::sizes);
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

  already_AddRefed<nsIDocument> GetImport();
  already_AddRefed<ImportLoader> GetImportLoader()
  {
    return nsRefPtr<ImportLoader>(mImportLoader).forget();
  }

protected:
  virtual ~HTMLLinkElement();

  
  virtual already_AddRefed<nsIURI> GetStyleSheetURL(bool* aIsInline) override;
  virtual void GetStyleSheetInfo(nsAString& aTitle,
                                 nsAString& aType,
                                 nsAString& aMedia,
                                 bool* aIsScoped,
                                 bool* aIsAlternate) override;
  virtual CORSMode GetCORSMode() const override;
protected:
  
  virtual void GetItemValueText(DOMString& text) override;
  virtual void SetItemValueText(const nsAString& text) override;
  nsRefPtr<nsDOMTokenList > mRelList;
private:
  nsRefPtr<ImportLoader> mImportLoader;
};

} 
} 

#endif 
