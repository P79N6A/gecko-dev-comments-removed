



































#include "nsCOMPtr.h"
#include "nsIDOMHTMLLabelElement.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIFormControl.h"
#include "nsIForm.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMXULDocument.h"
#include "nsIDocument.h"
#include "nsIFormControlFrame.h"
#include "nsIPresShell.h"
#include "nsGUIEvent.h"
#include "nsIEventStateManager.h"
#include "nsEventDispatcher.h"
#include "nsPIDOMWindow.h"

class nsHTMLLabelElement : public nsGenericHTMLFormElement,
                           public nsIDOMHTMLLabelElement
{
public:
  nsHTMLLabelElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLLabelElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFormElement::)

  
  NS_DECL_NSIDOMHTMLLABELELEMENT

  
  NS_IMETHOD_(PRInt32) GetType() const { return NS_FORM_LABEL; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsIFormSubmission* aFormSubmission,
                               nsIContent* aSubmitElement);

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);

  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

  virtual void SetFocus(nsPresContext* aContext);
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
  already_AddRefed<nsIContent> GetForContent();
  already_AddRefed<nsIContent> GetFirstFormControl(nsIContent *current);

  
  PRBool mHandlingEvent;
};




NS_IMPL_NS_NEW_HTML_ELEMENT(Label)


nsHTMLLabelElement::nsHTMLLabelElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLFormElement(aNodeInfo),
    mHandlingEvent(PR_FALSE)
{
}

nsHTMLLabelElement::~nsHTMLLabelElement()
{
}




NS_IMPL_ADDREF_INHERITED(nsHTMLLabelElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLLabelElement, nsGenericElement) 



NS_HTML_CONTENT_INTERFACE_TABLE_HEAD(nsHTMLLabelElement,
                                     nsGenericHTMLFormElement)
  NS_INTERFACE_TABLE_INHERITED1(nsHTMLLabelElement, nsIDOMHTMLLabelElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLLabelElement)





NS_IMPL_ELEMENT_CLONE(nsHTMLLabelElement)


NS_IMETHODIMP
nsHTMLLabelElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElement::GetForm(aForm);
}


NS_IMPL_STRING_ATTR(nsHTMLLabelElement, AccessKey, accesskey)
NS_IMPL_STRING_ATTR(nsHTMLLabelElement, HtmlFor, _for)

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
    RegUnRegAccessKey(PR_TRUE);
  }

  return rv;
}

void
nsHTMLLabelElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  if (IsInDoc()) {
    RegUnRegAccessKey(PR_FALSE);
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

nsresult
nsHTMLLabelElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  if (mHandlingEvent ||
      (!NS_IS_MOUSE_LEFT_CLICK(aVisitor.mEvent) &&
       aVisitor.mEvent->message != NS_FOCUS_CONTENT) ||
      aVisitor.mEventStatus == nsEventStatus_eConsumeNoDefault ||
      !aVisitor.mPresContext) {
    return NS_OK;
  }

  nsCOMPtr<nsIContent> content = GetForContent();
  if (content && !EventTargetIn(aVisitor.mEvent, content, this)) {
    mHandlingEvent = PR_TRUE;
    switch (aVisitor.mEvent->message) {
      case NS_MOUSE_CLICK:
        if (NS_IS_MOUSE_LEFT_CLICK(aVisitor.mEvent)) {
          if (ShouldFocus(this)) {
            
            aVisitor.mPresContext->EventStateManager()->
              ChangeFocusWith(content, nsIEventStateManager::eEventFocusedByKey);
          }

          
          
          
          
          
          
          nsEventStatus status = aVisitor.mEventStatus;
          
          
          DispatchClickEvent(aVisitor.mPresContext,
                             static_cast<nsInputEvent*>(aVisitor.mEvent),
                             content, PR_FALSE, &status);
          
        }
        break;
      case NS_FOCUS_CONTENT:
        
        
        
        
        
        
        {
          nsEvent event(NS_IS_TRUSTED_EVENT(aVisitor.mEvent), NS_FOCUS_CONTENT);
          event.flags |= NS_EVENT_FLAG_CANT_BUBBLE;
          nsEventStatus status = aVisitor.mEventStatus;
          DispatchEvent(aVisitor.mPresContext, &event,
                        content, PR_TRUE, &status);
          
        }
        break;
    }
    mHandlingEvent = PR_FALSE;
  }
  return NS_OK;
}

