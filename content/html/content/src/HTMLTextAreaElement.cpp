





#include "mozilla/dom/HTMLTextAreaElement.h"
#include "mozilla/dom/HTMLTextAreaElementBinding.h"
#include "mozilla/Util.h"

#include "nsIControllers.h"
#include "nsFocusManager.h"
#include "nsPIDOMWindow.h"
#include "nsContentCID.h"
#include "nsIComponentManager.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIFormControl.h"
#include "nsIForm.h"
#include "nsFormSubmission.h"
#include "nsIDOMEventTarget.h"
#include "nsAttrValueInlines.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsMappedAttributes.h"
#include "nsIFormControlFrame.h"
#include "nsITextControlFrame.h"
#include "nsLinebreakConverter.h"
#include "nsIDocument.h"
#include "nsIFrame.h"
#include "nsGUIEvent.h"
#include "nsPresState.h"
#include "nsReadableUtils.h"
#include "nsEventDispatcher.h"
#include "nsLayoutUtils.h"
#include "nsError.h"
#include "mozAutoDocUpdate.h"
#include "nsISupportsPrimitives.h"
#include "nsContentCreatorFunctions.h"
#include "nsIConstraintValidation.h"

#include "nsTextEditorState.h"

static NS_DEFINE_CID(kXULControllersCID,  NS_XULCONTROLLERS_CID);

#define NS_NO_CONTENT_DISPATCH (1 << 0)

NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(TextArea)

namespace mozilla {
namespace dom {

HTMLTextAreaElement::HTMLTextAreaElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                                         FromParser aFromParser)
  : nsGenericHTMLFormElement(aNodeInfo),
    mValueChanged(false),
    mHandlingSelect(false),
    mDoneAddingChildren(!aFromParser),
    mInhibitStateRestoration(!!(aFromParser & FROM_PARSER_FRAGMENT)),
    mDisabledChanged(false),
    mCanShowInvalidUI(true),
    mCanShowValidUI(true),
    mState(this)
{
  AddMutationObserver(this);

  
  
  
  
  
  AddStatesSilently(NS_EVENT_STATE_ENABLED |
                    NS_EVENT_STATE_OPTIONAL |
                    NS_EVENT_STATE_VALID);

  SetIsDOMBinding();
}


NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(HTMLTextAreaElement,
                                                nsGenericHTMLFormElement)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mValidity)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mControllers)
  tmp->mState.Unlink();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(HTMLTextAreaElement,
                                                  nsGenericHTMLFormElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mValidity)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mControllers)
  tmp->mState.Traverse(cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(HTMLTextAreaElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLTextAreaElement, Element)



NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(HTMLTextAreaElement)
  NS_HTML_CONTENT_INTERFACE_TABLE5(HTMLTextAreaElement,
                                   nsIDOMHTMLTextAreaElement,
                                   nsITextControlElement,
                                   nsIDOMNSEditableElement,
                                   nsIMutationObserver,
                                   nsIConstraintValidation)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLTextAreaElement,
                                               nsGenericHTMLFormElement)
NS_HTML_CONTENT_INTERFACE_MAP_END





NS_IMPL_ELEMENT_CLONE(HTMLTextAreaElement)


NS_IMPL_NSICONSTRAINTVALIDATION_EXCEPT_SETCUSTOMVALIDITY(HTMLTextAreaElement)


NS_IMETHODIMP
HTMLTextAreaElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElement::GetForm(aForm);
}




