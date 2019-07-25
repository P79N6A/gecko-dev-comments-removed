



































#include "nsCOMPtr.h"
#include "nsIDOMHTMLLabelElement.h"
#include "nsIDOMNSHTMLLabelElement.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIFormControl.h"
#include "nsIForm.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsGUIEvent.h"
#include "nsIEventStateManager.h"
#include "nsEventDispatcher.h"
#include "nsPIDOMWindow.h"
#include "nsFocusManager.h"

class nsHTMLLabelElement : public nsGenericHTMLFormElement,
                           public nsIDOMHTMLLabelElement,
                           public nsIDOMNSHTMLLabelElement
{
public:
  nsHTMLLabelElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLLabelElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFormElement::)

  
  NS_DECL_NSIDOMHTMLLABELELEMENT

  
  NS_DECL_NSIDOMNSHTMLLABELELEMENT

  
  NS_IMETHOD_(PRUint32) GetType() const { return NS_FORM_LABEL; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission,
                               nsIContent* aSubmitElement);

  NS_IMETHOD Focus();

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);

  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify);
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify);
  virtual void PerformAccesskey(PRBool aKeyCausesActivation,
                                PRBool aIsTrustedEvent);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:
  already_AddRefed<nsIContent> GetControlContent();
  already_AddRefed<nsIContent> GetFirstFormControl(nsIContent *current);

  
  PRPackedBool mHandlingEvent;
};




NS_IMPL_NS_NEW_HTML_ELEMENT(Label)


nsHTMLLabelElement::nsHTMLLabelElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLFormElement(aNodeInfo)
  , mHandlingEvent(PR_FALSE)
{
}

nsHTMLLabelElement::~nsHTMLLabelElement()
{
}




NS_IMPL_ADDREF_INHERITED(nsHTMLLabelElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLLabelElement, nsGenericElement) 


DOMCI_DATA(HTMLLabelElement, nsHTMLLabelElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLLabelElement)
  NS_HTML_CONTENT_INTERFACE_TABLE2(nsHTMLLabelElement,
                                   nsIDOMHTMLLabelElement,
                                   nsIDOMNSHTMLLabelElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLLabelElement,
                                               nsGenericHTMLFormElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLLabelElement)





NS_IMPL_ELEMENT_CLONE(nsHTMLLabelElement)


NS_IMETHODIMP
nsHTMLLabelElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElement::GetForm(aForm);
}

NS_IMETHODIMP
nsHTMLLabelElement::GetControl(nsIDOMHTMLElement** aElement)
{
  *aElement = nsnull;

  nsCOMPtr<nsIContent> content = GetControlContent();
  nsCOMPtr<nsIDOMHTMLElement> element = do_QueryInterface(content);

  element.swap(*aElement);
  return NS_OK;
}


NS_IMPL_STRING_ATTR(nsHTMLLabelElement, AccessKey, accesskey)
NS_IMPL_STRING_ATTR(nsHTMLLabelElement, HtmlFor, _for)

NS_IMETHODIMP
nsHTMLLabelElement::Focus()
{
  
  nsIFocusManager* fm = nsFocusManager::GetFocusManager();
  if (fm) {
    nsCOMPtr<nsIContent> content = GetControlContent();

    nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(content);
    if (elem)
      fm->SetFocus(elem, 0);
  }

  return NS_OK;
}

nsresult
nsHTMLLabelElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                               nsIContent* aBindingParent,
                               PRBool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLFormElement::BindToTree(aDocument, aParent,
                                                     aBindingParent,
                                                     aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDocument) {
    RegAccessKey();
  }

  return rv;
}

void
nsHTMLLabelElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  if (IsInDoc()) {
    UnregAccessKey();
  }

  nsGenericHTMLFormElement::UnbindFromTree(aDeep, aNullParent);
}

static PRBool
EventTargetIn(nsEvent *aEvent, nsIContent *aChild, nsIContent *aStop)
{
  nsCOMPtr<nsIContent> c = do_QueryInterface(aEvent->target);
  nsIContent *content = c;
  while (content) {
    if (content == aChild) {
      return PR_TRUE;
    }

    if (content == aStop) {
      break;
    }

    content = content->GetParent();
  }
  return PR_FALSE;
}

static void
DestroyMouseDownPoint(void *    ,
                      nsIAtom * ,
                      void *    aPropertyValue,
                      void *    )
{
  nsIntPoint *pt = (nsIntPoint *)aPropertyValue;
  delete pt;
}

