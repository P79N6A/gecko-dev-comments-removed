




#ifndef mozilla_dom_HTMLInputElement_h
#define mozilla_dom_HTMLInputElement_h

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"
#include "nsImageLoadingContent.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsITextControlElement.h"
#include "nsITimer.h"
#include "nsIPhonetic.h"
#include "nsIDOMNSEditableElement.h"
#include "nsCOMPtr.h"
#include "nsIConstraintValidation.h"
#include "mozilla/dom/HTMLFormElement.h" 
#include "mozilla/dom/HTMLInputElementBinding.h"
#include "nsIFilePicker.h"
#include "nsIContentPrefService2.h"
#include "mozilla/Decimal.h"
#include "nsContentUtils.h"
#include "nsTextEditorState.h"

class nsDOMFileList;
class nsIRadioGroupContainer;
class nsIRadioGroupVisitor;
class nsIRadioVisitor;

namespace mozilla {

class EventChainPostVisitor;
class EventChainPreVisitor;

namespace dom {

class Date;
class DirPickerFileListBuilderTask;

class UploadLastDir MOZ_FINAL : public nsIObserver, public nsSupportsWeakReference {

  ~UploadLastDir() {}

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  







  nsresult FetchDirectoryAndDisplayPicker(nsIDocument* aDoc,
                                          nsIFilePicker* aFilePicker,
                                          nsIFilePickerShownCallback* aFpCallback);

  





  nsresult StoreLastUsedDirectory(nsIDocument* aDoc, nsIFile* aDir);

  class ContentPrefCallback MOZ_FINAL : public nsIContentPrefCallback2
  {
    virtual ~ContentPrefCallback()
    { }

  public:
    ContentPrefCallback(nsIFilePicker* aFilePicker, nsIFilePickerShownCallback* aFpCallback)
    : mFilePicker(aFilePicker)
    , mFpCallback(aFpCallback)
    { }

    NS_DECL_ISUPPORTS
    NS_DECL_NSICONTENTPREFCALLBACK2

    nsCOMPtr<nsIFilePicker> mFilePicker;
    nsCOMPtr<nsIFilePickerShownCallback> mFpCallback;
    nsCOMPtr<nsIContentPref> mResult;
  };
};

class HTMLInputElement MOZ_FINAL : public nsGenericHTMLFormElementWithState,
                                   public nsImageLoadingContent,
                                   public nsIDOMHTMLInputElement,
                                   public nsITextControlElement,
                                   public nsIPhonetic,
                                   public nsIDOMNSEditableElement,
                                   public nsITimerCallback,
                                   public nsIConstraintValidation
{
  friend class DirPickerFileListBuilderTask;

public:
  using nsIConstraintValidation::GetValidationMessage;
  using nsIConstraintValidation::CheckValidity;
  using nsIConstraintValidation::WillValidate;
  using nsIConstraintValidation::Validity;
  using nsGenericHTMLFormElementWithState::GetForm;

  HTMLInputElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo,
                   mozilla::dom::FromParser aFromParser);

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLInputElement, input)

  
  NS_DECL_ISUPPORTS_INHERITED

  virtual int32_t TabIndexDefault() MOZ_OVERRIDE;
  using nsGenericHTMLElement::Focus;
  virtual void Blur(ErrorResult& aError) MOZ_OVERRIDE;
  virtual void Focus(ErrorResult& aError) MOZ_OVERRIDE;

  
  NS_DECL_NSIDOMHTMLINPUTELEMENT

  
  NS_DECL_NSIPHONETIC

  
  NS_IMETHOD GetEditor(nsIEditor** aEditor) MOZ_OVERRIDE
  {
    return nsGenericHTMLElement::GetEditor(aEditor);
  }