NS_IMETHODIMP
HTMLTextAreaElement::Select()
{
  
  

  FocusTristate state = FocusState();
  if (state == eUnfocusable) {
    return NS_OK;
  }

  nsIFocusManager* fm = nsFocusManager::GetFocusManager();

  nsRefPtr<nsPresContext> presContext = GetPresContext();
  if (state == eInactiveWindow) {
    if (fm)
      fm->SetFocus(this, nsIFocusManager::FLAG_NOSCROLL);
    SelectAll(presContext);
    return NS_OK;
  }

  nsEventStatus status = nsEventStatus_eIgnore;
  nsGUIEvent event(true, NS_FORM_SELECTED, nullptr);
  
  nsEventDispatcher::Dispatch(static_cast<nsIContent*>(this), presContext,
                              &event, nullptr, &status);

  
  
  if (status == nsEventStatus_eIgnore) {
    if (fm) {
      fm->SetFocus(this, nsIFocusManager::FLAG_NOSCROLL);

      
      nsCOMPtr<nsIDOMElement> focusedElement;
      fm->GetFocusedElement(getter_AddRefs(focusedElement));
      if (SameCOMIdentity(static_cast<nsIDOMNode*>(this), focusedElement)) {
        
        SelectAll(presContext);
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
HTMLTextAreaElement::SelectAll(nsPresContext* aPresContext)
{
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(true);

  if (formControlFrame) {
    formControlFrame->SetFormProperty(nsGkAtoms::select, EmptyString());
  }

  return NS_OK;
}

bool
HTMLTextAreaElement::IsHTMLFocusable(bool aWithMouse,
                                     bool *aIsFocusable, int32_t *aTabIndex)
{
  if (nsGenericHTMLFormElement::IsHTMLFocusable(aWithMouse, aIsFocusable, aTabIndex)) {
    return true;
  }

  
  *aIsFocusable = !IsDisabled();
  return false;
}

NS_IMPL_BOOL_ATTR(HTMLTextAreaElement, Autofocus, autofocus)
NS_IMPL_UINT_ATTR_NON_ZERO_DEFAULT_VALUE(HTMLTextAreaElement, Cols, cols, DEFAULT_COLS)
NS_IMPL_BOOL_ATTR(HTMLTextAreaElement, Disabled, disabled)
NS_IMPL_NON_NEGATIVE_INT_ATTR(HTMLTextAreaElement, MaxLength, maxlength)
NS_IMPL_STRING_ATTR(HTMLTextAreaElement, Name, name)
NS_IMPL_BOOL_ATTR(HTMLTextAreaElement, ReadOnly, readonly)
NS_IMPL_BOOL_ATTR(HTMLTextAreaElement, Required, required)
NS_IMPL_UINT_ATTR_NON_ZERO_DEFAULT_VALUE(HTMLTextAreaElement, Rows, rows, DEFAULT_ROWS_TEXTAREA)
NS_IMPL_STRING_ATTR(HTMLTextAreaElement, Wrap, wrap)
NS_IMPL_STRING_ATTR(HTMLTextAreaElement, Placeholder, placeholder)
  
int32_t
HTMLTextAreaElement::TabIndexDefault()
{
  return 0;
}

NS_IMETHODIMP 
HTMLTextAreaElement::GetType(nsAString& aType)
{
  aType.AssignLiteral("textarea");

  return NS_OK;
}

NS_IMETHODIMP 
HTMLTextAreaElement::GetValue(nsAString& aValue)
{
  GetValueInternal(aValue, true);
  return NS_OK;
}

void
HTMLTextAreaElement::GetValueInternal(nsAString& aValue, bool aIgnoreWrap) const
{
  mState.GetValue(aValue, aIgnoreWrap);
}

NS_IMETHODIMP_(nsIEditor*)
HTMLTextAreaElement::GetTextEditor()
{
  return GetEditor();
}

NS_IMETHODIMP_(nsISelectionController*)
HTMLTextAreaElement::GetSelectionController()
{
  return mState.GetSelectionController();
}

NS_IMETHODIMP_(nsFrameSelection*)
HTMLTextAreaElement::GetConstFrameSelection()
{
  return mState.GetConstFrameSelection();
}

NS_IMETHODIMP
HTMLTextAreaElement::BindToFrame(nsTextControlFrame* aFrame)
{
  return mState.BindToFrame(aFrame);
}

NS_IMETHODIMP_(void)
HTMLTextAreaElement::UnbindFromFrame(nsTextControlFrame* aFrame)
{
  if (aFrame) {
    mState.UnbindFromFrame(aFrame);
  }
}

NS_IMETHODIMP
HTMLTextAreaElement::CreateEditor()
{
  return mState.PrepareEditor();
}

NS_IMETHODIMP_(nsIContent*)
HTMLTextAreaElement::GetRootEditorNode()
{
  return mState.GetRootNode();
}

NS_IMETHODIMP_(nsIContent*)
HTMLTextAreaElement::CreatePlaceholderNode()
{
  NS_ENSURE_SUCCESS(mState.CreatePlaceholderNode(), nullptr);
  return mState.GetPlaceholderNode();
}

NS_IMETHODIMP_(nsIContent*)
HTMLTextAreaElement::GetPlaceholderNode()
{
  return mState.GetPlaceholderNode();
}

NS_IMETHODIMP_(void)
HTMLTextAreaElement::UpdatePlaceholderVisibility(bool aNotify)
{
  mState.UpdatePlaceholderVisibility(aNotify);
}

NS_IMETHODIMP_(bool)
HTMLTextAreaElement::GetPlaceholderVisibility()
{
  return mState.GetPlaceholderVisibility();
}

nsresult
HTMLTextAreaElement::SetValueInternal(const nsAString& aValue,
                                      bool aUserInput)
{
  
  
  
  SetValueChanged(true);
  mState.SetValue(aValue, aUserInput, true);

  return NS_OK;
}

NS_IMETHODIMP 
HTMLTextAreaElement::SetValue(const nsAString& aValue)
{
  
  
  
  
  
  
  
  nsAutoString currentValue;
  GetValueInternal(currentValue, true);

  SetValueInternal(aValue, false);

  if (mFocusedValue.Equals(currentValue)) {
    GetValueInternal(mFocusedValue, true);
  }

  return NS_OK;
}

NS_IMETHODIMP 
HTMLTextAreaElement::SetUserInput(const nsAString& aValue)
{
  if (!nsContentUtils::IsCallerChrome()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }
  SetValueInternal(aValue, true);
  return NS_OK;
}

NS_IMETHODIMP
HTMLTextAreaElement::SetValueChanged(bool aValueChanged)
{
  bool previousValue = mValueChanged;

  mValueChanged = aValueChanged;
  if (!aValueChanged && !mState.IsEmpty()) {
    mState.EmptyValue();
  }

  if (mValueChanged != previousValue) {
    UpdateState(true);
  }

  return NS_OK;
}

NS_IMETHODIMP
HTMLTextAreaElement::GetDefaultValue(nsAString& aDefaultValue)
{
  nsContentUtils::GetNodeTextContent(this, false, aDefaultValue);
  return NS_OK;
}  

NS_IMETHODIMP
HTMLTextAreaElement::SetDefaultValue(const nsAString& aDefaultValue)
{
  ErrorResult error;
  SetDefaultValue(aDefaultValue, error);
  return error.ErrorCode();
}

void
HTMLTextAreaElement::SetDefaultValue(const nsAString& aDefaultValue, ErrorResult& aError)
{
  nsresult rv = nsContentUtils::SetNodeTextContent(this, aDefaultValue, true);
  if (NS_SUCCEEDED(rv) && !mValueChanged) {
    Reset();
  }
  if (NS_FAILED(rv)) {
    aError.Throw(rv);
  }
}

bool
HTMLTextAreaElement::ParseAttribute(int32_t aNamespaceID,
                                    nsIAtom* aAttribute,
                                    const nsAString& aValue,
                                    nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::maxlength) {
      return aResult.ParseNonNegativeIntValue(aValue);
    } else if (aAttribute == nsGkAtoms::cols ||
               aAttribute == nsGkAtoms::rows) {
      return aResult.ParsePositiveIntValue(aValue);
    }
  }
  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

static void
MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                      nsRuleData* aData)
{
  nsGenericHTMLFormElement::MapDivAlignAttributeInto(aAttributes, aData);
  nsGenericHTMLFormElement::MapCommonAttributesInto(aAttributes, aData);
}

nsChangeHint
HTMLTextAreaElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                            int32_t aModType) const
{
  nsChangeHint retval =
      nsGenericHTMLFormElement::GetAttributeChangeHint(aAttribute, aModType);
  if (aAttribute == nsGkAtoms::rows ||
      aAttribute == nsGkAtoms::cols) {
    NS_UpdateHint(retval, NS_STYLE_HINT_REFLOW);
  } else if (aAttribute == nsGkAtoms::wrap) {
    NS_UpdateHint(retval, nsChangeHint_ReconstructFrame);
  } else if (aAttribute == nsGkAtoms::placeholder) {
    NS_UpdateHint(retval, NS_STYLE_HINT_FRAMECHANGE);
  }
  return retval;
}

NS_IMETHODIMP_(bool)
HTMLTextAreaElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry* const map[] = {
    sDivAlignAttributeMap,
    sCommonAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map);
}

