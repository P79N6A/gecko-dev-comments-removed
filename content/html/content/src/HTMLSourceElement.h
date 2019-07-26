





#ifndef mozilla_dom_HTMLSourceElement_h
#define mozilla_dom_HTMLSourceElement_h

#include "mozilla/Attributes.h"
#include "nsIDOMHTMLSourceElement.h"
#include "nsGenericHTMLElement.h"
#include "mozilla/dom/HTMLMediaElement.h"

namespace mozilla {
namespace dom {

class HTMLSourceElement MOZ_FINAL : public nsGenericHTMLElement,
                                    public nsIDOMHTMLSourceElement
{
public:
  HTMLSourceElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);
  virtual ~HTMLSourceElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMHTMLSOURCEELEMENT

  virtual nsresult Clone(mozilla::dom::NodeInfo* aNodeInfo, nsINode** aResult) const MOZ_OVERRIDE;

  
  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) MOZ_OVERRIDE;

  
  void GetSrc(nsString& aSrc)
  {
    GetURIAttr(nsGkAtoms::src, nullptr, aSrc);
  }
  void SetSrc(const nsAString& aSrc, mozilla::ErrorResult& rv)
  {
    SetHTMLAttr(nsGkAtoms::src, aSrc, rv);
  }

  void GetType(nsString& aType)
  {
    GetHTMLAttr(nsGkAtoms::type, aType);
  }
  void SetType(const nsAString& aType, ErrorResult& rv)
  {
    SetHTMLAttr(nsGkAtoms::type, aType, rv);
  }

  void GetSrcset(nsString& aSrcset)
  {
    GetHTMLAttr(nsGkAtoms::srcset, aSrcset);
  }
  void SetSrcset(const nsAString& aSrcset, mozilla::ErrorResult& rv)
  {
    SetHTMLAttr(nsGkAtoms::srcset, aSrcset, rv);
  }

  void GetSizes(nsString& aSizes)
  {
    GetHTMLAttr(nsGkAtoms::sizes, aSizes);
  }
  void SetSizes(const nsAString& aSizes, mozilla::ErrorResult& rv)
  {
    SetHTMLAttr(nsGkAtoms::sizes, aSizes, rv);
  }

  void GetMedia(nsString& aMedia)
  {
    GetHTMLAttr(nsGkAtoms::media, aMedia);
  }
  void SetMedia(const nsAString& aMedia, mozilla::ErrorResult& rv)
  {
    SetHTMLAttr(nsGkAtoms::media, aMedia, rv);
  }

  void GetKeySystem(nsString& aKeySystem) const
  {
    GetHTMLAttr(nsGkAtoms::keysystem, aKeySystem);
  }

  void SetKeySystem(const nsAString& aKeySystem)
  {
    SetHTMLAttr(nsGkAtoms::keysystem, aKeySystem);
  }

protected:
  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;

protected:
  virtual void GetItemValueText(nsAString& text) MOZ_OVERRIDE;
  virtual void SetItemValueText(const nsAString& text) MOZ_OVERRIDE;
};

} 
} 

#endif 
