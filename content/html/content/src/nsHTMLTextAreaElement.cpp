





































#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIDOMNSHTMLTextAreaElement.h"
#include "nsITextControlElement.h"
#include "nsIDOMNSEditableElement.h"
#include "nsIControllers.h"
#include "nsFocusManager.h"
#include "nsPIDOMWindow.h"
#include "nsContentCID.h"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIFormControl.h"
#include "nsIForm.h"
#include "nsFormSubmission.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsMappedAttributes.h"
#include "nsIFormControlFrame.h"
#include "nsITextControlFrame.h"
#include "nsEventStates.h"
#include "nsLinebreakConverter.h"
#include "nsIDocument.h"
#include "nsIFrame.h"
#include "nsIFormControlFrame.h"
#include "nsIPrivateDOMEvent.h"
#include "nsGUIEvent.h"
#include "nsLinebreakConverter.h"
#include "nsPresState.h"
#include "nsIDOMText.h"
#include "nsReadableUtils.h"
#include "nsEventDispatcher.h"
#include "nsLayoutUtils.h"
#include "nsLayoutErrors.h"
#include "nsStubMutationObserver.h"
#include "nsDOMError.h"
#include "mozAutoDocUpdate.h"
#include "nsISupportsPrimitives.h"
#include "nsContentCreatorFunctions.h"
#include "nsIConstraintValidation.h"
#include "nsHTMLFormElement.h"

#include "nsTextEditorState.h"

using namespace mozilla::dom;

static NS_DEFINE_CID(kXULControllersCID,  NS_XULCONTROLLERS_CID);

#define NS_NO_CONTENT_DISPATCH (1 << 0)

class nsHTMLTextAreaElement : public nsGenericHTMLFormElement,
                              public nsIDOMHTMLTextAreaElement,
                              public nsIDOMNSHTMLTextAreaElement,
                              public nsITextControlElement,
                              public nsIDOMNSEditableElement,
                              public nsStubMutationObserver,
                              public nsIConstraintValidation
{
public:
  using nsIConstraintValidation::GetValidationMessage;

  nsHTMLTextAreaElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                        mozilla::dom::FromParser aFromParser = mozilla::dom::NOT_FROM_PARSER);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFormElement::)

  
  NS_DECL_NSIDOMHTMLTEXTAREAELEMENT

  
  NS_DECL_NSIDOMNSHTMLTEXTAREAELEMENT

  
  NS_IMETHOD GetEditor(nsIEditor** aEditor)
  {
    return nsGenericHTMLElement::GetEditor(aEditor);
  }
  NS_IMETHOD SetUserInput(const nsAString& aInput);

  
  NS_IMETHOD_(PRUint32) GetType() const { return NS_FORM_TEXTAREA; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission);
  NS_IMETHOD SaveState();
  virtual PRBool RestoreState(nsPresState* aState);

  virtual void FieldSetDisabledChanged(PRBool aNotify);

  virtual nsEventStates IntrinsicState() const;

  
  NS_IMETHOD SetValueChanged(PRBool aValueChanged);
  NS_IMETHOD_(PRBool) IsSingleLineTextControl() const;
  NS_IMETHOD_(PRBool) IsTextArea() const;
  NS_IMETHOD_(PRBool) IsPlainTextControl() const;
  NS_IMETHOD_(PRBool) IsPasswordTextControl() const;
  NS_IMETHOD_(PRInt32) GetCols();
  NS_IMETHOD_(PRInt32) GetWrapCols();
  NS_IMETHOD_(PRInt32) GetRows();
  NS_IMETHOD_(void) GetDefaultValueFromContent(nsAString& aValue);
  NS_IMETHOD_(PRBool) ValueChanged() const;
  NS_IMETHOD_(void) GetTextEditorValue(nsAString& aValue, PRBool aIgnoreWrap) const;
  NS_IMETHOD_(void) SetTextEditorValue(const nsAString& aValue, PRBool aUserInput);
  NS_IMETHOD_(nsIEditor*) GetTextEditor();
  NS_IMETHOD_(nsISelectionController*) GetSelectionController();
  NS_IMETHOD_(nsFrameSelection*) GetConstFrameSelection();
  NS_IMETHOD BindToFrame(nsTextControlFrame* aFrame);
  NS_IMETHOD_(void) UnbindFromFrame(nsTextControlFrame* aFrame);
  NS_IMETHOD CreateEditor();
  NS_IMETHOD_(nsIContent*) GetRootEditorNode();
  NS_IMETHOD_(nsIContent*) CreatePlaceholderNode();
  NS_IMETHOD_(nsIContent*) GetPlaceholderNode();
  NS_IMETHOD_(void) UpdatePlaceholderText(PRBool aNotify);
  NS_IMETHOD_(void) SetPlaceholderClass(PRBool aVisible, PRBool aNotify);
  NS_IMETHOD_(void) InitializeKeyboardEventListeners();
  NS_IMETHOD_(void) OnValueChanged(PRBool aNotify);

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                               nsIContent* aBindingParent,
                               PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              PRInt32 aModType) const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

  virtual PRBool IsHTMLFocusable(PRBool aWithMouse, PRBool *aIsFocusable, PRInt32 *aTabIndex);

  virtual nsresult DoneAddingChildren(PRBool aHaveNotified);
  virtual PRBool IsDoneAddingChildren();

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  nsresult CopyInnerTo(nsGenericElement* aDest) const;

  


  virtual nsresult BeforeSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                 const nsAString* aValue, PRBool aNotify);

  
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  virtual void UpdateEditableState(PRBool aNotify)
  {
    return UpdateEditableFormControlState(aNotify);
  }

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsHTMLTextAreaElement,
                                           nsGenericHTMLFormElement)

  virtual nsXPCClassInfo* GetClassInfo();

  
  PRBool   IsTooLong();
  PRBool   IsValueMissing() const;
  void     UpdateTooLongValidityState();
  void     UpdateValueMissingValidityState();
  void     UpdateBarredFromConstraintValidation();
  nsresult GetValidationMessage(nsAString& aValidationMessage,
                                ValidityStateType aType);