  NS_IMETHOD SetUserInput(const nsAString& aInput) MOZ_OVERRIDE;

  
  NS_IMETHOD_(uint32_t) GetType() const MOZ_OVERRIDE { return mType; }
  NS_IMETHOD Reset() MOZ_OVERRIDE;
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission) MOZ_OVERRIDE;
  NS_IMETHOD SaveState() MOZ_OVERRIDE;
  virtual bool RestoreState(nsPresState* aState) MOZ_OVERRIDE;
  virtual bool AllowDrop() MOZ_OVERRIDE;
  virtual bool IsDisabledForEvents(uint32_t aMessage) MOZ_OVERRIDE;

  virtual void FieldSetDisabledChanged(bool aNotify) MOZ_OVERRIDE;

  
  virtual bool IsHTMLFocusable(bool aWithMouse, bool *aIsFocusable, int32_t *aTabIndex) MOZ_OVERRIDE;

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult) MOZ_OVERRIDE;
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              int32_t aModType) const MOZ_OVERRIDE;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const MOZ_OVERRIDE;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const MOZ_OVERRIDE;

  virtual nsresult PreHandleEvent(EventChainPreVisitor& aVisitor) MOZ_OVERRIDE;
  virtual nsresult PostHandleEvent(
                     EventChainPostVisitor& aVisitor) MOZ_OVERRIDE;
  void PostHandleEventForRangeThumb(EventChainPostVisitor& aVisitor);
  void StartRangeThumbDrag(WidgetGUIEvent* aEvent);
  void FinishRangeThumbDrag(WidgetGUIEvent* aEvent = nullptr);
  void CancelRangeThumbDrag(bool aIsForUserEvent = true);
  void SetValueOfRangeForUserEvent(Decimal aValue);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers) MOZ_OVERRIDE;
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true) MOZ_OVERRIDE;

  virtual void DoneCreatingElement() MOZ_OVERRIDE;

  virtual EventStates IntrinsicState() const MOZ_OVERRIDE;

  
private:
  virtual void AddStates(EventStates aStates);
  virtual void RemoveStates(EventStates aStates);