nsMapRuleToAttributesFunc
HTMLTextAreaElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}

bool
HTMLTextAreaElement::IsDisabledForEvents(uint32_t aMessage)
{
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(false);
  nsIFrame* formFrame = NULL;
  if (formControlFrame) {
    formFrame = do_QueryFrame(formControlFrame);
  }
  return IsElementDisabledForEvents(aMessage, formFrame);
}

nsresult
HTMLTextAreaElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  aVisitor.mCanHandle = false;
  if (IsDisabledForEvents(aVisitor.mEvent->message)) {
    return NS_OK;
  }

  
  
  if (aVisitor.mEvent->message == NS_FORM_SELECTED) {
    if (mHandlingSelect) {
      return NS_OK;
    }
    mHandlingSelect = true;
  }

  
  
  
  if (aVisitor.mEvent->mFlags.mNoContentDispatch) {
    aVisitor.mItemFlags |= NS_NO_CONTENT_DISPATCH;
  }
  if (aVisitor.mEvent->message == NS_MOUSE_CLICK &&
      aVisitor.mEvent->eventStructType == NS_MOUSE_EVENT &&
      static_cast<nsMouseEvent*>(aVisitor.mEvent)->button ==
        nsMouseEvent::eMiddleButton) {
    aVisitor.mEvent->mFlags.mNoContentDispatch = false;
  }

  
  if (aVisitor.mEvent->message == NS_BLUR_CONTENT) {
    FireChangeEventIfNeeded();
  }

  return nsGenericHTMLFormElement::PreHandleEvent(aVisitor);
}

void
HTMLTextAreaElement::FireChangeEventIfNeeded()
{
  nsString value;
  GetValueInternal(value, true);

  if (mFocusedValue.Equals(value)) {
    return;
  }

  
  mFocusedValue = value;
  nsContentUtils::DispatchTrustedEvent(OwnerDoc(),
                                       static_cast<nsIContent*>(this),
                                       NS_LITERAL_STRING("change"), true,
                                       false);
}

