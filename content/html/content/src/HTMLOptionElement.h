





#ifndef mozilla_dom_HTMLOptionElement_h__
#define mozilla_dom_HTMLOptionElement_h__

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLOptionElement.h"
#include "nsIJSNativeInitializer.h"
#include "mozilla/dom/HTMLFormElement.h"

namespace mozilla {
namespace dom {

class HTMLSelectElement;

class HTMLOptionElement MOZ_FINAL : public nsGenericHTMLElement,
                                    public nsIDOMHTMLOptionElement
{
public:
  explicit HTMLOptionElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  static already_AddRefed<HTMLOptionElement>
    Option(const GlobalObject& aGlobal,
           const Optional<nsAString>& aText,
           const Optional<nsAString>& aValue,
           const Optional<bool>& aDefaultSelected,
           const Optional<bool>& aSelected, ErrorResult& aError);

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLOptionElement, option)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  using mozilla::dom::Element::SetText;
  using mozilla::dom::Element::GetText;
  NS_DECL_NSIDOMHTMLOPTIONELEMENT

  bool Selected() const;
  bool DefaultSelected() const;

  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              int32_t aModType) const MOZ_OVERRIDE;

  virtual nsresult BeforeSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                 const nsAttrValueOrString* aValue,
                                 bool aNotify) MOZ_OVERRIDE;
  virtual nsresult AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify) MOZ_OVERRIDE;

  void SetSelectedInternal(bool aValue, bool aNotify);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) MOZ_OVERRIDE;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) MOZ_OVERRIDE;

  
  virtual EventStates IntrinsicState() const MOZ_OVERRIDE;

  virtual nsresult Clone(mozilla::dom::NodeInfo* aNodeInfo, nsINode** aResult) const MOZ_OVERRIDE;

  nsresult CopyInnerTo(mozilla::dom::Element* aDest);

  virtual bool IsDisabled() const MOZ_OVERRIDE {
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

  HTMLFormElement* GetForm();

  
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

  int32_t Index();

protected:
  virtual ~HTMLOptionElement();

  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;

  




  HTMLSelectElement* GetSelect();

  bool mSelectedChanged;
  bool mIsSelected;

  
  
  bool mIsInSetDefaultSelected;
};

} 
} 

#endif 
