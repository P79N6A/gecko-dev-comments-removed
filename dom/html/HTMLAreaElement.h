





#ifndef mozilla_dom_HTMLAreaElement_h
#define mozilla_dom_HTMLAreaElement_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/Link.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsDOMTokenList.h"
#include "nsIDOMHTMLAreaElement.h"
#include "nsIURL.h"

class nsIDocument;

namespace mozilla {
class EventChainPostVisitor;
class EventChainPreVisitor;
namespace dom {

class HTMLAreaElement final : public nsGenericHTMLElement,
                              public nsIDOMHTMLAreaElement,
                              public Link
{
public:
  explicit HTMLAreaElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLAreaElement,
                                           nsGenericHTMLElement)

  
  NS_DECL_SIZEOF_EXCLUDING_THIS

  virtual int32_t TabIndexDefault() override;

  
  NS_DECL_NSIDOMHTMLAREAELEMENT

  virtual nsresult PreHandleEvent(EventChainPreVisitor& aVisitor) override;
  virtual nsresult PostHandleEvent(EventChainPostVisitor& aVisitor) override;
  virtual bool IsLink(nsIURI** aURI) const override;
  virtual void GetLinkTarget(nsAString& aTarget) override;
  virtual already_AddRefed<nsIURI> GetHrefURI() const override;

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

  virtual nsresult Clone(mozilla::dom::NodeInfo* aNodeInfo, nsINode** aResult) const override;

  virtual EventStates IntrinsicState() const override;

  

  
  void SetAlt(const nsAString& aAlt, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::alt, aAlt, aError);
  }

  
  void SetCoords(const nsAString& aCoords, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::coords, aCoords, aError);
  }

  
  void SetShape(const nsAString& aShape, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::shape, aShape, aError);
  }

  void GetHref(nsAString& aHref, ErrorResult& aError)
  {
    aError = GetHref(aHref);
  }
  void SetHref(const nsAString& aHref, ErrorResult& aError)
  {
    aError = SetHref(aHref);
  }

  
  void SetTarget(const nsAString& aTarget, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::target, aTarget, aError);
  }

  
  void SetDownload(const nsAString& aDownload, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::download, aDownload, aError);
  }

  
  void SetPing(const nsAString& aPing, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::ping, aPing, aError);
  }
  
  void GetRel(DOMString& aValue)
  {
    GetHTMLAttr(nsGkAtoms::rel, aValue);
  }

  void SetRel(const nsAString& aRel, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::rel, aRel, aError);
  } 
  nsDOMTokenList* RelList();

  void SetReferrer(const nsAString& aValue, mozilla::ErrorResult& rv)
  {
    SetHTMLAttr(nsGkAtoms::referrer, aValue, rv);
  }
  void GetReferrer(nsAString& aReferrer)
  {
    GetHTMLAttr(nsGkAtoms::referrer, aReferrer);
  }

  

  using Link::GetProtocol;
  using Link::SetProtocol;

  
  

  
  

  using Link::GetHost;
  using Link::SetHost;

  using Link::GetHostname;
  using Link::SetHostname;

  using Link::GetPort;
  using Link::SetPort;

  using Link::GetPathname;
  using Link::SetPathname;

  using Link::GetSearch;
  using Link::SetSearch;

  using Link::GetHash;
  using Link::SetHash;

  

  bool NoHref() const
  {
    return GetBoolAttr(nsGkAtoms::nohref);
  }

  void SetNoHref(bool aValue, ErrorResult& aError)
  {
    SetHTMLBoolAttr(nsGkAtoms::nohref, aValue, aError);
  }

  void Stringify(nsAString& aResult, ErrorResult& aError)
  {
    GetHref(aResult, aError);
  }

protected:
  virtual ~HTMLAreaElement();

  virtual JSObject* WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  virtual void GetItemValueText(DOMString& text) override;
  virtual void SetItemValueText(const nsAString& text) override;
  nsRefPtr<nsDOMTokenList > mRelList;
};

} 
} 

#endif 