nsresult
HTMLTextAreaElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  if (aVisitor.mEvent->message == NS_FORM_SELECTED) {
    mHandlingSelect = false;
  }

  if (aVisitor.mEvent->message == NS_FOCUS_CONTENT ||
      aVisitor.mEvent->message == NS_BLUR_CONTENT) {
    if (aVisitor.mEvent->message == NS_FOCUS_CONTENT) {
      
      
      GetValueInternal(mFocusedValue, true);
      mCanShowInvalidUI = !IsValid() && ShouldShowValidityUI();

      
      
      mCanShowValidUI = ShouldShowValidityUI();
    } else { 
      mCanShowInvalidUI = true;
      mCanShowValidUI = true;
    }

    UpdateState(true);
  }

  
  aVisitor.mEvent->mFlags.mNoContentDispatch =
    ((aVisitor.mItemFlags & NS_NO_CONTENT_DISPATCH) != 0);

  return NS_OK;
}

void
HTMLTextAreaElement::DoneAddingChildren(bool aHaveNotified)
{
  if (!mValueChanged) {
    if (!mDoneAddingChildren) {
      
      
      Reset();
    }
    if (!mInhibitStateRestoration) {
      RestoreFormControlState(this, this);
    }
  }

  mDoneAddingChildren = true;
}

bool
HTMLTextAreaElement::IsDoneAddingChildren()
{
  return mDoneAddingChildren;
}



nsIControllers*
HTMLTextAreaElement::GetControllers(ErrorResult& aError)
{
  if (!mControllers)
  {
    nsresult rv;
    mControllers = do_CreateInstance(kXULControllersCID, &rv);
    if (NS_FAILED(rv)) {
      aError.Throw(rv);
      return nullptr;
    }

    nsCOMPtr<nsIController> controller = do_CreateInstance("@mozilla.org/editor/editorcontroller;1", &rv);
    if (NS_FAILED(rv)) {
      aError.Throw(rv);
      return nullptr;
    }

    mControllers->AppendController(controller);

    controller = do_CreateInstance("@mozilla.org/editor/editingcontroller;1", &rv);
    if (NS_FAILED(rv)) {
      aError.Throw(rv);
      return nullptr;
    }

    mControllers->AppendController(controller);
  }

  return mControllers;
}

NS_IMETHODIMP
HTMLTextAreaElement::GetControllers(nsIControllers** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  ErrorResult error;
  *aResult = GetControllers(error);
  NS_IF_ADDREF(*aResult);

  return error.ErrorCode();
}

uint32_t
HTMLTextAreaElement::GetTextLength()
{
  nsAutoString val;
  GetValue(val);
  return val.Length();
}

NS_IMETHODIMP
HTMLTextAreaElement::GetTextLength(int32_t *aTextLength)
{
  NS_ENSURE_ARG_POINTER(aTextLength);
  *aTextLength = GetTextLength();

  return NS_OK;
}

NS_IMETHODIMP
HTMLTextAreaElement::GetSelectionStart(int32_t *aSelectionStart)
{
  NS_ENSURE_ARG_POINTER(aSelectionStart);

  ErrorResult error;
  *aSelectionStart = GetSelectionStart(error);
  return error.ErrorCode();
}

uint32_t
HTMLTextAreaElement::GetSelectionStart(ErrorResult& aError)
{
  int32_t selStart, selEnd;
  nsresult rv = GetSelectionRange(&selStart, &selEnd);

  if (NS_FAILED(rv) && mState.IsSelectionCached()) {
    return mState.GetSelectionProperties().mStart;
  }
  if (NS_FAILED(rv)) {
    aError.Throw(rv);
  }
  return selStart;
}

NS_IMETHODIMP
HTMLTextAreaElement::SetSelectionStart(int32_t aSelectionStart)
{
  ErrorResult error;
  SetSelectionStart(aSelectionStart, error);
  return error.ErrorCode();
}

void
HTMLTextAreaElement::SetSelectionStart(uint32_t aSelectionStart, ErrorResult& aError)
{
  if (mState.IsSelectionCached()) {
    mState.GetSelectionProperties().mStart = aSelectionStart;
    return;
  }

  nsAutoString direction;
  nsresult rv = GetSelectionDirection(direction);
  if (NS_FAILED(rv)) {
    aError.Throw(rv);
    return;
  }
  int32_t start, end;
  rv = GetSelectionRange(&start, &end);
  if (NS_FAILED(rv)) {
    aError.Throw(rv);
    return;
  }
  start = aSelectionStart;
  if (end < start) {
    end = start;
  }
  rv = SetSelectionRange(start, end, direction);
  if (NS_FAILED(rv)) {
    aError.Throw(rv);
  }
}

NS_IMETHODIMP
HTMLTextAreaElement::GetSelectionEnd(int32_t *aSelectionEnd)
{
  NS_ENSURE_ARG_POINTER(aSelectionEnd);

  ErrorResult error;
  *aSelectionEnd = GetSelectionEnd(error);
  return error.ErrorCode();
}