public:

  
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

  void GetDisplayFileName(nsAString& aFileName) const;

  const nsTArray<nsCOMPtr<nsIDOMFile> >& GetFilesInternal() const
  {
    return mFiles;
  }

  void SetFiles(const nsTArray<nsCOMPtr<nsIDOMFile> >& aFiles, bool aSetValueChanged);
  void SetFiles(nsIDOMFileList* aFiles, bool aSetValueChanged);

  
  void PickerClosed();

  void SetCheckedChangedInternal(bool aCheckedChanged);
  bool GetCheckedChanged() const {
    return mCheckedChanged;
  }
  void AddedToRadioGroup();
  void WillRemoveFromRadioGroup();

 






  already_AddRefed<nsIDOMHTMLInputElement> GetSelectedRadioButton();

  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(HTMLInputElement,
                                           nsGenericHTMLFormElementWithState)

  static UploadLastDir* gUploadLastDir;
  
  
  static void InitUploadLastDir();
  static void DestroyUploadLastDir();

  void MaybeLoadImage();

  void SetSelectionProperties(const nsTextEditorState::SelectionProperties& aProps)
  {
    MOZ_ASSERT(mType == NS_FORM_INPUT_NUMBER);
    mSelectionCached = true;
    mSelectionProperties = aProps;
  }
  bool IsSelectionCached() const
  {
    MOZ_ASSERT(mType == NS_FORM_INPUT_NUMBER);
    return mSelectionCached;
  }
  void ClearSelectionCached()
  {
    MOZ_ASSERT(mType == NS_FORM_INPUT_NUMBER);
    mSelectionCached = false;
  }
  nsTextEditorState::SelectionProperties& GetSelectionProperties()
  {
    MOZ_ASSERT(mType == NS_FORM_INPUT_NUMBER);
    return mSelectionProperties;
  }

  
  NS_DECL_NSITIMERCALLBACK

  
  
  
  using nsImageLoadingContent::Notify;

  
  bool     IsTooLong();
  bool     IsValueMissing() const;
  bool     HasTypeMismatch() const;
  bool     HasPatternMismatch() const;
  bool     IsRangeOverflow() const;
  bool     IsRangeUnderflow() const;
  bool     HasStepMismatch(bool aUseZeroIfValueNaN = false) const;
  bool     HasBadInput() const;
  void     UpdateTooLongValidityState();
  void     UpdateValueMissingValidityState();
  void     UpdateTypeMismatchValidityState();
  void     UpdatePatternMismatchValidityState();
  void     UpdateRangeOverflowValidityState();
  void     UpdateRangeUnderflowValidityState();
  void     UpdateStepMismatchValidityState();
  void     UpdateBadInputValidityState();
  void     UpdateAllValidityStates(bool aNotify);
  void     UpdateBarredFromConstraintValidation();
  nsresult GetValidationMessage(nsAString& aValidationMessage,
                                ValidityStateType aType) MOZ_OVERRIDE;
  







  void     UpdateValueMissingValidityStateForRadio(bool aIgnoreSelf);

  














  void SetFilePickerFiltersFromAccept(nsIFilePicker* filePicker);

  








  void UpdateValidityUIBits(bool aIsFocused);

  


  void FireChangeEventIfNeeded();

  





  Decimal GetValueAsDecimal() const;

  







  Decimal GetMinimum() const;

  







  Decimal GetMaximum() const;

  

  
  void SetAccept(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::accept, aValue, aRv);
  }

  
  void SetAlt(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::alt, aValue, aRv);
  }

  
  void SetAutocomplete(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::autocomplete, aValue, aRv);
  }

  void GetAutocompleteInfo(AutocompleteInfo& aInfo);

  bool Autofocus() const
  {
    return GetBoolAttr(nsGkAtoms::autofocus);
  }

  void SetAutofocus(bool aValue, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::autofocus, aValue, aRv);
  }

  bool DefaultChecked() const
  {
    return HasAttr(kNameSpaceID_None, nsGkAtoms::checked);
  }

  void SetDefaultChecked(bool aValue, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::checked, aValue, aRv);
  }

  bool Checked() const
  {
    return mChecked;
  }
  

  bool Disabled() const
  {
    return GetBoolAttr(nsGkAtoms::disabled);
  }

  void SetDisabled(bool aValue,ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::disabled, aValue, aRv);
  }

  

  nsDOMFileList* GetFiles();

  void OpenDirectoryPicker(ErrorResult& aRv);
  void CancelDirectoryPickerScanIfRunning();

  void StartProgressEventTimer();
  void MaybeDispatchProgressEvent(bool aFinalProgress);
  void DispatchProgressEvent(const nsAString& aType,
                             bool aLengthComputable,
                             uint64_t aLoaded, uint64_t aTotal);

  
  void SetFormAction(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::formaction, aValue, aRv);
  }

  
  void SetFormEnctype(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::formenctype, aValue, aRv);
  }

  
  void SetFormMethod(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::formmethod, aValue, aRv);
  }

  bool FormNoValidate() const
  {
    return GetBoolAttr(nsGkAtoms::formnovalidate);
  }

  void SetFormNoValidate(bool aValue, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::formnovalidate, aValue, aRv);
  }

  
  void SetFormTarget(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::formtarget, aValue, aRv);
  }

  uint32_t Height();

  void SetHeight(uint32_t aValue, ErrorResult& aRv)
  {
    SetUnsignedIntAttr(nsGkAtoms::height, aValue, aRv);
  }

  bool Indeterminate() const
  {
    return mIndeterminate;
  }
  

  
  void SetInputMode(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::inputmode, aValue, aRv);
  }

  nsGenericHTMLElement* GetList() const;

  
  void SetMax(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::max, aValue, aRv);
  }

  int32_t MaxLength() const
  {
    return GetIntAttr(nsGkAtoms::maxlength, -1);
  }

  void SetMaxLength(int32_t aValue, ErrorResult& aRv)
  {
    if (aValue < 0) {
      aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
      return;
    }

    SetHTMLIntAttr(nsGkAtoms::maxlength, aValue, aRv);
  }

  
  void SetMin(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::min, aValue, aRv);
  }

  bool Multiple() const
  {
    return GetBoolAttr(nsGkAtoms::multiple);
  }

  void SetMultiple(bool aValue, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::multiple, aValue, aRv);
  }

  
  void SetName(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::name, aValue, aRv);
  }

  
  void SetPattern(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::pattern, aValue, aRv);
  }

  
  void SetPlaceholder(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::placeholder, aValue, aRv);
  }

  bool ReadOnly() const
  {
    return GetBoolAttr(nsGkAtoms::readonly);
  }

  void SetReadOnly(bool aValue, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::readonly, aValue, aRv);
  }

  bool Required() const
  {
    return GetBoolAttr(nsGkAtoms::required);
  }

  void SetRequired(bool aValue, ErrorResult& aRv)
  {
    SetHTMLBoolAttr(nsGkAtoms::required, aValue, aRv);
  }

  uint32_t Size() const
  {
    return GetUnsignedIntAttr(nsGkAtoms::size, DEFAULT_COLS);
  }

  void SetSize(uint32_t aValue, ErrorResult& aRv)
  {
    if (aValue == 0) {
      aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
      return;
    }

    SetUnsignedIntAttr(nsGkAtoms::size, aValue, aRv);
  }

  
  void SetSrc(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::src, aValue, aRv);
  }

  
  void SetStep(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::step, aValue, aRv);
  }

  
  void SetType(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::type, aValue, aRv);
    if (aValue.EqualsLiteral("number")) {
      
      
      
      
      FlushFrames();
    }
  }

  
  void SetDefaultValue(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::value, aValue, aRv);
  }

  
  void SetValue(const nsAString& aValue, ErrorResult& aRv);

  Nullable<Date> GetValueAsDate(ErrorResult& aRv);

  void SetValueAsDate(Nullable<Date>, ErrorResult& aRv);

  double ValueAsNumber() const
  {
    return DoesValueAsNumberApply() ? GetValueAsDecimal().toDouble()
                                    : UnspecifiedNaN<double>();
  }

  void SetValueAsNumber(double aValue, ErrorResult& aRv);

  uint32_t Width();

  void SetWidth(uint32_t aValue, ErrorResult& aRv)
  {
    SetUnsignedIntAttr(nsGkAtoms::width, aValue, aRv);
  }

  void StepUp(int32_t aN, ErrorResult& aRv)
  {
    aRv = ApplyStep(aN);
  }

  void StepDown(int32_t aN, ErrorResult& aRv)
  {
    aRv = ApplyStep(-aN);
  }

  





  Decimal GetStep() const;

  void GetValidationMessage(nsAString& aValidationMessage, ErrorResult& aRv);

  

  

  int32_t GetSelectionStart(ErrorResult& aRv);
  void SetSelectionStart(int32_t aValue, ErrorResult& aRv);

  int32_t GetSelectionEnd(ErrorResult& aRv);
  void SetSelectionEnd(int32_t aValue, ErrorResult& aRv);

  void GetSelectionDirection(nsAString& aValue, ErrorResult& aRv);
  void SetSelectionDirection(const nsAString& aValue, ErrorResult& aRv);

  void SetSelectionRange(int32_t aStart, int32_t aEnd,
                         const Optional< nsAString >& direction,
                         ErrorResult& aRv);

  void SetRangeText(const nsAString& aReplacement, ErrorResult& aRv);

  void SetRangeText(const nsAString& aReplacement, uint32_t aStart,
                    uint32_t aEnd, const SelectionMode& aSelectMode,
                    ErrorResult& aRv, int32_t aSelectionStart = -1,
                    int32_t aSelectionEnd = -1);

  
  void SetAlign(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::align, aValue, aRv);
  }

  
  void SetUseMap(const nsAString& aValue, ErrorResult& aRv)
  {
    SetHTMLAttr(nsGkAtoms::usemap, aValue, aRv);
  }

  nsIControllers* GetControllers(ErrorResult& aRv);

  int32_t GetTextLength(ErrorResult& aRv);

  void MozGetFileNameArray(nsTArray< nsString >& aFileNames);

  void MozSetFileNameArray(const Sequence< nsString >& aFileNames);

  HTMLInputElement* GetOwnerNumberControl();

  void StartNumberControlSpinnerSpin();
  void StopNumberControlSpinnerSpin();
  void StepNumberControlForUserEvent(int32_t aDirection);

  



  static void HandleNumberControlSpin(void* aData);

  bool NumberSpinnerUpButtonIsDepressed() const
  {
    return mNumberControlSpinnerIsSpinning && mNumberControlSpinnerSpinsUp;
  }

  bool NumberSpinnerDownButtonIsDepressed() const
  {
    return mNumberControlSpinnerIsSpinning && !mNumberControlSpinnerSpinsUp;
  }

  bool MozIsTextField(bool aExcludePassword);

  nsIEditor* GetEditor();

  

  

  








  static Decimal StringToDecimal(const nsAString& aValue);