protected:
  using nsGenericHTMLFormElement::IsSingleLineTextControl; 

  nsCOMPtr<nsIControllers> mControllers;
  
  PRPackedBool             mValueChanged;
  
  PRPackedBool             mHandlingSelect;
  

  PRPackedBool             mDoneAddingChildren;
  
  PRPackedBool             mInhibitStateRestoration;
  
  PRPackedBool             mDisabledChanged;
  
  PRPackedBool             mCanShowInvalidUI;
  
  PRPackedBool             mCanShowValidUI;

  
  nsRefPtr<nsTextEditorState> mState;

  NS_IMETHOD SelectAll(nsPresContext* aPresContext);
  






  void GetValueInternal(nsAString& aValue, PRBool aIgnoreWrap) const;

  nsresult SetValueInternal(const nsAString& aValue,
                            PRBool aUserInput);
  nsresult GetSelectionRange(PRInt32* aSelectionStart, PRInt32* aSelectionEnd);

  




  void ContentChanged(nsIContent* aContent);

  virtual nsresult AfterSetAttr(PRInt32 aNamespaceID, nsIAtom *aName,
                                const nsAString* aValue, PRBool aNotify);

  





  bool ShouldShowValidityUI() const {
    






    if (mForm && mForm->HasEverTriedInvalidSubmit()) {
      return true;
    }

    return mValueChanged;
  }

  


  PRBool IsMutable() const;

  




  bool IsValueEmpty() const;
};


NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(TextArea)


nsHTMLTextAreaElement::nsHTMLTextAreaElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                                             FromParser aFromParser)
  : nsGenericHTMLFormElement(aNodeInfo),
    mValueChanged(PR_FALSE),
    mHandlingSelect(PR_FALSE),
    mDoneAddingChildren(!aFromParser),
    mInhibitStateRestoration(!!(aFromParser & FROM_PARSER_FRAGMENT)),
    mDisabledChanged(PR_FALSE),
    mCanShowInvalidUI(PR_TRUE),
    mCanShowValidUI(PR_TRUE),
    mState(new nsTextEditorState(this))
{
  AddMutationObserver(this);

  
  
  
  
  
  AddStatesSilently(NS_EVENT_STATE_ENABLED |
                    NS_EVENT_STATE_OPTIONAL |
                    NS_EVENT_STATE_VALID);
}


NS_IMPL_CYCLE_COLLECTION_CLASS(nsHTMLTextAreaElement)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsHTMLTextAreaElement,
                                                nsGenericHTMLFormElement)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mControllers)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHTMLTextAreaElement,
                                                  nsGenericHTMLFormElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mControllers)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_MEMBER(mState, nsTextEditorState)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsHTMLTextAreaElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLTextAreaElement, nsGenericElement) 


