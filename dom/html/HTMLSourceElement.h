





#ifndef mozilla_dom_HTMLSourceElement_h
#define mozilla_dom_HTMLSourceElement_h

#include "mozilla/Attributes.h"
#include "nsIDOMHTMLSourceElement.h"
#include "nsGenericHTMLElement.h"
#include "mozilla/dom/HTMLMediaElement.h"

class nsMediaList;

namespace mozilla {
namespace dom {

class HTMLSourceElement final : public nsGenericHTMLElement,
                                public nsIDOMHTMLSourceElement
{
public:
  explicit HTMLSourceElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  
  NS_DECL_ISUPPORTS_INHERITED

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLSourceElement, source)

  
  NS_DECL_NSIDOMHTMLSOURCEELEMENT

  virtual nsresult Clone(mozilla::dom::NodeInfo* aNodeInfo, nsINode** aResult) const override;

  
  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) override;

  
  
  bool MatchesCurrentMedia();

  
  
  
  static bool WouldMatchMediaForDocument(const nsAString& aMediaStr,
                                         const nsIDocument *aDocument);

  
  void GetSrc(nsString& aSrc)
  {
    GetURIAttr(nsGkAtoms::src, nullptr, aSrc);
  }
  void SetSrc(const nsAString& aSrc, mozilla::ErrorResult& rv)
  {
    SetHTMLAttr(nsGkAtoms::src, aSrc, rv);
  }

  void GetType(DOMString& aType)
  {
    GetHTMLAttr(nsGkAtoms::type, aType);
  }
  void SetType(const nsAString& aType, ErrorResult& rv)
  {
    SetHTMLAttr(nsGkAtoms::type, aType, rv);
  }

  void GetSrcset(DOMString& aSrcset)
  {
    GetHTMLAttr(nsGkAtoms::srcset, aSrcset);
  }
  void SetSrcset(const nsAString& aSrcset, mozilla::ErrorResult& rv)
  {
    SetHTMLAttr(nsGkAtoms::srcset, aSrcset, rv);
  }

  void GetSizes(DOMString& aSizes)
  {
    GetHTMLAttr(nsGkAtoms::sizes, aSizes);
  }
  void SetSizes(const nsAString& aSizes, mozilla::ErrorResult& rv)
  {
    SetHTMLAttr(nsGkAtoms::sizes, aSizes, rv);
  }

  void GetMedia(DOMString& aMedia)
  {
    GetHTMLAttr(nsGkAtoms::media, aMedia);
  }
  void SetMedia(const nsAString& aMedia, mozilla::ErrorResult& rv)
  {
    SetHTMLAttr(nsGkAtoms::media, aMedia, rv);
  }

protected:
  virtual ~HTMLSourceElement();

  virtual JSObject* WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

protected:
  virtual void GetItemValueText(DOMString& text) override;
  virtual void SetItemValueText(const nsAString& text) override;

  virtual nsresult AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue,
                                bool aNotify) override;


private:
  nsRefPtr<nsMediaList> mMediaList;
};

} 
} 

#endif 
