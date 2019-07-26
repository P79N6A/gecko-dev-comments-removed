




#ifndef nsHTMLInputElement_h__
#define nsHTMLInputElement_h__

#include "nsGenericHTMLElement.h"
#include "nsImageLoadingContent.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsITextControlElement.h"
#include "nsIPhonetic.h"
#include "nsIDOMNSEditableElement.h"
#include "nsTextEditorState.h"
#include "nsCOMPtr.h"
#include "nsIConstraintValidation.h"
#include "nsDOMFile.h"
#include "nsHTMLFormElement.h" 
#include "nsIFile.h"
#include "nsIFilePicker.h"

class nsDOMFileList;
class nsIFilePicker;
class nsIRadioGroupContainer;
class nsIRadioGroupVisitor;
class nsIRadioVisitor;

class UploadLastDir MOZ_FINAL : public nsIObserver, public nsSupportsWeakReference {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  






  nsresult FetchLastUsedDirectory(nsIDocument* aDoc, nsIFile** aFile);

  






  nsresult StoreLastUsedDirectory(nsIDocument* aDoc, nsIDOMFile* aDomFile);
};

class nsHTMLInputElement : public nsGenericHTMLFormElement,
                           public nsImageLoadingContent,
                           public nsIDOMHTMLInputElement,
                           public nsITextControlElement,
                           public nsIPhonetic,
                           public nsIDOMNSEditableElement,
                           public nsIConstraintValidation
{
public:
  using nsIConstraintValidation::GetValidationMessage;

  nsHTMLInputElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                     mozilla::dom::FromParser aFromParser);
  virtual ~nsHTMLInputElement();

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(nsHTMLInputElement, input)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC
  virtual int32_t TabIndexDefault() MOZ_OVERRIDE;
  virtual void Focus(mozilla::ErrorResult& aError) MOZ_OVERRIDE;

  
  NS_DECL_NSIDOMHTMLINPUTELEMENT

  
  NS_DECL_NSIPHONETIC

  
  NS_IMETHOD GetEditor(nsIEditor** aEditor)
  {
    return nsGenericHTMLElement::GetEditor(aEditor);
  }

  NS_IMETHOD SetUserInput(const nsAString& aInput);

  
  NS_IMETHOD_(uint32_t) GetType() const { return mType; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission);
  NS_IMETHOD SaveState();
  virtual bool RestoreState(nsPresState* aState);
  virtual bool AllowDrop();
  virtual bool IsDisabledForEvents(uint32_t aMessage);

  virtual void FieldSetDisabledChanged(bool aNotify);

  
  virtual bool IsHTMLFocusable(bool aWithMouse, bool *aIsFocusable, int32_t *aTabIndex);

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              int32_t aModType) const;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);
  void PostHandleEventForRangeThumb(nsEventChainPostVisitor& aVisitor);
  void StartRangeThumbDrag(nsGUIEvent* aEvent);
  void FinishRangeThumbDrag(nsGUIEvent* aEvent = nullptr);
  void CancelRangeThumbDrag(bool aIsForUserEvent = true);
  void SetValueOfRangeForUserEvent(double aValue);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);

  virtual void DoneCreatingElement();

  virtual nsEventStates IntrinsicState() const;

  
  NS_IMETHOD SetValueChanged(bool aValueChanged);
  NS_IMETHOD_(bool) IsSingleLineTextControl() const;
  NS_IMETHOD_(bool) IsTextArea() const;
  NS_IMETHOD_(bool) IsPlainTextControl() const;
  NS_IMETHOD_(bool) IsPasswordTextControl() const;
  NS_IMETHOD_(int32_t) GetCols();
  NS_IMETHOD_(int32_t) GetWrapCols();
  NS_IMETHOD_(int32_t) GetRows();
  NS_IMETHOD_(void) GetDefaultValueFromContent(nsAString& aValue);
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

  void GetDisplayFileName(nsAString& aFileName) const;
  const nsCOMArray<nsIDOMFile>& GetFiles() const;
  void SetFiles(const nsCOMArray<nsIDOMFile>& aFiles, bool aSetValueChanged);
  void SetFiles(nsIDOMFileList* aFiles, bool aSetValueChanged);

  void SetCheckedChangedInternal(bool aCheckedChanged);
  bool GetCheckedChanged() const {
    return mCheckedChanged;
  }
  void AddedToRadioGroup();
  void WillRemoveFromRadioGroup();

 






  already_AddRefed<nsIDOMHTMLInputElement> GetSelectedRadioButton();

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  NS_IMETHOD FireAsyncClickHandler();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsHTMLInputElement,
                                           nsGenericHTMLFormElement)

  static UploadLastDir* gUploadLastDir;
  
  
  static void InitUploadLastDir();
  static void DestroyUploadLastDir();

  void MaybeLoadImage();

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  bool     IsTooLong();
  bool     IsValueMissing() const;
  bool     HasTypeMismatch() const;
  bool     HasPatternMismatch() const;
  bool     IsRangeOverflow() const;
  bool     IsRangeUnderflow() const;
  bool     HasStepMismatch() const;
  void     UpdateTooLongValidityState();
  void     UpdateValueMissingValidityState();
  void     UpdateTypeMismatchValidityState();
  void     UpdatePatternMismatchValidityState();
  void     UpdateRangeOverflowValidityState();
  void     UpdateRangeUnderflowValidityState();
  void     UpdateStepMismatchValidityState();
  void     UpdateAllValidityStates(bool aNotify);
  void     UpdateBarredFromConstraintValidation();
  nsresult GetValidationMessage(nsAString& aValidationMessage,
                                ValidityStateType aType);
  







  void     UpdateValueMissingValidityStateForRadio(bool aIgnoreSelf);

  














  void SetFilePickerFiltersFromAccept(nsIFilePicker* filePicker);

  












  int32_t GetFilterFromAccept();

  








  void UpdateValidityUIBits(bool aIsFocused);

  bool DefaultChecked() const {
    return HasAttr(kNameSpaceID_None, nsGkAtoms::checked);
  }

  bool Indeterminate() const { return mIndeterminate; }
  bool Checked() const { return mChecked; }

  


  void FireChangeEventIfNeeded();

  





  double GetValueAsDouble() const;

  







  double GetMinimum() const;

  







  double GetMaximum() const;

