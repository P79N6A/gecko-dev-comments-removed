





#include "mozilla/Util.h"

#include "nsIDOMHTMLTextAreaElement.h"
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
#include "nsAttrValueInlines.h"
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
#include "nsGUIEvent.h"
#include "nsPresState.h"
#include "nsReadableUtils.h"
#include "nsEventDispatcher.h"
#include "nsLayoutUtils.h"
#include "nsError.h"
#include "nsStubMutationObserver.h"
#include "mozAutoDocUpdate.h"
#include "nsISupportsPrimitives.h"
#include "nsContentCreatorFunctions.h"
#include "nsIConstraintValidation.h"
#include "nsHTMLFormElement.h"

#include "nsTextEditorState.h"

using namespace mozilla;
using namespace mozilla::dom;

static NS_DEFINE_CID(kXULControllersCID,  NS_XULCONTROLLERS_CID);

#define NS_NO_CONTENT_DISPATCH (1 << 0)

class nsHTMLTextAreaElement : public nsGenericHTMLFormElement,
                              public nsIDOMHTMLTextAreaElement,
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

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  virtual int32_t TabIndexDefault() MOZ_OVERRIDE;

  
  NS_DECL_NSIDOMHTMLTEXTAREAELEMENT

  
  NS_IMETHOD GetEditor(nsIEditor** aEditor)
  {
    return nsGenericHTMLElement::GetEditor(aEditor);
  }
  NS_IMETHOD SetUserInput(const nsAString& aInput);

  
  NS_IMETHOD_(uint32_t) GetType() const { return NS_FORM_TEXTAREA; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission);
  NS_IMETHOD SaveState();
  virtual bool RestoreState(nsPresState* aState);
  virtual bool IsDisabledForEvents(uint32_t aMessage);

  virtual void FieldSetDisabledChanged(bool aNotify);

  virtual nsEventStates IntrinsicState() const;

  
  NS_IMETHOD SetValueChanged(bool aValueChanged);
  NS_IMETHOD_(bool) IsSingleLineTextControl() const;
  NS_IMETHOD_(bool) IsTextArea() const;
  NS_IMETHOD_(bool) IsPlainTextControl() const;
  NS_IMETHOD_(bool) IsPasswordTextControl() const;
  NS_IMETHOD_(int32_t) GetCols();
  NS_IMETHOD_(int32_t) GetWrapCols();
  NS_IMETHOD_(int32_t) GetRows();
  NS_IMETHOD_(bool) ValueChanged() const;
  NS_IMETHOD_(void) GetTextEditorValue(nsAString& aValue, bool aIgnoreWrap) const;
  NS_IMETHOD_(nsIEditor*) GetTextEditor();
  NS_IMETHOD_(nsISelectionController*) GetSelectionController();
  NS_IMETHOD_(nsFrameSelection*) GetConstFrameSelection();
  NS_IMETHOD BindToFrame(nsTextControlFrame* aFrame);
  NS_IMETHOD_(void) UnbindFromFrame(nsTextControlFrame* aFrame);
  NS_IMETHOD CreateEditor();
  NS_IMETHOD_(nsIContent*) GetRootEditorNode();
  NS_IMETHOD_(nsIContent*) CreatePlaceholderNode();
  NS_IMETHOD_(nsIContent*) GetPlaceholderNode();
  NS_IMETHOD_(void) UpdatePlaceholderVisibility(bool aNotify);
  NS_IMETHOD_(bool) GetPlaceholderVisibility();
  NS_IMETHOD_(void) InitializeKeyboardEventListeners();
  NS_IMETHOD_(void) OnValueChanged(bool aNotify);
  NS_IMETHOD_(bool) HasCachedSelection();

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                               nsIContent* aBindingParent,
                               bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              int32_t aModType) const;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

  virtual bool IsHTMLFocusable(bool aWithMouse, bool *aIsFocusable, int32_t *aTabIndex);

  virtual void DoneAddingChildren(bool aHaveNotified);
  virtual bool IsDoneAddingChildren();

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  nsresult CopyInnerTo(Element* aDest);

  


  virtual nsresult BeforeSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                 const nsAttrValueOrString* aValue,
                                 bool aNotify);

  
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsHTMLTextAreaElement,
                                           nsGenericHTMLFormElement)

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  bool     IsTooLong();
  bool     IsValueMissing() const;
  void     UpdateTooLongValidityState();
  void     UpdateValueMissingValidityState();
  void     UpdateBarredFromConstraintValidation();
  nsresult GetValidationMessage(nsAString& aValidationMessage,
                                ValidityStateType aType);

