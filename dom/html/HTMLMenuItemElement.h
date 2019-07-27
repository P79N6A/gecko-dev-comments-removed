




#ifndef mozilla_dom_HTMLMenuItemElement_h
#define mozilla_dom_HTMLMenuItemElement_h

#include "mozilla/Attributes.h"
#include "nsIDOMHTMLMenuItemElement.h"
#include "nsGenericHTMLElement.h"

namespace mozilla {

class EventChainPreVisitor;

namespace dom {

class Visitor;

class HTMLMenuItemElement final : public nsGenericHTMLElement,
                                  public nsIDOMHTMLMenuItemElement
{
public:
  using mozilla::dom::Element::GetText;

  HTMLMenuItemElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo,
                      mozilla::dom::FromParser aFromParser);

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLMenuItemElement, menuitem)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMHTMLMENUITEMELEMENT

  virtual nsresult PreHandleEvent(EventChainPreVisitor& aVisitor) override;
  virtual nsresult PostHandleEvent(
                     EventChainPostVisitor& aVisitor) override;

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) override;

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult) override;

  virtual void DoneCreatingElement() override;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  uint8_t GetType() const { return mType; }

  


  bool IsChecked() const { return mChecked; }
  bool IsCheckedDirty() const { return mCheckedDirty; }

  void GetText(nsAString& aText);

  

  
  void SetType(const nsAString& aType, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::type, aType, aError);
  }

  
  void SetLabel(const nsAString& aLabel, ErrorResult& aError)
  {
    SetAttrHelper(nsGkAtoms::label, aLabel);
  }

  
  void SetIcon(const nsAString& aIcon, ErrorResult& aError)
  {
    SetAttrHelper(nsGkAtoms::icon, aIcon);
  }

  bool Disabled() const
  {
    return GetBoolAttr(nsGkAtoms::disabled);
  }
  void SetDisabled(bool aDisabled, ErrorResult& aError)
  {
    SetHTMLBoolAttr(nsGkAtoms::disabled, aDisabled, aError);
  }

  bool Checked() const
  {
    return mChecked;
  }
  void SetChecked(bool aChecked, ErrorResult& aError)
  {
    aError = SetChecked(aChecked);
  }

  
  void SetRadiogroup(const nsAString& aRadiogroup, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::radiogroup, aRadiogroup, aError);
  }

  bool DefaultChecked() const
  {
    return GetBoolAttr(nsGkAtoms::checked);
  }
  void SetDefaultChecked(bool aDefault, ErrorResult& aError)
  {
    SetHTMLBoolAttr(nsGkAtoms::checked, aDefault, aError);
  }

protected:
  virtual ~HTMLMenuItemElement();

  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;


protected:
  virtual nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify) override;

  void WalkRadioGroup(Visitor* aVisitor);

  HTMLMenuItemElement* GetSelectedRadio();

  void AddedToRadioGroup();

  void InitChecked();

  friend class ClearCheckedVisitor;
  friend class SetCheckedDirtyVisitor;

  void ClearChecked() { mChecked = false; }
  void SetCheckedDirty() { mCheckedDirty = true; }

private:
  uint8_t mType : 2;
  bool mParserCreating : 1;
  bool mShouldInitChecked : 1;
  bool mCheckedDirty : 1;
  bool mChecked : 1;
};

} 
} 

#endif 
