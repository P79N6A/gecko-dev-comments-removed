





































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




#define BF_DISABLED_CHANGED 0
#define BF_VALUE_CHANGED 1
#define BF_CHECKED_CHANGED 2
#define BF_CHECKED 3
#define BF_HANDLING_SELECT_EVENT 4
#define BF_SHOULD_INIT_CHECKED 5
#define BF_PARSER_CREATING 6
#define BF_IN_INTERNAL_ACTIVATE 7
#define BF_CHECKED_IS_TOGGLED 8
#define BF_INDETERMINATE 9
#define BF_INHIBIT_RESTORATION 10
#define BF_CAN_SHOW_INVALID_UI 11
#define BF_CAN_SHOW_VALID_UI 12

#define GET_BOOLBIT(bitfield, field) (((bitfield) & (0x01 << (field))) \
                                        ? true : false)
#define SET_BOOLBIT(bitfield, field, b) ((b) \
                                        ? ((bitfield) |=  (0x01 << (field))) \
                                        : ((bitfield) &= ~(0x01 << (field))))

class nsDOMFileList;
class nsIRadioGroupContainer;
class nsIRadioGroupVisitor;
class nsIRadioVisitor;

class UploadLastDir : public nsIObserver, public nsSupportsWeakReference {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  






  nsresult FetchLastUsedDirectory(nsIURI* aURI, nsILocalFile** aFile);

  