nsresult
nsHTMLLabelElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  if (mHandlingEvent ||
      (!NS_IS_MOUSE_LEFT_CLICK(aVisitor.mEvent) &&
       aVisitor.mEvent->message != NS_MOUSE_BUTTON_DOWN) ||
      aVisitor.mEventStatus == nsEventStatus_eConsumeNoDefault ||
      !aVisitor.mPresContext) {
    return NS_OK;
  }

  nsCOMPtr<nsIContent> content = GetControlContent();

  if (content && !EventTargetIn(aVisitor.mEvent, content, this)) {
    mHandlingEvent = PR_TRUE;
    switch (aVisitor.mEvent->message) {
      case NS_MOUSE_BUTTON_DOWN:
        NS_ASSERTION(aVisitor.mEvent->eventStructType == NS_MOUSE_EVENT,
                     "wrong event struct for event");
        if (static_cast<nsMouseEvent*>(aVisitor.mEvent)->button ==
            nsMouseEvent::eLeftButton) {
          
          
          nsIntPoint *curPoint = new nsIntPoint(aVisitor.mEvent->refPoint);
          SetProperty(nsGkAtoms::labelMouseDownPtProperty,
                      static_cast<void *>(curPoint),
                      DestroyMouseDownPoint);
        }
        break;

      case NS_MOUSE_CLICK:
        if (NS_IS_MOUSE_LEFT_CLICK(aVisitor.mEvent)) {
          const nsMouseEvent* event =
            static_cast<const nsMouseEvent*>(aVisitor.mEvent);
          nsIntPoint *mouseDownPoint = static_cast<nsIntPoint *>
            (GetProperty(nsGkAtoms::labelMouseDownPtProperty));

          PRBool dragSelect = PR_FALSE;
          if (mouseDownPoint) {
            nsIntPoint dragDistance = *mouseDownPoint;
            DeleteProperty(nsGkAtoms::labelMouseDownPtProperty);

            dragDistance -= aVisitor.mEvent->refPoint;
            const int CLICK_DISTANCE = 2;
            dragSelect = dragDistance.x > CLICK_DISTANCE ||
                         dragDistance.x < -CLICK_DISTANCE ||
                         dragDistance.y > CLICK_DISTANCE ||
                         dragDistance.y < -CLICK_DISTANCE;
          }

          
          
          
          if (dragSelect || event->clickCount > 1 ||
              event->isShift || event->isControl || event->isAlt ||
              event->isMeta) {
            break;
          }

          nsIFocusManager* fm = nsFocusManager::GetFocusManager();
          if (fm) {
            
            
            
            
            
            nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(content);
            fm->SetFocus(elem, nsIFocusManager::FLAG_BYMOVEFOCUS);
          }

          
          
          
          
          
          
          nsEventStatus status = aVisitor.mEventStatus;
          
          
          DispatchClickEvent(aVisitor.mPresContext,
                             static_cast<nsInputEvent*>(aVisitor.mEvent),
                             content, PR_FALSE, &status);
          
        }
        break;
    }
    mHandlingEvent = PR_FALSE;
  }
  return NS_OK;
}

nsresult
nsHTMLLabelElement::Reset()
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLLabelElement::SubmitNamesValues(nsFormSubmission* aFormSubmission,
                                      nsIContent* aSubmitElement)
{
  return NS_OK;
}

nsresult
nsHTMLLabelElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName, nsIAtom* aPrefix,
                            const nsAString& aValue, PRBool aNotify)
{
  if (aName == nsGkAtoms::accesskey && kNameSpaceID_None == aNameSpaceID) {
    UnregAccessKey();
  }

  nsresult rv =
      nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix, aValue,
                                    aNotify);

  if (aName == nsGkAtoms::accesskey && kNameSpaceID_None == aNameSpaceID &&
      !aValue.IsEmpty()) {
    SetFlags(NODE_HAS_ACCESSKEY);
    RegAccessKey();
  }

  return rv;
}

nsresult
nsHTMLLabelElement::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                              PRBool aNotify)
{
  if (aAttribute == nsGkAtoms::accesskey &&
      kNameSpaceID_None == aNameSpaceID) {
    
    UnregAccessKey();
    UnsetFlags(NODE_HAS_ACCESSKEY);
  }

  return nsGenericHTMLElement::UnsetAttr(aNameSpaceID, aAttribute, aNotify);
}

void
nsHTMLLabelElement::PerformAccesskey(PRBool aKeyCausesActivation,
                                     PRBool aIsTrustedEvent)
{
  if (!aKeyCausesActivation) {
    nsCOMPtr<nsIContent> content = GetControlContent();
    if (content)
      content->PerformAccesskey(aKeyCausesActivation, aIsTrustedEvent);
  } else {
    nsPresContext *presContext = GetPresContext();
    if (!presContext)
      return;

    
    nsMouseEvent event(aIsTrustedEvent, NS_MOUSE_CLICK,
                       nsnull, nsMouseEvent::eReal);
    event.inputSource = nsIDOMNSMouseEvent::MOZ_SOURCE_KEYBOARD;

    nsAutoPopupStatePusher popupStatePusher(aIsTrustedEvent ?
                                            openAllowed : openAbused);

    nsEventDispatcher::Dispatch(static_cast<nsIContent*>(this), presContext,
                                &event);
  }
}

already_AddRefed<nsIContent>
nsHTMLLabelElement::GetControlContent()
{
  nsAutoString elementId;

  if (!GetAttr(kNameSpaceID_None, nsGkAtoms::_for, elementId)) {
    
    
    return GetFirstFormControl(this);
  }

  
  
  nsIDocument* doc = GetCurrentDoc();
  if (!doc) {
    return nsnull;
  }

  nsIContent* content = doc->GetElementById(elementId);
  if (!content) {
    return nsnull;
  }

  nsCOMPtr<nsIFormControl> element = do_QueryInterface(content);
  if (element && element->IsLabelableControl()) {
    NS_ADDREF(content);
    return content;
  }

  return nsnull;
}

already_AddRefed<nsIContent>
nsHTMLLabelElement::GetFirstFormControl(nsIContent *current)
{
  PRUint32 numNodes = current->GetChildCount();

  for (PRUint32 i = 0; i < numNodes; i++) {
    nsIContent *child = current->GetChildAt(i);
    if (!child) {
      continue;
    }

    nsCOMPtr<nsIFormControl> element = do_QueryInterface(child);
    if (element && element->IsLabelableControl()) {
      NS_ADDREF(child);
      return child;
    }

    nsIContent* content = GetFirstFormControl(child).get();
    if (content) {
      return content;
    }
  }

  return nsnull;
}

