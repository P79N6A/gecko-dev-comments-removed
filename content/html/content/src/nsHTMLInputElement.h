





































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
                                        ? PR_TRUE : PR_FALSE)
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

  UploadLastDir();

  






  nsresult FetchLastUsedDirectory(nsIURI* aURI, nsILocalFile** aFile);

  






  nsresult StoreLastUsedDirectory(nsIURI* aURI, nsILocalFile* aFile);
private:
  
  nsInterfaceHashtable<nsStringHashKey, nsILocalFile> mUploadLastDirStore;
  PRBool mInPrivateBrowsing;
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
  virtual PRBool RestoreState(nsPresState* aState);
  virtual PRBool AllowDrop();

  virtual void FieldSetDisabledChanged(nsEventStates aStates, PRBool aNotify);

  
  virtual PRBool IsHTMLFocusable(PRBool aWithMouse, PRBool *aIsFocusable, PRInt32 *aTabIndex);

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              PRInt32 aModType) const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);

  virtual void DoneCreatingElement();

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

  void GetDisplayFileName(nsAString& aFileName) const;
  const nsCOMArray<nsIDOMFile>& GetFiles() const;
  void SetFiles(const nsCOMArray<nsIDOMFile>& aFiles, bool aSetValueChanged);

  void SetCheckedChangedInternal(PRBool aCheckedChanged);
  PRBool GetCheckedChanged() const {
    return GET_BOOLBIT(mBitField, BF_CHECKED_CHANGED);
  }
  void AddedToRadioGroup();
  void WillRemoveFromRadioGroup();
  



  virtual already_AddRefed<nsIRadioGroupContainer> GetRadioGroupContainer();

 






  already_AddRefed<nsIDOMHTMLInputElement> GetSelectedRadioButton();

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  NS_IMETHOD FireAsyncClickHandler();

  virtual void UpdateEditableState()
  {
    return UpdateEditableFormControlState();
  }

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsHTMLInputElement,
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

  
  PRBool   IsTooLong();
  PRBool   IsValueMissing() const;
  PRBool   HasTypeMismatch() const;
  PRBool   HasPatternMismatch() const;
  void     UpdateTooLongValidityState();
  void     UpdateValueMissingValidityState();
  void     UpdateTypeMismatchValidityState();
  void     UpdatePatternMismatchValidityState();
  void     UpdateAllValidityStates(PRBool aNotify);
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

  







  static PRBool IsValidEmailAddress(const nsAString& aValue);

  









  static PRBool IsValidEmailAddressList(const nsAString& aValue);

  
  nsresult SetValueInternal(const nsAString& aValue,
                            PRBool aUserInput,
                            PRBool aSetValueChanged);

  nsresult GetValueInternal(nsAString& aValue) const;

  




  bool IsValueEmpty() const;

  void ClearFiles(bool aSetValueChanged) {
    nsCOMArray<nsIDOMFile> files;
    SetFiles(files, aSetValueChanged);
  }

  nsresult SetIndeterminateInternal(PRBool aValue,
                                    PRBool aShouldInvalidate);

  nsresult GetSelectionRange(PRInt32* aSelectionStart, PRInt32* aSelectionEnd);

  




  PRBool GetNameIfExists(nsAString& aName) {
    GetAttr(kNameSpaceID_None, nsGkAtoms::name, aName);
    return !aName.IsEmpty();
  }

  


  virtual nsresult BeforeSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                 const nsAString* aValue, PRBool aNotify);
  


  virtual nsresult AfterSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                const nsAString* aValue, PRBool aNotify);

  


  PRBool DispatchSelectEvent(nsPresContext* aPresContext);

  void SelectAll(nsPresContext* aPresContext);
  PRBool IsImage() const
  {
    return AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                       nsGkAtoms::image, eIgnoreCase);
  }

  



  nsresult VisitGroup(nsIRadioVisitor* aVisitor, PRBool aFlushContent);

  



  nsresult DoSetChecked(PRBool aValue, PRBool aNotify, PRBool aSetValueChanged);

  




  void DoSetCheckedChanged(PRBool aCheckedChanged, PRBool aNotify);

  



  void SetCheckedInternal(PRBool aValue, PRBool aNotify);

  


  PRBool GetChecked() const
  {
    return GET_BOOLBIT(mBitField, BF_CHECKED);
  }

  nsresult RadioSetChecked(PRBool aNotify);
  void SetCheckedChanged(PRBool aCheckedChanged);

  



  nsresult MaybeSubmitForm(nsPresContext* aPresContext);

  


  nsresult UpdateFileList();

  



  PRBool NeedToInitializeEditorForEvent(nsEventChainPreVisitor& aVisitor) const;

  


  ValueModeType GetValueMode() const;

  






  PRBool IsMutable() const;

  


  PRBool DoesReadOnlyApply() const;

  


  PRBool DoesRequiredApply() const;

  


  PRBool DoesPatternApply() const;

  


  bool MaxLengthApplies() const { return IsSingleLineTextControl(false, mType); }

  void FreeData();
  nsTextEditorState *GetEditorState() const;

  


  void HandleTypeChange(PRUint8 aNewType);

  



  void SanitizeValue(nsAString& aValue);

  


  bool PlaceholderApplies() const { return IsSingleLineTextControl(false, mType); }

  




  nsresult SetDefaultValueAsValue();

  



  PRBool GetValueChanged() const {
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
