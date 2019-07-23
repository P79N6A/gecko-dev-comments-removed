



































#include "nsIDOMHTMLButtonElement.h"
#include "nsIDOMNSHTMLButtonElement.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIFormControl.h"
#include "nsIForm.h"
#include "nsIFormSubmission.h"
#include "nsIURL.h"

#include "nsIFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIEventStateManager.h"
#include "nsIDOMEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsIDocument.h"
#include "nsGUIEvent.h"
#include "nsUnicharUtils.h"
#include "nsLayoutUtils.h"
#include "nsEventDispatcher.h"
#include "nsPresState.h"
#include "nsLayoutErrors.h"
#include "nsFocusManager.h"
#include "nsHTMLFormElement.h"

#define NS_IN_SUBMIT_CLICK      (1 << 0)
#define NS_OUTER_ACTIVATE_EVENT (1 << 1)

class nsHTMLButtonElement : public nsGenericHTMLFormElement,
                            public nsIDOMHTMLButtonElement,
                            public nsIDOMNSHTMLButtonElement
{
public:
  nsHTMLButtonElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLButtonElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFormElement::)

  
  NS_DECL_NSIDOMHTMLBUTTONELEMENT

  
  
  
  NS_IMETHOD Blur();
  NS_IMETHOD Focus();
  NS_IMETHOD Click();
  NS_IMETHOD SetType(const nsAString& aType);

  
  NS_IMETHOD_(PRInt32) GetType() const { return mType; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsIFormSubmission* aFormSubmission,
                               nsIContent* aSubmitElement);
  NS_IMETHOD SaveState();
  PRBool RestoreState(nsPresState* aState);

  


  virtual nsresult BeforeSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                 const nsAString* aValue, PRBool aNotify);
  
  
  virtual PRBool IsHTMLFocusable(PRBool *aIsFocusable, PRInt32 *aTabIndex);
  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual void DoneCreatingElement();

protected:
  PRInt8 mType;
  PRPackedBool mHandlingClick;
  PRPackedBool mDisabledChanged;
  PRPackedBool mInInternalActivate;

private:
  
  nsresult SetDefaultValue(const nsAString& aDefaultValue);
  nsresult GetDefaultValue(nsAString& aDefaultValue);
};





NS_IMPL_NS_NEW_HTML_ELEMENT(Button)


nsHTMLButtonElement::nsHTMLButtonElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLFormElement(aNodeInfo),
    mType(NS_FORM_BUTTON_SUBMIT),  
    mHandlingClick(PR_FALSE),
    mDisabledChanged(PR_FALSE),
    mInInternalActivate(PR_FALSE)
{
}

nsHTMLButtonElement::~nsHTMLButtonElement()
{
}



NS_IMPL_ADDREF_INHERITED(nsHTMLButtonElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLButtonElement, nsGenericElement)



NS_INTERFACE_TABLE_HEAD(nsHTMLButtonElement)
  NS_HTML_CONTENT_INTERFACE_TABLE2(nsHTMLButtonElement,
                                   nsIDOMHTMLButtonElement,
                                   nsIDOMNSHTMLButtonElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLButtonElement,
                                               nsGenericHTMLFormElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLButtonElement)




NS_IMPL_ELEMENT_CLONE(nsHTMLButtonElement)




NS_IMETHODIMP
nsHTMLButtonElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElement::GetForm(aForm);
}

NS_IMPL_STRING_ATTR(nsHTMLButtonElement, AccessKey, accesskey)
NS_IMPL_BOOL_ATTR(nsHTMLButtonElement, Disabled, disabled)
NS_IMPL_STRING_ATTR(nsHTMLButtonElement, Name, name)
NS_IMPL_INT_ATTR_DEFAULT_VALUE(nsHTMLButtonElement, TabIndex, tabindex, 0)
NS_IMPL_STRING_ATTR(nsHTMLButtonElement, Value, value)
NS_IMPL_STRING_ATTR_DEFAULT_VALUE(nsHTMLButtonElement, Type, type, "submit")