uint32_t
HTMLTextAreaElement::GetSelectionEnd(ErrorResult& aError)
{
  int32_t selStart, selEnd;
  nsresult rv = GetSelectionRange(&selStart, &selEnd);

  if (NS_FAILED(rv) && mState.IsSelectionCached()) {
    return mState.GetSelectionProperties().mEnd;
  }
  if (NS_FAILED(rv)) {
    aError.Throw(rv);
  }
  return selEnd;
}

NS_IMETHODIMP
HTMLTextAreaElement::SetSelectionEnd(int32_t aSelectionEnd)
{
  ErrorResult error;
  SetSelectionEnd(aSelectionEnd, error);
  return error.ErrorCode();
}

void
HTMLTextAreaElement::SetSelectionEnd(uint32_t aSelectionEnd, ErrorResult& aError)
{
  if (mState.IsSelectionCached()) {
    mState.GetSelectionProperties().mEnd = aSelectionEnd;
    return;
  }

  nsAutoString direction;
  nsresult rv = GetSelectionDirection(direction);
  if (NS_FAILED(rv)) {
    aError.Throw(rv);
    return;
  }
  int32_t start, end;
  rv = GetSelectionRange(&start, &end);
  if (NS_FAILED(rv)) {
    aError.Throw(rv);
    return;
  }
  end = aSelectionEnd;
  if (start > end) {
    start = end;
  }
  rv = SetSelectionRange(start, end, direction);
  if (NS_FAILED(rv)) {
    aError.Throw(rv);
  }
}

nsresult
HTMLTextAreaElement::GetSelectionRange(int32_t* aSelectionStart,
                                       int32_t* aSelectionEnd)
{
  nsresult rv = NS_ERROR_FAILURE;
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(true);

  if (formControlFrame) {
    nsITextControlFrame* textControlFrame = do_QueryFrame(formControlFrame);
    if (textControlFrame)
      rv = textControlFrame->GetSelectionRange(aSelectionStart, aSelectionEnd);
  }

  return rv;
}

static void
DirectionToName(nsITextControlFrame::SelectionDirection dir, nsAString& aDirection)
{
  if (dir == nsITextControlFrame::eNone) {
    aDirection.AssignLiteral("none");
  } else if (dir == nsITextControlFrame::eForward) {
    aDirection.AssignLiteral("forward");
  } else if (dir == nsITextControlFrame::eBackward) {
    aDirection.AssignLiteral("backward");
  } else {
    NS_NOTREACHED("Invalid SelectionDirection value");
  }
}

nsresult
HTMLTextAreaElement::GetSelectionDirection(nsAString& aDirection)
{
  ErrorResult error;
  GetSelectionDirection(aDirection, error);
  return error.ErrorCode();
}

void
HTMLTextAreaElement::GetSelectionDirection(nsAString& aDirection, ErrorResult& aError)
{
  nsresult rv = NS_ERROR_FAILURE;
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(true);

  if (formControlFrame) {
    nsITextControlFrame* textControlFrame = do_QueryFrame(formControlFrame);
    if (textControlFrame) {
      nsITextControlFrame::SelectionDirection dir;
      rv = textControlFrame->GetSelectionRange(nullptr, nullptr, &dir);
      if (NS_SUCCEEDED(rv)) {
        DirectionToName(dir, aDirection);
      }
    }
  }

  if (NS_FAILED(rv)) {
    if (mState.IsSelectionCached()) {
      DirectionToName(mState.GetSelectionProperties().mDirection, aDirection);
      return;
    }
    aError.Throw(rv);
  }
}

NS_IMETHODIMP
HTMLTextAreaElement::SetSelectionDirection(const nsAString& aDirection)
{
  ErrorResult error;
  SetSelectionDirection(aDirection, error);
  return error.ErrorCode();
}

void
HTMLTextAreaElement::SetSelectionDirection(const nsAString& aDirection, ErrorResult& aError)
{
  if (mState.IsSelectionCached()) {
    nsITextControlFrame::SelectionDirection dir = nsITextControlFrame::eNone;
    if (aDirection.EqualsLiteral("forward")) {
      dir = nsITextControlFrame::eForward;
    } else if (aDirection.EqualsLiteral("backward")) {
      dir = nsITextControlFrame::eBackward;
    }
    mState.GetSelectionProperties().mDirection = dir;
    return;
  }

  int32_t start, end;
  nsresult rv = GetSelectionRange(&start, &end);
  if (NS_SUCCEEDED(rv)) {
    rv = SetSelectionRange(start, end, aDirection);
  }
  if (NS_FAILED(rv)) {
    aError.Throw(rv);
  }
}

NS_IMETHODIMP
HTMLTextAreaElement::SetSelectionRange(int32_t aSelectionStart,
                                       int32_t aSelectionEnd,
                                       const nsAString& aDirection)
{
  ErrorResult error;
  Optional<nsAString> dir;
  dir = &aDirection;
  SetSelectionRange(aSelectionStart, aSelectionEnd, dir, error);
  return error.ErrorCode();
}

