







#include "HTMLLabelElement.h"
#include "mozilla/dom/HTMLLabelElementBinding.h"
#include "nsEventDispatcher.h"
#include "nsFocusManager.h"
#include "nsGUIEvent.h"
#include "nsIDOMMouseEvent.h"



NS_IMPL_NS_NEW_HTML_ELEMENT(Label)

namespace mozilla {
namespace dom {

HTMLLabelElement::~HTMLLabelElement()
{
}

JSObject*
HTMLLabelElement::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aScope)
{
  return HTMLLabelElementBinding::Wrap(aCx, aScope, this);
}



NS_IMPL_ISUPPORTS_INHERITED1(HTMLLabelElement, nsGenericHTMLFormElement,
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
EventTargetIn(nsEvent *aEvent, nsIContent *aChild, nsIContent *aStop)
{
  nsCOMPtr<nsIContent> c = do_QueryInterface(aEvent->target);
  nsIContent *content = c;
  while (content) {
    if (content == aChild) {
      return true;
    }

    if (content == aStop) {
      break;
    }

    content = content->GetParent();
  }
  return false;
}

static void
DestroyMouseDownPoint(void *    ,
                      nsIAtom * ,
                      void *    aPropertyValue,
                      void *    )
{
  LayoutDeviceIntPoint* pt = static_cast<LayoutDeviceIntPoint*>(aPropertyValue);
  delete pt;
}

nsresult
HTMLLabelElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  if (mHandlingEvent ||
      (!aVisitor.mEvent->IsLeftClickEvent() &&
       aVisitor.mEvent->message != NS_MOUSE_BUTTON_DOWN) ||
      aVisitor.mEventStatus == nsEventStatus_eConsumeNoDefault ||
      !aVisitor.mPresContext ||
      
      aVisitor.mEvent->mFlags.mMultipleActionsPrevented) {
    return NS_OK;
  }

  
  nsRefPtr<Element> content = GetLabeledElement();

  if (content && !EventTargetIn(aVisitor.mEvent, content, this)) {
    mHandlingEvent = true;
    switch (aVisitor.mEvent->message) {
      case NS_MOUSE_BUTTON_DOWN:
        NS_ASSERTION(aVisitor.mEvent->eventStructType == NS_MOUSE_EVENT,
                     "wrong event struct for event");
        if (static_cast<nsMouseEvent*>(aVisitor.mEvent)->button ==
            nsMouseEvent::eLeftButton) {
          
          
          LayoutDeviceIntPoint* curPoint =
            new LayoutDeviceIntPoint(aVisitor.mEvent->refPoint);
          SetProperty(nsGkAtoms::labelMouseDownPtProperty,
                      static_cast<void*>(curPoint),
                      DestroyMouseDownPoint);
        }
        break;

      case NS_MOUSE_CLICK:
        if (aVisitor.mEvent->IsLeftClickEvent()) {
          const nsMouseEvent* event =
            static_cast<const nsMouseEvent*>(aVisitor.mEvent);
          LayoutDeviceIntPoint* mouseDownPoint =
            static_cast<LayoutDeviceIntPoint*>(
              GetProperty(nsGkAtoms::labelMouseDownPtProperty));

          bool dragSelect = false;
          if (mouseDownPoint) {
            LayoutDeviceIntPoint dragDistance = *mouseDownPoint;
            DeleteProperty(nsGkAtoms::labelMouseDownPtProperty);

            dragDistance -= aVisitor.mEvent->refPoint;
            const int CLICK_DISTANCE = 2;
            dragSelect = dragDistance.x > CLICK_DISTANCE ||
                         dragDistance.x < -CLICK_DISTANCE ||
                         dragDistance.y > CLICK_DISTANCE ||
                         dragDistance.y < -CLICK_DISTANCE;
          }
          
          
          if (dragSelect || event->IsShift() || event->IsControl() ||
              event->IsAlt() || event->IsMeta()) {
            break;
          }
          
          
          if (event->clickCount <= 1) {
            nsIFocusManager* fm = nsFocusManager::GetFocusManager();
            if (fm) {
              
              
              
              
              
              nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(content);
              fm->SetFocus(elem, nsIFocusManager::FLAG_BYMOVEFOCUS);
            }
          }
          
          
          
          
          
          
          nsEventStatus status = aVisitor.mEventStatus;
          
          
          EventFlags eventFlags;
          eventFlags.mMultipleActionsPrevented = true;
          DispatchClickEvent(aVisitor.mPresContext,
                             static_cast<nsInputEvent*>(aVisitor.mEvent),
                             content, false, &eventFlags, &status);
          
          
          aVisitor.mEvent->mFlags.mMultipleActionsPrevented = true;
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
    nsPresContext *presContext = GetPresContext();
    if (!presContext)
      return;

    
    nsMouseEvent event(aIsTrustedEvent, NS_MOUSE_CLICK,
                       nullptr, nsMouseEvent::eReal);
    event.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_KEYBOARD;

    nsAutoPopupStatePusher popupStatePusher(aIsTrustedEvent ?
                                            openAllowed : openAbused);

    nsEventDispatcher::Dispatch(static_cast<nsIContent*>(this), presContext,
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

  
  
  nsIDocument* doc = GetCurrentDoc();
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