NS_IMETHODIMP
nsHTMLButtonElement::Blur()
{
  return nsGenericHTMLElement::Blur();
}

NS_IMETHODIMP
nsHTMLButtonElement::Focus()
{
  return nsGenericHTMLElement::Focus();
}

NS_IMETHODIMP
nsHTMLButtonElement::Click()
{
  if (mHandlingClick)
    return NS_OK;

  mHandlingClick = PR_TRUE;
  
  
  nsCOMPtr<nsIDocument> doc = GetCurrentDoc();

  if (doc) {
    nsIPresShell *shell = doc->GetPrimaryShell();
    if (shell) {
      nsCOMPtr<nsPresContext> context = shell->GetPresContext();
      if (context) {
        
        
        
        nsMouseEvent event(nsContentUtils::IsCallerChrome(),
                           NS_MOUSE_CLICK, nsnull,
                           nsMouseEvent::eReal);
        nsEventStatus status = nsEventStatus_eIgnore;
        nsEventDispatcher::Dispatch(static_cast<nsIContent*>(this), context,
                                    &event, nsnull, &status);
      }
    }
  }

  mHandlingClick = PR_FALSE;

  return NS_OK;
}

PRBool
nsHTMLButtonElement::IsHTMLFocusable(PRBool *aIsFocusable, PRInt32 *aTabIndex)
{
  if (nsGenericHTMLElement::IsHTMLFocusable(aIsFocusable, aTabIndex)) {
    return PR_TRUE;
  }
  if (aTabIndex && (sTabFocusModel & eTabFocus_formElementsMask) == 0) {
    *aTabIndex = -1;
  }

  *aIsFocusable = !HasAttr(kNameSpaceID_None, nsGkAtoms::disabled);

  return PR_FALSE;
}

static const nsAttrValue::EnumTable kButtonTypeTable[] = {
  { "button", NS_FORM_BUTTON_BUTTON },
  { "reset", NS_FORM_BUTTON_RESET },
  { "submit", NS_FORM_BUTTON_SUBMIT },
  { 0 }
};

PRBool
nsHTMLButtonElement::ParseAttribute(PRInt32 aNamespaceID,
                                    nsIAtom* aAttribute,
                                    const nsAString& aValue,
                                    nsAttrValue& aResult)
{
  if (aAttribute == nsGkAtoms::type && kNameSpaceID_None == aNamespaceID) {
    
    
    PRBool res = aResult.ParseEnumValue(aValue, kButtonTypeTable);
    if (res) {
      mType = aResult.GetEnumValue();
    }
    return res;
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

nsresult
nsHTMLButtonElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  
  aVisitor.mCanHandle = PR_FALSE;
  PRBool bDisabled;
  nsresult rv = GetDisabled(&bDisabled);
  if (NS_FAILED(rv) || bDisabled) {
    return rv;
  }

  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_FALSE);

  if (formControlFrame) {
    nsIFrame* formFrame = do_QueryFrame(formControlFrame);
    if (formFrame) {
      const nsStyleUserInterface* uiStyle = formFrame->GetStyleUserInterface();

      if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE ||
          uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED)
        return NS_OK;
    }
  }

  
  
  
  
  PRBool outerActivateEvent =
    (NS_IS_MOUSE_LEFT_CLICK(aVisitor.mEvent) ||
     (aVisitor.mEvent->message == NS_UI_ACTIVATE &&
      !mInInternalActivate));

  if (outerActivateEvent) {
    aVisitor.mItemFlags |= NS_OUTER_ACTIVATE_EVENT;
    if (mType == NS_FORM_BUTTON_SUBMIT && mForm) {
      aVisitor.mItemFlags |= NS_IN_SUBMIT_CLICK;
      
      
      
      mForm->OnSubmitClickBegin();
    }
  }

  return nsGenericHTMLElement::PreHandleEvent(aVisitor);
}