DOMCI_NODE_DATA(HTMLTextAreaElement, nsHTMLTextAreaElement)


NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsHTMLTextAreaElement)
  NS_HTML_CONTENT_INTERFACE_TABLE6(nsHTMLTextAreaElement,
                                   nsIDOMHTMLTextAreaElement,
                                   nsIDOMNSHTMLTextAreaElement,
                                   nsITextControlElement,
                                   nsIDOMNSEditableElement,
                                   nsIMutationObserver,
                                   nsIConstraintValidation)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLTextAreaElement,
                                               nsGenericHTMLFormElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLTextAreaElement)





NS_IMPL_ELEMENT_CLONE(nsHTMLTextAreaElement)


NS_IMPL_NSICONSTRAINTVALIDATION_EXCEPT_SETCUSTOMVALIDITY(nsHTMLTextAreaElement)


NS_IMETHODIMP
nsHTMLTextAreaElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElement::GetForm(aForm);
}




NS_IMETHODIMP
nsHTMLTextAreaElement::Select()
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
  nsGUIEvent event(PR_TRUE, NS_FORM_SELECTED, nsnull);
  
  nsEventDispatcher::Dispatch(static_cast<nsIContent*>(this), presContext,
                              &event, nsnull, &status);

  
  
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
nsHTMLTextAreaElement::SelectAll(nsPresContext* aPresContext)
{
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    formControlFrame->SetFormProperty(nsGkAtoms::select, EmptyString());
  }

  return NS_OK;
}

PRBool
nsHTMLTextAreaElement::IsHTMLFocusable(PRBool aWithMouse,
                                       PRBool *aIsFocusable, PRInt32 *aTabIndex)
{
  if (nsGenericHTMLFormElement::IsHTMLFocusable(aWithMouse, aIsFocusable, aTabIndex)) {
    return PR_TRUE;
  }

  
  *aIsFocusable = !IsDisabled();
  return PR_FALSE;
}

NS_IMPL_BOOL_ATTR(nsHTMLTextAreaElement, Autofocus, autofocus)
NS_IMPL_UINT_ATTR_NON_ZERO_DEFAULT_VALUE(nsHTMLTextAreaElement, Cols, cols, DEFAULT_COLS)
NS_IMPL_BOOL_ATTR(nsHTMLTextAreaElement, Disabled, disabled)
NS_IMPL_NON_NEGATIVE_INT_ATTR(nsHTMLTextAreaElement, MaxLength, maxlength)
NS_IMPL_STRING_ATTR(nsHTMLTextAreaElement, Name, name)
NS_IMPL_BOOL_ATTR(nsHTMLTextAreaElement, ReadOnly, readonly)
NS_IMPL_BOOL_ATTR(nsHTMLTextAreaElement, Required, required)
NS_IMPL_UINT_ATTR_NON_ZERO_DEFAULT_VALUE(nsHTMLTextAreaElement, Rows, rows, DEFAULT_ROWS_TEXTAREA)
NS_IMPL_INT_ATTR(nsHTMLTextAreaElement, TabIndex, tabindex)
NS_IMPL_STRING_ATTR(nsHTMLTextAreaElement, Wrap, wrap)
NS_IMPL_STRING_ATTR(nsHTMLTextAreaElement, Placeholder, placeholder)
  

