





#ifndef mozilla_dom_HTMLTextAreaElement_h
#define mozilla_dom_HTMLTextAreaElement_h

#include "mozilla/Attributes.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsITextControlElement.h"
#include "nsIDOMNSEditableElement.h"
#include "nsCOMPtr.h"
#include "nsGenericHTMLElement.h"
#include "nsStubMutationObserver.h"
#include "nsIConstraintValidation.h"
#include "mozilla/dom/HTMLFormElement.h"
#include "mozilla/dom/HTMLInputElementBinding.h"
#include "nsGkAtoms.h"

#include "nsTextEditorState.h"

class nsFormSubmission;
class nsIControllers;
class nsIDocument;
class nsPresContext;
class nsPresState;

namespace mozilla {

class EventChainPostVisitor;
class EventChainPreVisitor;
class EventStates;

namespace dom {

class HTMLTextAreaElement final : public nsGenericHTMLFormElementWithState,
                                  public nsIDOMHTMLTextAreaElement,
                                  public nsITextControlElement,
                                  public nsIDOMNSEditableElement,
                                  public nsStubMutationObserver,
                                  public nsIConstraintValidation
{
public:
  using nsIConstraintValidation::GetValidationMessage;

  explicit HTMLTextAreaElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo,
                               FromParser aFromParser = NOT_FROM_PARSER);

  
  NS_DECL_ISUPPORTS_INHERITED

  virtual int32_t TabIndexDefault() override;

  
  virtual bool IsInteractiveHTMLContent(bool aIgnoreTabindex) const override
  {
    return true;
  }

  
  NS_DECL_NSIDOMHTMLTEXTAREAELEMENT

  
  NS_IMETHOD GetEditor(nsIEditor** aEditor) override
  {
    return nsGenericHTMLElement::GetEditor(aEditor);
  }
  NS_IMETHOD SetUserInput(const nsAString& aInput) override;

  
  NS_IMETHOD_(uint32_t) GetType() const override { return NS_FORM_TEXTAREA; }
  NS_IMETHOD Reset() override;
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission) override;
  NS_IMETHOD SaveState() override;
  virtual bool RestoreState(nsPresState* aState) override;
  virtual bool IsDisabledForEvents(uint32_t aMessage) override;

  virtual void FieldSetDisabledChanged(bool aNotify) override;

