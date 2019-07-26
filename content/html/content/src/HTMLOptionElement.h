





#ifndef mozilla_dom_HTMLOptionElement_h__
#define mozilla_dom_HTMLOptionElement_h__

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLOptionElement.h"
#include "nsIJSNativeInitializer.h"
#include "nsHTMLFormElement.h"

class nsHTMLSelectElement;

namespace mozilla {
namespace dom {

class HTMLOptionElement : public nsGenericHTMLElement,
                          public nsIDOMHTMLOptionElement,
                          public nsIJSNativeInitializer
{
public:
  HTMLOptionElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLOptionElement();

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLOptionElement, option)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  using mozilla::dom::Element::SetText;
  using mozilla::dom::Element::GetText;
  NS_DECL_NSIDOMHTMLOPTIONELEMENT

  bool Selected() const;
  bool DefaultSelected() const;

  
  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext* aContext,
                        JSObject* aObj, uint32_t argc, jsval* argv);

  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              int32_t aModType) const;

  virtual nsresult BeforeSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                 const nsAttrValueOrString* aValue,
                                 bool aNotify);

  void SetSelectedInternal(bool aValue, bool aNotify);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);

  
  virtual nsEventStates IntrinsicState() const;

  virtual nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const;

  nsresult CopyInnerTo(mozilla::dom::Element* aDest);

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  virtual bool IsDisabled() const {
    return HasAttr(kNameSpaceID_None, nsGkAtoms::disabled);
  }

  bool Disabled() const
  {
    return GetBoolAttr(nsGkAtoms::disabled);
  }

  void SetDisabled(bool aValue, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::disabled, aValue, aRv);
  }

  nsHTMLFormElement* GetForm();

  
  void SetLabel(const nsAString& aLabel, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::label, aLabel, aError);
  }

  
  void SetDefaultSelected(bool aValue, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::selected, aValue, aRv);
  }

  
  void SetSelected(bool aValue, ErrorResult& aRv)
  {
    aRv = SetSelected(aValue);
  }

  
  void SetValue(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::value, aValue, aRv);
  }

  
  void SetText(const nsAString& aValue, ErrorResult& aRv)
  {
    aRv = SetText(aValue);
  }

  int32_t GetIndex(ErrorResult& aRv)
  {
    int32_t id = 0;
    aRv = GetIndex(&id);
    return id;
  }

protected:
  virtual JSObject* WrapNode(JSContext* aCx, JSObject* aScope,
                             bool* aTriedToWrap) MOZ_OVERRIDE;

  




  nsHTMLSelectElement* GetSelect();

  bool mSelectedChanged;
  bool mIsSelected;

  
  
  bool mIsInSetDefaultSelected;
};

} 
} 

#endif 