NS_IMETHODIMP 
nsHTMLTextAreaElement::GetType(nsAString& aType)
{
  aType.AssignLiteral("textarea");

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLTextAreaElement::GetValue(nsAString& aValue)
{
  GetValueInternal(aValue, PR_TRUE);
  return NS_OK;
}

void
nsHTMLTextAreaElement::GetValueInternal(nsAString& aValue, PRBool aIgnoreWrap) const
{
  mState->GetValue(aValue, aIgnoreWrap);
}

NS_IMETHODIMP_(nsIEditor*)
nsHTMLTextAreaElement::GetTextEditor()
{
  return mState->GetEditor();
}

NS_IMETHODIMP_(nsISelectionController*)
nsHTMLTextAreaElement::GetSelectionController()
{
  return mState->GetSelectionController();
}

NS_IMETHODIMP_(nsFrameSelection*)
nsHTMLTextAreaElement::GetConstFrameSelection()
{
  return mState->GetConstFrameSelection();
}

NS_IMETHODIMP
nsHTMLTextAreaElement::BindToFrame(nsTextControlFrame* aFrame)
{
  return mState->BindToFrame(aFrame);
}

NS_IMETHODIMP_(void)
nsHTMLTextAreaElement::UnbindFromFrame(nsTextControlFrame* aFrame)
{
  if (aFrame) {
    mState->UnbindFromFrame(aFrame);
  }
}

NS_IMETHODIMP
nsHTMLTextAreaElement::CreateEditor()
{
  return mState->PrepareEditor();
}

NS_IMETHODIMP_(nsIContent*)
nsHTMLTextAreaElement::GetRootEditorNode()
{
  return mState->GetRootNode();
}

NS_IMETHODIMP_(nsIContent*)
nsHTMLTextAreaElement::CreatePlaceholderNode()
{
  NS_ENSURE_SUCCESS(mState->CreatePlaceholderNode(), nsnull);
  return mState->GetPlaceholderNode();
}

NS_IMETHODIMP_(nsIContent*)
nsHTMLTextAreaElement::GetPlaceholderNode()
{
  return mState->GetPlaceholderNode();
}

NS_IMETHODIMP_(void)
nsHTMLTextAreaElement::UpdatePlaceholderText(PRBool aNotify)
{
  mState->UpdatePlaceholderText(aNotify);
}

NS_IMETHODIMP_(void)
nsHTMLTextAreaElement::SetPlaceholderClass(PRBool aVisible, PRBool aNotify)
{
  mState->SetPlaceholderClass(aVisible, aNotify);
}

nsresult
nsHTMLTextAreaElement::SetValueInternal(const nsAString& aValue,
                                        PRBool aUserInput)
{
  
  
  
  SetValueChanged(PR_TRUE);
  mState->SetValue(aValue, aUserInput);

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLTextAreaElement::SetValue(const nsAString& aValue)
{
  return SetValueInternal(aValue, PR_FALSE);
}

NS_IMETHODIMP 
nsHTMLTextAreaElement::SetUserInput(const nsAString& aValue)
{
  if (!nsContentUtils::IsCallerTrustedForWrite()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }
  SetValueInternal(aValue, PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::SetValueChanged(PRBool aValueChanged)
{
  PRBool previousValue = mValueChanged;

  mValueChanged = aValueChanged;
  if (!aValueChanged && !mState->IsEmpty()) {
    mState->EmptyValue();
  }

  if (mValueChanged != previousValue) {
    UpdateState(true);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::GetDefaultValue(nsAString& aDefaultValue)
{
  nsContentUtils::GetNodeTextContent(this, PR_FALSE, aDefaultValue);
  return NS_OK;
}  

NS_IMETHODIMP
nsHTMLTextAreaElement::SetDefaultValue(const nsAString& aDefaultValue)
{
  nsresult rv = nsContentUtils::SetNodeTextContent(this, aDefaultValue, PR_TRUE);
  if (NS_SUCCEEDED(rv) && !mValueChanged) {
    Reset();
  }
  return rv;
}

PRBool
nsHTMLTextAreaElement::ParseAttribute(PRInt32 aNamespaceID,
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
nsHTMLTextAreaElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              PRInt32 aModType) const
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

NS_IMETHODIMP_(PRBool)
nsHTMLTextAreaElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry* const map[] = {
    sDivAlignAttributeMap,
    sCommonAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
}

nsMapRuleToAttributesFunc
nsHTMLTextAreaElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}

nsresult
nsHTMLTextAreaElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  
  aVisitor.mCanHandle = PR_FALSE;
  if (IsDisabled()) {
    return NS_OK;
  }

  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_FALSE);
  nsIFrame* formFrame = nsnull;

  if (formControlFrame &&
      (formFrame = do_QueryFrame(formControlFrame))) {
    const nsStyleUserInterface* uiStyle = formFrame->GetStyleUserInterface();

    if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE ||
        uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED) {
      return NS_OK;
    }
  }

  
  
  if (aVisitor.mEvent->message == NS_FORM_SELECTED) {
    if (mHandlingSelect) {
      return NS_OK;
    }
    mHandlingSelect = PR_TRUE;
  }

  
  
  
  if (aVisitor.mEvent->flags & NS_EVENT_FLAG_NO_CONTENT_DISPATCH)
    aVisitor.mItemFlags |= NS_NO_CONTENT_DISPATCH;
  if (aVisitor.mEvent->message == NS_MOUSE_CLICK &&
      aVisitor.mEvent->eventStructType == NS_MOUSE_EVENT &&
      static_cast<nsMouseEvent*>(aVisitor.mEvent)->button ==
        nsMouseEvent::eMiddleButton) {
    aVisitor.mEvent->flags &= ~NS_EVENT_FLAG_NO_CONTENT_DISPATCH;
  }

  
  if (aVisitor.mEvent->message == NS_BLUR_CONTENT) {
    nsIFrame* primaryFrame = GetPrimaryFrame();
    if (primaryFrame) {
      nsITextControlFrame* textFrame = do_QueryFrame(primaryFrame);
      if (textFrame) {
        textFrame->CheckFireOnChange();
      }
    }
  }

  return nsGenericHTMLFormElement::PreHandleEvent(aVisitor);
}

nsresult
nsHTMLTextAreaElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  if (aVisitor.mEvent->message == NS_FORM_SELECTED) {
    mHandlingSelect = PR_FALSE;
  }

  if (aVisitor.mEvent->message == NS_FOCUS_CONTENT ||
      aVisitor.mEvent->message == NS_BLUR_CONTENT) {
    if (aVisitor.mEvent->message == NS_FOCUS_CONTENT) {
      
      
      mCanShowInvalidUI = !IsValid() && ShouldShowValidityUI();

      
      
      mCanShowValidUI = ShouldShowValidityUI();
    } else { 
      mCanShowInvalidUI = PR_TRUE;
      mCanShowValidUI = PR_TRUE;
    }

    UpdateState(true);
  }

  
  aVisitor.mEvent->flags |= (aVisitor.mItemFlags & NS_NO_CONTENT_DISPATCH)
    ? NS_EVENT_FLAG_NO_CONTENT_DISPATCH : NS_EVENT_FLAG_NONE;

  return NS_OK;
}

nsresult
nsHTMLTextAreaElement::DoneAddingChildren(PRBool aHaveNotified)
{
  if (!mValueChanged) {
    if (!mDoneAddingChildren) {
      
      
      Reset();
    }
    if (!mInhibitStateRestoration) {
      RestoreFormControlState(this, this);
    }
  }

  mDoneAddingChildren = PR_TRUE;

  return NS_OK;
}

PRBool
nsHTMLTextAreaElement::IsDoneAddingChildren()
{
  return mDoneAddingChildren;
}



NS_IMETHODIMP
nsHTMLTextAreaElement::GetControllers(nsIControllers** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  if (!mControllers)
  {
    nsresult rv;
    mControllers = do_CreateInstance(kXULControllersCID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIController> controller = do_CreateInstance("@mozilla.org/editor/editorcontroller;1", &rv);
    if (NS_FAILED(rv))
      return rv;

    mControllers->AppendController(controller);
  }

  *aResult = mControllers;
  NS_IF_ADDREF(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::GetTextLength(PRInt32 *aTextLength)
{
  NS_ENSURE_ARG_POINTER(aTextLength);
  nsAutoString val;
  nsresult rv = GetValue(val);
  *aTextLength = val.Length();

  return rv;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::GetSelectionStart(PRInt32 *aSelectionStart)
{
  NS_ENSURE_ARG_POINTER(aSelectionStart);
  
  PRInt32 selEnd;
  return GetSelectionRange(aSelectionStart, &selEnd);
}

NS_IMETHODIMP
nsHTMLTextAreaElement::SetSelectionStart(PRInt32 aSelectionStart)
{
  nsresult rv = NS_ERROR_FAILURE;
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame){
    nsITextControlFrame* textControlFrame = do_QueryFrame(formControlFrame);
    if (textControlFrame) {
      rv = textControlFrame->SetSelectionStart(aSelectionStart);
      if (NS_SUCCEEDED(rv)) {
        rv = textControlFrame->ScrollSelectionIntoView();
      }
    }
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::GetSelectionEnd(PRInt32 *aSelectionEnd)
{
  NS_ENSURE_ARG_POINTER(aSelectionEnd);
  
  PRInt32 selStart;
  return GetSelectionRange(&selStart, aSelectionEnd);
}

NS_IMETHODIMP
nsHTMLTextAreaElement::SetSelectionEnd(PRInt32 aSelectionEnd)
{
  nsresult rv = NS_ERROR_FAILURE;
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    nsITextControlFrame* textControlFrame = do_QueryFrame(formControlFrame);
    if (textControlFrame) {
      rv = textControlFrame->SetSelectionEnd(aSelectionEnd);
      if (NS_SUCCEEDED(rv)) {
        rv = textControlFrame->ScrollSelectionIntoView();
      }
    }
  }

  return rv;
}

nsresult
nsHTMLTextAreaElement::GetSelectionRange(PRInt32* aSelectionStart,
                                      PRInt32* aSelectionEnd)
{
  nsresult rv = NS_ERROR_FAILURE;
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    nsITextControlFrame* textControlFrame = do_QueryFrame(formControlFrame);
    if (textControlFrame)
      rv = textControlFrame->GetSelectionRange(aSelectionStart, aSelectionEnd);
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::SetSelectionRange(PRInt32 aSelectionStart, PRInt32 aSelectionEnd)
{ 
  nsresult rv = NS_ERROR_FAILURE;
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    nsITextControlFrame* textControlFrame = do_QueryFrame(formControlFrame);
    if (textControlFrame) {
      rv = textControlFrame->SetSelectionRange(aSelectionStart, aSelectionEnd);
      if (NS_SUCCEEDED(rv)) {
        rv = textControlFrame->ScrollSelectionIntoView();
      }
    }
  }

  return rv;
} 

nsresult
nsHTMLTextAreaElement::Reset()
{
  nsresult rv;

  
  
  SetValue(EmptyString());
  nsAutoString resetVal;
  GetDefaultValue(resetVal);
  rv = SetValue(resetVal);
  NS_ENSURE_SUCCESS(rv, rv);

  SetValueChanged(PR_FALSE);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::SubmitNamesValues(nsFormSubmission* aFormSubmission)
{
  nsresult rv = NS_OK;

  
  if (IsDisabled()) {
    return NS_OK;
  }

  
  
  
  nsAutoString name;
  GetAttr(kNameSpaceID_None, nsGkAtoms::name, name);
  if (name.IsEmpty()) {
    return NS_OK;
  }

  
  
  
  nsAutoString value;
  GetValueInternal(value, PR_FALSE);

  
  
  
  rv = aFormSubmission->AddNameValuePair(name, value);

  return rv;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::SaveState()
{
  nsresult rv = NS_OK;

  
  nsPresState *state = nsnull;
  if (mValueChanged) {
    rv = GetPrimaryPresState(this, &state);
    if (state) {
      nsAutoString value;
      GetValueInternal(value, PR_TRUE);

      rv = nsLinebreakConverter::ConvertStringLineBreaks(
               value,
               nsLinebreakConverter::eLinebreakPlatform,
               nsLinebreakConverter::eLinebreakContent);
      NS_ASSERTION(NS_SUCCEEDED(rv), "Converting linebreaks failed!");

      nsCOMPtr<nsISupportsString> pState
        (do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID));
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

PRBool
nsHTMLTextAreaElement::RestoreState(nsPresState* aState)
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

  return PR_FALSE;
}

nsEventStates
nsHTMLTextAreaElement::IntrinsicState() const
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
           mCanShowInvalidUI && ShouldShowValidityUI())) {
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

  if (HasAttr(kNameSpaceID_None, nsGkAtoms::placeholder) &&
      !nsContentUtils::IsFocusedContent((nsIContent*)(this)) &&
      IsValueEmpty()) {
    state |= NS_EVENT_STATE_MOZ_PLACEHOLDER;
  }

  return state;
}

nsresult
nsHTMLTextAreaElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                                  nsIContent* aBindingParent,
                                  PRBool aCompileEventHandlers)
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
nsHTMLTextAreaElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  nsGenericHTMLFormElement::UnbindFromTree(aDeep, aNullParent);

  
  UpdateValueMissingValidityState();
  UpdateBarredFromConstraintValidation();

  
  UpdateState(false);
}

nsresult
nsHTMLTextAreaElement::BeforeSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                     const nsAString* aValue, PRBool aNotify)
{
  if (aNotify && aName == nsGkAtoms::disabled &&
      aNameSpaceID == kNameSpaceID_None) {
    mDisabledChanged = PR_TRUE;
  }

  return nsGenericHTMLFormElement::BeforeSetAttr(aNameSpaceID, aName,
                                                 aValue, aNotify);
}

void
nsHTMLTextAreaElement::CharacterDataChanged(nsIDocument* aDocument,
                                            nsIContent* aContent,
                                            CharacterDataChangeInfo* aInfo)
{
  ContentChanged(aContent);
}

void
nsHTMLTextAreaElement::ContentAppended(nsIDocument* aDocument,
                                       nsIContent* aContainer,
                                       nsIContent* aFirstNewContent,
                                       PRInt32 )
{
  ContentChanged(aFirstNewContent);
}

void
nsHTMLTextAreaElement::ContentInserted(nsIDocument* aDocument,
                                       nsIContent* aContainer,
                                       nsIContent* aChild,
                                       PRInt32 )
{
  ContentChanged(aChild);
}

void
nsHTMLTextAreaElement::ContentRemoved(nsIDocument* aDocument,
                                      nsIContent* aContainer,
                                      nsIContent* aChild,
                                      PRInt32 aIndexInContainer,
                                      nsIContent* aPreviousSibling)
{
  ContentChanged(aChild);
}

void
nsHTMLTextAreaElement::ContentChanged(nsIContent* aContent)
{
  if (!mValueChanged && mDoneAddingChildren &&
      nsContentUtils::IsInSameAnonymousTree(this, aContent)) {
    
    
    nsCOMPtr<nsIMutationObserver> kungFuDeathGrip(this);
    Reset();
  }
}

nsresult
nsHTMLTextAreaElement::AfterSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                    const nsAString* aValue, PRBool aNotify)
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

    if (aName == nsGkAtoms::readonly) {
      UpdateEditableState(aNotify);
    }
    UpdateState(aNotify);
  }

  return nsGenericHTMLFormElement::AfterSetAttr(aNameSpaceID, aName, aValue,
                                                aNotify);
}