protected:
  
  
  using nsGenericHTMLFormElement::IsSingleLineTextControl;

  




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

  void ClearFiles(bool aSetValueChanged) {
    nsCOMArray<nsIDOMFile> files;
    SetFiles(files, aSetValueChanged);
  }

  nsresult SetIndeterminateInternal(bool aValue,
                                    bool aShouldInvalidate);

  nsresult GetSelectionRange(int32_t* aSelectionStart, int32_t* aSelectionEnd);

  


  virtual nsresult BeforeSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                 const nsAttrValueOrString* aValue,
                                 bool aNotify);
  


  virtual nsresult AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify);

  


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

  



  bool NeedToInitializeEditorForEvent(nsEventChainPreVisitor& aVisitor) const;

  


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

  








  bool ConvertStringToNumber(nsAString& aValue, double& aResultValue) const;

  










  bool ConvertNumberToString(double aValue, nsAString& aResultString) const;

  





  bool IsValidDate(const nsAString& aValue) const;

  





  bool GetValueAsDate(const nsAString& aValue,
                      uint32_t* aYear,
                      uint32_t* aMonth,
                      uint32_t* aDay) const;

  


  uint32_t NumberOfDaysInMonth(uint32_t aMonth, uint32_t aYear) const;

  






  bool IsValidTime(const nsAString& aValue) const;

  










  static bool ParseTime(const nsAString& aValue, uint32_t* aResult);

  




  void SetValue(double aValue);

  


  void UpdateHasRange();

   




  double GetStepScaleFactor() const;

  





  double GetStep() const;

  





  double GetStepBase() const;

  



  double GetDefaultStep() const;

  





  nsresult ApplyStep(int32_t aStep);

  


  static bool IsExperimentalMobileType(uint8_t aType)
  {
    return aType == NS_FORM_INPUT_NUMBER || aType == NS_FORM_INPUT_DATE ||
           aType == NS_FORM_INPUT_TIME;
  }

  




  bool ShouldPreventDOMActivateDispatch(nsIDOMEventTarget* aOriginalTarget);

  nsCOMPtr<nsIControllers> mControllers;

  







  union InputData {
    


    PRUnichar*               mValue;
    


    nsTextEditorState*       mState;
  } mInputData;
  









  nsCOMArray<nsIDOMFile>   mFiles;

  nsRefPtr<nsDOMFileList>  mFileList;

  nsString mStaticDocFileList;
  
  






  nsString mFocusedValue;  

  




  double mRangeThumbDragStartValue;

  
  static const double kStepScaleFactorDate;
  static const double kStepScaleFactorNumberRange;
  static const double kStepScaleFactorTime;

  
  static const double kDefaultStepBase;

  
  static const double kDefaultStep;
  static const double kDefaultStepTime;

  
  static const double kStepAny;

  



  uint8_t                  mType;
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

private:

  



  bool MayFireChangeOnBlur() const {
    return MayFireChangeOnBlur(mType);
  }

  static bool MayFireChangeOnBlur(uint8_t aType) {
    return IsSingleLineTextControl(false, aType) ||
           aType == NS_FORM_INPUT_RANGE;
  }

  struct nsFilePickerFilter {
    nsFilePickerFilter()
      : mFilterMask(0), mIsTrusted(false) {}

    nsFilePickerFilter(int32_t aFilterMask)
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

  class AsyncClickHandler
    : public nsRunnable
  {
  public:
    AsyncClickHandler(nsHTMLInputElement* aInput);
    NS_IMETHOD Run();

  protected:
    nsRefPtr<nsHTMLInputElement> mInput;
    PopupControlState mPopupControlState;
  };

  class nsFilePickerShownCallback
    : public nsIFilePickerShownCallback
  {
  public:
    nsFilePickerShownCallback(nsHTMLInputElement* aInput,
                              nsIFilePicker* aFilePicker,
                              bool aMulti);
    virtual ~nsFilePickerShownCallback()
    { }

    NS_DECL_ISUPPORTS

    NS_IMETHOD Done(int16_t aResult);

  private:
    nsCOMPtr<nsIFilePicker> mFilePicker;
    nsRefPtr<nsHTMLInputElement> mInput;
    bool mMulti;
  };
};

#endif