void
HTMLTextAreaElement::SetSelectionRange(uint32_t aSelectionStart,
                                       uint32_t aSelectionEnd,
                                       const Optional<nsAString>& aDirection,
                                       ErrorResult& aError)
{
  nsresult rv = NS_ERROR_FAILURE;
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(true);

  if (formControlFrame) {
    nsITextControlFrame* textControlFrame = do_QueryFrame(formControlFrame);
    if (textControlFrame) {
      
      
      
      nsITextControlFrame::SelectionDirection dir = nsITextControlFrame::eForward;
      if (aDirection.WasPassed() && aDirection.Value().EqualsLiteral("backward")) {
        dir = nsITextControlFrame::eBackward;
      }

      rv = textControlFrame->SetSelectionRange(aSelectionStart, aSelectionEnd, dir);
      if (NS_SUCCEEDED(rv)) {
        rv = textControlFrame->ScrollSelectionIntoView();
      }
    }
  }

  if (NS_FAILED(rv)) {
    aError.Throw(rv);
  }
}

nsresult
HTMLTextAreaElement::Reset()
{
  nsresult rv;

  
  
  SetValue(EmptyString());
  nsAutoString resetVal;
  GetDefaultValue(resetVal);
  rv = SetValue(resetVal);
  NS_ENSURE_SUCCESS(rv, rv);

  SetValueChanged(false);
  return NS_OK;
}

NS_IMETHODIMP
HTMLTextAreaElement::SubmitNamesValues(nsFormSubmission* aFormSubmission)
{
  
  if (IsDisabled()) {
    return NS_OK;
  }

  
  
  
  nsAutoString name;
  GetAttr(kNameSpaceID_None, nsGkAtoms::name, name);
  if (name.IsEmpty()) {
    return NS_OK;
  }

  
  
  
  nsAutoString value;
  GetValueInternal(value, false);

  
  
  
  return aFormSubmission->AddNameValuePair(name, value);
}

NS_IMETHODIMP
HTMLTextAreaElement::SaveState()
{
  nsresult rv = NS_OK;

  
  nsPresState *state = nullptr;
  if (mValueChanged) {
    rv = GetPrimaryPresState(this, &state);
    if (state) {
      nsAutoString value;
      GetValueInternal(value, true);

      rv = nsLinebreakConverter::ConvertStringLineBreaks(
               value,
               nsLinebreakConverter::eLinebreakPlatform,
               nsLinebreakConverter::eLinebreakContent);
      NS_ASSERTION(NS_SUCCEEDED(rv), "Converting linebreaks failed!");

      nsCOMPtr<nsISupportsString> pState =
        do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID);
      if (!pState) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      pState->SetData(value);
      state->SetStateProperty(pState);
    }
  }

  if (mDisabledChanged) {
    if (!state) {
      rv = GetPrimaryPresState(this, &state);
    }
    if (state) {
      
      
      state->SetDisabled(HasAttr(kNameSpaceID_None, nsGkAtoms::disabled));
    }
  }
  return rv;
}

bool
HTMLTextAreaElement::RestoreState(nsPresState* aState)
{
  nsCOMPtr<nsISupportsString> state
    (do_QueryInterface(aState->GetStateProperty()));
  
  if (state) {
    nsAutoString data;
    state->GetData(data);
    SetValue(data);
  }

  if (aState->IsDisabledSet()) {
    SetDisabled(aState->GetDisabled());
  }

  return false;
}

nsEventStates
HTMLTextAreaElement::IntrinsicState() const
{
  nsEventStates state = nsGenericHTMLFormElement::IntrinsicState();

  if (HasAttr(kNameSpaceID_None, nsGkAtoms::required)) {
    state |= NS_EVENT_STATE_REQUIRED;
  } else {
    state |= NS_EVENT_STATE_OPTIONAL;
  }

  if (IsCandidateForConstraintValidation()) {
    if (IsValid()) {
      state |= NS_EVENT_STATE_VALID;
    } else {
      state |= NS_EVENT_STATE_INVALID;
      
      
      if ((!mForm || !mForm->HasAttr(kNameSpaceID_None, nsGkAtoms::novalidate)) &&
          (GetValidityState(VALIDITY_STATE_CUSTOM_ERROR) ||
           (mCanShowInvalidUI && ShouldShowValidityUI()))) {
        state |= NS_EVENT_STATE_MOZ_UI_INVALID;
      }
    }

    
    
    
    
    
    
    
    
    
    if ((!mForm || !mForm->HasAttr(kNameSpaceID_None, nsGkAtoms::novalidate)) &&
        (mCanShowValidUI && ShouldShowValidityUI() &&
         (IsValid() || (state.HasState(NS_EVENT_STATE_MOZ_UI_INVALID) &&
                        !mCanShowInvalidUI)))) {
      state |= NS_EVENT_STATE_MOZ_UI_VALID;
    }
  }

  return state;
}