nsresult
nsHTMLTextAreaElement::CopyInnerTo(nsGenericElement* aDest) const
{
  nsresult rv = nsGenericHTMLFormElement::CopyInnerTo(aDest);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDest->GetOwnerDoc()->IsStaticDocument()) {
    nsAutoString value;
    const_cast<nsHTMLTextAreaElement*>(this)->GetValue(value);
    static_cast<nsHTMLTextAreaElement*>(aDest)->SetValue(value);
  }
  return NS_OK;
}

PRBool
nsHTMLTextAreaElement::IsMutable() const
{
  return (!HasAttr(kNameSpaceID_None, nsGkAtoms::readonly) && !IsDisabled());
}

bool
nsHTMLTextAreaElement::IsValueEmpty() const
{
  nsAutoString value;
  GetValueInternal(value, PR_TRUE);

  return value.IsEmpty();
}



NS_IMETHODIMP
nsHTMLTextAreaElement::SetCustomValidity(const nsAString& aError)
{
  nsIConstraintValidation::SetCustomValidity(aError);

  UpdateState(true);

  return NS_OK;
}

PRBool
nsHTMLTextAreaElement::IsTooLong()
{
  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::maxlength) || !mValueChanged) {
    return PR_FALSE;
  }

  PRInt32 maxLength = -1;
  GetMaxLength(&maxLength);

  
  if (maxLength == -1) {
    return PR_FALSE;
  }

  PRInt32 textLength = -1;
  GetTextLength(&textLength);

  return textLength > maxLength;
}

