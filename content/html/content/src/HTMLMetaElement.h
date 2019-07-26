




#ifndef mozilla_dom_HTMLMetaElement_h
#define mozilla_dom_HTMLMetaElement_h

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLMetaElement.h"

namespace mozilla {
namespace dom {

class HTMLMetaElement : public nsGenericHTMLElement,
                        public nsIDOMHTMLMetaElement
{
public:
  HTMLMetaElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLMetaElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLMETAELEMENT

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  void CreateAndDispatchEvent(nsIDocument* aDoc, const nsAString& aEventName);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  void SetName(const nsAString& aName, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::name, aName, aRv);
  }
  
  void SetHttpEquiv(const nsAString& aHttpEquiv, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::httpEquiv, aHttpEquiv, aRv);
  }
  
  void SetContent(const nsAString& aContent, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::content, aContent, aRv);
  }
  
  void SetScheme(const nsAString& aScheme, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::scheme, aScheme, aRv);
  }

  virtual JSObject*
  WrapNode(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

protected:
  virtual void GetItemValueText(nsAString& text);
  virtual void SetItemValueText(const nsAString& text);
};

} 
} 

#endif 