nsresult
HTMLTextAreaElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                                nsIContent* aBindingParent,
                                bool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLFormElement::BindToTree(aDocument, aParent,
                                                     aBindingParent,
                                                     aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  UpdateValueMissingValidityState();
  UpdateBarredFromConstraintValidation();

  
  UpdateState(false);

  return rv;
}

void
HTMLTextAreaElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  nsGenericHTMLFormElement::UnbindFromTree(aDeep, aNullParent);

  
  UpdateValueMissingValidityState();
  UpdateBarredFromConstraintValidation();

  
  UpdateState(false);
}

nsresult
HTMLTextAreaElement::BeforeSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                   const nsAttrValueOrString* aValue,
                                   bool aNotify)
{
  if (aNotify && aName == nsGkAtoms::disabled &&
      aNameSpaceID == kNameSpaceID_None) {
    mDisabledChanged = true;
  }

  return nsGenericHTMLFormElement::BeforeSetAttr(aNameSpaceID, aName,
                                                 aValue, aNotify);
}

void
HTMLTextAreaElement::CharacterDataChanged(nsIDocument* aDocument,
                                          nsIContent* aContent,
                                          CharacterDataChangeInfo* aInfo)
{
  ContentChanged(aContent);
}

void
HTMLTextAreaElement::ContentAppended(nsIDocument* aDocument,
                                     nsIContent* aContainer,
                                     nsIContent* aFirstNewContent,
                                     int32_t )
{
  ContentChanged(aFirstNewContent);
}

void
HTMLTextAreaElement::ContentInserted(nsIDocument* aDocument,
                                     nsIContent* aContainer,
                                     nsIContent* aChild,
                                     int32_t )
{
  ContentChanged(aChild);
}

void
HTMLTextAreaElement::ContentRemoved(nsIDocument* aDocument,
                                    nsIContent* aContainer,
                                    nsIContent* aChild,
                                    int32_t aIndexInContainer,
                                    nsIContent* aPreviousSibling)
{
  ContentChanged(aChild);
}

void
HTMLTextAreaElement::ContentChanged(nsIContent* aContent)
{
  if (!mValueChanged && mDoneAddingChildren &&
      nsContentUtils::IsInSameAnonymousTree(this, aContent)) {
    
    
    nsCOMPtr<nsIMutationObserver> kungFuDeathGrip(this);
    Reset();
  }
}

nsresult
HTMLTextAreaElement::AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                  const nsAttrValue* aValue, bool aNotify)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    if (aName == nsGkAtoms::required || aName == nsGkAtoms::disabled ||
        aName == nsGkAtoms::readonly) {
      UpdateValueMissingValidityState();

      
      if (aName == nsGkAtoms::readonly || aName == nsGkAtoms::disabled) {
        UpdateBarredFromConstraintValidation();
      }
    } else if (aName == nsGkAtoms::maxlength) {
      UpdateTooLongValidityState();
    }

    UpdateState(aNotify);
  }

  return nsGenericHTMLFormElement::AfterSetAttr(aNameSpaceID, aName, aValue,
                                                aNotify);
}

nsresult
HTMLTextAreaElement::CopyInnerTo(Element* aDest)
{
  nsresult rv = nsGenericHTMLFormElement::CopyInnerTo(aDest);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDest->OwnerDoc()->IsStaticDocument()) {
    nsAutoString value;
    GetValueInternal(value, true);
    static_cast<HTMLTextAreaElement*>(aDest)->SetValue(value);
  }
  return NS_OK;
}

bool
HTMLTextAreaElement::IsMutable() const
{
  return (!HasAttr(kNameSpaceID_None, nsGkAtoms::readonly) && !IsDisabled());
}

bool
HTMLTextAreaElement::IsValueEmpty() const
{
  nsAutoString value;
  GetValueInternal(value, true);

  return value.IsEmpty();
}



NS_IMETHODIMP
HTMLTextAreaElement::SetCustomValidity(const nsAString& aError)
{
  nsIConstraintValidation::SetCustomValidity(aError);

  UpdateState(true);

  return NS_OK;
}

bool
HTMLTextAreaElement::IsTooLong()
{
  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::maxlength) || !mValueChanged) {
    return false;
  }

  int32_t maxLength = -1;
  GetMaxLength(&maxLength);

  
  if (maxLength == -1) {
    return false;
  }

  int32_t textLength = -1;
  GetTextLength(&textLength);

  return textLength > maxLength;
}

bool
HTMLTextAreaElement::IsValueMissing() const
{
  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::required) || !IsMutable()) {
    return false;
  }

  return IsValueEmpty();
}

void
HTMLTextAreaElement::UpdateTooLongValidityState()
{
  
#if 0
  SetValidityState(VALIDITY_STATE_TOO_LONG, IsTooLong());
#endif
}

void
HTMLTextAreaElement::UpdateValueMissingValidityState()
{
  SetValidityState(VALIDITY_STATE_VALUE_MISSING, IsValueMissing());
}

