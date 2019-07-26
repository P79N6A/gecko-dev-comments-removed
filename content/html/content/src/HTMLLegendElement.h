




#ifndef mozilla_dom_HTMLLegendElement_h
#define mozilla_dom_HTMLLegendElement_h

#include "nsIDOMHTMLLegendElement.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLFormElement.h"

namespace mozilla {
namespace dom {

class HTMLLegendElement : public nsGenericHTMLElement,
                          public nsIDOMHTMLLegendElement
{
public:
  HTMLLegendElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
    SetIsDOMBinding();
  }
  virtual ~HTMLLegendElement();

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLLegendElement, legend)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLLEGENDELEMENT

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  virtual void Focus(ErrorResult& aError) MOZ_OVERRIDE;

  virtual void PerformAccesskey(bool aKeyCausesActivation,
                                bool aIsTrustedEvent);

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              int32_t aModType) const;
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

  virtual nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const;

  Element* GetFormElement()
  {
    nsCOMPtr<nsIFormControl> fieldsetControl = do_QueryInterface(GetFieldSet());

    return fieldsetControl ? fieldsetControl->GetFormElement() : nullptr;
  }

  virtual nsIDOMNode* AsDOMNode() { return this; }

  



  already_AddRefed<nsHTMLFormElement> GetForm();

  
  void SetAlign(const nsAString& aAlign, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::align, aAlign, aError);
  }

  nsINode* GetParentObject() {
    Element* form = GetFormElement();
    return form ? static_cast<nsINode*>(form)
                : nsGenericHTMLElement::GetParentObject();
  }

protected:
  virtual JSObject* WrapNode(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

  



  nsIContent* GetFieldSet();
};

} 
} 

#endif 