void
nsHTMLLabelElement::SetFocus(nsPresContext* aContext)
{
  
  
  nsCOMPtr<nsIContent> content = GetForContent();
  if (content)
    content->SetFocus(aContext);
}

nsresult
nsHTMLLabelElement::Reset()
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLLabelElement::SubmitNamesValues(nsIFormSubmission* aFormSubmission,
                                      nsIContent* aSubmitElement)
{
  return NS_OK;
}

nsresult
nsHTMLLabelElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName, nsIAtom* aPrefix,
                            const nsAString& aValue, PRBool aNotify)
{
  if (aName == nsGkAtoms::accesskey && kNameSpaceID_None == aNameSpaceID) {
    RegUnRegAccessKey(PR_FALSE);
  }

  nsresult rv =
      nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix, aValue,
                                    aNotify);

  if (aName == nsGkAtoms::accesskey && kNameSpaceID_None == aNameSpaceID &&
      !aValue.IsEmpty()) {
    RegUnRegAccessKey(PR_TRUE);
  }

  return rv;
}

nsresult
nsHTMLLabelElement::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                              PRBool aNotify)
{
  if (aAttribute == nsGkAtoms::accesskey &&
      kNameSpaceID_None == aNameSpaceID) {
    RegUnRegAccessKey(PR_FALSE);
  }

  return nsGenericHTMLElement::UnsetAttr(aNameSpaceID, aAttribute, aNotify);
}

void
nsHTMLLabelElement::PerformAccesskey(PRBool aKeyCausesActivation,
                                     PRBool aIsTrustedEvent)
{
  if (!aKeyCausesActivation) {
    nsCOMPtr<nsIContent> content = GetForContent();
    if (content)
      content->PerformAccesskey(aKeyCausesActivation, aIsTrustedEvent);
  } else {
    nsPresContext *presContext = GetPresContext();
    if (!presContext)
      return;

    
    nsMouseEvent event(aIsTrustedEvent, NS_MOUSE_CLICK,
                       nsnull, nsMouseEvent::eReal);

    nsAutoPopupStatePusher popupStatePusher(aIsTrustedEvent ?
                                            openAllowed : openAbused);

    nsEventDispatcher::Dispatch(static_cast<nsIContent*>(this), presContext,
                                &event);
  }
}

inline PRBool IsNonLabelFormControl(nsIContent *aContent)
{
  return aContent->IsNodeOfType(nsINode::eHTML_FORM_CONTROL) &&
         aContent->Tag() != nsGkAtoms::label;
}

already_AddRefed<nsIContent>
nsHTMLLabelElement::GetForContent()
{
  nsresult rv;

  
  nsAutoString elementId;
  rv = GetHtmlFor(elementId);
  if (NS_SUCCEEDED(rv) && !elementId.IsEmpty()) {
    
    nsCOMPtr<nsIDOMDocument> domDoc;
    GetOwnerDocument(getter_AddRefs(domDoc));
    if (domDoc) {
      nsCOMPtr<nsIDOMElement> domElement;
      domDoc->GetElementById(elementId, getter_AddRefs(domElement));
      nsIContent *result = nsnull;
      if (domElement) {
        CallQueryInterface(domElement, &result);
        if (result && !IsNonLabelFormControl(result)) {
          NS_RELEASE(result); 
        }
      }
      return result;
    }
  } else {
    
    
    return GetFirstFormControl(this);
  }
  return nsnull;
}

already_AddRefed<nsIContent>
nsHTMLLabelElement::GetFirstFormControl(nsIContent *current)
{
  PRUint32 numNodes = current->GetChildCount();

  for (PRUint32 i = 0; i < numNodes; i++) {
    nsIContent *child = current->GetChildAt(i);
    if (child) {
      if (IsNonLabelFormControl(child)) {
        NS_ADDREF(child);
        return child;
      }

      nsIContent* content = GetFirstFormControl(child).get();
      if (content) {
        return content;
      }
    }
  }

  return nsnull;
}
