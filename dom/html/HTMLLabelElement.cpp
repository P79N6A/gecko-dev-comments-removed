







#include "HTMLLabelElement.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/dom/HTMLLabelElementBinding.h"
#include "nsFocusManager.h"
#include "nsIDOMMouseEvent.h"
#include "nsQueryObject.h"



NS_IMPL_NS_NEW_HTML_ELEMENT(Label)

namespace mozilla {
namespace dom {

HTMLLabelElement::~HTMLLabelElement()
{
}

JSObject*
HTMLLabelElement::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto)
{
  return HTMLLabelElementBinding::Wrap(aCx, this, aGivenProto);
}



NS_IMPL_ISUPPORTS_INHERITED(HTMLLabelElement, nsGenericHTMLFormElement,
                            nsIDOMHTMLLabelElement)



NS_IMPL_ELEMENT_CLONE(HTMLLabelElement)

NS_IMETHODIMP
HTMLLabelElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElement::GetForm(aForm);
}

NS_IMETHODIMP
HTMLLabelElement::GetControl(nsIDOMHTMLElement** aElement)
{
  nsCOMPtr<nsIDOMHTMLElement> element = do_QueryObject(GetLabeledElement());
  element.forget(aElement);
  return NS_OK;
}

NS_IMETHODIMP
HTMLLabelElement::SetHtmlFor(const nsAString& aHtmlFor)
{
  ErrorResult rv;
  SetHtmlFor(aHtmlFor, rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP
HTMLLabelElement::GetHtmlFor(nsAString& aHtmlFor)
{
  nsString htmlFor;
  GetHtmlFor(htmlFor);
  aHtmlFor = htmlFor;
  return NS_OK;
}

void
HTMLLabelElement::Focus(ErrorResult& aError)
{
  
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (fm) {
    nsCOMPtr<nsIDOMElement> elem = do_QueryObject(GetLabeledElement());
    if (elem)
      fm->SetFocus(elem, 0);
  }
}

static bool
InInteractiveHTMLContent(nsIContent* aContent, nsIContent* aStop)
{
  nsIContent* content = aContent;
  while (content && content != aStop) {
    if (content->IsElement() &&
        content->AsElement()->IsInteractiveHTMLContent(true)) {
      return true;
    }
    content = content->GetParent();
  }
  return false;
}

nsresult
HTMLLabelElement::PostHandleEvent(EventChainPostVisitor& aVisitor)
{
  WidgetMouseEvent* mouseEvent = aVisitor.mEvent->AsMouseEvent();
  if (mHandlingEvent ||
      (!(mouseEvent && mouseEvent->IsLeftClickEvent()) &&
       aVisitor.mEvent->message != NS_MOUSE_BUTTON_DOWN) ||
      aVisitor.mEventStatus == nsEventStatus_eConsumeNoDefault ||
      !aVisitor.mPresContext ||
      
      aVisitor.mEvent->mFlags.mMultipleActionsPrevented) {
    return NS_OK;
  }

  nsCOMPtr<nsIContent> target = do_QueryInterface(aVisitor.mEvent->target);
  if (InInteractiveHTMLContent(target, this)) {
    return NS_OK;
  }

  
  nsRefPtr<Element> content = GetLabeledElement();

  if (content) {
    mHandlingEvent = true;
    switch (aVisitor.mEvent->message) {
      case NS_MOUSE_BUTTON_DOWN:
        if (mouseEvent->button == WidgetMouseEvent::eLeftButton) {
          
          
          LayoutDeviceIntPoint* curPoint =
            new LayoutDeviceIntPoint(mouseEvent->refPoint);
          SetProperty(nsGkAtoms::labelMouseDownPtProperty,
                      static_cast<void*>(curPoint),
                      nsINode::DeleteProperty<LayoutDeviceIntPoint>);
        }
        break;

      case NS_MOUSE_CLICK:
        if (mouseEvent->IsLeftClickEvent()) {
          LayoutDeviceIntPoint* mouseDownPoint =
            static_cast<LayoutDeviceIntPoint*>(
              GetProperty(nsGkAtoms::labelMouseDownPtProperty));

          bool dragSelect = false;
          if (mouseDownPoint) {
            LayoutDeviceIntPoint dragDistance = *mouseDownPoint;
            DeleteProperty(nsGkAtoms::labelMouseDownPtProperty);

            dragDistance -= mouseEvent->refPoint;
            const int CLICK_DISTANCE = 2;
            dragSelect = dragDistance.x > CLICK_DISTANCE ||
                         dragDistance.x < -CLICK_DISTANCE ||
                         dragDistance.y > CLICK_DISTANCE ||
                         dragDistance.y < -CLICK_DISTANCE;
          }
          
          
          if (dragSelect || mouseEvent->IsShift() || mouseEvent->IsControl() ||
              mouseEvent->IsAlt() || mouseEvent->IsMeta()) {
            break;
          }
          
          
          if (mouseEvent->clickCount <= 1) {
            nsIFocusManager* fm = nsFocusManager::GetFocusManager();
            if (fm) {
              
              
              
              
              
              nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(content);
              fm->SetFocus(elem, nsIFocusManager::FLAG_BYMOVEFOCUS);
            }
          }
          
          
          
          
          
          
          nsEventStatus status = aVisitor.mEventStatus;
          
          
          EventFlags eventFlags;
          eventFlags.mMultipleActionsPrevented = true;
          DispatchClickEvent(aVisitor.mPresContext, mouseEvent,
                             content, false, &eventFlags, &status);
          
          
          mouseEvent->mFlags.mMultipleActionsPrevented = true;
        }
        break;
    }
    mHandlingEvent = false;
  }
  return NS_OK;
}

nsresult
HTMLLabelElement::Reset()
{
  return NS_OK;
}

NS_IMETHODIMP
HTMLLabelElement::SubmitNamesValues(nsFormSubmission* aFormSubmission)
{
  return NS_OK;
}

void
HTMLLabelElement::PerformAccesskey(bool aKeyCausesActivation,
                                   bool aIsTrustedEvent)
{
  if (!aKeyCausesActivation) {
    nsRefPtr<Element> element = GetLabeledElement();
    if (element)
      element->PerformAccesskey(aKeyCausesActivation, aIsTrustedEvent);
  } else {
    nsPresContext *presContext = GetPresContext(eForUncomposedDoc);
    if (!presContext)
      return;

    
    WidgetMouseEvent event(aIsTrustedEvent, NS_MOUSE_CLICK,
                           nullptr, WidgetMouseEvent::eReal);
    event.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_KEYBOARD;

    nsAutoPopupStatePusher popupStatePusher(aIsTrustedEvent ?
                                            openAllowed : openAbused);

    EventDispatcher::Dispatch(static_cast<nsIContent*>(this), presContext,
                              &event);
  }
}

nsGenericHTMLElement*
HTMLLabelElement::GetLabeledElement() const
{
  nsAutoString elementId;

  if (!GetAttr(kNameSpaceID_None, nsGkAtoms::_for, elementId)) {
    
    
    return GetFirstLabelableDescendant();
  }

  
  
  
  
  
  nsIDocument* doc = GetUncomposedDoc();
  if (!doc) {
    return nullptr;
  }

  Element* element = doc->GetElementById(elementId);
  if (element && element->IsLabelable()) {
    return static_cast<nsGenericHTMLElement*>(element);
  }

  return nullptr;
}

nsGenericHTMLElement*
HTMLLabelElement::GetFirstLabelableDescendant() const
{
  for (nsIContent* cur = nsINode::GetFirstChild(); cur;
       cur = cur->GetNextNode(this)) {
    Element* element = cur->IsElement() ? cur->AsElement() : nullptr;
    if (element && element->IsLabelable()) {
      return static_cast<nsGenericHTMLElement*>(element);
    }
  }

  return nullptr;
}

} 
} 
