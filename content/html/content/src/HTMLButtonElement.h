




#ifndef mozilla_dom_HTMLButtonElement_h
#define mozilla_dom_HTMLButtonElement_h

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLButtonElement.h"
#include "nsIConstraintValidation.h"

namespace mozilla {
namespace dom {

class HTMLButtonElement : public nsGenericHTMLFormElement,
                          public nsIDOMHTMLButtonElement,
                          public nsIConstraintValidation
{
public:
  using nsIConstraintValidation::GetValidationMessage;

  HTMLButtonElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                      FromParser aFromParser = NOT_FROM_PARSER);
  virtual ~HTMLButtonElement();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLButtonElement,
                                           nsGenericHTMLFormElement)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  virtual int32_t TabIndexDefault() MOZ_OVERRIDE;

  
  NS_DECL_NSIDOMHTMLBUTTONELEMENT

  
  NS_IMETHOD_(uint32_t) GetType() const { return mType; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission);
  NS_IMETHOD SaveState();
  bool RestoreState(nsPresState* aState);
  virtual bool IsDisabledForEvents(uint32_t aMessage);

  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

  
  virtual nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const;
  virtual nsIDOMNode* AsDOMNode() { return this; }
  virtual JSObject*
  WrapNode(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  virtual void DoneCreatingElement();

  
  nsEventStates IntrinsicState() const;
  


  virtual nsresult BeforeSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                 const nsAttrValueOrString* aValue,
                                 bool aNotify);
  


  nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                        const nsAttrValue* aValue, bool aNotify);
  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult);

  
  virtual bool IsHTMLFocusable(bool aWithMouse,
                               bool* aIsFocusable,
                               int32_t* aTabIndex);

  
  bool Autofocus() const
  {
    return GetBoolAttr(nsGkAtoms::autofocus);
  }
  void SetAutofocus(bool aAutofocus, ErrorResult& aError)
  {
    SetHTMLBoolAttr(nsGkAtoms::autofocus, aAutofocus, aError);
  }
  bool Disabled() const
  {
    return GetBoolAttr(nsGkAtoms::disabled);
  }
  void SetDisabled(bool aDisabled, ErrorResult& aError)
  {
    SetHTMLBoolAttr(nsGkAtoms::disabled, aDisabled, aError);
  }
  
  using nsGenericHTMLFormElement::GetForm;
  
  void SetFormAction(const nsAString& aFormAction, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::formaction, aFormAction, aRv);
  }
  
  void SetFormEnctype(const nsAString& aFormEnctype, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::formenctype, aFormEnctype, aRv);
  }
  
  void SetFormMethod(const nsAString& aFormMethod, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::formmethod, aFormMethod, aRv);
  }
  bool FormNoValidate() const
  {
    return GetBoolAttr(nsGkAtoms::formnovalidate);
  }
  void SetFormNoValidate(bool aFormNoValidate, ErrorResult& aError)
  {
    SetHTMLBoolAttr(nsGkAtoms::formnovalidate, aFormNoValidate, aError);
  }
  
  void SetFormTarget(const nsAString& aFormTarget, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::formtarget, aFormTarget, aRv);
  }
  
  void SetName(const nsAString& aName, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::name, aName, aRv);
  }
  
  void SetType(const nsAString& aType, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::type, aType, aRv);
  }
  
  void SetValue(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::value, aValue, aRv);
  }

  
  
  
  
  using nsIConstraintValidation::CheckValidity;
  

protected:
  uint8_t mType;
  bool mDisabledChanged;
  bool mInInternalActivate;
  bool mInhibitStateRestoration;
};

} 
} 

#endif 