  nsresult StoreLastUsedDirectory(nsIURI* aURI, nsILocalFile* aFile);
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

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFormElement::)

  
  NS_DECL_NSIDOMHTMLINPUTELEMENT

  
  NS_DECL_NSIPHONETIC

  
  NS_IMETHOD GetEditor(nsIEditor** aEditor)
  {
    return nsGenericHTMLElement::GetEditor(aEditor);
  }

  
  NS_FORWARD_NSIDOMHTMLELEMENT_NOFOCUSCLICK(nsGenericHTMLFormElement::)
  NS_IMETHOD Focus();
  NS_IMETHOD Click();

  NS_IMETHOD SetUserInput(const nsAString& aInput);

  
  NS_IMETHOD_(PRUint32) GetType() const { return mType; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission);
  NS_IMETHOD SaveState();
  virtual bool RestoreState(nsPresState* aState);
  virtual bool AllowDrop();

  virtual void FieldSetDisabledChanged(bool aNotify);

  
  virtual bool IsHTMLFocusable(bool aWithMouse, bool *aIsFocusable, PRInt32 *aTabIndex);

  virtual bool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              PRInt32 aModType) const;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

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
  NS_IMETHOD_(PRInt32) GetCols();
  NS_IMETHOD_(PRInt32) GetWrapCols();
  NS_IMETHOD_(PRInt32) GetRows();
  NS_IMETHOD_(void) GetDefaultValueFromContent(nsAString& aValue);
  NS_IMETHOD_(bool) ValueChanged() const;
  NS_IMETHOD_(void) GetTextEditorValue(nsAString& aValue, bool aIgnoreWrap) const;
  NS_IMETHOD_(void) SetTextEditorValue(const nsAString& aValue, bool aUserInput);
  NS_IMETHOD_(nsIEditor*) GetTextEditor();
  NS_IMETHOD_(nsISelectionController*) GetSelectionController();
  NS_IMETHOD_(nsFrameSelection*) GetConstFrameSelection();
  NS_IMETHOD BindToFrame(nsTextControlFrame* aFrame);
  NS_IMETHOD_(void) UnbindFromFrame(nsTextControlFrame* aFrame);
  NS_IMETHOD CreateEditor();
  NS_IMETHOD_(nsIContent*) GetRootEditorNode();
  NS_IMETHOD_(nsIContent*) CreatePlaceholderNode();
  NS_IMETHOD_(nsIContent*) GetPlaceholderNode();
  NS_IMETHOD_(void) UpdatePlaceholderText(bool aNotify);
  NS_IMETHOD_(void) SetPlaceholderClass(bool aVisible, bool aNotify);
  NS_IMETHOD_(void) InitializeKeyboardEventListeners();
  NS_IMETHOD_(void) OnValueChanged(bool aNotify);
  NS_IMETHOD_(bool) HasCachedSelection();

  void GetDisplayFileName(nsAString& aFileName) const;
  const nsCOMArray<nsIDOMFile>& GetFiles() const;
  void SetFiles(const nsCOMArray<nsIDOMFile>& aFiles, bool aSetValueChanged);
  void SetFiles(nsIDOMFileList* aFiles, bool aSetValueChanged);

  void SetCheckedChangedInternal(bool aCheckedChanged);
  bool GetCheckedChanged() const {
    return GET_BOOLBIT(mBitField, BF_CHECKED_CHANGED);
  }
  void AddedToRadioGroup();
  void WillRemoveFromRadioGroup();

 






  already_AddRefed<nsIDOMHTMLInputElement> GetSelectedRadioButton();

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  NS_IMETHOD FireAsyncClickHandler();

  virtual void UpdateEditableState(bool aNotify)
  {
    return UpdateEditableFormControlState(aNotify);
  }

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsHTMLInputElement,
                                           nsGenericHTMLFormElement)

  static UploadLastDir* gUploadLastDir;
  
  
  static void InitUploadLastDir();
  static void DestroyUploadLastDir();

  void MaybeLoadImage();

  virtual nsXPCClassInfo* GetClassInfo();

  static nsHTMLInputElement* FromContent(nsIContent *aContent)
  {
    if (aContent->NodeInfo()->Equals(nsGkAtoms::input, kNameSpaceID_XHTML))
      return static_cast<nsHTMLInputElement*>(aContent);
    return NULL;
  }

  
  bool     IsTooLong();
  bool     IsValueMissing() const;
  bool     HasTypeMismatch() const;
  bool     HasPatternMismatch() const;
  void     UpdateTooLongValidityState();
  void     UpdateValueMissingValidityState();
  void     UpdateTypeMismatchValidityState();
  void     UpdatePatternMismatchValidityState();
  void     UpdateAllValidityStates(bool aNotify);
  void     UpdateBarredFromConstraintValidation();
  nsresult GetValidationMessage(nsAString& aValidationMessage,
                                ValidityStateType aType);
  







  void     UpdateValueMissingValidityStateForRadio(bool aIgnoreSelf);

  












  PRInt32 GetFilterFromAccept();

  








  void UpdateValidityUIBits(bool aIsFocused);

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

  nsresult GetSelectionRange(PRInt32* aSelectionStart, PRInt32* aSelectionEnd);

  


  virtual nsresult BeforeSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                 const nsAString* aValue, bool aNotify);
  


  virtual nsresult AfterSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                const nsAString* aValue, bool aNotify);

  


  bool DispatchSelectEvent(nsPresContext* aPresContext);

  void SelectAll(nsPresContext* aPresContext);
  bool IsImage() const
  {
    return AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                       nsGkAtoms::image, eIgnoreCase);
  }

  



  nsresult VisitGroup(nsIRadioVisitor* aVisitor, bool aFlushContent);

  



  nsresult DoSetChecked(bool aValue, bool aNotify, bool aSetValueChanged);

  




  void DoSetCheckedChanged(bool aCheckedChanged, bool aNotify);

  



  void SetCheckedInternal(bool aValue, bool aNotify);

  


  bool GetChecked() const
  {
    return GET_BOOLBIT(mBitField, BF_CHECKED);
  }

  nsresult RadioSetChecked(bool aNotify);
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

  


  bool MaxLengthApplies() const { return IsSingleLineTextControl(false, mType); }

  void FreeData();
  nsTextEditorState *GetEditorState() const;

  


  void HandleTypeChange(PRUint8 aNewType);

  



  void SanitizeValue(nsAString& aValue);

  


  bool PlaceholderApplies() const { return IsSingleLineTextControl(false, mType); }

  




  nsresult SetDefaultValueAsValue();

  



  bool GetValueChanged() const {
    return GET_BOOLBIT(mBitField, BF_VALUE_CHANGED);
  }

  





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
        return GetValueChanged();
      default:
        NS_NOTREACHED("We should not be there: there are no other modes.");
        return false;
    }
  }

  





  nsIRadioGroupContainer* GetRadioGroupContainer() const;

  nsCOMPtr<nsIControllers> mControllers;

  



  PRUint8                  mType;
  



  PRInt16                  mBitField;
  







  union InputData {
    


    char*                    mValue;
    


    nsTextEditorState*       mState;
  } mInputData;
  









  nsCOMArray<nsIDOMFile>   mFiles;

  nsRefPtr<nsDOMFileList>  mFileList;

  nsString mStaticDocFileList;
};

#endif