PRBool
nsHTMLTextAreaElement::IsValueMissing() const
{
  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::required) || !IsMutable()) {
    return PR_FALSE;
  }

  return IsValueEmpty();
}

void
nsHTMLTextAreaElement::UpdateTooLongValidityState()
{
  
#if 0
  SetValidityState(VALIDITY_STATE_TOO_LONG, IsTooLong());
#endif
}

void
nsHTMLTextAreaElement::UpdateValueMissingValidityState()
{
  SetValidityState(VALIDITY_STATE_VALUE_MISSING, IsValueMissing());
}

void
nsHTMLTextAreaElement::UpdateBarredFromConstraintValidation()
{
  SetBarredFromConstraintValidation(HasAttr(kNameSpaceID_None,
                                            nsGkAtoms::readonly) ||
                                    IsDisabled());
}

nsresult
nsHTMLTextAreaElement::GetValidationMessage(nsAString& aValidationMessage,
                                            ValidityStateType aType)
{
  nsresult rv = NS_OK;

  switch (aType)
  {
    case VALIDITY_STATE_TOO_LONG:
      {
        nsXPIDLString message;
        PRInt32 maxLength = -1;
        PRInt32 textLength = -1;
        nsAutoString strMaxLength;
        nsAutoString strTextLength;

        GetMaxLength(&maxLength);
        GetTextLength(&textLength);

        strMaxLength.AppendInt(maxLength);
        strTextLength.AppendInt(textLength);

        const PRUnichar* params[] = { strMaxLength.get(), strTextLength.get() };
        rv = nsContentUtils::FormatLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                   "FormValidationTextTooLong",
                                                   params, 2, message);
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

NS_IMETHODIMP_(PRBool)
nsHTMLTextAreaElement::IsSingleLineTextControl() const
{
  return PR_FALSE;
}

NS_IMETHODIMP_(PRBool)
nsHTMLTextAreaElement::IsTextArea() const
{
  return PR_TRUE;
}

NS_IMETHODIMP_(PRBool)
nsHTMLTextAreaElement::IsPlainTextControl() const
{
  
  return PR_TRUE;
}

NS_IMETHODIMP_(PRBool)
nsHTMLTextAreaElement::IsPasswordTextControl() const
{
  return PR_FALSE;
}

NS_IMETHODIMP_(PRInt32)
nsHTMLTextAreaElement::GetCols()
{
  const nsAttrValue* attr = GetParsedAttr(nsGkAtoms::cols);
  if (attr) {
    PRInt32 cols = attr->Type() == nsAttrValue::eInteger ?
                   attr->GetIntegerValue() : 0;
    
    return (cols <= 0) ? 1 : cols;
  }

  return DEFAULT_COLS;
}

NS_IMETHODIMP_(PRInt32)
nsHTMLTextAreaElement::GetWrapCols()
{
  
  nsHTMLTextWrap wrapProp;
  nsITextControlElement::GetWrapPropertyEnum(this, wrapProp);
  if (wrapProp == nsITextControlElement::eHTMLTextWrap_Off) {
    
    return -1;
  }

  
  return GetCols();
}


NS_IMETHODIMP_(PRInt32)
nsHTMLTextAreaElement::GetRows()
{
  const nsAttrValue* attr = GetParsedAttr(nsGkAtoms::rows);
  if (attr && attr->Type() == nsAttrValue::eInteger) {
    PRInt32 rows = attr->GetIntegerValue();
    return (rows <= 0) ? DEFAULT_ROWS_TEXTAREA : rows;
  }

  return DEFAULT_ROWS_TEXTAREA;
}

NS_IMETHODIMP_(void)
nsHTMLTextAreaElement::GetDefaultValueFromContent(nsAString& aValue)
{
  GetDefaultValue(aValue);
}

NS_IMETHODIMP_(PRBool)
nsHTMLTextAreaElement::ValueChanged() const
{
  return mValueChanged;
}

NS_IMETHODIMP_(void)
nsHTMLTextAreaElement::GetTextEditorValue(nsAString& aValue,
                                          PRBool aIgnoreWrap) const
{
  mState->GetValue(aValue, aIgnoreWrap);
}

NS_IMETHODIMP_(void)
nsHTMLTextAreaElement::SetTextEditorValue(const nsAString& aValue,
                                          PRBool aUserInput)
{
  mState->SetValue(aValue, aUserInput);
}

NS_IMETHODIMP_(void)
nsHTMLTextAreaElement::InitializeKeyboardEventListeners()
{
  mState->InitializeKeyboardEventListeners();
}

NS_IMETHODIMP_(void)
nsHTMLTextAreaElement::OnValueChanged(PRBool aNotify)
{
  
  PRBool validBefore = IsValid();
  UpdateTooLongValidityState();
  UpdateValueMissingValidityState();

  if (validBefore != IsValid() ||
      (HasAttr(kNameSpaceID_None, nsGkAtoms::placeholder)
       && !nsContentUtils::IsFocusedContent((nsIContent*)(this)))) {
    UpdateState(aNotify);
  }
}

void
nsHTMLTextAreaElement::FieldSetDisabledChanged(PRBool aNotify)
{
  UpdateValueMissingValidityState();
  UpdateBarredFromConstraintValidation();

  nsGenericHTMLFormElement::FieldSetDisabledChanged(aNotify);
}

