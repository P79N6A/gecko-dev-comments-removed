







#ifndef HTMLLabelElement_h
#define HTMLLabelElement_h

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLLabelElement.h"

namespace mozilla {
namespace dom {

class HTMLLabelElement : public nsGenericHTMLFormElement,
                         public nsIDOMHTMLLabelElement
{
public:
  HTMLLabelElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericHTMLFormElement(aNodeInfo),
      mHandlingEvent(false)
  {
    SetIsDOMBinding();
  }
  virtual ~HTMLLabelElement();

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLLabelElement, label)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLLABELELEMENT

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  using nsGenericHTMLFormElement::GetForm;
  void GetHtmlFor(nsString& aHtmlFor)
  {
    GetHTMLAttr(nsGkAtoms::_for, aHtmlFor);
  }
  void SetHtmlFor(const nsAString& aHtmlFor, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::_for, aHtmlFor, aError);
  }
  nsGenericHTMLElement* GetControl() const
  {
    return GetLabeledElement();
  }

  virtual void Focus(mozilla::ErrorResult& aError) MOZ_OVERRIDE;

  
  NS_IMETHOD_(uint32_t) GetType() const { return NS_FORM_LABEL; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission);

  virtual bool IsDisabled() const { return false; }

  
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);
  virtual void PerformAccesskey(bool aKeyCausesActivation,
                                bool aIsTrustedEvent);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }

  nsGenericHTMLElement* GetLabeledElement() const;
protected:
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope,
                             bool *aTriedToWrap) MOZ_OVERRIDE;

  nsGenericHTMLElement* GetFirstLabelableDescendant() const;

  
  bool mHandlingEvent;
};

} 
} 

#endif 