  virtual EventStates IntrinsicState() const override;

  
  NS_IMETHOD SetValueChanged(bool aValueChanged) override;
  NS_IMETHOD_(bool) IsSingleLineTextControl() const override;
  NS_IMETHOD_(bool) IsTextArea() const override;
  NS_IMETHOD_(bool) IsPlainTextControl() const override;
  NS_IMETHOD_(bool) IsPasswordTextControl() const override;
  NS_IMETHOD_(int32_t) GetCols() override;
  NS_IMETHOD_(int32_t) GetWrapCols() override;
  NS_IMETHOD_(int32_t) GetRows() override;
  NS_IMETHOD_(void) GetDefaultValueFromContent(nsAString& aValue) override;
  NS_IMETHOD_(bool) ValueChanged() const override;
  NS_IMETHOD_(void) GetTextEditorValue(nsAString& aValue, bool aIgnoreWrap) const override;
  NS_IMETHOD_(nsIEditor*) GetTextEditor() override;
  NS_IMETHOD_(nsISelectionController*) GetSelectionController() override;
  NS_IMETHOD_(nsFrameSelection*) GetConstFrameSelection() override;
  NS_IMETHOD BindToFrame(nsTextControlFrame* aFrame) override;
  NS_IMETHOD_(void) UnbindFromFrame(nsTextControlFrame* aFrame) override;
  NS_IMETHOD CreateEditor() override;
  NS_IMETHOD_(nsIContent*) GetRootEditorNode() override;
  NS_IMETHOD_(Element*) CreatePlaceholderNode() override;
  NS_IMETHOD_(Element*) GetPlaceholderNode() override;
  NS_IMETHOD_(void) UpdatePlaceholderVisibility(bool aNotify) override;
  NS_IMETHOD_(bool) GetPlaceholderVisibility() override;
  NS_IMETHOD_(void) InitializeKeyboardEventListeners() override;
  NS_IMETHOD_(void) OnValueChanged(bool aNotify) override;
  NS_IMETHOD_(bool) HasCachedSelection() override;

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                               nsIContent* aBindingParent,
                               bool aCompileEventHandlers) override;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) override;
  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult) override;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const override;
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              int32_t aModType) const override;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const override;

  virtual nsresult PreHandleEvent(EventChainPreVisitor& aVisitor) override;
  virtual nsresult PostHandleEvent(
                     EventChainPostVisitor& aVisitor) override;

  virtual bool IsHTMLFocusable(bool aWithMouse, bool *aIsFocusable, int32_t *aTabIndex) override;

  virtual void DoneAddingChildren(bool aHaveNotified) override;
  virtual bool IsDoneAddingChildren() override;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  nsresult CopyInnerTo(Element* aDest);

  


  virtual nsresult BeforeSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                 const nsAttrValueOrString* aValue,
                                 bool aNotify) override;

  
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLTextAreaElement,
                                           nsGenericHTMLFormElementWithState)

  
  bool     IsTooLong();
  bool     IsValueMissing() const;
  void     UpdateTooLongValidityState();
  void     UpdateValueMissingValidityState();
  void     UpdateBarredFromConstraintValidation();
  nsresult GetValidationMessage(nsAString& aValidationMessage,
                                ValidityStateType aType) override;

  
  bool Autofocus()
  {
    return GetBoolAttr(nsGkAtoms::autofocus);
  }
  void SetAutofocus(bool aAutoFocus, ErrorResult& aError)
  {
    SetHTMLBoolAttr(nsGkAtoms::autofocus, aAutoFocus, aError);
  }
  uint32_t Cols()
  {
    return GetIntAttr(nsGkAtoms::cols, DEFAULT_COLS);
  }
  void SetCols(uint32_t aCols, ErrorResult& aError)
  {
    if (aCols == 0) {
      aError.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    } else {
      SetUnsignedIntAttr(nsGkAtoms::cols, aCols, aError);
    }
  }
  bool Disabled()
  {
    return GetBoolAttr(nsGkAtoms::disabled);
  }
  void SetDisabled(bool aDisabled, ErrorResult& aError)
  {
    SetHTMLBoolAttr(nsGkAtoms::disabled, aDisabled, aError);
  }
  
  using nsGenericHTMLFormElementWithState::GetForm;
  int32_t MaxLength()
  {
    return GetIntAttr(nsGkAtoms::maxlength, -1);
  }
  void SetMaxLength(int32_t aMaxLength, ErrorResult& aError)
  {
    if (aMaxLength < 0) {
      aError.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    } else {
      SetHTMLIntAttr(nsGkAtoms::maxlength, aMaxLength, aError);
    }
  }
  
  void SetName(const nsAString& aName, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::name, aName, aError);
  }
  
  void SetPlaceholder(const nsAString& aPlaceholder, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::placeholder, aPlaceholder, aError);
  }
  bool ReadOnly()
  {
    return GetBoolAttr(nsGkAtoms::readonly);
  }
  void SetReadOnly(bool aReadOnly, ErrorResult& aError)
  {
    SetHTMLBoolAttr(nsGkAtoms::readonly, aReadOnly, aError);
  }
  bool Required()
  {
    return GetBoolAttr(nsGkAtoms::required);
  }

  void SetRangeText(const nsAString& aReplacement, ErrorResult& aRv);

  void SetRangeText(const nsAString& aReplacement, uint32_t aStart,
                    uint32_t aEnd, const SelectionMode& aSelectMode,
                    ErrorResult& aRv, int32_t aSelectionStart = -1,
                    int32_t aSelectionEnd = -1);

  void SetRequired(bool aRequired, ErrorResult& aError)
  {
    SetHTMLBoolAttr(nsGkAtoms::required, aRequired, aError);
  }
  uint32_t Rows()
  {
    return GetIntAttr(nsGkAtoms::rows, DEFAULT_ROWS_TEXTAREA);
  }
  void SetRows(uint32_t aRows, ErrorResult& aError)
  {
    if (aRows == 0) {
      aError.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    } else {
      SetUnsignedIntAttr(nsGkAtoms::rows, aRows, aError);
    }
  }
  
  void SetWrap(const nsAString& aWrap, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::wrap, aWrap, aError);
  }
  
  
  void SetDefaultValue(const nsAString& aDefaultValue, ErrorResult& aError);
  
  uint32_t GetTextLength();
  
  
  
  
  using nsIConstraintValidation::CheckValidity;
  
  
  uint32_t GetSelectionStart(ErrorResult& aError);
  void SetSelectionStart(uint32_t aSelectionStart, ErrorResult& aError);
  uint32_t GetSelectionEnd(ErrorResult& aError);
  void SetSelectionEnd(uint32_t aSelectionEnd, ErrorResult& aError);
  void GetSelectionDirection(nsAString& aDirection, ErrorResult& aError);
  void SetSelectionDirection(const nsAString& aDirection, ErrorResult& aError);
  void SetSelectionRange(uint32_t aSelectionStart, uint32_t aSelectionEnd, const Optional<nsAString>& aDirecton, ErrorResult& aError);
  nsIControllers* GetControllers(ErrorResult& aError);
  nsIEditor* GetEditor()
  {
    return mState.GetEditor();
  }

protected:
  virtual ~HTMLTextAreaElement() {}

  
  using nsGenericHTMLFormElementWithState::IsSingleLineTextControl;

  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

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

  





  nsresult SetValueInternal(const nsAString& aValue, uint32_t aFlags);

  nsresult GetSelectionRange(int32_t* aSelectionStart, int32_t* aSelectionEnd);

  




  void ContentChanged(nsIContent* aContent);

  virtual nsresult AfterSetAttr(int32_t aNamespaceID, nsIAtom *aName,
                                const nsAttrValue* aValue, bool aNotify) override;

  





  bool ShouldShowValidityUI() const {
    






    if (mForm && mForm->HasEverTriedInvalidSubmit()) {
      return true;
    }

    return mValueChanged;
  }

  


  bool IsMutable() const;

  




  bool IsValueEmpty() const;

private:
  static void MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                    nsRuleData* aData);
};

} 
} 

#endif