void
HTMLTextAreaElement::UpdateBarredFromConstraintValidation()
{
  SetBarredFromConstraintValidation(HasAttr(kNameSpaceID_None,
                                            nsGkAtoms::readonly) ||
                                    IsDisabled());
}

nsresult
HTMLTextAreaElement::GetValidationMessage(nsAString& aValidationMessage,
                                          ValidityStateType aType)
{
  nsresult rv = NS_OK;

  switch (aType)
  {
    case VALIDITY_STATE_TOO_LONG:
      {
        nsXPIDLString message;
        int32_t maxLength = -1;
        int32_t textLength = -1;
        nsAutoString strMaxLength;
        nsAutoString strTextLength;

        GetMaxLength(&maxLength);
        GetTextLength(&textLength);

        strMaxLength.AppendInt(maxLength);
        strTextLength.AppendInt(textLength);

        const PRUnichar* params[] = { strMaxLength.get(), strTextLength.get() };
        rv = nsContentUtils::FormatLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                   "FormValidationTextTooLong",
                                                   params, message);
        aValidationMessage = message;
      }
      break;
    case VALIDITY_STATE_VALUE_MISSING:
      {
        nsXPIDLString message;
        rv = nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                "FormValidationValueMissing",
                                                message);
        aValidationMessage = message;
      }
      break;
    default:
      rv = nsIConstraintValidation::GetValidationMessage(aValidationMessage, aType);
  }

  return rv;
}

NS_IMETHODIMP_(bool)
HTMLTextAreaElement::IsSingleLineTextControl() const
{
  return false;
}

NS_IMETHODIMP_(bool)
HTMLTextAreaElement::IsTextArea() const
{
  return true;
}

NS_IMETHODIMP_(bool)
HTMLTextAreaElement::IsPlainTextControl() const
{
  
  return true;
}

NS_IMETHODIMP_(bool)
HTMLTextAreaElement::IsPasswordTextControl() const
{
  return false;
}

NS_IMETHODIMP_(int32_t)
HTMLTextAreaElement::GetCols()
{
  const nsAttrValue* attr = GetParsedAttr(nsGkAtoms::cols);
  if (attr) {
    int32_t cols = attr->Type() == nsAttrValue::eInteger ?
                   attr->GetIntegerValue() : 0;
    
    return (cols <= 0) ? 1 : cols;
  }

  return DEFAULT_COLS;
}

NS_IMETHODIMP_(int32_t)
HTMLTextAreaElement::GetWrapCols()
{
  
  nsHTMLTextWrap wrapProp;
  nsITextControlElement::GetWrapPropertyEnum(this, wrapProp);
  if (wrapProp == nsITextControlElement::eHTMLTextWrap_Off) {
    
    return -1;
  }

  
  return GetCols();
}


NS_IMETHODIMP_(int32_t)
HTMLTextAreaElement::GetRows()
{
  const nsAttrValue* attr = GetParsedAttr(nsGkAtoms::rows);
  if (attr && attr->Type() == nsAttrValue::eInteger) {
    int32_t rows = attr->GetIntegerValue();
    return (rows <= 0) ? DEFAULT_ROWS_TEXTAREA : rows;
  }

  return DEFAULT_ROWS_TEXTAREA;
}

NS_IMETHODIMP_(void)
nsHTMLTextAreaElement::GetDefaultValueFromContent(nsAString& aValue)
{
  GetDefaultValue(aValue);
}

NS_IMETHODIMP_(bool)
HTMLTextAreaElement::ValueChanged() const
{
  return mValueChanged;
}

NS_IMETHODIMP_(void)
HTMLTextAreaElement::GetTextEditorValue(nsAString& aValue,
                                        bool aIgnoreWrap) const
{
  mState.GetValue(aValue, aIgnoreWrap);
}

NS_IMETHODIMP_(void)
HTMLTextAreaElement::InitializeKeyboardEventListeners()
{
  mState.InitializeKeyboardEventListeners();
}

NS_IMETHODIMP_(void)
HTMLTextAreaElement::OnValueChanged(bool aNotify)
{
  
  bool validBefore = IsValid();
  UpdateTooLongValidityState();
  UpdateValueMissingValidityState();

  if (validBefore != IsValid()) {
    UpdateState(aNotify);
  }
}

NS_IMETHODIMP_(bool)
HTMLTextAreaElement::HasCachedSelection()
{
  return mState.IsSelectionCached();
}

void
HTMLTextAreaElement::FieldSetDisabledChanged(bool aNotify)
{
  UpdateValueMissingValidityState();
  UpdateBarredFromConstraintValidation();

  nsGenericHTMLFormElement::FieldSetDisabledChanged(aNotify);
}

JSObject*
HTMLTextAreaElement::WrapNode(JSContext* aCx, JSObject* aScope)
{
  return HTMLTextAreaElementBinding::Wrap(aCx, aScope, this);
}

} 
} 