nsresult
nsHTMLButtonElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  nsresult rv = NS_OK;
  if (!aVisitor.mPresContext) {
    return rv;
  }

  if (aVisitor.mEventStatus != nsEventStatus_eConsumeNoDefault &&
      NS_IS_MOUSE_LEFT_CLICK(aVisitor.mEvent)) {
    nsUIEvent actEvent(NS_IS_TRUSTED_EVENT(aVisitor.mEvent), NS_UI_ACTIVATE, 1);

    nsCOMPtr<nsIPresShell> shell = aVisitor.mPresContext->GetPresShell();
    if (shell) {
      nsEventStatus status = nsEventStatus_eIgnore;
      mInInternalActivate = PR_TRUE;
      shell->HandleDOMEventWithTarget(this, &actEvent, &status);
      mInInternalActivate = PR_FALSE;

      
      
      if (status == nsEventStatus_eConsumeNoDefault)
        aVisitor.mEventStatus = status;
    }
  }

  
  if ((aVisitor.mItemFlags & NS_IN_SUBMIT_CLICK) && mForm) {
    
    
    
    
    mForm->OnSubmitClickEnd();
  }

  if (nsEventStatus_eIgnore == aVisitor.mEventStatus) {
    switch (aVisitor.mEvent->message) {
      case NS_KEY_PRESS:
      case NS_KEY_UP:
        {
          
          
          nsKeyEvent * keyEvent = (nsKeyEvent *)aVisitor.mEvent;
          if ((keyEvent->keyCode == NS_VK_RETURN &&
               NS_KEY_PRESS == aVisitor.mEvent->message) ||
              keyEvent->keyCode == NS_VK_SPACE &&
              NS_KEY_UP == aVisitor.mEvent->message) {
            nsEventStatus status = nsEventStatus_eIgnore;

            nsMouseEvent event(NS_IS_TRUSTED_EVENT(aVisitor.mEvent),
                               NS_MOUSE_CLICK, nsnull,
                               nsMouseEvent::eReal);
            nsEventDispatcher::Dispatch(static_cast<nsIContent*>(this),
                                        aVisitor.mPresContext, &event, nsnull,
                                        &status);
          }
        }
        break;

      case NS_MOUSE_BUTTON_DOWN:
        {
          if (aVisitor.mEvent->eventStructType == NS_MOUSE_EVENT) {
            if (static_cast<nsMouseEvent*>(aVisitor.mEvent)->button ==
                  nsMouseEvent::eLeftButton) {
              aVisitor.mPresContext->EventStateManager()->
                SetContentState(this, NS_EVENT_STATE_ACTIVE);
              nsIFocusManager* fm = nsFocusManager::GetFocusManager();
              if (fm)
                fm->SetFocus(this, nsIFocusManager::FLAG_BYMOUSE |
                                   nsIFocusManager::FLAG_NOSCROLL);
              aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
            } else if (static_cast<nsMouseEvent*>(aVisitor.mEvent)->button ==
                         nsMouseEvent::eMiddleButton ||
                       static_cast<nsMouseEvent*>(aVisitor.mEvent)->button ==
                         nsMouseEvent::eRightButton) {
              
              
              if (aVisitor.mDOMEvent) {
                aVisitor.mDOMEvent->StopPropagation();
              }
            }
          }
        }
        break;

      
      
      case NS_MOUSE_BUTTON_UP:
      case NS_MOUSE_DOUBLECLICK:
        {
          if (aVisitor.mEvent->eventStructType == NS_MOUSE_EVENT &&
              aVisitor.mDOMEvent &&
              (static_cast<nsMouseEvent*>(aVisitor.mEvent)->button ==
                 nsMouseEvent::eMiddleButton ||
               static_cast<nsMouseEvent*>(aVisitor.mEvent)->button ==
                 nsMouseEvent::eRightButton)) {
            aVisitor.mDOMEvent->StopPropagation();
          }
        }
        break;

      case NS_MOUSE_ENTER_SYNTH:
        {
          aVisitor.mPresContext->EventStateManager()->
            SetContentState(this, NS_EVENT_STATE_HOVER);
          aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
        }
        break;

        
      case NS_MOUSE_EXIT_SYNTH:
        {
          aVisitor.mPresContext->EventStateManager()->
            SetContentState(nsnull, NS_EVENT_STATE_HOVER);
          aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
        }
        break;

      default:
        break;
    }
    if (aVisitor.mItemFlags & NS_OUTER_ACTIVATE_EVENT) {
      if (mForm && (mType == NS_FORM_BUTTON_SUBMIT ||
                    mType == NS_FORM_BUTTON_RESET)) {
        nsFormEvent event(PR_TRUE,
                          (mType == NS_FORM_BUTTON_RESET)
                          ? NS_FORM_RESET : NS_FORM_SUBMIT);
        event.originator     = this;
        nsEventStatus status = nsEventStatus_eIgnore;

        nsCOMPtr<nsIPresShell> presShell =
          aVisitor.mPresContext->GetPresShell();
        
        
        
        
        
        
        if (presShell) {
          
          nsRefPtr<nsHTMLFormElement> form(mForm);
          presShell->HandleDOMEventWithTarget(mForm, &event, &status);
        }
      }
    }
  } else if ((aVisitor.mItemFlags & NS_IN_SUBMIT_CLICK) && mForm) {
    
    
    
    
    
    mForm->FlushPendingSubmission();
  } 

  return rv;
}