protected:
  using nsGenericHTMLFormElement::IsSingleLineTextControl; 

  nsCOMPtr<nsIControllers> mControllers;
  
  bool                     mValueChanged;
  
  bool                     mHandlingSelect;
  

  bool                     mDoneAddingChildren;
  
  bool                     mInhibitStateRestoration;
  
  bool                     mDisabledChanged;
  
  bool                     mCanShowInvalidUI;
  
  bool                     mCanShowValidUI;
  
  void FireChangeEventIfNeeded();
  
  nsString mFocusedValue;

  
  nsTextEditorState mState;

  NS_IMETHOD SelectAll(nsPresContext* aPresContext);
  






  void GetValueInternal(nsAString& aValue, bool aIgnoreWrap) const;

  nsresult SetValueInternal(const nsAString& aValue,
                            bool aUserInput);
  nsresult GetSelectionRange(int32_t* aSelectionStart, int32_t* aSelectionEnd);

  




  void ContentChanged(nsIContent* aContent);

  virtual nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom *aName,
                                const nsAttrValue* aValue, bool aNotify);

  





  bool ShouldShowValidityUI() const {
    






    if (mForm && mForm->HasEverTriedInvalidSubmit()) {
      return true;
    }

    return mValueChanged;
  }

  


  bool IsMutable() const;

  




  bool IsValueEmpty() const;
};


NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(TextArea)


nsHTMLTextAreaElement::nsHTMLTextAreaElement(already_AddRefed<nsINodeInfo> aNodeInfo,
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
}


NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsHTMLTextAreaElement,
                                                nsGenericHTMLFormElement)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mValidity)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mControllers)
  tmp->mState.Unlink();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHTMLTextAreaElement,
                                                  nsGenericHTMLFormElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mValidity)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mControllers)
  tmp->mState.Traverse(cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsHTMLTextAreaElement, Element)
NS_IMPL_RELEASE_INHERITED(nsHTMLTextAreaElement, Element)


DOMCI_NODE_DATA(HTMLTextAreaElement, nsHTMLTextAreaElement)


NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsHTMLTextAreaElement)
  NS_HTML_CONTENT_INTERFACE_TABLE5(nsHTMLTextAreaElement,
                                   nsIDOMHTMLTextAreaElement,
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
nsHTMLTextAreaElement::SelectAll(nsPresContext* aPresContext)
{
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(true);

  if (formControlFrame) {
    formControlFrame->SetFormProperty(nsGkAtoms::select, EmptyString());
  }

  return NS_OK;
}

bool
nsHTMLTextAreaElement::IsHTMLFocusable(bool aWithMouse,
                                       bool *aIsFocusable, int32_t *aTabIndex)
{
  if (nsGenericHTMLFormElement::IsHTMLFocusable(aWithMouse, aIsFocusable, aTabIndex)) {
    return true;
  }

  
  *aIsFocusable = !IsDisabled();
  return false;
}

NS_IMPL_BOOL_ATTR(nsHTMLTextAreaElement, Autofocus, autofocus)
NS_IMPL_UINT_ATTR_NON_ZERO_DEFAULT_VALUE(nsHTMLTextAreaElement, Cols, cols, DEFAULT_COLS)
NS_IMPL_BOOL_ATTR(nsHTMLTextAreaElement, Disabled, disabled)
NS_IMPL_NON_NEGATIVE_INT_ATTR(nsHTMLTextAreaElement, MaxLength, maxlength)
NS_IMPL_STRING_ATTR(nsHTMLTextAreaElement, Name, name)
NS_IMPL_BOOL_ATTR(nsHTMLTextAreaElement, ReadOnly, readonly)
NS_IMPL_BOOL_ATTR(nsHTMLTextAreaElement, Required, required)
NS_IMPL_UINT_ATTR_NON_ZERO_DEFAULT_VALUE(nsHTMLTextAreaElement, Rows, rows, DEFAULT_ROWS_TEXTAREA)
NS_IMPL_STRING_ATTR(nsHTMLTextAreaElement, Wrap, wrap)
NS_IMPL_STRING_ATTR(nsHTMLTextAreaElement, Placeholder, placeholder)
  
int32_t
nsHTMLTextAreaElement::TabIndexDefault()
{
  return 0;
}

NS_IMETHODIMP 
nsHTMLTextAreaElement::GetType(nsAString& aType)
{
  aType.AssignLiteral("textarea");

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLTextAreaElement::GetValue(nsAString& aValue)
{
  GetValueInternal(aValue, true);
  return NS_OK;
}

void
nsHTMLTextAreaElement::GetValueInternal(nsAString& aValue, bool aIgnoreWrap) const
{
  mState.GetValue(aValue, aIgnoreWrap);
}

NS_IMETHODIMP_(nsIEditor*)
nsHTMLTextAreaElement::GetTextEditor()
{
  return mState.GetEditor();
}

NS_IMETHODIMP_(nsISelectionController*)
nsHTMLTextAreaElement::GetSelectionController()
{
  return mState.GetSelectionController();
}

NS_IMETHODIMP_(nsFrameSelection*)
nsHTMLTextAreaElement::GetConstFrameSelection()
{
  return mState.GetConstFrameSelection();
}

NS_IMETHODIMP
nsHTMLTextAreaElement::BindToFrame(nsTextControlFrame* aFrame)
{
  return mState.BindToFrame(aFrame);
}

NS_IMETHODIMP_(void)
nsHTMLTextAreaElement::UnbindFromFrame(nsTextControlFrame* aFrame)
{
  if (aFrame) {
    mState.UnbindFromFrame(aFrame);
  }
}

NS_IMETHODIMP
nsHTMLTextAreaElement::CreateEditor()
{
  return mState.PrepareEditor();
}

NS_IMETHODIMP_(nsIContent*)
nsHTMLTextAreaElement::GetRootEditorNode()
{
  return mState.GetRootNode();
}

NS_IMETHODIMP_(nsIContent*)
nsHTMLTextAreaElement::CreatePlaceholderNode()
{
  NS_ENSURE_SUCCESS(mState.CreatePlaceholderNode(), nullptr);
  return mState.GetPlaceholderNode();
}

NS_IMETHODIMP_(nsIContent*)
nsHTMLTextAreaElement::GetPlaceholderNode()
{
  return mState.GetPlaceholderNode();
}

NS_IMETHODIMP_(void)
nsHTMLTextAreaElement::UpdatePlaceholderVisibility(bool aNotify)
{
  mState.UpdatePlaceholderVisibility(aNotify);
}

NS_IMETHODIMP_(bool)
nsHTMLTextAreaElement::GetPlaceholderVisibility()
{
  return mState.GetPlaceholderVisibility();
}

nsresult
nsHTMLTextAreaElement::SetValueInternal(const nsAString& aValue,
                                        bool aUserInput)
{
  
  
  
  SetValueChanged(true);
  mState.SetValue(aValue, aUserInput, true);

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLTextAreaElement::SetValue(const nsAString& aValue)
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
nsHTMLTextAreaElement::SetUserInput(const nsAString& aValue)
{
  if (!nsContentUtils::IsCallerChrome()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }
  SetValueInternal(aValue, true);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::SetValueChanged(bool aValueChanged)
{
  bool previousValue = mValueChanged;

  mValueChanged = aValueChanged;

  if (mValueChanged != previousValue) {
    UpdateState(true);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::GetDefaultValue(nsAString& aDefaultValue)
{
  nsContentUtils::GetNodeTextContent(this, false, aDefaultValue);
  return NS_OK;
}  

NS_IMETHODIMP
nsHTMLTextAreaElement::SetDefaultValue(const nsAString& aDefaultValue)
{
  nsresult rv = nsContentUtils::SetNodeTextContent(this, aDefaultValue, true);
  if (NS_SUCCEEDED(rv) && !mValueChanged) {
    Reset();
  }
  return rv;
}

bool
nsHTMLTextAreaElement::ParseAttribute(int32_t aNamespaceID,
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
nsHTMLTextAreaElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry* const map[] = {
    sDivAlignAttributeMap,
    sCommonAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map);
}

nsMapRuleToAttributesFunc
nsHTMLTextAreaElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}

bool
nsHTMLTextAreaElement::IsDisabledForEvents(uint32_t aMessage)
{
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(false);
  nsIFrame* formFrame = NULL;
  if (formControlFrame) {
    formFrame = do_QueryFrame(formControlFrame);
  }
  return IsElementDisabledForEvents(aMessage, formFrame);
}

nsresult
nsHTMLTextAreaElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
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
nsHTMLTextAreaElement::FireChangeEventIfNeeded()
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
nsHTMLTextAreaElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
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
nsHTMLTextAreaElement::DoneAddingChildren(bool aHaveNotified)
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

    controller = do_CreateInstance("@mozilla.org/editor/editingcontroller;1", &rv);
    if (NS_FAILED(rv))
      return rv;

    mControllers->AppendController(controller);
  }

  *aResult = mControllers;
  NS_IF_ADDREF(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::GetTextLength(int32_t *aTextLength)
{
  NS_ENSURE_ARG_POINTER(aTextLength);
  nsAutoString val;
  nsresult rv = GetValue(val);
  *aTextLength = val.Length();

  return rv;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::GetSelectionStart(int32_t *aSelectionStart)
{
  NS_ENSURE_ARG_POINTER(aSelectionStart);

  int32_t selEnd;
  nsresult rv = GetSelectionRange(aSelectionStart, &selEnd);

  if (NS_FAILED(rv) && mState.IsSelectionCached()) {
    *aSelectionStart = mState.GetSelectionProperties().mStart;
    return NS_OK;
  }
  return rv;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::SetSelectionStart(int32_t aSelectionStart)
{
  if (mState.IsSelectionCached()) {
    mState.GetSelectionProperties().mStart = aSelectionStart;
    return NS_OK;
  }

  nsAutoString direction;
  nsresult rv = GetSelectionDirection(direction);
  NS_ENSURE_SUCCESS(rv, rv);
  int32_t start, end;
  rv = GetSelectionRange(&start, &end);
  NS_ENSURE_SUCCESS(rv, rv);
  start = aSelectionStart;
  if (end < start) {
    end = start;
  }
  return SetSelectionRange(start, end, direction);
}

NS_IMETHODIMP
nsHTMLTextAreaElement::GetSelectionEnd(int32_t *aSelectionEnd)
{
  NS_ENSURE_ARG_POINTER(aSelectionEnd);

  int32_t selStart;
  nsresult rv = GetSelectionRange(&selStart, aSelectionEnd);

  if (NS_FAILED(rv) && mState.IsSelectionCached()) {
    *aSelectionEnd = mState.GetSelectionProperties().mEnd;
    return NS_OK;
  }
  return rv;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::SetSelectionEnd(int32_t aSelectionEnd)
{
  if (mState.IsSelectionCached()) {
    mState.GetSelectionProperties().mEnd = aSelectionEnd;
    return NS_OK;
  }

  nsAutoString direction;
  nsresult rv = GetSelectionDirection(direction);
  NS_ENSURE_SUCCESS(rv, rv);
  int32_t start, end;
  rv = GetSelectionRange(&start, &end);
  NS_ENSURE_SUCCESS(rv, rv);
  end = aSelectionEnd;
  if (start > end) {
    start = end;
  }
  return SetSelectionRange(start, end, direction);
}

nsresult
nsHTMLTextAreaElement::GetSelectionRange(int32_t* aSelectionStart,
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
nsHTMLTextAreaElement::GetSelectionDirection(nsAString& aDirection)
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
      return NS_OK;
    }
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::SetSelectionDirection(const nsAString& aDirection) {
  if (mState.IsSelectionCached()) {
    nsITextControlFrame::SelectionDirection dir = nsITextControlFrame::eNone;
    if (aDirection.EqualsLiteral("forward")) {
      dir = nsITextControlFrame::eForward;
    } else if (aDirection.EqualsLiteral("backward")) {
      dir = nsITextControlFrame::eBackward;
    }
    mState.GetSelectionProperties().mDirection = dir;
    return NS_OK;
  }

  int32_t start, end;
  nsresult rv = GetSelectionRange(&start, &end);
  if (NS_SUCCEEDED(rv)) {
    rv = SetSelectionRange(start, end, aDirection);
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::SetSelectionRange(int32_t aSelectionStart,
                                         int32_t aSelectionEnd,
                                         const nsAString& aDirection)
{ 
  nsresult rv = NS_ERROR_FAILURE;
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(true);

  if (formControlFrame) {
    nsITextControlFrame* textControlFrame = do_QueryFrame(formControlFrame);
    if (textControlFrame) {
      
      
      
      nsITextControlFrame::SelectionDirection dir = nsITextControlFrame::eForward;
      if (aDirection.EqualsLiteral("backward")) {
        dir = nsITextControlFrame::eBackward;
      }

      rv = textControlFrame->SetSelectionRange(aSelectionStart, aSelectionEnd, dir);
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

  SetValueChanged(false);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTextAreaElement::SubmitNamesValues(nsFormSubmission* aFormSubmission)
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
nsHTMLTextAreaElement::SaveState()
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

  return false;
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
nsHTMLTextAreaElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
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
nsHTMLTextAreaElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  nsGenericHTMLFormElement::UnbindFromTree(aDeep, aNullParent);

  
  UpdateValueMissingValidityState();
  UpdateBarredFromConstraintValidation();

  
  UpdateState(false);
}

nsresult
nsHTMLTextAreaElement::BeforeSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
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
                                       int32_t )
{
  ContentChanged(aFirstNewContent);
}

void
nsHTMLTextAreaElement::ContentInserted(nsIDocument* aDocument,
                                       nsIContent* aContainer,
                                       nsIContent* aChild,
                                       int32_t )
{
  ContentChanged(aChild);
}

void
nsHTMLTextAreaElement::ContentRemoved(nsIDocument* aDocument,
                                      nsIContent* aContainer,
                                      nsIContent* aChild,
                                      int32_t aIndexInContainer,
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
nsHTMLTextAreaElement::AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
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
nsHTMLTextAreaElement::CopyInnerTo(Element* aDest)
{
  nsresult rv = nsGenericHTMLFormElement::CopyInnerTo(aDest);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDest->OwnerDoc()->IsStaticDocument()) {
    nsAutoString value;
    GetValueInternal(value, true);
    static_cast<nsHTMLTextAreaElement*>(aDest)->SetValue(value);
  }
  return NS_OK;
}

bool
nsHTMLTextAreaElement::IsMutable() const
{
  return (!HasAttr(kNameSpaceID_None, nsGkAtoms::readonly) && !IsDisabled());
}

bool
nsHTMLTextAreaElement::IsValueEmpty() const
{
  nsAutoString value;
  GetValueInternal(value, true);

  return value.IsEmpty();
}



NS_IMETHODIMP
nsHTMLTextAreaElement::SetCustomValidity(const nsAString& aError)
{
  nsIConstraintValidation::SetCustomValidity(aError);

  UpdateState(true);

  return NS_OK;
}

bool
nsHTMLTextAreaElement::IsTooLong()
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
nsHTMLTextAreaElement::IsValueMissing() const
{
  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::required) || !IsMutable()) {
    return false;
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
nsHTMLTextAreaElement::IsSingleLineTextControl() const
{
  return false;
}

NS_IMETHODIMP_(bool)
nsHTMLTextAreaElement::IsTextArea() const
{
  return true;
}

NS_IMETHODIMP_(bool)
nsHTMLTextAreaElement::IsPlainTextControl() const
{
  
  return true;
}

NS_IMETHODIMP_(bool)
nsHTMLTextAreaElement::IsPasswordTextControl() const
{
  return false;
}

NS_IMETHODIMP_(int32_t)
nsHTMLTextAreaElement::GetCols()
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
nsHTMLTextAreaElement::GetWrapCols()
{
  
  nsHTMLTextWrap wrapProp;
  nsITextControlElement::GetWrapPropertyEnum(this, wrapProp);
  if (wrapProp == nsITextControlElement::eHTMLTextWrap_Off) {
    
    return -1;
  }

  
  return GetCols();
}


NS_IMETHODIMP_(int32_t)
nsHTMLTextAreaElement::GetRows()
{
  const nsAttrValue* attr = GetParsedAttr(nsGkAtoms::rows);
  if (attr && attr->Type() == nsAttrValue::eInteger) {
    int32_t rows = attr->GetIntegerValue();
    return (rows <= 0) ? DEFAULT_ROWS_TEXTAREA : rows;
  }

  return DEFAULT_ROWS_TEXTAREA;
}

NS_IMETHODIMP_(bool)
nsHTMLTextAreaElement::ValueChanged() const
{
  return mValueChanged;
}

NS_IMETHODIMP_(void)
nsHTMLTextAreaElement::GetTextEditorValue(nsAString& aValue,
                                          bool aIgnoreWrap) const
{
  mState.GetValue(aValue, aIgnoreWrap);
}

NS_IMETHODIMP_(void)
nsHTMLTextAreaElement::InitializeKeyboardEventListeners()
{
  mState.InitializeKeyboardEventListeners();
}

NS_IMETHODIMP_(void)
nsHTMLTextAreaElement::OnValueChanged(bool aNotify)
{
  
  bool validBefore = IsValid();
  UpdateTooLongValidityState();
  UpdateValueMissingValidityState();

  if (validBefore != IsValid()) {
    UpdateState(aNotify);
  }
}

NS_IMETHODIMP_(bool)
nsHTMLTextAreaElement::HasCachedSelection()
{
  return mState.IsSelectionCached();
}

void
nsHTMLTextAreaElement::FieldSetDisabledChanged(bool aNotify)
{
  UpdateValueMissingValidityState();
  UpdateBarredFromConstraintValidation();

  nsGenericHTMLFormElement::FieldSetDisabledChanged(aNotify);
}
