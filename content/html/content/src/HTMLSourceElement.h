





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
  HTMLSourceElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLSourceElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLSOURCEELEMENT

  virtual nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const MOZ_OVERRIDE;

  
  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) MOZ_OVERRIDE;

  virtual nsIDOMNode* AsDOMNode() MOZ_OVERRIDE { return this; }

  
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

  void GetMedia(nsString& aMedia)
  {
    GetHTMLAttr(nsGkAtoms::media, aMedia);
  }
  void SetMedia(const nsAString& aMedia, mozilla::ErrorResult& rv)
  {
    SetHTMLAttr(nsGkAtoms::media, aMedia, rv);
  }

protected:
  virtual JSObject* WrapNode(JSContext* aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

protected:
  virtual void GetItemValueText(nsAString& text) MOZ_OVERRIDE;
  virtual void SetItemValueText(const nsAString& text) MOZ_OVERRIDE;
};

} 
} 

#endif 
