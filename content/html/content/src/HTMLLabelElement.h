







#ifndef HTMLLabelElement_h
#define HTMLLabelElement_h

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLLabelElement.h"

namespace mozilla {
class EventChainPostVisitor;
namespace dom {

class HTMLLabelElement MOZ_FINAL : public nsGenericHTMLFormElement,
                                   public nsIDOMHTMLLabelElement
{
public:
  HTMLLabelElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsGenericHTMLFormElement(aNodeInfo),
      mHandlingEvent(false)
  {
  }

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLLabelElement, label)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMHTMLLABELELEMENT

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

  using nsGenericHTMLElement::Focus;
  virtual void Focus(mozilla::ErrorResult& aError) MOZ_OVERRIDE;

  
  NS_IMETHOD_(uint32_t) GetType() const MOZ_OVERRIDE { return NS_FORM_LABEL; }
  NS_IMETHOD Reset() MOZ_OVERRIDE;
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission) MOZ_OVERRIDE;

  virtual bool IsDisabled() const MOZ_OVERRIDE { return false; }

  
  virtual nsresult PostHandleEvent(
                     EventChainPostVisitor& aVisitor) MOZ_OVERRIDE;
  virtual void PerformAccesskey(bool aKeyCausesActivation,
                                bool aIsTrustedEvent) MOZ_OVERRIDE;
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  nsGenericHTMLElement* GetLabeledElement() const;
protected:
  virtual ~HTMLLabelElement();

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

  nsGenericHTMLElement* GetFirstLabelableDescendant() const;

  
  bool mHandlingEvent;
};

} 
} 

#endif 
