





































#ifndef nsHTMLInputElement_h__
#define nsHTMLInputElement_h__

#include "nsGenericHTMLElement.h"
#include "nsImageLoadingContent.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMNSHTMLInputElement.h"
#include "nsITextControlElement.h"
#include "nsIPhonetic.h"
#include "nsIDOMNSEditableElement.h"
#include "nsIFileControlElement.h"

#include "nsTextEditorState.h"
#include "nsCOMPtr.h"




#define BF_DISABLED_CHANGED 0
#define BF_HANDLING_CLICK 1
#define BF_VALUE_CHANGED 2
#define BF_CHECKED_CHANGED 3
#define BF_CHECKED 4
#define BF_HANDLING_SELECT_EVENT 5
#define BF_SHOULD_INIT_CHECKED 6
#define BF_PARSER_CREATING 7
#define BF_IN_INTERNAL_ACTIVATE 8
#define BF_CHECKED_IS_TOGGLED 9
#define BF_INDETERMINATE 10
#define BF_INHIBIT_RESTORATION 11

#define GET_BOOLBIT(bitfield, field) (((bitfield) & (0x01 << (field))) \
                                        ? PR_TRUE : PR_FALSE)
#define SET_BOOLBIT(bitfield, field, b) ((b) \
                                        ? ((bitfield) |=  (0x01 << (field))) \
                                        : ((bitfield) &= ~(0x01 << (field))))

class nsDOMFileList;

class nsHTMLInputElement : public nsGenericHTMLFormElement,
                           public nsImageLoadingContent,
                           public nsIDOMHTMLInputElement,
                           public nsIDOMNSHTMLInputElement,
                           public nsITextControlElement,
                           public nsIPhonetic,
                           public nsIDOMNSEditableElement,
                           public nsIFileControlElement
{
public:
  nsHTMLInputElement(nsINodeInfo *aNodeInfo, PRUint32 aFromParser);
  virtual ~nsHTMLInputElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFormElement::)

  
  NS_DECL_NSIDOMHTMLINPUTELEMENT

  
  NS_DECL_NSIDOMNSHTMLINPUTELEMENT

  
  NS_DECL_NSIPHONETIC

  
  NS_IMETHOD GetEditor(nsIEditor** aEditor)
  {
    return nsGenericHTMLElement::GetEditor(aEditor);
  }
  NS_IMETHOD SetUserInput(const nsAString& aInput);

  
  NS_IMETHOD_(PRUint32) GetType() const { return mType; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission,
                               nsIContent* aSubmitElement);
  NS_IMETHOD SaveState();
  virtual PRBool RestoreState(nsPresState* aState);
  virtual PRBool AllowDrop();

  
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

  virtual PRInt32 IntrinsicState() const;

  
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
  NS_IMETHOD_(nsIContent*) GetPlaceholderNode();
  NS_IMETHOD_(void) UpdatePlaceholderText(PRBool aNotify);
  NS_IMETHOD_(void) SetPlaceholderClass(PRBool aVisible, PRBool aNotify);
  NS_IMETHOD_(void) InitializeKeyboardEventListeners();

  
  virtual void GetDisplayFileName(nsAString& aFileName);
  virtual void GetFileArray(nsCOMArray<nsIFile> &aFile);
  virtual void SetFileNames(const nsTArray<nsString>& aFileNames);

  void SetCheckedChangedInternal(PRBool aCheckedChanged);
  PRBool GetCheckedChanged();
  void AddedToRadioGroup(PRBool aNotify = PR_TRUE);
  void WillRemoveFromRadioGroup();
  



  virtual already_AddRefed<nsIRadioGroupContainer> GetRadioGroupContainer();

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual void UpdateEditableState()
  {
    return UpdateEditableFormControlState();
  }

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsHTMLInputElement,
                                                     nsGenericHTMLFormElement)

  void MaybeLoadImage();
protected:
  
  
  using nsGenericHTMLFormElement::IsSingleLineTextControl;

  
  nsresult SetValueInternal(const nsAString& aValue,
                            PRBool aUserInput);

  void ClearFileNames() {
    nsTArray<nsString> fileNames;
    SetFileNames(fileNames);
  }

  void SetSingleFileName(const nsAString& aFileName) {
    nsAutoTArray<nsString, 1> fileNames;
    fileNames.AppendElement(aFileName);
    SetFileNames(fileNames);
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

  virtual PRBool AcceptAutofocus() const
  {
    return PR_TRUE;
  }

  


  void FireOnChange();

  



  nsresult VisitGroup(nsIRadioVisitor* aVisitor, PRBool aFlushContent);

  



  nsresult DoSetChecked(PRBool aValue, PRBool aNotify = PR_TRUE);

  




  void DoSetCheckedChanged(PRBool aCheckedChanged, PRBool aNotify);

  



  nsresult SetCheckedInternal(PRBool aValue, PRBool aNotify);

  


  PRBool GetChecked() const
  {
    return GET_BOOLBIT(mBitField, BF_CHECKED);
  }

  nsresult RadioSetChecked(PRBool aNotify);
  void SetCheckedChanged(PRBool aCheckedChanged);

  



  nsresult MaybeSubmitForm(nsPresContext* aPresContext);

  


  nsresult UpdateFileList();

  



  PRBool NeedToInitializeEditorForEvent(nsEventChainPreVisitor& aVisitor) const;

  void FreeData();
  nsTextEditorState *GetEditorState() const;

  


  void HandleTypeChange(PRUint8 aNewType);

  nsCOMPtr<nsIControllers> mControllers;

  



  PRUint8                  mType;
  



  PRInt16                  mBitField;
  







  union InputData {
    


    char*                    mValue;
    


    nsTextEditorState*       mState;
  } mInputData;
  









  nsTArray<nsString>       mFileNames;

  nsRefPtr<nsDOMFileList>  mFileList;
};

#endif
