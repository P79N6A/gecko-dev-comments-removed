





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

class HTMLTextAreaElement MOZ_FINAL : public nsGenericHTMLFormElementWithState,
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

  virtual int32_t TabIndexDefault() MOZ_OVERRIDE;

  
  NS_DECL_NSIDOMHTMLTEXTAREAELEMENT

  
  NS_IMETHOD GetEditor(nsIEditor** aEditor) MOZ_OVERRIDE
  {
    return nsGenericHTMLElement::GetEditor(aEditor);
  }
  NS_IMETHOD SetUserInput(const nsAString& aInput) MOZ_OVERRIDE;

  
  NS_IMETHOD_(uint32_t) GetType() const MOZ_OVERRIDE { return NS_FORM_TEXTAREA; }
  NS_IMETHOD Reset() MOZ_OVERRIDE;
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission) MOZ_OVERRIDE;
  NS_IMETHOD SaveState() MOZ_OVERRIDE;
  virtual bool RestoreState(nsPresState* aState) MOZ_OVERRIDE;
  virtual bool IsDisabledForEvents(uint32_t aMessage) MOZ_OVERRIDE;

  virtual void FieldSetDisabledChanged(bool aNotify) MOZ_OVERRIDE;

  virtual EventStates IntrinsicState() const MOZ_OVERRIDE;

  
  NS_IMETHOD SetValueChanged(bool aValueChanged) MOZ_OVERRIDE;
  NS_IMETHOD_(bool) IsSingleLineTextControl() const MOZ_OVERRIDE;
  NS_IMETHOD_(bool) IsTextArea() const MOZ_OVERRIDE;
  NS_IMETHOD_(bool) IsPlainTextControl() const MOZ_OVERRIDE;
  NS_IMETHOD_(bool) IsPasswordTextControl() const MOZ_OVERRIDE;
  NS_IMETHOD_(int32_t) GetCols() MOZ_OVERRIDE;
  NS_IMETHOD_(int32_t) GetWrapCols() MOZ_OVERRIDE;
  NS_IMETHOD_(int32_t) GetRows() MOZ_OVERRIDE;
  NS_IMETHOD_(void) GetDefaultValueFromContent(nsAString& aValue) MOZ_OVERRIDE;
  NS_IMETHOD_(bool) ValueChanged() const MOZ_OVERRIDE;
  NS_IMETHOD_(void) GetTextEditorValue(nsAString& aValue, bool aIgnoreWrap) const MOZ_OVERRIDE;
  NS_IMETHOD_(nsIEditor*) GetTextEditor() MOZ_OVERRIDE;
  NS_IMETHOD_(nsISelectionController*) GetSelectionController() MOZ_OVERRIDE;
  NS_IMETHOD_(nsFrameSelection*) GetConstFrameSelection() MOZ_OVERRIDE;
  NS_IMETHOD BindToFrame(nsTextControlFrame* aFrame) MOZ_OVERRIDE;
  NS_IMETHOD_(void) UnbindFromFrame(nsTextControlFrame* aFrame) MOZ_OVERRIDE;
  NS_IMETHOD CreateEditor() MOZ_OVERRIDE;
  NS_IMETHOD_(nsIContent*) GetRootEditorNode() MOZ_OVERRIDE;
  NS_IMETHOD_(Element*) CreatePlaceholderNode() MOZ_OVERRIDE;
  NS_IMETHOD_(Element*) GetPlaceholderNode() MOZ_OVERRIDE;
  NS_IMETHOD_(void) UpdatePlaceholderVisibility(bool aNotify) MOZ_OVERRIDE;
  NS_IMETHOD_(bool) GetPlaceholderVisibility() MOZ_OVERRIDE;
  NS_IMETHOD_(void) InitializeKeyboardEventListeners() MOZ_OVERRIDE;
  NS_IMETHOD_(void) OnValueChanged(bool aNotify) MOZ_OVERRIDE;
  NS_IMETHOD_(bool) HasCachedSelection() MOZ_OVERRIDE;

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                               nsIContent* aBindingParent,
                               bool aCompileEventHandlers) MOZ_OVERRIDE;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) MOZ_OVERRIDE;
  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult) MOZ_OVERRIDE;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const MOZ_OVERRIDE;
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              int32_t aModType) const MOZ_OVERRIDE;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const MOZ_OVERRIDE;

  virtual nsresult PreHandleEvent(EventChainPreVisitor& aVisitor) MOZ_OVERRIDE;
  virtual nsresult PostHandleEvent(
                     EventChainPostVisitor& aVisitor) MOZ_OVERRIDE;

  virtual bool IsHTMLFocusable(bool aWithMouse, bool *aIsFocusable, int32_t *aTabIndex) MOZ_OVERRIDE;

  virtual void DoneAddingChildren(bool aHaveNotified) MOZ_OVERRIDE;
  virtual bool IsDoneAddingChildren() MOZ_OVERRIDE;

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  nsresult CopyInnerTo(Element* aDest);

  


  virtual nsresult BeforeSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                 const nsAttrValueOrString* aValue,
                                 bool aNotify) MOZ_OVERRIDE;

  
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
                                ValidityStateType aType) MOZ_OVERRIDE;

  
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

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

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
                                const nsAttrValue* aValue, bool aNotify) MOZ_OVERRIDE;

  





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