protected:
  virtual ~HTMLInputElement();

  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;

  
  
  using nsGenericHTMLFormElementWithState::IsSingleLineTextControl;

  




  enum ValueModeType
  {
    
    
    VALUE_MODE_VALUE,
    
    
    VALUE_MODE_DEFAULT,
    
    
    VALUE_MODE_DEFAULT_ON,
    
    
    
    
    VALUE_MODE_FILENAME
  };

  







  static bool IsValidEmailAddress(const nsAString& aValue);

  









  static bool IsValidEmailAddressList(const nsAString& aValue);

  













  static bool DigitSubStringToNumber(const nsAString& aValue, uint32_t aStart,
                                     uint32_t aLen, uint32_t* aResult);

  
  nsresult SetValueInternal(const nsAString& aValue,
                            bool aUserInput,
                            bool aSetValueChanged);

  nsresult GetValueInternal(nsAString& aValue) const;

  




  bool IsValueEmpty() const;

  void ClearFiles(bool aSetValueChanged);

  void SetIndeterminateInternal(bool aValue,
                                bool aShouldInvalidate);

  nsresult GetSelectionRange(int32_t* aSelectionStart, int32_t* aSelectionEnd);

  


  virtual nsresult BeforeSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                 const nsAttrValueOrString* aValue,
                                 bool aNotify) MOZ_OVERRIDE;
  


  virtual nsresult AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify) MOZ_OVERRIDE;

  


  bool DispatchSelectEvent(nsPresContext* aPresContext);

  void SelectAll(nsPresContext* aPresContext);
  bool IsImage() const
  {
    return AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                       nsGkAtoms::image, eIgnoreCase);
  }

  



  nsresult VisitGroup(nsIRadioVisitor* aVisitor, bool aFlushContent);

  



  void DoSetChecked(bool aValue, bool aNotify, bool aSetValueChanged);

  




  void DoSetCheckedChanged(bool aCheckedChanged, bool aNotify);

  



  void SetCheckedInternal(bool aValue, bool aNotify);

  void RadioSetChecked(bool aNotify);
  void SetCheckedChanged(bool aCheckedChanged);

  



  nsresult MaybeSubmitForm(nsPresContext* aPresContext);

  


  nsresult UpdateFileList();

  


  void AfterSetFiles(bool aSetValueChanged);

  



  bool NeedToInitializeEditorForEvent(EventChainPreVisitor& aVisitor) const;

  


  ValueModeType GetValueMode() const;

  






  bool IsMutable() const;

  


  bool DoesReadOnlyApply() const;

  


  bool DoesRequiredApply() const;

  


  bool DoesPatternApply() const;

  


  bool DoesMinMaxApply() const;

  


  bool DoesStepApply() const { return DoesMinMaxApply(); }

  


  bool DoStepDownStepUpApply() const { return DoesStepApply(); }

  


  bool DoesValueAsNumberApply() const { return DoesMinMaxApply(); }

  


  bool MaxLengthApplies() const { return IsSingleLineTextControl(false, mType); }

  void FreeData();
  nsTextEditorState *GetEditorState() const;

  


  void HandleTypeChange(uint8_t aNewType);

  



  void SanitizeValue(nsAString& aValue);

  


  bool PlaceholderApplies() const;

  




  nsresult SetDefaultValueAsValue();

  virtual void SetDirectionIfAuto(bool aAuto, bool aNotify);

  





  bool ShouldShowValidityUI() const {
    





    if (mForm && mForm->HasEverTriedInvalidSubmit()) {
      return true;
    }

    switch (GetValueMode()) {
      case VALUE_MODE_DEFAULT:
        return true;
      case VALUE_MODE_DEFAULT_ON:
        return GetCheckedChanged();
      case VALUE_MODE_VALUE:
      case VALUE_MODE_FILENAME:
        return mValueChanged;
      default:
        NS_NOTREACHED("We should not be there: there are no other modes.");
        return false;
    }
  }

  





  nsIRadioGroupContainer* GetRadioGroupContainer() const;

  








  bool ConvertStringToNumber(nsAString& aValue, Decimal& aResultValue) const;

  










  bool ConvertNumberToString(Decimal aValue, nsAString& aResultString) const;

  





  bool IsValidSimpleColor(const nsAString& aValue) const;

  





  bool IsValidDate(const nsAString& aValue) const;

  





  bool GetValueAsDate(const nsAString& aValue,
                      uint32_t* aYear,
                      uint32_t* aMonth,
                      uint32_t* aDay) const;

  


  uint32_t NumberOfDaysInMonth(uint32_t aMonth, uint32_t aYear) const;

  






  bool IsValidTime(const nsAString& aValue) const;

  










  static bool ParseTime(const nsAString& aValue, uint32_t* aResult);

  




  void SetValue(Decimal aValue);

  


  void UpdateHasRange();

   




  Decimal GetStepScaleFactor() const;

  





  Decimal GetStepBase() const;

  



  Decimal GetDefaultStep() const;

  enum StepCallerType {
    CALLED_FOR_USER_EVENT,
    CALLED_FOR_SCRIPT
  };

  











  nsresult GetValueIfStepped(int32_t aStepCount,
                             StepCallerType aCallerType,
                             Decimal* aNextStep);

  





  nsresult ApplyStep(int32_t aStep);

  


  static bool IsExperimentalMobileType(uint8_t aType)
  {
    return aType == NS_FORM_INPUT_DATE || aType == NS_FORM_INPUT_TIME;
  }

  


  void FlushFrames();

  




  bool ShouldPreventDOMActivateDispatch(EventTarget* aOriginalTarget);

  




  nsresult MaybeInitPickers(EventChainPostVisitor& aVisitor);

  enum FilePickerType {
    FILE_PICKER_FILE,
    FILE_PICKER_DIRECTORY
  };
  nsresult InitFilePicker(FilePickerType aType);
  nsresult InitColorPicker();

  






  bool IsPopupBlocked() const;

  nsCOMPtr<nsIControllers> mControllers;

  







  union InputData {
    


    char16_t*               mValue;
    


    nsTextEditorState*       mState;
  } mInputData;
  









  nsTArray<nsCOMPtr<nsIDOMFile> >   mFiles;

  nsRefPtr<nsDOMFileList>  mFileList;

  nsRefPtr<DirPickerFileListBuilderTask> mDirPickerFileListBuilderTask;

  nsString mStaticDocFileList;
  
  






  nsString mFocusedValue;  

  




  Decimal mRangeThumbDragStartValue;

  




  nsCOMPtr<nsITimer> mProgressTimer;

  




  nsTextEditorState::SelectionProperties mSelectionProperties;

  
  static const Decimal kStepScaleFactorDate;
  static const Decimal kStepScaleFactorNumberRange;
  static const Decimal kStepScaleFactorTime;

  
  static const Decimal kDefaultStepBase;

  
  static const Decimal kDefaultStep;
  static const Decimal kDefaultStepTime;

  
  static const Decimal kStepAny;

  



  uint8_t                  mType;
  nsContentUtils::AutocompleteAttrState mAutocompleteAttrState;
  bool                     mDisabledChanged     : 1;
  bool                     mValueChanged        : 1;
  bool                     mCheckedChanged      : 1;
  bool                     mChecked             : 1;
  bool                     mHandlingSelectEvent : 1;
  bool                     mShouldInitChecked   : 1;
  bool                     mParserCreating      : 1;
  bool                     mInInternalActivate  : 1;
  bool                     mCheckedIsToggled    : 1;
  bool                     mIndeterminate       : 1;
  bool                     mInhibitRestoration  : 1;
  bool                     mCanShowValidUI      : 1;
  bool                     mCanShowInvalidUI    : 1;
  bool                     mHasRange            : 1;
  bool                     mIsDraggingRange     : 1;
  bool                     mProgressTimerIsActive : 1;
  bool                     mNumberControlSpinnerIsSpinning : 1;
  bool                     mNumberControlSpinnerSpinsUp : 1;
  bool                     mPickerRunning : 1;
  bool                     mSelectionCached : 1;

