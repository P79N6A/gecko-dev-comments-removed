







#ifndef HTMLLabelElement_h
#define HTMLLabelElement_h

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLLabelElement.h"

namespace mozilla {
class EventChainPostVisitor;
namespace dom {

class HTMLLabelElement final : public nsGenericHTMLFormElement,
                               public nsIDOMHTMLLabelElement
{
public:
  explicit HTMLLabelElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsGenericHTMLFormElement(aNodeInfo),
      mHandlingEvent(false)
  {
  }

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLLabelElement, label)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual bool IsInteractiveHTMLContent(bool aIgnoreTabindex) const override
  {
    return true;
  }

  
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
  virtual void Focus(mozilla::ErrorResult& aError) override;

  
  NS_IMETHOD_(uint32_t) GetType() const override { return NS_FORM_LABEL; }
  NS_IMETHOD Reset() override;
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission) override;

  virtual bool IsDisabled() const override { return false; }

  
  virtual nsresult PostHandleEvent(
                     EventChainPostVisitor& aVisitor) override;
  virtual void PerformAccesskey(bool aKeyCausesActivation,
                                bool aIsTrustedEvent) override;
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  nsGenericHTMLElement* GetLabeledElement() const;
protected:
  virtual ~HTMLLabelElement();

  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

  nsGenericHTMLElement* GetFirstLabelableDescendant() const;

  
  bool mHandlingEvent;
};

} 
} 

#endif 