nsresult
nsHTMLButtonElement::GetDefaultValue(nsAString& aDefaultValue)
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::value, aDefaultValue);
  return NS_OK;
}

nsresult
nsHTMLButtonElement::SetDefaultValue(const nsAString& aDefaultValue)
{
  return SetAttr(kNameSpaceID_None, nsGkAtoms::value, aDefaultValue, PR_TRUE);
}

NS_IMETHODIMP
nsHTMLButtonElement::Reset()
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLButtonElement::SubmitNamesValues(nsIFormSubmission* aFormSubmission,
                                       nsIContent* aSubmitElement)
{
  nsresult rv = NS_OK;

  
  
  
  if (aSubmitElement != this) {
    return NS_OK;
  }

  
  
  
  PRBool disabled;
  rv = GetDisabled(&disabled);
  if (NS_FAILED(rv) || disabled) {
    return rv;
  }

  
  
  
  nsAutoString name;
  if (!GetAttr(kNameSpaceID_None, nsGkAtoms::name, name)) {
    return NS_OK;
  }

  
  
  
  nsAutoString value;
  rv = GetValue(value);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  
  rv = aFormSubmission->AddNameValuePair(this, name, value);

  return rv;
}

void
nsHTMLButtonElement::DoneCreatingElement()
{
  
  RestoreFormControlState(this, this);
}

nsresult
nsHTMLButtonElement::BeforeSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                   const nsAString* aValue, PRBool aNotify)
{
  if (aNotify && aName == nsGkAtoms::disabled &&
      aNameSpaceID == kNameSpaceID_None) {
    mDisabledChanged = PR_TRUE;
  }

  return nsGenericHTMLFormElement::BeforeSetAttr(aNameSpaceID, aName,
                                                 aValue, aNotify);
}

NS_IMETHODIMP
nsHTMLButtonElement::SaveState()
{
  if (!mDisabledChanged) {
    return NS_OK;
  }
  
  nsPresState *state = nsnull;
  nsresult rv = GetPrimaryPresState(this, &state);
  if (state) {
    PRBool disabled;
    GetDisabled(&disabled);
    state->SetDisabled(disabled);
  }

  return rv;
}

PRBool
nsHTMLButtonElement::RestoreState(nsPresState* aState)
{
  if (aState && aState->IsDisabledSet()) {
    SetDisabled(aState->GetDisabled());
  }

  return PR_FALSE;
}