private:
  static void MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                    nsRuleData* aData);

  



  bool MayFireChangeOnBlur() const {
    return MayFireChangeOnBlur(mType);
  }

  


  bool SupportsSetRangeText() const {
    return mType == NS_FORM_INPUT_TEXT || mType == NS_FORM_INPUT_SEARCH ||
           mType == NS_FORM_INPUT_URL || mType == NS_FORM_INPUT_TEL ||
           mType == NS_FORM_INPUT_PASSWORD || mType == NS_FORM_INPUT_NUMBER;
  }

  static bool MayFireChangeOnBlur(uint8_t aType) {
    return IsSingleLineTextControl(false, aType) ||
           aType == NS_FORM_INPUT_RANGE ||
           aType == NS_FORM_INPUT_NUMBER;
  }

  struct nsFilePickerFilter {
    nsFilePickerFilter()
      : mFilterMask(0), mIsTrusted(false) {}

    explicit nsFilePickerFilter(int32_t aFilterMask)
      : mFilterMask(aFilterMask), mIsTrusted(true) {}

    nsFilePickerFilter(const nsString& aTitle,
                       const nsString& aFilter,
                       const bool aIsTrusted = false)
      : mFilterMask(0), mTitle(aTitle), mFilter(aFilter), mIsTrusted(aIsTrusted) {}

    nsFilePickerFilter(const nsFilePickerFilter& other) {
      mFilterMask = other.mFilterMask;
      mTitle = other.mTitle;
      mFilter = other.mFilter;
      mIsTrusted = other.mIsTrusted;
    }

    bool operator== (const nsFilePickerFilter& other) const {
      if ((mFilter == other.mFilter) && (mFilterMask == other.mFilterMask)) {
        NS_ASSERTION(mIsTrusted == other.mIsTrusted,
                     "Filter with similar list of extensions and mask should"
                     " have the same trusted flag value");
        return true;
      } else {
        return false;
      }
    }
    
    
    int32_t mFilterMask;
    
    
    nsString mTitle;
    nsString mFilter;
    
    
    
    
    
    bool mIsTrusted; 
  };

  class nsFilePickerShownCallback
    : public nsIFilePickerShownCallback
  {
    virtual ~nsFilePickerShownCallback()
    { }

  public:
    nsFilePickerShownCallback(HTMLInputElement* aInput,
                              nsIFilePicker* aFilePicker);
    NS_DECL_ISUPPORTS

    NS_IMETHOD Done(int16_t aResult) MOZ_OVERRIDE;

  private:
    nsCOMPtr<nsIFilePicker> mFilePicker;
    nsRefPtr<HTMLInputElement> mInput;
  };
};

} 
} 

#endif
