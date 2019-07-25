







#ifndef nsHTMLLabelElement_h
#define nsHTMLLabelElement_h

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLLabelElement.h"

class nsHTMLLabelElement : public nsGenericHTMLFormElement,
                           public nsIDOMHTMLLabelElement
{
public:
  nsHTMLLabelElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLLabelElement();

  static nsHTMLLabelElement* FromContent(nsIContent* aPossibleLabel)
  {
    if (aPossibleLabel->IsHTML(nsGkAtoms::label)) {
      return static_cast<nsHTMLLabelElement*>(aPossibleLabel);
    }

    return nullptr;
  }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFormElement::)

  
  NS_DECL_NSIDOMHTMLLABELELEMENT

  
  NS_FORWARD_NSIDOMHTMLELEMENT_BASIC(nsGenericHTMLFormElement::)
  NS_IMETHOD Click() {
    return nsGenericHTMLFormElement::Click();
  }
  NS_IMETHOD GetTabIndex(int32_t* aTabIndex) {
    return nsGenericHTMLFormElement::GetTabIndex(aTabIndex);
  }
  NS_IMETHOD SetTabIndex(int32_t aTabIndex) {
    return nsGenericHTMLFormElement::SetTabIndex(aTabIndex);
  }
  NS_IMETHOD Focus();
  NS_IMETHOD GetDraggable(bool* aDraggable) {
    return nsGenericHTMLFormElement::GetDraggable(aDraggable);
  }
  NS_IMETHOD GetInnerHTML(nsAString& aInnerHTML) {
    return nsGenericHTMLFormElement::GetInnerHTML(aInnerHTML);
  }
  NS_IMETHOD SetInnerHTML(const nsAString& aInnerHTML) {
    return nsGenericHTMLFormElement::SetInnerHTML(aInnerHTML);
  }

  
  NS_IMETHOD_(uint32_t) GetType() const { return NS_FORM_LABEL; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission);

  virtual bool IsDisabled() const { return false; }

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);

  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

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
  virtual void PerformAccesskey(bool aKeyCausesActivation,
                                bool aIsTrustedEvent);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  mozilla::dom::Element* GetLabeledElement();
protected:
  mozilla::dom::Element* GetFirstLabelableDescendant();

  
  bool mHandlingEvent;
};

#endif 
