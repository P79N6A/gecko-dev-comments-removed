





































#include "nsIDOMHTMLInputElement.h"
#include "nsITextControlElement.h"
#include "nsIFileControlElement.h"
#include "nsIDOMNSEditableElement.h"
#include "nsIRadioVisitor.h"
#include "nsIPhonetic.h"

#include "nsIControllers.h"
#include "nsFocusManager.h"
#include "nsPIDOMWindow.h"
#include "nsContentCID.h"
#include "nsIComponentManager.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsMappedAttributes.h"
#include "nsIFormControl.h"
#include "nsIForm.h"
#include "nsFormSubmission.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIFormControlFrame.h"
#include "nsITextControlFrame.h"
#include "nsIFrame.h"
#include "nsIEventStateManager.h"
#include "nsIServiceManager.h"
#include "nsIScriptSecurityManager.h"
#include "nsDOMError.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIEditor.h"
#include "nsGUIEvent.h"

#include "nsPresState.h"
#include "nsLayoutErrors.h"
#include "nsIDOMEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMHTMLCollection.h"
#include "nsLinebreakConverter.h" 
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsEventDispatcher.h"
#include "nsLayoutUtils.h"
#include "nsWidgetsCID.h"
#include "nsILookAndFeel.h"

#include "nsIDOMMutationEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsMutationEvent.h"
#include "nsIEventListenerManager.h"

#include "nsRuleData.h"


#include "nsIRadioGroupContainer.h"


#include "nsIFile.h"
#include "nsILocalFile.h"
#include "nsNetUtil.h"
#include "nsDOMFile.h"


#include "nsImageLoadingContent.h"
#include "nsIDOMWindowInternal.h"

#include "mozAutoDocUpdate.h"
#include "nsHTMLFormElement.h"
#include "nsContentCreatorFunctions.h"
#include "nsCharSeparatedTokenizer.h"

#include "nsTextEditRules.h"

#include "nsHTMLInputElement.h"



static NS_DEFINE_CID(kXULControllersCID,  NS_XULCONTROLLERS_CID);
static NS_DEFINE_CID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);


#define NS_OUTER_ACTIVATE_EVENT   (1 << 9)
#define NS_ORIGINAL_CHECKED_VALUE (1 << 10)
#define NS_NO_CONTENT_DISPATCH    (1 << 11)
#define NS_ORIGINAL_INDETERMINATE_VALUE (1 << 12)
#define NS_CONTROL_TYPE(bits)  ((bits) & ~( \
  NS_OUTER_ACTIVATE_EVENT | NS_ORIGINAL_CHECKED_VALUE | NS_NO_CONTENT_DISPATCH | \
  NS_ORIGINAL_INDETERMINATE_VALUE))



static PRInt32 gSelectTextFieldOnFocus;

static const nsAttrValue::EnumTable kInputTypeTable[] = {
  { "button", NS_FORM_INPUT_BUTTON },
  { "checkbox", NS_FORM_INPUT_CHECKBOX },
  { "email", NS_FORM_INPUT_EMAIL },
  { "file", NS_FORM_INPUT_FILE },
  { "hidden", NS_FORM_INPUT_HIDDEN },
  { "reset", NS_FORM_INPUT_RESET },
  { "image", NS_FORM_INPUT_IMAGE },
  { "password", NS_FORM_INPUT_PASSWORD },
  { "radio", NS_FORM_INPUT_RADIO },
  { "search", NS_FORM_INPUT_SEARCH },
  { "submit", NS_FORM_INPUT_SUBMIT },
  { "tel", NS_FORM_INPUT_TEL },
  { "text", NS_FORM_INPUT_TEXT },
  { 0 }
};


static const nsAttrValue::EnumTable* kInputDefaultType = &kInputTypeTable[12];

#define NS_INPUT_ELEMENT_STATE_IID                 \
{ /* dc3b3d14-23e2-4479-b513-7b369343e3a0 */       \
  0xdc3b3d14,                                      \
  0x23e2,                                          \
  0x4479,                                          \
  {0xb5, 0x13, 0x7b, 0x36, 0x93, 0x43, 0xe3, 0xa0} \
}

class nsHTMLInputElementState : public nsISupports
{
  public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_INPUT_ELEMENT_STATE_IID)
    NS_DECL_ISUPPORTS

    PRBool IsCheckedSet() {
      return mCheckedSet;
    }

    PRBool GetChecked() {
      return mChecked;
    }

    void SetChecked(PRBool aChecked) {
      mChecked = aChecked;
      mCheckedSet = PR_TRUE;
    }

    const nsString& GetValue() {
      return mValue;
    }

    void SetValue(const nsAString &aValue) {
      mValue = aValue;
    }

    const nsTArray<nsString>& GetFilenames() {
      return mFilenames;
    }

    void SetFilenames(const nsTArray<nsString> &aFilenames) {
      mFilenames = aFilenames;
    }

    nsHTMLInputElementState()
      : mValue()
      , mChecked(PR_FALSE)
      , mCheckedSet(PR_FALSE)
    {};
 
  protected:
    nsString mValue;
    nsTArray<nsString> mFilenames;
    PRPackedBool mChecked;
    PRPackedBool mCheckedSet;
};

NS_IMPL_ISUPPORTS1(nsHTMLInputElementState, nsHTMLInputElementState)
NS_DEFINE_STATIC_IID_ACCESSOR(nsHTMLInputElementState, NS_INPUT_ELEMENT_STATE_IID)

#ifdef ACCESSIBILITY

static nsresult FireEventForAccessibility(nsIDOMHTMLInputElement* aTarget,
                                          nsPresContext* aPresContext,
                                          const nsAString& aEventType);
#endif





NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(Input)

nsHTMLInputElement::nsHTMLInputElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                                       PRUint32 aFromParser)
  : nsGenericHTMLFormElement(aNodeInfo),
    mType(kInputDefaultType->value),
    mBitField(0)
{
  SET_BOOLBIT(mBitField, BF_PARSER_CREATING, aFromParser);
  SET_BOOLBIT(mBitField, BF_INHIBIT_RESTORATION,
      aFromParser & NS_FROM_PARSER_FRAGMENT);
  mInputData.mState = new nsTextEditorState(this);
  NS_ADDREF(mInputData.mState);
}

nsHTMLInputElement::~nsHTMLInputElement()
{
  DestroyImageLoadingContent();
  FreeData();
}

void
nsHTMLInputElement::FreeData()
{
  if (!IsSingleLineTextControl(PR_FALSE)) {
    nsMemory::Free(mInputData.mValue);
    mInputData.mValue = nsnull;
  } else {
    UnbindFromFrame(nsnull);
    NS_IF_RELEASE(mInputData.mState);
  }
}

nsTextEditorState*
nsHTMLInputElement::GetEditorState() const
{
  if (!IsSingleLineTextControl(PR_FALSE)) {
    return nsnull;
  }

  NS_ASSERTION(mInputData.mState,
    "Single line text controls need to have a state associated with them");

  return mInputData.mState;
}




NS_IMPL_CYCLE_COLLECTION_CLASS(nsHTMLInputElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHTMLInputElement,
                                                  nsGenericHTMLFormElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mControllers)
  if (tmp->IsSingleLineTextControl(PR_FALSE)) {
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NATIVE_MEMBER(mInputData.mState, nsTextEditorState)
  }
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsHTMLInputElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLInputElement, nsGenericElement) 


DOMCI_NODE_DATA(HTMLInputElement, nsHTMLInputElement)


NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsHTMLInputElement)
  NS_HTML_CONTENT_INTERFACE_TABLE8(nsHTMLInputElement,
                                   nsIDOMHTMLInputElement,
                                   nsITextControlElement,
                                   nsIFileControlElement,
                                   nsIPhonetic,
                                   imgIDecoderObserver,
                                   nsIImageLoadingContent,
                                   imgIContainerObserver,
                                   nsIDOMNSEditableElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLInputElement,
                                               nsGenericHTMLFormElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLInputElement)


NS_IMPL_NSCONSTRAINTVALIDATION(nsHTMLInputElement)



nsresult
nsHTMLInputElement::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
{
  *aResult = nsnull;

  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
  nsHTMLInputElement *it = new nsHTMLInputElement(ni.forget(), PR_FALSE);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsINode> kungFuDeathGrip = it;
  nsresult rv = CopyInnerTo(it);
  NS_ENSURE_SUCCESS(rv, rv);

  switch (mType) {
    case NS_FORM_INPUT_EMAIL:
    case NS_FORM_INPUT_SEARCH:
    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_PASSWORD:
    case NS_FORM_INPUT_TEL:
      if (GET_BOOLBIT(mBitField, BF_VALUE_CHANGED)) {
        
        
        
        nsAutoString value;
        const_cast<nsHTMLInputElement*>(this)->GetValue(value);
        
        it->SetValueInternal(value, PR_FALSE, PR_TRUE);
      }
      break;
    case NS_FORM_INPUT_FILE:
      it->mFileNames = mFileNames;
      break;
    case NS_FORM_INPUT_RADIO:
    case NS_FORM_INPUT_CHECKBOX:
      if (GET_BOOLBIT(mBitField, BF_CHECKED_CHANGED)) {
        
        
        it->DoSetChecked(GetChecked(), PR_FALSE, PR_TRUE);
      }
      break;
    case NS_FORM_INPUT_IMAGE:
      if (it->GetOwnerDoc()->IsStaticDocument()) {
        CreateStaticImageClone(it);
      }
      break;
    default:
      break;
  }

  kungFuDeathGrip.swap(*aResult);

  return NS_OK;
}

nsresult
nsHTMLInputElement::BeforeSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                  const nsAString* aValue,
                                  PRBool aNotify)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    
    
    
    
    
    if ((aName == nsGkAtoms::name ||
         (aName == nsGkAtoms::type && !mForm)) &&
        mType == NS_FORM_INPUT_RADIO &&
        (mForm || !(GET_BOOLBIT(mBitField, BF_PARSER_CREATING)))) {
      WillRemoveFromRadioGroup();
    } else if (aNotify && aName == nsGkAtoms::src &&
               mType == NS_FORM_INPUT_IMAGE) {
      if (aValue) {
        LoadImage(*aValue, PR_TRUE, aNotify);
      } else {
        
        CancelImageRequests(aNotify);
      }
    } else if (aNotify && aName == nsGkAtoms::disabled) {
      SET_BOOLBIT(mBitField, BF_DISABLED_CHANGED, PR_TRUE);
    }
  }

  return nsGenericHTMLFormElement::BeforeSetAttr(aNameSpaceID, aName,
                                                 aValue, aNotify);
}

nsresult
nsHTMLInputElement::AfterSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                 const nsAString* aValue,
                                 PRBool aNotify)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    
    
    
    
    
    if ((aName == nsGkAtoms::name ||
         (aName == nsGkAtoms::type && !mForm)) &&
        mType == NS_FORM_INPUT_RADIO &&
        (mForm || !(GET_BOOLBIT(mBitField, BF_PARSER_CREATING)))) {
      AddedToRadioGroup();
    }

    
    
    
    if (aName == nsGkAtoms::value &&
        !GET_BOOLBIT(mBitField, BF_VALUE_CHANGED)) {
      SetDefaultValueAsValue();
    }

    
    
    
    if (aName == nsGkAtoms::checked &&
        !GET_BOOLBIT(mBitField, BF_CHECKED_CHANGED)) {
      
      
      if (GET_BOOLBIT(mBitField, BF_PARSER_CREATING)) {
        SET_BOOLBIT(mBitField, BF_SHOULD_INIT_CHECKED, PR_TRUE);
      } else {
        PRBool defaultChecked;
        GetDefaultChecked(&defaultChecked);
        DoSetChecked(defaultChecked, PR_TRUE, PR_TRUE);
        SetCheckedChanged(PR_FALSE);
      }
    }

    if (aName == nsGkAtoms::type) {
      
      
      nsIDocument* document = GetCurrentDoc();
      MOZ_AUTO_DOC_UPDATE(document, UPDATE_CONTENT_STATE, aNotify);

      UpdateEditableState();

      if (!aValue) {
        
        
        
        HandleTypeChange(kInputDefaultType->value);
      }
    
      
      
      if (mInputData.mValue &&
          mType != NS_FORM_INPUT_EMAIL &&
          mType != NS_FORM_INPUT_TEXT &&
          mType != NS_FORM_INPUT_SEARCH &&
          mType != NS_FORM_INPUT_PASSWORD &&
          mType != NS_FORM_INPUT_TEL &&
          mType != NS_FORM_INPUT_FILE) {
        SetAttr(kNameSpaceID_None, nsGkAtoms::value,
                NS_ConvertUTF8toUTF16(mInputData.mValue), PR_FALSE);
        FreeData();
      }

      if (mType != NS_FORM_INPUT_IMAGE) {
        
        
        
        CancelImageRequests(aNotify);
      } else if (aNotify) {
        
        
        nsAutoString src;
        if (GetAttr(kNameSpaceID_None, nsGkAtoms::src, src)) {
          LoadImage(src, PR_FALSE, aNotify);
        }
      }

      if (aNotify && document) {
        
        
        
        
        
        
        
        document->ContentStatesChanged(this, nsnull,
                                       NS_EVENT_STATE_CHECKED |
                                       NS_EVENT_STATE_DEFAULT |
                                       NS_EVENT_STATE_BROKEN |
                                       NS_EVENT_STATE_USERDISABLED |
                                       NS_EVENT_STATE_SUPPRESSED |
                                       NS_EVENT_STATE_LOADING |
                                       NS_EVENT_STATE_INDETERMINATE |
                                       NS_EVENT_STATE_MOZ_READONLY |
                                       NS_EVENT_STATE_MOZ_READWRITE);
      }
    }

    
    
    if (aNotify && aName == nsGkAtoms::readonly &&
        IsSingleLineTextControl(PR_FALSE)) {
      UpdateEditableState();

      nsIDocument* document = GetCurrentDoc();
      if (document) {
        mozAutoDocUpdate upd(document, UPDATE_CONTENT_STATE, PR_TRUE);
        document->ContentStatesChanged(this, nsnull,
                                       NS_EVENT_STATE_MOZ_READONLY |
                                       NS_EVENT_STATE_MOZ_READWRITE);
      }
    }
  }

  return nsGenericHTMLFormElement::AfterSetAttr(aNameSpaceID, aName,
                                                aValue, aNotify);
}



NS_IMETHODIMP
nsHTMLInputElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElement::GetForm(aForm);
}

NS_IMPL_STRING_ATTR(nsHTMLInputElement, DefaultValue, value)
NS_IMPL_BOOL_ATTR(nsHTMLInputElement, DefaultChecked, checked)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Accept, accept)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, AccessKey, accesskey)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Align, align)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Alt, alt)
NS_IMPL_BOOL_ATTR(nsHTMLInputElement, Autofocus, autofocus)

NS_IMPL_BOOL_ATTR(nsHTMLInputElement, Disabled, disabled)
NS_IMPL_BOOL_ATTR(nsHTMLInputElement, Multiple, multiple)
NS_IMPL_NON_NEGATIVE_INT_ATTR(nsHTMLInputElement, MaxLength, maxlength)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Name, name)
NS_IMPL_BOOL_ATTR(nsHTMLInputElement, ReadOnly, readonly)
NS_IMPL_BOOL_ATTR(nsHTMLInputElement, Required, required)
NS_IMPL_URI_ATTR(nsHTMLInputElement, Src, src)
NS_IMPL_INT_ATTR_DEFAULT_VALUE(nsHTMLInputElement, TabIndex, tabindex, 0)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, UseMap, usemap)


NS_IMPL_STRING_ATTR(nsHTMLInputElement, Placeholder, placeholder)
NS_IMPL_ENUM_ATTR_DEFAULT_VALUE(nsHTMLInputElement, Type, type,
                                kInputDefaultType->tag)

NS_IMETHODIMP
nsHTMLInputElement::GetIndeterminate(PRBool* aValue)
{
  *aValue = GET_BOOLBIT(mBitField, BF_INDETERMINATE);
  return NS_OK;
}

nsresult
nsHTMLInputElement::SetIndeterminateInternal(PRBool aValue,
                                             PRBool aShouldInvalidate)
{
  SET_BOOLBIT(mBitField, BF_INDETERMINATE, aValue);

  if (aShouldInvalidate) {
    
    nsIFrame* frame = GetPrimaryFrame();
    if (frame)
      frame->InvalidateOverflowRect();
  }

  
  nsIDocument* document = GetCurrentDoc();
  if (document) {
    mozAutoDocUpdate upd(document, UPDATE_CONTENT_STATE, PR_TRUE);
    document->ContentStatesChanged(this, nsnull, NS_EVENT_STATE_INDETERMINATE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::SetIndeterminate(PRBool aValue)
{
  return SetIndeterminateInternal(aValue, PR_TRUE);
}

NS_IMETHODIMP
nsHTMLInputElement::GetSize(PRUint32* aValue)
{
  const nsAttrValue* attrVal = mAttrsAndChildren.GetAttr(nsGkAtoms::size);
  if (attrVal && attrVal->Type() == nsAttrValue::eInteger) {
    *aValue = attrVal->GetIntegerValue();
  }
  else {
    *aValue = 0;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::SetSize(PRUint32 aValue)
{
  nsAutoString val;
  val.AppendInt(aValue);

  return SetAttr(kNameSpaceID_None, nsGkAtoms::size, val, PR_TRUE);
}

NS_IMETHODIMP 
nsHTMLInputElement::GetValue(nsAString& aValue)
{
  nsTextEditorState* state = GetEditorState();
  if (state) {
    state->GetValue(aValue, PR_TRUE);
    return NS_OK;
  }

  if (mType == NS_FORM_INPUT_FILE) {
    if (nsContentUtils::IsCallerTrustedForCapability("UniversalFileRead")) {
      if (!mFileNames.IsEmpty()) {
        aValue = mFileNames[0];
      }
      else {
        aValue.Truncate();
      }
    } else {
      
      nsCOMArray<nsIFile> files;
      GetFileArray(files);
      if (files.Count() == 0 || NS_FAILED(files[0]->GetLeafName(aValue))) {
        aValue.Truncate();
      }
    }
    
    return NS_OK;
  }

  
  if (!GetAttr(kNameSpaceID_None, nsGkAtoms::value, aValue) &&
      (mType == NS_FORM_INPUT_RADIO || mType == NS_FORM_INPUT_CHECKBOX)) {
    
    aValue.AssignLiteral("on");
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLInputElement::SetValue(const nsAString& aValue)
{
  
  
  if (mType == NS_FORM_INPUT_FILE) {
    if (!aValue.IsEmpty()) {
      if (!nsContentUtils::IsCallerTrustedForCapability("UniversalFileRead")) {
        
        
        return NS_ERROR_DOM_SECURITY_ERR;
      }
      SetSingleFileName(aValue);
    }
    else {
      ClearFileNames();
    }
  }
  else {
    SetValueInternal(aValue, PR_FALSE, PR_TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLInputElement::MozGetFileNameArray(PRUint32 *aLength, PRUnichar ***aFileNames)
{
  if (!nsContentUtils::IsCallerTrustedForCapability("UniversalFileRead")) {
    
    
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  *aLength = mFileNames.Length();
  PRUnichar **ret =
    static_cast<PRUnichar **>(NS_Alloc(mFileNames.Length() * sizeof(PRUnichar*)));
  
  for (PRUint32 i = 0; i <  mFileNames.Length(); i++) {
    ret[i] = NS_strdup(mFileNames[i].get());
  }

  *aFileNames = ret;

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLInputElement::MozSetFileNameArray(const PRUnichar **aFileNames, PRUint32 aLength)
{
  if (!nsContentUtils::IsCallerTrustedForCapability("UniversalFileRead")) {
    
    
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsTArray<nsString> fileNames(aLength);
  for (PRUint32 i = 0; i < aLength; ++i) {
    fileNames.AppendElement(aFileNames[i]);
  }

  SetFileNames(fileNames);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::MozIsTextField(PRBool aExcludePassword, PRBool* aResult)
{
  *aResult = IsSingleLineTextControl(aExcludePassword);

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLInputElement::SetUserInput(const nsAString& aValue)
{
  if (!nsContentUtils::IsCallerTrustedForWrite()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  if (mType == NS_FORM_INPUT_FILE)
  {
    SetSingleFileName(aValue);
  } else {
    SetValueInternal(aValue, PR_TRUE, PR_TRUE);
  }
  return NS_OK;
}

NS_IMETHODIMP_(nsIEditor*)
nsHTMLInputElement::GetTextEditor()
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    return state->GetEditor();
  }
  return nsnull;
}

NS_IMETHODIMP_(nsISelectionController*)
nsHTMLInputElement::GetSelectionController()
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    return state->GetSelectionController();
  }
  return nsnull;
}

nsFrameSelection*
nsHTMLInputElement::GetConstFrameSelection()
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    return state->GetConstFrameSelection();
  }
  return nsnull;
}

NS_IMETHODIMP
nsHTMLInputElement::BindToFrame(nsTextControlFrame* aFrame)
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    return state->BindToFrame(aFrame);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP_(void)
nsHTMLInputElement::UnbindFromFrame(nsTextControlFrame* aFrame)
{
  nsTextEditorState *state = GetEditorState();
  if (state && aFrame) {
    state->UnbindFromFrame(aFrame);
  }
}

NS_IMETHODIMP
nsHTMLInputElement::CreateEditor()
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    return state->PrepareEditor();
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP_(nsIContent*)
nsHTMLInputElement::GetRootEditorNode()
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    return state->GetRootNode();
  }
  return nsnull;
}

NS_IMETHODIMP_(nsIContent*)
nsHTMLInputElement::GetPlaceholderNode()
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    return state->GetPlaceholderNode();
  }
  return nsnull;
}

NS_IMETHODIMP_(void)
nsHTMLInputElement::UpdatePlaceholderText(PRBool aNotify)
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    state->UpdatePlaceholderText(aNotify);
  }
}

NS_IMETHODIMP_(void)
nsHTMLInputElement::SetPlaceholderClass(PRBool aVisible, PRBool aNotify)
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    state->SetPlaceholderClass(aVisible, aNotify);
  }
}

void
nsHTMLInputElement::GetDisplayFileName(nsAString& aValue)
{
  aValue.Truncate();
  for (PRUint32 i = 0; i < mFileNames.Length(); ++i) {
    if (i == 0) {
      aValue.Append(mFileNames[i]);
    }
    else {
      aValue.Append(NS_LITERAL_STRING(", ") + mFileNames[i]);
    }
  }
}

void
nsHTMLInputElement::SetFileNames(const nsTArray<nsString>& aFileNames)
{
  mFileNames = aFileNames;
#if DEBUG
  for (PRUint32 i = 0; i < (PRUint32)aFileNames.Length(); ++i) {
    NS_ASSERTION(!aFileNames[i].IsEmpty(), "Empty file name");
  }
#endif
  
  
  
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_FALSE);
  if (formControlFrame) {
    nsAutoString readableValue;
    GetDisplayFileName(readableValue);
    formControlFrame->SetFormProperty(nsGkAtoms::value, readableValue);
  }

  UpdateFileList();
  
  SetValueChanged(PR_TRUE);
}

void
nsHTMLInputElement::GetFileArray(nsCOMArray<nsIFile> &aFiles)
{
  aFiles.Clear();

  if (mType != NS_FORM_INPUT_FILE) {
    return;
  }

  for (PRUint32 i = 0; i < mFileNames.Length(); ++i) {
    nsCOMPtr<nsIFile> file;
    if (StringBeginsWith(mFileNames[i], NS_LITERAL_STRING("file:"),
                         nsCaseInsensitiveStringComparator())) {
      
      
      NS_GetFileFromURLSpec(NS_ConvertUTF16toUTF8(mFileNames[i]),
                            getter_AddRefs(file));
    }

    if (!file) {
      
      nsCOMPtr<nsILocalFile> localFile;
      NS_NewLocalFile(mFileNames[i], PR_FALSE, getter_AddRefs(localFile));
      
      file = dont_AddRef(static_cast<nsIFile*>(localFile.forget().get()));
    }

    if (file) {
      aFiles.AppendObject(file);
    }
  }
}

nsresult
nsHTMLInputElement::UpdateFileList()
{
  if (mFileList) {
    mFileList->Clear();

    nsIDocument* doc = GetOwnerDoc();

    nsCOMArray<nsIFile> files;
    GetFileArray(files);
    for (PRUint32 i = 0; i < (PRUint32)files.Count(); ++i) {
      nsRefPtr<nsDOMFile> domFile = new nsDOMFile(files[i], doc);
      if (domFile) {
        if (!mFileList->Append(domFile)) {
          return NS_ERROR_FAILURE;
        }
      }
    }
  }

  return NS_OK;
}

nsresult
nsHTMLInputElement::SetValueInternal(const nsAString& aValue,
                                     PRBool aUserInput,
                                     PRBool aSetValueChanged)
{
  NS_PRECONDITION(mType != NS_FORM_INPUT_FILE,
                  "Don't call SetValueInternal for file inputs");

  if (mType == NS_FORM_INPUT_FILE) {
    return NS_ERROR_UNEXPECTED;
  }

  if (IsSingleLineTextControl(PR_FALSE)) {
    
    
    
    nsAutoString value(aValue);
    SanitizeValue(value);

    if (aSetValueChanged) {
      SetValueChanged(PR_TRUE);
    }
    mInputData.mState->SetValue(value, aUserInput);

    return NS_OK;
  }

  
  
  
  
  
  if (mType == NS_FORM_INPUT_HIDDEN) {
    SetValueChanged(PR_TRUE);
  }

  
  return nsGenericHTMLFormElement::SetAttr(kNameSpaceID_None,
                                           nsGkAtoms::value, aValue,
                                           PR_TRUE);
}

NS_IMETHODIMP
nsHTMLInputElement::SetValueChanged(PRBool aValueChanged)
{
  SET_BOOLBIT(mBitField, BF_VALUE_CHANGED, aValueChanged);
  if (!aValueChanged) {
    if (!IsSingleLineTextControl(PR_FALSE)) {
      FreeData();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLInputElement::GetChecked(PRBool* aChecked)
{
  *aChecked = GetChecked();
  return NS_OK;
}

void
nsHTMLInputElement::SetCheckedChanged(PRBool aCheckedChanged)
{
  DoSetCheckedChanged(aCheckedChanged, PR_TRUE);
}

void
nsHTMLInputElement::DoSetCheckedChanged(PRBool aCheckedChanged,
                                        PRBool aNotify)
{
  if (mType == NS_FORM_INPUT_RADIO) {
    if (GET_BOOLBIT(mBitField, BF_CHECKED_CHANGED) != aCheckedChanged) {
      nsCOMPtr<nsIRadioVisitor> visitor;
      NS_GetRadioSetCheckedChangedVisitor(aCheckedChanged,
                                          getter_AddRefs(visitor));
      VisitGroup(visitor, aNotify);
    }
  } else {
    SetCheckedChangedInternal(aCheckedChanged);
  }
}

void
nsHTMLInputElement::SetCheckedChangedInternal(PRBool aCheckedChanged)
{
  SET_BOOLBIT(mBitField, BF_CHECKED_CHANGED, aCheckedChanged);
}


PRBool
nsHTMLInputElement::GetCheckedChanged()
{
  return GET_BOOLBIT(mBitField, BF_CHECKED_CHANGED);
}

NS_IMETHODIMP
nsHTMLInputElement::SetChecked(PRBool aChecked)
{
  return DoSetChecked(aChecked, PR_TRUE, PR_TRUE);
}

nsresult
nsHTMLInputElement::DoSetChecked(PRBool aChecked, PRBool aNotify,
                                 PRBool aSetValueChanged)
{
  nsresult rv = NS_OK;

  
  
  
  if (aSetValueChanged) {
    DoSetCheckedChanged(PR_TRUE, aNotify);
  }

  
  
  
  
  
  if (GetChecked() == aChecked) {
    return NS_OK;
  }

  
  
  
  if (mType == NS_FORM_INPUT_RADIO) {
    
    
    
    if (aChecked) {
      rv = RadioSetChecked(aNotify);
    } else {
      rv = SetCheckedInternal(PR_FALSE, aNotify);
      nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
      if (container) {
        nsAutoString name;
        if (GetNameIfExists(name)) {
          container->SetCurrentRadioButton(name, nsnull);
        }
      }
    }
  } else {
    rv = SetCheckedInternal(aChecked, aNotify);
  }

  return rv;
}

nsresult
nsHTMLInputElement::RadioSetChecked(PRBool aNotify)
{
  nsresult rv = NS_OK;

  
  
  
  nsCOMPtr<nsIDOMHTMLInputElement> currentlySelected = GetSelectedRadioButton();

  
  
  
  if (currentlySelected) {
    
    
    rv = static_cast<nsHTMLInputElement*>
                    (static_cast<nsIDOMHTMLInputElement*>(currentlySelected))->SetCheckedInternal(PR_FALSE, PR_TRUE);
  }

  
  
  
  if (NS_SUCCEEDED(rv)) {
    rv = SetCheckedInternal(PR_TRUE, aNotify);
  }

  
  
  
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
  nsAutoString name;
  if (container && GetNameIfExists(name)) {
    rv = container->SetCurrentRadioButton(name, this);
  }

  return rv;
}

 already_AddRefed<nsIRadioGroupContainer>
nsHTMLInputElement::GetRadioGroupContainer()
{
  nsIRadioGroupContainer* retval = nsnull;
  if (mForm) {
    CallQueryInterface(mForm, &retval);
  } else {
    nsIDocument* currentDoc = GetCurrentDoc();
    if (currentDoc) {
      CallQueryInterface(currentDoc, &retval);
    }
  }
  return retval;
}

already_AddRefed<nsIDOMHTMLInputElement>
nsHTMLInputElement::GetSelectedRadioButton()
{
  nsIDOMHTMLInputElement* selected;
  nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();

  if (!container) {
    return nsnull;
  }

  nsAutoString name;
  if (!GetNameIfExists(name)) {
    return nsnull;
  }

  container->GetCurrentRadioButton(name, &selected);
  return selected;
}

nsresult
nsHTMLInputElement::MaybeSubmitForm(nsPresContext* aPresContext)
{
  if (!mForm) {
    
    return NS_OK;
  }
  
  nsCOMPtr<nsIPresShell> shell = aPresContext->GetPresShell();
  if (!shell) {
    return NS_OK;
  }

  
  nsIFormControl* submitControl = mForm->GetDefaultSubmitElement();
  if (submitControl) {
    nsCOMPtr<nsIContent> submitContent(do_QueryInterface(submitControl));
    NS_ASSERTION(submitContent, "Form control not implementing nsIContent?!");
    
    
    nsMouseEvent event(PR_TRUE, NS_MOUSE_CLICK, nsnull, nsMouseEvent::eReal);
    nsEventStatus status = nsEventStatus_eIgnore;
    shell->HandleDOMEventWithTarget(submitContent, &event, &status);
  } else if (mForm->HasSingleTextControl()) {
    
    
    nsRefPtr<nsHTMLFormElement> form(mForm);
    nsFormEvent event(PR_TRUE, NS_FORM_SUBMIT);
    nsEventStatus status  = nsEventStatus_eIgnore;
    shell->HandleDOMEventWithTarget(mForm, &event, &status);
  }

  return NS_OK;
}

nsresult
nsHTMLInputElement::SetCheckedInternal(PRBool aChecked, PRBool aNotify)
{
  
  
  
  SET_BOOLBIT(mBitField, BF_CHECKED, aChecked);

  
  
  
  if (mType == NS_FORM_INPUT_CHECKBOX || mType == NS_FORM_INPUT_RADIO) {
    nsIFrame* frame = GetPrimaryFrame();
    if (frame) {
      frame->InvalidateOverflowRect();
    }
  }

  
  
  if (aNotify) {
    nsIDocument* document = GetCurrentDoc();
    if (document) {
      mozAutoDocUpdate upd(document, UPDATE_CONTENT_STATE, aNotify);
      document->ContentStatesChanged(this, nsnull, NS_EVENT_STATE_CHECKED);
    }
  }

  return NS_OK;
}


void
nsHTMLInputElement::FireOnChange()
{
  
  
  
  nsEventStatus status = nsEventStatus_eIgnore;
  nsEvent event(PR_TRUE, NS_FORM_CHANGE);
  nsRefPtr<nsPresContext> presContext = GetPresContext();
  nsEventDispatcher::Dispatch(static_cast<nsIContent*>(this), presContext,
                              &event, nsnull, &status);
}

NS_IMETHODIMP
nsHTMLInputElement::Blur()
{
  return nsGenericHTMLElement::Blur();
}

NS_IMETHODIMP
nsHTMLInputElement::Focus()
{
  if (mType == NS_FORM_INPUT_FILE) {
    
    nsIFrame* frame = GetPrimaryFrame();
    if (frame) {
      nsIFrame* childFrame = frame->GetFirstChild(nsnull);
      while (childFrame) {
        
        nsCOMPtr<nsIFormControl> formCtrl =
          do_QueryInterface(childFrame->GetContent());
        if (formCtrl && formCtrl->GetType() == NS_FORM_INPUT_BUTTON) {
          nsCOMPtr<nsIDOMElement> element(do_QueryInterface(formCtrl));
          nsIFocusManager* fm = nsFocusManager::GetFocusManager();
          if (fm && element)
            fm->SetFocus(element, 0);
          break;
        }

        childFrame = childFrame->GetNextSibling();
      }
    }

    return NS_OK;
  }

  return nsGenericHTMLElement::Focus();
}

NS_IMETHODIMP
nsHTMLInputElement::Select()
{
  if (!IsSingleLineTextControl(PR_FALSE)) {
    return NS_OK;
  }

  
  

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

  if (DispatchSelectEvent(presContext) && fm) {
    fm->SetFocus(this, nsIFocusManager::FLAG_NOSCROLL);

    
    nsCOMPtr<nsIDOMElement> focusedElement;
    fm->GetFocusedElement(getter_AddRefs(focusedElement));
    if (SameCOMIdentity(static_cast<nsIDOMNode *>(this), focusedElement)) {
      
      SelectAll(presContext);
    }
  }

  return NS_OK;
}

PRBool
nsHTMLInputElement::DispatchSelectEvent(nsPresContext* aPresContext)
{
  nsEventStatus status = nsEventStatus_eIgnore;

  
  if (!GET_BOOLBIT(mBitField, BF_HANDLING_SELECT_EVENT)) {
    nsEvent event(nsContentUtils::IsCallerChrome(), NS_FORM_SELECTED);

    SET_BOOLBIT(mBitField, BF_HANDLING_SELECT_EVENT, PR_TRUE);
    nsEventDispatcher::Dispatch(static_cast<nsIContent*>(this),
                                aPresContext, &event, nsnull, &status);
    SET_BOOLBIT(mBitField, BF_HANDLING_SELECT_EVENT, PR_FALSE);
  }

  
  
  return (status == nsEventStatus_eIgnore);
}
    
void
nsHTMLInputElement::SelectAll(nsPresContext* aPresContext)
{
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    formControlFrame->SetFormProperty(nsGkAtoms::select, EmptyString());
  }
}

NS_IMETHODIMP
nsHTMLInputElement::Click()
{
  nsresult rv = NS_OK;

  if (GET_BOOLBIT(mBitField, BF_HANDLING_CLICK)) 
      return rv;                      

  
  nsAutoString disabled;
  if (HasAttr(kNameSpaceID_None, nsGkAtoms::disabled)) {
    return NS_OK;
  }

  
  
  if (mType == NS_FORM_INPUT_BUTTON   ||
      mType == NS_FORM_INPUT_CHECKBOX ||
      mType == NS_FORM_INPUT_RADIO    ||
      mType == NS_FORM_INPUT_RESET    ||
      mType == NS_FORM_INPUT_SUBMIT   ||
      mType == NS_FORM_INPUT_IMAGE) {

    
    nsCOMPtr<nsIDocument> doc = GetCurrentDoc();
    if (!doc) {
      return rv;
    }

    nsCOMPtr<nsIPresShell> shell = doc->GetShell();
    nsRefPtr<nsPresContext> context = nsnull;
    if (shell) {
      context = shell->GetPresContext();
    }

    if (!context) {
      doc->FlushPendingNotifications(Flush_Frames);
      shell = doc->GetShell();
      if (shell) {
        context = shell->GetPresContext();
      }
    }

    if (context) {
      
      
      
      nsMouseEvent event(nsContentUtils::IsCallerChrome(),
                         NS_MOUSE_CLICK, nsnull, nsMouseEvent::eReal);
      event.inputSource = nsIDOMNSMouseEvent::MOZ_SOURCE_UNKNOWN;
      nsEventStatus status = nsEventStatus_eIgnore;

      SET_BOOLBIT(mBitField, BF_HANDLING_CLICK, PR_TRUE);

      nsEventDispatcher::Dispatch(static_cast<nsIContent*>(this), context,
                                  &event, nsnull, &status);

      SET_BOOLBIT(mBitField, BF_HANDLING_CLICK, PR_FALSE);
    }
  }

  return NS_OK;
}

PRBool
nsHTMLInputElement::NeedToInitializeEditorForEvent(nsEventChainPreVisitor& aVisitor) const
{
  
  
  
  
  
  if (IsSingleLineTextControl(PR_FALSE) &&
      aVisitor.mEvent->eventStructType != NS_MUTATION_EVENT) {

    switch (aVisitor.mEvent->message) {
    case NS_MOUSE_MOVE:
    case NS_MOUSE_ENTER:
    case NS_MOUSE_EXIT:
    case NS_MOUSE_ENTER_SYNTH:
    case NS_MOUSE_EXIT_SYNTH:
      return PR_FALSE;
      break;
    }
    return PR_TRUE;
  }
  return PR_FALSE;
}

nsresult
nsHTMLInputElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  
  aVisitor.mCanHandle = PR_FALSE;
  PRBool disabled;
  nsresult rv = GetDisabled(&disabled);
  NS_ENSURE_SUCCESS(rv, rv);
  if (disabled) {
    return NS_OK;
  }
  
  
  
  {
    nsIFrame* frame = GetPrimaryFrame();
    if (frame) {
      const nsStyleUserInterface* uiStyle = frame->GetStyleUserInterface();

      if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE ||
          uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED) {
        return NS_OK;
      }
    }
  }

  
  if (NeedToInitializeEditorForEvent(aVisitor)) {
    nsITextControlFrame* textControlFrame = do_QueryFrame(GetPrimaryFrame());
    if (textControlFrame)
      textControlFrame->EnsureEditorInitialized();
  }

  
  if (!aVisitor.mPresContext) {
    return nsGenericHTMLElement::PreHandleEvent(aVisitor);
  }
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  PRBool outerActivateEvent =
    (NS_IS_MOUSE_LEFT_CLICK(aVisitor.mEvent) ||
     (aVisitor.mEvent->message == NS_UI_ACTIVATE &&
      !GET_BOOLBIT(mBitField, BF_IN_INTERNAL_ACTIVATE)));

  if (outerActivateEvent) {
    aVisitor.mItemFlags |= NS_OUTER_ACTIVATE_EVENT;
  }

  PRBool originalCheckedValue = PR_FALSE;

  if (outerActivateEvent) {
    SET_BOOLBIT(mBitField, BF_CHECKED_IS_TOGGLED, PR_FALSE);

    switch(mType) {
      case NS_FORM_INPUT_CHECKBOX:
        {
          if (GET_BOOLBIT(mBitField, BF_INDETERMINATE)) {
            
            SetIndeterminateInternal(PR_FALSE, PR_FALSE);
            aVisitor.mItemFlags |= NS_ORIGINAL_INDETERMINATE_VALUE;
          }

          GetChecked(&originalCheckedValue);
          DoSetChecked(!originalCheckedValue, PR_TRUE, PR_TRUE);
          SET_BOOLBIT(mBitField, BF_CHECKED_IS_TOGGLED, PR_TRUE);
        }
        break;

      case NS_FORM_INPUT_RADIO:
        {
          nsCOMPtr<nsIDOMHTMLInputElement> selectedRadioButton = GetSelectedRadioButton();
          aVisitor.mItemData = selectedRadioButton;

          originalCheckedValue = GetChecked();
          if (!originalCheckedValue) {
            DoSetChecked(PR_TRUE, PR_TRUE, PR_TRUE);
            SET_BOOLBIT(mBitField, BF_CHECKED_IS_TOGGLED, PR_TRUE);
          }
        }
        break;

      case NS_FORM_INPUT_SUBMIT:
      case NS_FORM_INPUT_IMAGE:
        if(mForm) {
          
          
          
          mForm->OnSubmitClickBegin();
        }
        break;

      default:
        break;
    } 
  }

  if (originalCheckedValue) {
    aVisitor.mItemFlags |= NS_ORIGINAL_CHECKED_VALUE;
  }

  
  
  
  if (aVisitor.mEvent->flags & NS_EVENT_FLAG_NO_CONTENT_DISPATCH) {
    aVisitor.mItemFlags |= NS_NO_CONTENT_DISPATCH;
  }
  if (IsSingleLineTextControl(PR_FALSE) &&
      aVisitor.mEvent->message == NS_MOUSE_CLICK &&
      aVisitor.mEvent->eventStructType == NS_MOUSE_EVENT &&
      static_cast<nsMouseEvent*>(aVisitor.mEvent)->button ==
        nsMouseEvent::eMiddleButton) {
    aVisitor.mEvent->flags &= ~NS_EVENT_FLAG_NO_CONTENT_DISPATCH;
  }

  
  aVisitor.mItemFlags |= mType;

  
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

static PRBool
SelectTextFieldOnFocus()
{
  if (!gSelectTextFieldOnFocus) {
    nsCOMPtr<nsILookAndFeel> lookNFeel(do_GetService(kLookAndFeelCID));
    if (lookNFeel) {
      PRInt32 selectTextfieldsOnKeyFocus = -1;
      lookNFeel->GetMetric(nsILookAndFeel::eMetric_SelectTextfieldsOnKeyFocus,
                           selectTextfieldsOnKeyFocus);
      gSelectTextFieldOnFocus = selectTextfieldsOnKeyFocus != 0 ? 1 : -1;
    }
    else {
      gSelectTextFieldOnFocus = -1;
    }
  }

  return gSelectTextFieldOnFocus == 1;
}

nsresult
nsHTMLInputElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  if (!aVisitor.mPresContext) {
    return NS_OK;
  }

  
  
  if (mType == NS_FORM_INPUT_FILE) {
    nsCOMPtr<nsIContent> maybeButton =
      do_QueryInterface(aVisitor.mEvent->originalTarget);
    if (maybeButton &&
      maybeButton->IsRootOfNativeAnonymousSubtree() &&
      maybeButton->AttrValueIs(kNameSpaceID_None,
                               nsGkAtoms::type,
                               nsGkAtoms::button,
                               eCaseMatters)) {
        return NS_OK;
    }
  }

  nsresult rv = NS_OK;
  PRBool outerActivateEvent = !!(aVisitor.mItemFlags & NS_OUTER_ACTIVATE_EVENT);
  PRBool originalCheckedValue =
    !!(aVisitor.mItemFlags & NS_ORIGINAL_CHECKED_VALUE);
  PRBool noContentDispatch = !!(aVisitor.mItemFlags & NS_NO_CONTENT_DISPATCH);
  PRUint8 oldType = NS_CONTROL_TYPE(aVisitor.mItemFlags);
  
  
  
  
  
  
  
  if (aVisitor.mEventStatus != nsEventStatus_eConsumeNoDefault &&
      !IsSingleLineTextControl(PR_TRUE) &&
      NS_IS_MOUSE_LEFT_CLICK(aVisitor.mEvent)) {
    nsUIEvent actEvent(NS_IS_TRUSTED_EVENT(aVisitor.mEvent), NS_UI_ACTIVATE, 1);

    nsCOMPtr<nsIPresShell> shell = aVisitor.mPresContext->GetPresShell();
    if (shell) {
      nsEventStatus status = nsEventStatus_eIgnore;
      SET_BOOLBIT(mBitField, BF_IN_INTERNAL_ACTIVATE, PR_TRUE);
      rv = shell->HandleDOMEventWithTarget(this, &actEvent, &status);
      SET_BOOLBIT(mBitField, BF_IN_INTERNAL_ACTIVATE, PR_FALSE);

      
      
      if (status == nsEventStatus_eConsumeNoDefault)
        aVisitor.mEventStatus = status;
    }
  }

  if (outerActivateEvent) {
    switch(oldType) {
      case NS_FORM_INPUT_SUBMIT:
      case NS_FORM_INPUT_IMAGE:
        if(mForm) {
          
          
          
          
          mForm->OnSubmitClickEnd();
        }
        break;
    } 
  }

  
  aVisitor.mEvent->flags |=
    noContentDispatch ? NS_EVENT_FLAG_NO_CONTENT_DISPATCH : NS_EVENT_FLAG_NONE;

  
  if (GET_BOOLBIT(mBitField, BF_CHECKED_IS_TOGGLED) && outerActivateEvent) {
    if (aVisitor.mEventStatus == nsEventStatus_eConsumeNoDefault) {
      
      
      
      nsCOMPtr<nsIDOMHTMLInputElement> selectedRadioButton =
        do_QueryInterface(aVisitor.mItemData);
      if (selectedRadioButton) {
        selectedRadioButton->SetChecked(PR_TRUE);
        
        
        if (mType != NS_FORM_INPUT_RADIO) {
          DoSetChecked(PR_FALSE, PR_TRUE, PR_TRUE);
        }
      } else if (oldType == NS_FORM_INPUT_CHECKBOX) {
        PRBool originalIndeterminateValue =
          !!(aVisitor.mItemFlags & NS_ORIGINAL_INDETERMINATE_VALUE);
        SetIndeterminateInternal(originalIndeterminateValue, PR_FALSE);
        DoSetChecked(originalCheckedValue, PR_TRUE, PR_TRUE);
      }
    } else {
      FireOnChange();
#ifdef ACCESSIBILITY
      
      if (mType == NS_FORM_INPUT_CHECKBOX) {
        FireEventForAccessibility(this, aVisitor.mPresContext,
                                  NS_LITERAL_STRING("CheckboxStateChange"));
      } else {
        FireEventForAccessibility(this, aVisitor.mPresContext,
                                  NS_LITERAL_STRING("RadioStateChange"));
        
        nsCOMPtr<nsIDOMHTMLInputElement> previous =
          do_QueryInterface(aVisitor.mItemData);
        if(previous) {
          FireEventForAccessibility(previous, aVisitor.mPresContext,
                                    NS_LITERAL_STRING("RadioStateChange"));
        }
      }
#endif
    }
  }

  if (NS_SUCCEEDED(rv)) {
    if (nsEventStatus_eIgnore == aVisitor.mEventStatus) {
      switch (aVisitor.mEvent->message) {

        case NS_FOCUS_CONTENT:
        {
          
          
          
          
          nsIFocusManager* fm = nsFocusManager::GetFocusManager();
          if (fm && IsSingleLineTextControl(PR_FALSE) &&
              !(static_cast<nsFocusEvent *>(aVisitor.mEvent))->fromRaise &&
              SelectTextFieldOnFocus()) {
            nsIDocument* document = GetCurrentDoc();
            if (document) {
              PRUint32 lastFocusMethod;
              fm->GetLastFocusMethod(document->GetWindow(), &lastFocusMethod);
              if (lastFocusMethod &
                  (nsIFocusManager::FLAG_BYKEY | nsIFocusManager::FLAG_BYMOVEFOCUS)) {
                nsRefPtr<nsPresContext> presContext = GetPresContext();
                if (DispatchSelectEvent(presContext)) {
                  SelectAll(presContext);
                }
              }
            }
          }
          break;
        }

        case NS_KEY_PRESS:
        case NS_KEY_UP:
        {
          
          
          nsKeyEvent * keyEvent = (nsKeyEvent *)aVisitor.mEvent;

          if ((aVisitor.mEvent->message == NS_KEY_PRESS &&
               keyEvent->keyCode == NS_VK_RETURN) ||
              (aVisitor.mEvent->message == NS_KEY_UP &&
               keyEvent->keyCode == NS_VK_SPACE)) {
            switch(mType) {
              case NS_FORM_INPUT_CHECKBOX:
              case NS_FORM_INPUT_RADIO:
              {
                
                if (keyEvent->keyCode != NS_VK_SPACE) {
                  MaybeSubmitForm(aVisitor.mPresContext);

                  break;  
                }
                
              }
              case NS_FORM_INPUT_BUTTON:
              case NS_FORM_INPUT_RESET:
              case NS_FORM_INPUT_SUBMIT:
              case NS_FORM_INPUT_IMAGE: 
              {
                nsMouseEvent event(NS_IS_TRUSTED_EVENT(aVisitor.mEvent),
                                   NS_MOUSE_CLICK, nsnull, nsMouseEvent::eReal);
                event.inputSource = nsIDOMNSMouseEvent::MOZ_SOURCE_KEYBOARD;
                nsEventStatus status = nsEventStatus_eIgnore;

                nsEventDispatcher::Dispatch(static_cast<nsIContent*>(this),
                                            aVisitor.mPresContext, &event,
                                            nsnull, &status);
              } 
            } 
          }
          if (aVisitor.mEvent->message == NS_KEY_PRESS &&
              mType == NS_FORM_INPUT_RADIO && !keyEvent->isAlt &&
              !keyEvent->isControl && !keyEvent->isMeta) {
            PRBool isMovingBack = PR_FALSE;
            switch (keyEvent->keyCode) {
              case NS_VK_UP: 
              case NS_VK_LEFT:
                isMovingBack = PR_TRUE;
              case NS_VK_DOWN:
              case NS_VK_RIGHT:
              
              nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
              if (container) {
                nsAutoString name;
                if (GetNameIfExists(name)) {
                  nsCOMPtr<nsIDOMHTMLInputElement> selectedRadioButton;
                  container->GetNextRadioButton(name, isMovingBack, this,
                                                getter_AddRefs(selectedRadioButton));
                  nsCOMPtr<nsIContent> radioContent =
                    do_QueryInterface(selectedRadioButton);
                  if (radioContent) {
                    rv = selectedRadioButton->Focus();
                    if (NS_SUCCEEDED(rv)) {
                      nsEventStatus status = nsEventStatus_eIgnore;
                      nsMouseEvent event(NS_IS_TRUSTED_EVENT(aVisitor.mEvent),
                                         NS_MOUSE_CLICK, nsnull,
                                         nsMouseEvent::eReal);
                      event.inputSource = nsIDOMNSMouseEvent::MOZ_SOURCE_KEYBOARD;
                      rv = nsEventDispatcher::Dispatch(radioContent,
                                                       aVisitor.mPresContext,
                                                       &event, nsnull, &status);
                      if (NS_SUCCEEDED(rv)) {
                        aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
                      }
                    }
                  }
                }
              }
            }
          }

          












          if (aVisitor.mEvent->message == NS_KEY_PRESS &&
              (keyEvent->keyCode == NS_VK_RETURN ||
               keyEvent->keyCode == NS_VK_ENTER) &&
              (mType == NS_FORM_INPUT_TEXT ||
               mType == NS_FORM_INPUT_EMAIL ||
               mType == NS_FORM_INPUT_SEARCH ||
               mType == NS_FORM_INPUT_PASSWORD ||
               mType == NS_FORM_INPUT_TEL ||
               mType == NS_FORM_INPUT_FILE)) {

            PRBool isButton = PR_FALSE;
            
            
            if (mType == NS_FORM_INPUT_FILE) {
              nsCOMPtr<nsIContent> maybeButton =
                do_QueryInterface(aVisitor.mEvent->originalTarget);
              if (maybeButton) {
                isButton = maybeButton->AttrValueIs(kNameSpaceID_None,
                                                    nsGkAtoms::type,
                                                    nsGkAtoms::button,
                                                    eCaseMatters);
              }
            }

            if (!isButton) {
              nsIFrame* primaryFrame = GetPrimaryFrame();
              if (primaryFrame) {
                nsITextControlFrame* textFrame = do_QueryFrame(primaryFrame);
              
                
                if (textFrame) {
                  textFrame->CheckFireOnChange();
                }
              }

              rv = MaybeSubmitForm(aVisitor.mPresContext);
              NS_ENSURE_SUCCESS(rv, rv);
            }
          }

        } break; 

        case NS_MOUSE_BUTTON_DOWN:
        case NS_MOUSE_BUTTON_UP:
        case NS_MOUSE_DOUBLECLICK:
        {
          
          
          if (aVisitor.mEvent->eventStructType == NS_MOUSE_EVENT &&
              (static_cast<nsMouseEvent*>(aVisitor.mEvent)->button ==
                 nsMouseEvent::eMiddleButton ||
               static_cast<nsMouseEvent*>(aVisitor.mEvent)->button ==
                 nsMouseEvent::eRightButton)) {
            if (mType == NS_FORM_INPUT_BUTTON ||
                mType == NS_FORM_INPUT_RESET ||
                mType == NS_FORM_INPUT_SUBMIT) {
              if (aVisitor.mDOMEvent) {
                aVisitor.mDOMEvent->StopPropagation();
              } else {
                rv = NS_ERROR_FAILURE;
              }
            }

          }
          break;
        }
        default:
          break;
      }

      if (outerActivateEvent) {
        if (mForm && (oldType == NS_FORM_INPUT_SUBMIT ||
                      oldType == NS_FORM_INPUT_IMAGE)) {
          if (mType != NS_FORM_INPUT_SUBMIT && mType != NS_FORM_INPUT_IMAGE) {
            
            
            
            mForm->FlushPendingSubmission();
          }
        }
        switch(mType) {
        case NS_FORM_INPUT_RESET:
        case NS_FORM_INPUT_SUBMIT:
        case NS_FORM_INPUT_IMAGE:
          if (mForm) {
            nsFormEvent event(PR_TRUE, (mType == NS_FORM_INPUT_RESET) ?
                              NS_FORM_RESET : NS_FORM_SUBMIT);
            event.originator      = this;
            nsEventStatus status  = nsEventStatus_eIgnore;

            nsCOMPtr<nsIPresShell> presShell =
              aVisitor.mPresContext->GetPresShell();

            
            
            
            if (presShell) {
              
              nsRefPtr<nsHTMLFormElement> form(mForm);
              presShell->HandleDOMEventWithTarget(mForm, &event, &status);
            }
          }
          break;

        default:
          break;
        } 
      } 
    } else if (outerActivateEvent &&
               (oldType == NS_FORM_INPUT_SUBMIT ||
                oldType == NS_FORM_INPUT_IMAGE) &&
               mForm) {
      
      
      
      
      mForm->FlushPendingSubmission();
    }
  } 

  return rv;
}

void
nsHTMLInputElement::MaybeLoadImage()
{
  
  
  nsAutoString uri;
  if (mType == NS_FORM_INPUT_IMAGE &&
      GetAttr(kNameSpaceID_None, nsGkAtoms::src, uri) &&
      (NS_FAILED(LoadImage(uri, PR_FALSE, PR_TRUE)) ||
       !LoadingEnabled())) {
    CancelImageRequests(PR_TRUE);
  }
}

nsresult
nsHTMLInputElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                               nsIContent* aBindingParent,
                               PRBool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLFormElement::BindToTree(aDocument, aParent,
                                                     aBindingParent,
                                                     aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mType == NS_FORM_INPUT_IMAGE) {
    
    
    if (HasAttr(kNameSpaceID_None, nsGkAtoms::src)) {
      ClearBrokenState();
      nsContentUtils::AddScriptRunner(
        NS_NewRunnableMethod(this, &nsHTMLInputElement::MaybeLoadImage));
    }
  }

  
  
  if (aDocument && !mForm && mType == NS_FORM_INPUT_RADIO) {
    AddedToRadioGroup();
  }

  return rv;
}

void
nsHTMLInputElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  
  
  
  
  
  if (!mForm && mType == NS_FORM_INPUT_RADIO) {
    WillRemoveFromRadioGroup();
  }

  nsGenericHTMLFormElement::UnbindFromTree(aDeep, aNullParent);
}

void
nsHTMLInputElement::HandleTypeChange(PRUint8 aNewType)
{
  
  PRBool isNewTypeSingleLine =
    IsSingleLineTextControlInternal(PR_FALSE, aNewType);
  PRBool isCurrentTypeSingleLine =
    IsSingleLineTextControl(PR_FALSE);
  if (isNewTypeSingleLine && !isCurrentTypeSingleLine) {
    FreeData();
    mInputData.mState = new nsTextEditorState(this);
    NS_ADDREF(mInputData.mState);
  } else if (isCurrentTypeSingleLine && !isNewTypeSingleLine) {
    FreeData();
  }

  mType = aNewType;

  
  
  
  
  if (IsSingleLineTextControlInternal(PR_FALSE, mType)) {
    nsAutoString value;
    GetValue(value);
    
    SetValueInternal(value, PR_FALSE, PR_FALSE);
  }
}

void
nsHTMLInputElement::SanitizeValue(nsAString& aValue)
{
  switch (mType) {
    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_SEARCH:
    case NS_FORM_INPUT_TEL:
    case NS_FORM_INPUT_PASSWORD:
    case NS_FORM_INPUT_EMAIL:
      {
        PRUnichar crlf[] = { PRUnichar('\r'), PRUnichar('\n'), 0 };
        aValue.StripChars(crlf);
      }
      break;
  }
}

PRBool
nsHTMLInputElement::ParseAttribute(PRInt32 aNamespaceID,
                                   nsIAtom* aAttribute,
                                   const nsAString& aValue,
                                   nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::type) {
      
      
      PRInt32 newType;
      PRBool success = aResult.ParseEnumValue(aValue, kInputTypeTable, PR_FALSE);
      if (success) {
        newType = aResult.GetEnumValue();
      } else {
        newType = kInputDefaultType->value;
      }

      if (newType != mType) {
        
        
        
        
        
        
        if (newType == NS_FORM_INPUT_FILE || mType == NS_FORM_INPUT_FILE) {
          
          
          
          ClearFileNames();
        }

        HandleTypeChange(newType);
      }

      return success;
    }
    if (aAttribute == nsGkAtoms::width) {
      return aResult.ParseSpecialIntValue(aValue, PR_TRUE);
    }
    if (aAttribute == nsGkAtoms::height) {
      return aResult.ParseSpecialIntValue(aValue, PR_TRUE);
    }
    if (aAttribute == nsGkAtoms::maxlength) {
      return aResult.ParseNonNegativeIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::size) {
      return aResult.ParseIntWithBounds(aValue, 0);
    }
    if (aAttribute == nsGkAtoms::border) {
      return aResult.ParseIntWithBounds(aValue, 0);
    }
    if (aAttribute == nsGkAtoms::align) {
      return ParseAlignValue(aValue, aResult);
    }
    if (ParseImageAttribute(aAttribute, aValue, aResult)) {
      
      
      
      
      return PR_TRUE;
    }
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

static void
MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                      nsRuleData* aData)
{
  const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::type);
  if (value && value->Type() == nsAttrValue::eEnum &&
      value->GetEnumValue() == NS_FORM_INPUT_IMAGE) {
    nsGenericHTMLFormElement::MapImageBorderAttributeInto(aAttributes, aData);
    nsGenericHTMLFormElement::MapImageMarginAttributeInto(aAttributes, aData);
    nsGenericHTMLFormElement::MapImageSizeAttributesInto(aAttributes, aData);
    
    nsGenericHTMLFormElement::MapImageAlignAttributeInto(aAttributes, aData);
  } 

  nsGenericHTMLFormElement::MapCommonAttributesInto(aAttributes, aData);
}

nsChangeHint
nsHTMLInputElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                           PRInt32 aModType) const
{
  nsChangeHint retval =
    nsGenericHTMLFormElement::GetAttributeChangeHint(aAttribute, aModType);
  if (aAttribute == nsGkAtoms::type) {
    NS_UpdateHint(retval, NS_STYLE_HINT_FRAMECHANGE);
  } else if (mType == NS_FORM_INPUT_IMAGE &&
             (aAttribute == nsGkAtoms::alt ||
              aAttribute == nsGkAtoms::value)) {
    
    
    NS_UpdateHint(retval, NS_STYLE_HINT_FRAMECHANGE);
  } else if (aAttribute == nsGkAtoms::value) {
    NS_UpdateHint(retval, NS_STYLE_HINT_REFLOW);
  } else if (aAttribute == nsGkAtoms::size &&
             IsSingleLineTextControl(PR_FALSE)) {
    NS_UpdateHint(retval, NS_STYLE_HINT_REFLOW);
  }
  return retval;
}

NS_IMETHODIMP_(PRBool)
nsHTMLInputElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::align },
    { &nsGkAtoms::type },
    { nsnull },
  };

  static const MappedAttributeEntry* const map[] = {
    attributes,
    sCommonAttributeMap,
    sImageMarginSizeAttributeMap,
    sImageBorderAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
}

nsMapRuleToAttributesFunc
nsHTMLInputElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}




NS_IMETHODIMP
nsHTMLInputElement::GetControllers(nsIControllers** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  
  if (IsSingleLineTextControl(PR_FALSE))
  {
    if (!mControllers)
    {
      nsresult rv;
      mControllers = do_CreateInstance(kXULControllersCID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIController>
        controller(do_CreateInstance("@mozilla.org/editor/editorcontroller;1",
                                     &rv));
      NS_ENSURE_SUCCESS(rv, rv);

      mControllers->AppendController(controller);
    }
  }

  *aResult = mControllers;
  NS_IF_ADDREF(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::GetTextLength(PRInt32* aTextLength)
{
  nsAutoString val;

  nsresult rv = GetValue(val);

  *aTextLength = val.Length();

  return rv;
}

NS_IMETHODIMP
nsHTMLInputElement::SetSelectionRange(PRInt32 aSelectionStart,
                                      PRInt32 aSelectionEnd)
{
  nsresult rv = NS_ERROR_FAILURE;
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    nsITextControlFrame* textControlFrame = do_QueryFrame(formControlFrame);
    if (textControlFrame)
      rv = textControlFrame->SetSelectionRange(aSelectionStart, aSelectionEnd);
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLInputElement::GetSelectionStart(PRInt32* aSelectionStart)
{
  NS_ENSURE_ARG_POINTER(aSelectionStart);
  
  PRInt32 selEnd;
  return GetSelectionRange(aSelectionStart, &selEnd);
}

NS_IMETHODIMP
nsHTMLInputElement::SetSelectionStart(PRInt32 aSelectionStart)
{
  nsresult rv = NS_ERROR_FAILURE;
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    nsITextControlFrame* textControlFrame = do_QueryFrame(formControlFrame);
    if (textControlFrame)
      rv = textControlFrame->SetSelectionStart(aSelectionStart);
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLInputElement::GetSelectionEnd(PRInt32* aSelectionEnd)
{
  NS_ENSURE_ARG_POINTER(aSelectionEnd);
  
  PRInt32 selStart;
  return GetSelectionRange(&selStart, aSelectionEnd);
}


NS_IMETHODIMP
nsHTMLInputElement::SetSelectionEnd(PRInt32 aSelectionEnd)
{
  nsresult rv = NS_ERROR_FAILURE;
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    nsITextControlFrame* textControlFrame = do_QueryFrame(formControlFrame);
    if (textControlFrame)
      rv = textControlFrame->SetSelectionEnd(aSelectionEnd);
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLInputElement::GetFiles(nsIDOMFileList** aFileList)
{
  *aFileList = nsnull;

  if (mType != NS_FORM_INPUT_FILE) {
    return NS_OK;
  }

  if (!mFileList) {
    mFileList = new nsDOMFileList();
    if (!mFileList) return NS_ERROR_OUT_OF_MEMORY;

    UpdateFileList();
  }

  NS_ADDREF(*aFileList = mFileList);

  return NS_OK;
}

nsresult
nsHTMLInputElement::GetSelectionRange(PRInt32* aSelectionStart,
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
nsHTMLInputElement::GetPhonetic(nsAString& aPhonetic)
{
  aPhonetic.Truncate(0);
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    nsITextControlFrame* textControlFrame = do_QueryFrame(formControlFrame);
    if (textControlFrame)
      textControlFrame->GetPhonetic(aPhonetic);
  }

  return NS_OK;
}

#ifdef ACCESSIBILITY
 nsresult
FireEventForAccessibility(nsIDOMHTMLInputElement* aTarget,
                          nsPresContext* aPresContext,
                          const nsAString& aEventType)
{
  nsCOMPtr<nsIDOMEvent> event;
  if (NS_SUCCEEDED(nsEventDispatcher::CreateEvent(aPresContext, nsnull,
                                                  NS_LITERAL_STRING("Events"),
                                                  getter_AddRefs(event)))) {
    event->InitEvent(aEventType, PR_TRUE, PR_TRUE);

    nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(event));
    if (privateEvent) {
      privateEvent->SetTrusted(PR_TRUE);
    }

    nsEventDispatcher::DispatchDOMEvent(aTarget, nsnull, event, aPresContext, nsnull);
  }

  return NS_OK;
}
#endif

nsresult
nsHTMLInputElement::SetDefaultValueAsValue()
{
  switch (mType) {
    case NS_FORM_INPUT_CHECKBOX:
    case NS_FORM_INPUT_RADIO:
    {
      PRBool resetVal;
      GetDefaultChecked(&resetVal);
      return DoSetChecked(resetVal, PR_TRUE, PR_FALSE);
    }
    case NS_FORM_INPUT_SEARCH:
    case NS_FORM_INPUT_PASSWORD:
    case NS_FORM_INPUT_EMAIL:
    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_TEL:
    {
      nsAutoString resetVal;
      GetDefaultValue(resetVal);
      
      return SetValueInternal(resetVal, PR_FALSE, PR_FALSE);
    }
    case NS_FORM_INPUT_FILE:
    {
      
      ClearFileNames();
      break;
    }
    
    case NS_FORM_INPUT_HIDDEN:
    default:
      break;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::Reset()
{
  nsresult rv = SetDefaultValueAsValue();
  NS_ENSURE_SUCCESS(rv, rv);

  switch (mType) {
    case NS_FORM_INPUT_CHECKBOX:
    case NS_FORM_INPUT_RADIO:
      SetCheckedChanged(PR_FALSE);
      break;
    case NS_FORM_INPUT_SEARCH:
    case NS_FORM_INPUT_PASSWORD:
    case NS_FORM_INPUT_EMAIL:
    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_TEL:
      SetValueChanged(PR_FALSE);
      break;
    default:
      break;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::SubmitNamesValues(nsFormSubmission* aFormSubmission,
                                      nsIContent* aSubmitElement)
{
  nsresult rv = NS_OK;

  
  
  
  
  
  PRBool disabled;
  rv = GetDisabled(&disabled);
  if (disabled || mType == NS_FORM_INPUT_RESET ||
      mType == NS_FORM_INPUT_BUTTON ||
      ((mType == NS_FORM_INPUT_SUBMIT || mType == NS_FORM_INPUT_IMAGE) &&
       aSubmitElement != this) ||
      ((mType == NS_FORM_INPUT_RADIO || mType == NS_FORM_INPUT_CHECKBOX) &&
       !GetChecked())) {
    return NS_OK;
  }

  
  nsAutoString name;
  PRBool nameThere = GetNameIfExists(name);

  
  if (mType == NS_FORM_INPUT_IMAGE) {
    
    nsIntPoint* lastClickedPoint =
      static_cast<nsIntPoint*>(GetProperty(nsGkAtoms::imageClickedPoint));
    PRInt32 x, y;
    if (lastClickedPoint) {
      
      x = lastClickedPoint->x;
      y = lastClickedPoint->y;
    } else {
      x = y = 0;
    }

    nsAutoString xVal, yVal;
    xVal.AppendInt(x);
    yVal.AppendInt(y);

    if (!name.IsEmpty()) {
      aFormSubmission->AddNameValuePair(name + NS_LITERAL_STRING(".x"), xVal);
      aFormSubmission->AddNameValuePair(name + NS_LITERAL_STRING(".y"), yVal);
    } else {
      
      
      aFormSubmission->AddNameValuePair(NS_LITERAL_STRING("x"), xVal);
      aFormSubmission->AddNameValuePair(NS_LITERAL_STRING("y"), yVal);
    }

    return NS_OK;
  }

  
  
  

  
  if (!nameThere) {
    return NS_OK;
  }

  
  nsAutoString value;
  rv = GetValue(value);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (mType == NS_FORM_INPUT_SUBMIT && value.IsEmpty() &&
      !HasAttr(kNameSpaceID_None, nsGkAtoms::value)) {
    
    nsXPIDLString defaultValue;
    nsContentUtils::GetLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                       "Submit", defaultValue);
    value = defaultValue;
  }
      
  
  
  
  if (mType == NS_FORM_INPUT_FILE) {
    

    nsCOMArray<nsIFile> files;
    GetFileArray(files);

    for (PRUint32 i = 0; i < (PRUint32)files.Count(); ++i) {
      aFormSubmission->AddNameFilePair(name, files[i]);
    }

    if (files.Count() == 0) {
      
      
      aFormSubmission->AddNameFilePair(name, nsnull);

    }

    return NS_OK;
  }

  if (mType == NS_FORM_INPUT_HIDDEN && name.EqualsLiteral("_charset_")) {
    nsCString charset;
    aFormSubmission->GetCharset(charset);
    rv = aFormSubmission->AddNameValuePair(name,
                                           NS_ConvertASCIItoUTF16(charset));
  }
  else if (IsSingleLineTextControl(PR_TRUE) &&
           name.EqualsLiteral("isindex") &&
           aFormSubmission->SupportsIsindexSubmission()) {
    rv = aFormSubmission->AddIsindex(value);
  }
  else {
    rv = aFormSubmission->AddNameValuePair(name, value);
  }

  return rv;
}


NS_IMETHODIMP
nsHTMLInputElement::SaveState()
{
  nsresult rv = NS_OK;

  nsRefPtr<nsHTMLInputElementState> inputState = nsnull;

  switch (mType) {
    case NS_FORM_INPUT_CHECKBOX:
    case NS_FORM_INPUT_RADIO:
      {
        PRBool checked = GetChecked();
        PRBool defaultChecked = PR_FALSE;
        GetDefaultChecked(&defaultChecked);
        
        
        
        if (mType == NS_FORM_INPUT_RADIO || checked != defaultChecked) {
          inputState = new nsHTMLInputElementState();
          if (!inputState) {
            return NS_ERROR_OUT_OF_MEMORY;
          }

          inputState->SetChecked(checked);
        }
        break;
      }

    
    case NS_FORM_INPUT_PASSWORD:
      break;
    case NS_FORM_INPUT_EMAIL:
    case NS_FORM_INPUT_SEARCH:
    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_TEL:
    case NS_FORM_INPUT_HIDDEN:
      {
        if (GET_BOOLBIT(mBitField, BF_VALUE_CHANGED)) {
          inputState = new nsHTMLInputElementState();
          if (!inputState) {
            return NS_ERROR_OUT_OF_MEMORY;
          }

          nsAutoString value;
          GetValue(value);
          rv = nsLinebreakConverter::ConvertStringLineBreaks(
                 value,
                 nsLinebreakConverter::eLinebreakPlatform,
                 nsLinebreakConverter::eLinebreakContent);
          NS_ASSERTION(NS_SUCCEEDED(rv), "Converting linebreaks failed!");
          inputState->SetValue(value);
       }
      break;
    }
    case NS_FORM_INPUT_FILE:
      {
        if (!mFileNames.IsEmpty()) {
          inputState = new nsHTMLInputElementState();
          if (!inputState) {
            return NS_ERROR_OUT_OF_MEMORY;
          }

          inputState->SetFilenames(mFileNames);
        }
        break;
      }
  }
  
  nsPresState* state = nsnull;
  if (inputState) {
    rv = GetPrimaryPresState(this, &state);
    if (state) {
      state->SetStateProperty(inputState);
    }
  }

  if (GET_BOOLBIT(mBitField, BF_DISABLED_CHANGED)) {
    rv |= GetPrimaryPresState(this, &state);
    if (state) {
      PRBool disabled;
      GetDisabled(&disabled);
      state->SetDisabled(disabled);
    }
  }

  return rv;
}

void
nsHTMLInputElement::DoneCreatingElement()
{
  SET_BOOLBIT(mBitField, BF_PARSER_CREATING, PR_FALSE);

  
  
  
  
  PRBool restoredCheckedState =
      GET_BOOLBIT(mBitField, BF_INHIBIT_RESTORATION) ?
      PR_FALSE :
      RestoreFormControlState(this, this);

  
  
  
  
  if (!restoredCheckedState &&
      GET_BOOLBIT(mBitField, BF_SHOULD_INIT_CHECKED)) {
    PRBool resetVal;
    GetDefaultChecked(&resetVal);
    DoSetChecked(resetVal, PR_FALSE, PR_TRUE);
    DoSetCheckedChanged(PR_FALSE, PR_FALSE);
  }

  SET_BOOLBIT(mBitField, BF_SHOULD_INIT_CHECKED, PR_FALSE);
}

PRInt32
nsHTMLInputElement::IntrinsicState() const
{
  
  
  
  PRInt32 state = nsGenericHTMLFormElement::IntrinsicState();
  if (mType == NS_FORM_INPUT_CHECKBOX || mType == NS_FORM_INPUT_RADIO) {
    
    if (GET_BOOLBIT(mBitField, BF_CHECKED)) {
      state |= NS_EVENT_STATE_CHECKED;
    }

    
    if (mType == NS_FORM_INPUT_CHECKBOX && GET_BOOLBIT(mBitField, BF_INDETERMINATE)) {
      state |= NS_EVENT_STATE_INDETERMINATE;
    }

    
    
    
    PRBool defaultState = PR_FALSE;
    const_cast<nsHTMLInputElement*>(this)->GetDefaultChecked(&defaultState);
    if (defaultState) {
      state |= NS_EVENT_STATE_DEFAULT;
    }
  } else if (mType == NS_FORM_INPUT_IMAGE) {
    state |= nsImageLoadingContent::ImageState();
  }

  return state;
}

PRBool
nsHTMLInputElement::RestoreState(nsPresState* aState)
{
  PRBool restoredCheckedState = PR_FALSE;

  nsCOMPtr<nsHTMLInputElementState> inputState
    (do_QueryInterface(aState->GetStateProperty()));

  if (inputState) {
    switch (mType) {
      case NS_FORM_INPUT_CHECKBOX:
      case NS_FORM_INPUT_RADIO:
        {
          if (inputState->IsCheckedSet()) {
            restoredCheckedState = PR_TRUE;
            DoSetChecked(inputState->GetChecked(), PR_TRUE, PR_TRUE);
          }
          break;
        }

      case NS_FORM_INPUT_EMAIL:
      case NS_FORM_INPUT_SEARCH:
      case NS_FORM_INPUT_TEXT:
      case NS_FORM_INPUT_TEL:
      case NS_FORM_INPUT_HIDDEN:
        {
          SetValueInternal(inputState->GetValue(), PR_FALSE, PR_TRUE);
          break;
        }
      case NS_FORM_INPUT_FILE:
        {
          SetFileNames(inputState->GetFilenames());
          break;
        }
    }
  }

  if (aState->IsDisabledSet()) {
    SetDisabled(aState->GetDisabled());
  }

  return restoredCheckedState;
}

PRBool
nsHTMLInputElement::AllowDrop()
{
  

  return mType != NS_FORM_INPUT_FILE;
}





void
nsHTMLInputElement::AddedToRadioGroup(PRBool aNotify)
{
  
  aNotify = aNotify && !GET_BOOLBIT(mBitField, BF_PARSER_CREATING);

  
  
  
  
  if (!mForm && !(IsInDoc() && GetParent())) {
    return;
  }

  
  
  
  
  if (GetChecked()) {
    
    
    
    
    
    
    
    RadioSetChecked(aNotify);
  }

  
  
  
  
  PRBool checkedChanged = GET_BOOLBIT(mBitField, BF_CHECKED_CHANGED);
  nsCOMPtr<nsIRadioVisitor> visitor;
  nsresult rv = NS_GetRadioGetCheckedChangedVisitor(&checkedChanged, this,
                                           getter_AddRefs(visitor));
  if (NS_FAILED(rv)) { return; }
  
  VisitGroup(visitor, aNotify);
  SetCheckedChangedInternal(checkedChanged);
  
  
  
  
  nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
  if (container) {
    nsAutoString name;
    if (GetNameIfExists(name)) {
      container->AddToRadioGroup(name, static_cast<nsIFormControl*>(this));
    }
  }
}

void
nsHTMLInputElement::WillRemoveFromRadioGroup()
{
  
  
  
  
  if (!mForm && !(IsInDoc() && GetParent())) {
    return;
  }

  
  
  
  
  nsAutoString name;
  PRBool gotName = PR_FALSE;
  if (GetChecked()) {
    if (!gotName) {
      if (!GetNameIfExists(name)) {
        
        return;
      }
      gotName = PR_TRUE;
    }

    nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
    if (container) {
      container->SetCurrentRadioButton(name, nsnull);
    }
  }
  
  
  
  
  nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
  if (container) {
    if (!gotName) {
      if (!GetNameIfExists(name)) {
        
        return;
      }
      gotName = PR_TRUE;
    }
    container->RemoveFromRadioGroup(name,
                                    static_cast<nsIFormControl*>(this));
  }
}

PRBool
nsHTMLInputElement::IsHTMLFocusable(PRBool aWithMouse, PRBool *aIsFocusable, PRInt32 *aTabIndex)
{
  if (nsGenericHTMLElement::IsHTMLFocusable(aWithMouse, aIsFocusable, aTabIndex)) {
    return PR_TRUE;
  }

  if (HasAttr(kNameSpaceID_None, nsGkAtoms::disabled)) {
    *aIsFocusable = PR_FALSE;
    return PR_TRUE;
  }

  if (IsSingleLineTextControl(PR_FALSE)) {
    *aIsFocusable = PR_TRUE;
    return PR_FALSE;
  }

#ifdef XP_MACOSX
  const PRBool defaultFocusable = !aWithMouse || nsFocusManager::sMouseFocusesFormControl;
#else
  const PRBool defaultFocusable = PR_TRUE;
#endif

  if (mType == NS_FORM_INPUT_FILE) {
    if (aTabIndex) {
      *aTabIndex = -1;
    }
    *aIsFocusable = defaultFocusable;
    return PR_TRUE;
  }

  if (mType == NS_FORM_INPUT_HIDDEN) {
    if (aTabIndex) {
      *aTabIndex = -1;
    }
    *aIsFocusable = PR_FALSE;
    return PR_FALSE;
  }

  if (!aTabIndex) {
    
    *aIsFocusable = defaultFocusable;
    return PR_FALSE;
  }

  if (mType != NS_FORM_INPUT_RADIO) {
    *aIsFocusable = defaultFocusable;
    return PR_FALSE;
  }

  if (GetChecked()) {
    
    *aIsFocusable = defaultFocusable;
    return PR_FALSE;
  }

  
  
  nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
  nsAutoString name;
  if (!container || !GetNameIfExists(name)) {
    *aIsFocusable = defaultFocusable;
    return PR_FALSE;
  }

  nsCOMPtr<nsIDOMHTMLInputElement> currentRadio;
  container->GetCurrentRadioButton(name, getter_AddRefs(currentRadio));
  if (currentRadio) {
    *aTabIndex = -1;
  }
  *aIsFocusable = defaultFocusable;
  return PR_FALSE;
}

nsresult
nsHTMLInputElement::VisitGroup(nsIRadioVisitor* aVisitor, PRBool aFlushContent)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
  if (container) {
    nsAutoString name;
    if (GetNameIfExists(name)) {
      rv = container->WalkRadioGroup(name, aVisitor, aFlushContent);
    } else {
      PRBool stop;
      aVisitor->Visit(this, &stop);
    }
  } else {
    PRBool stop;
    aVisitor->Visit(this, &stop);
  }
  return rv;
}

nsHTMLInputElement::ValueModeType
nsHTMLInputElement::GetValueMode() const
{
  switch (mType)
  {
    case NS_FORM_INPUT_HIDDEN:
    case NS_FORM_INPUT_SUBMIT:
    case NS_FORM_INPUT_BUTTON:
    case NS_FORM_INPUT_RESET:
    case NS_FORM_INPUT_IMAGE:
      return VALUE_MODE_DEFAULT;
    case NS_FORM_INPUT_CHECKBOX:
    case NS_FORM_INPUT_RADIO:
      return VALUE_MODE_DEFAULT_ON;
    case NS_FORM_INPUT_FILE:
      return VALUE_MODE_FILENAME;
#ifdef DEBUG
    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_PASSWORD:
    case NS_FORM_INPUT_SEARCH:
    case NS_FORM_INPUT_TEL:
    case NS_FORM_INPUT_EMAIL:
      return VALUE_MODE_VALUE;
    default:
      NS_NOTYETIMPLEMENTED("Unexpected input type in GetValueMode()");
      return VALUE_MODE_VALUE;
#else 
    default:
      return VALUE_MODE_VALUE;
#endif 
  }
}

PRBool
nsHTMLInputElement::IsMutable() const
{
  return !HasAttr(kNameSpaceID_None, nsGkAtoms::disabled) &&
         GetCurrentDoc() &&
         !(DoesReadOnlyApply() &&
           HasAttr(kNameSpaceID_None, nsGkAtoms::readonly));
}

PRBool
nsHTMLInputElement::DoesReadOnlyApply() const
{
  switch (mType)
  {
    case NS_FORM_INPUT_HIDDEN:
    case NS_FORM_INPUT_BUTTON:
    case NS_FORM_INPUT_IMAGE:
    case NS_FORM_INPUT_RESET:
    case NS_FORM_INPUT_SUBMIT:
    case NS_FORM_INPUT_RADIO:
    case NS_FORM_INPUT_FILE:
    case NS_FORM_INPUT_CHECKBOX:
    
    
    
      return PR_FALSE;
#ifdef DEBUG
    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_PASSWORD:
    case NS_FORM_INPUT_SEARCH:
    case NS_FORM_INPUT_TEL:
    case NS_FORM_INPUT_EMAIL:
      return PR_TRUE;
    default:
      NS_NOTYETIMPLEMENTED("Unexpected input type in DoesReadOnlyApply()");
      return PR_TRUE;
#else 
    default:
      return PR_TRUE;
#endif 
  }
}

PRBool
nsHTMLInputElement::DoesRequiredApply() const
{
  switch (mType)
  {
    case NS_FORM_INPUT_HIDDEN:
    case NS_FORM_INPUT_BUTTON:
    case NS_FORM_INPUT_IMAGE:
    case NS_FORM_INPUT_RESET:
    case NS_FORM_INPUT_SUBMIT:
    
    
    
      return PR_FALSE;
#ifdef DEBUG
    case NS_FORM_INPUT_RADIO:
    case NS_FORM_INPUT_CHECKBOX:
    case NS_FORM_INPUT_FILE:
    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_PASSWORD:
    case NS_FORM_INPUT_SEARCH:
    case NS_FORM_INPUT_TEL:
    case NS_FORM_INPUT_EMAIL:
      return PR_TRUE;
    default:
      NS_NOTYETIMPLEMENTED("Unexpected input type in DoesRequiredApply()");
      return PR_TRUE;
#else 
    default:
      return PR_TRUE;
#endif 
  }
}



PRBool
nsHTMLInputElement::IsTooLong()
{
  if (!GET_BOOLBIT(mBitField, BF_VALUE_CHANGED)) {
    return PR_FALSE;
  }

  PRInt32 maxLength = -1;
  PRInt32 textLength = -1;

  GetMaxLength(&maxLength);
  GetTextLength(&textLength);

  return (maxLength >= 0) && (textLength > maxLength);
}

PRBool
nsHTMLInputElement::IsValueMissing()
{
  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::required) ||
      !DoesRequiredApply()) {
    return PR_FALSE;
  }

  if (GetValueMode() == VALUE_MODE_VALUE) {
    if (!IsMutable()) {
      return PR_FALSE;
    }

    nsAutoString value;
    nsresult rv = GetValue(value);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    return value.IsEmpty();
  }

  if (mType == NS_FORM_INPUT_CHECKBOX) {
    return !GetChecked();
  }

  if (mType == NS_FORM_INPUT_RADIO) {
    nsCOMPtr<nsIDOMHTMLInputElement> selected = GetSelectedRadioButton();
    return !selected;
  }

  if (mType == NS_FORM_INPUT_FILE) {
    nsCOMArray<nsIFile> files;
    GetFileArray(files);
    return !files.Count();
  }

  return PR_FALSE;
}

PRBool
nsHTMLInputElement::HasTypeMismatch()
{
  if (mType == NS_FORM_INPUT_EMAIL) {
    nsAutoString value;
    NS_ENSURE_SUCCESS(GetValue(value), PR_FALSE);

    if (value.IsEmpty()) {
      return PR_FALSE;
    }

    return HasAttr(kNameSpaceID_None, nsGkAtoms::multiple) ?
           !IsValidEmailAddressList(value) :
           !IsValidEmailAddress(value);
  }

  return PR_FALSE;
}

PRBool
nsHTMLInputElement::IsBarredFromConstraintValidation()
{
  return mType == NS_FORM_INPUT_HIDDEN ||
         mType == NS_FORM_INPUT_BUTTON ||
         mType == NS_FORM_INPUT_RESET ||
         HasAttr(kNameSpaceID_None, nsGkAtoms::readonly);
}

nsresult
nsHTMLInputElement::GetValidationMessage(nsAString& aValidationMessage,
                                         ValidationMessageType aType)
{
  nsresult rv = NS_OK;

  switch (aType)
  {
    case VALIDATION_MESSAGE_TOO_LONG:
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

      const PRUnichar* params[] = { strTextLength.get(), strMaxLength.get() };
      rv = nsContentUtils::FormatLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                 "ElementSuffersFromBeingTooLong",
                                                 params, 2, message);
      aValidationMessage = message;
      break;
    }
    case VALIDATION_MESSAGE_VALUE_MISSING:
    {
      nsXPIDLString message;
      nsCAutoString key;
      switch (mType)
      {
        case NS_FORM_INPUT_FILE:
          key.Assign("FileElementSuffersFromBeingMissing");
          break;
        case NS_FORM_INPUT_CHECKBOX:
          key.Assign("CheckboxElementSuffersFromBeingMissing");
          break;
        case NS_FORM_INPUT_RADIO:
          key.Assign("RadioElementSuffersFromBeingMissing");
          break;
        default:
          key.Assign("TextElementSuffersFromBeingMissing");
      }
      rv = nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                              key.get(), message);
      aValidationMessage = message;
      break;
    }
    case VALIDATION_MESSAGE_TYPE_MISMATCH:
    {
      NS_ASSERTION(mType == NS_FORM_INPUT_EMAIL,
                   "Only email type can suffer from a type mismatch!");

      nsXPIDLString message;
      rv = nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                              "ElementSuffersFromInvalidEmail",
                                              message);
      aValidationMessage = message;
      break;
    }
    default:
      rv = nsConstraintValidation::GetValidationMessage(aValidationMessage, aType);
  }

  return rv;
}


PRBool
nsHTMLInputElement::IsValidEmailAddressList(const nsAString& aValue)
{
  nsCharSeparatedTokenizerTemplate<nsContentUtils::IsHTMLWhitespace>
    tokenizer(aValue, ',');

  while (tokenizer.hasMoreTokens()) {
    if (!IsValidEmailAddress(tokenizer.nextToken())) {
      return PR_FALSE;
    }
  }

  return !tokenizer.lastTokenEndedWithSeparator();
}


PRBool
nsHTMLInputElement::IsValidEmailAddress(const nsAString& aValue)
{
  PRUint32 i = 0;
  PRUint32 length = aValue.Length();

  
  
  if (length == 0 || aValue[0] == '@' || aValue[length-1] == '.') {
    return PR_FALSE;
  }

  
  for (; i < length && aValue[i] != '@'; ++i) {
    PRUnichar c = aValue[i];

    
    if (!(nsCRT::IsAsciiAlpha(c) || nsCRT::IsAsciiDigit(c) ||
          c == '.' || c == '!' || c == '#' || c == '$' || c == '%' ||
          c == '&' || c == '\''|| c == '*' || c == '+' || c == '-' ||
          c == '/' || c == '=' || c == '?' || c == '^' || c == '_' ||
          c == '`' || c == '{' || c == '|' || c == '}' || c == '~' )) {
      return PR_FALSE;
    }
  }

  
  
  if (++i >= length) {
    return PR_FALSE;
  }

  
  if (aValue[i] == '.') {
    return PR_FALSE;
  }

  
  
  PRBool dotFound = PR_FALSE;

  
  for (; i < length; ++i) {
    PRUnichar c = aValue[i];

    if (c == '.') {
      dotFound = PR_TRUE;
      
      if (aValue[i-1] == '.') {
        return PR_FALSE;
      }
    } else if (!(nsCRT::IsAsciiAlpha(c) || nsCRT::IsAsciiDigit(c) ||
                 c == '-')) {
      
      return PR_FALSE;
    }
  }

  return dotFound;
}









class nsRadioVisitor : public nsIRadioVisitor {
public:
  nsRadioVisitor() { }
  virtual ~nsRadioVisitor() { }

  NS_DECL_ISUPPORTS

  NS_IMETHOD Visit(nsIFormControl* aRadio, PRBool* aStop) = 0;
};

NS_IMPL_ISUPPORTS1(nsRadioVisitor, nsIRadioVisitor)





class nsRadioSetCheckedChangedVisitor : public nsRadioVisitor {
public:
  nsRadioSetCheckedChangedVisitor(PRBool aCheckedChanged) :
    nsRadioVisitor(), mCheckedChanged(aCheckedChanged)
    { }

  virtual ~nsRadioSetCheckedChangedVisitor() { }

  NS_IMETHOD Visit(nsIFormControl* aRadio, PRBool* aStop)
  {
    nsRefPtr<nsHTMLInputElement> radio =
      static_cast<nsHTMLInputElement*>(aRadio);
    NS_ASSERTION(radio, "Visit() passed a null button!");
    radio->SetCheckedChangedInternal(mCheckedChanged);
    return NS_OK;
  }

protected:
  PRPackedBool mCheckedChanged;
};




class nsRadioGetCheckedChangedVisitor : public nsRadioVisitor {
public:
  nsRadioGetCheckedChangedVisitor(PRBool* aCheckedChanged,
                                  nsIFormControl* aExcludeElement) :
    nsRadioVisitor(),
    mCheckedChanged(aCheckedChanged),
    mExcludeElement(aExcludeElement)
    { }

  virtual ~nsRadioGetCheckedChangedVisitor() { }

  NS_IMETHOD Visit(nsIFormControl* aRadio, PRBool* aStop)
  {
    if (aRadio == mExcludeElement) {
      return NS_OK;
    }
    nsRefPtr<nsHTMLInputElement> radio =
      static_cast<nsHTMLInputElement*>(aRadio);
    NS_ASSERTION(radio, "Visit() passed a null button!");
    *mCheckedChanged = radio->GetCheckedChanged();
    *aStop = PR_TRUE;
    return NS_OK;
  }

protected:
  PRBool* mCheckedChanged;
  nsIFormControl* mExcludeElement;
};

nsresult
NS_GetRadioSetCheckedChangedVisitor(PRBool aCheckedChanged,
                                    nsIRadioVisitor** aVisitor)
{
  
  
  
  
  
  static nsIRadioVisitor* sVisitorTrue = nsnull;
  static nsIRadioVisitor* sVisitorFalse = nsnull;

  
  
  
  if (aCheckedChanged) {
    if (!sVisitorTrue) {
      sVisitorTrue = new nsRadioSetCheckedChangedVisitor(PR_TRUE);
      if (!sVisitorTrue) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      NS_ADDREF(sVisitorTrue);
      nsresult rv =
        nsContentUtils::ReleasePtrOnShutdown((nsISupports**)&sVisitorTrue);
      if (NS_FAILED(rv)) {
        NS_RELEASE(sVisitorTrue);
        return rv;
      }
    }
    *aVisitor = sVisitorTrue;
  }
  
  
  
  else {
    if (!sVisitorFalse) {
      sVisitorFalse = new nsRadioSetCheckedChangedVisitor(PR_FALSE);
      if (!sVisitorFalse) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      NS_ADDREF(sVisitorFalse);
      nsresult rv =
        nsContentUtils::ReleasePtrOnShutdown((nsISupports**)&sVisitorFalse);
      if (NS_FAILED(rv)) {
        NS_RELEASE(sVisitorFalse);
        return rv;
      }
    }
    *aVisitor = sVisitorFalse;
  }

  NS_ADDREF(*aVisitor);
  return NS_OK;
}

nsresult
NS_GetRadioGetCheckedChangedVisitor(PRBool* aCheckedChanged,
                                    nsIFormControl* aExcludeElement,
                                    nsIRadioVisitor** aVisitor)
{
  *aVisitor = new nsRadioGetCheckedChangedVisitor(aCheckedChanged,
                                                  aExcludeElement);
  if (!*aVisitor) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(*aVisitor);

  return NS_OK;
}

NS_IMETHODIMP_(PRBool)
nsHTMLInputElement::IsSingleLineTextControl() const
{
  return IsSingleLineTextControl(PR_FALSE);
}

NS_IMETHODIMP_(PRBool)
nsHTMLInputElement::IsTextArea() const
{
  return PR_FALSE;
}

NS_IMETHODIMP_(PRBool)
nsHTMLInputElement::IsPlainTextControl() const
{
  
  return PR_TRUE;
}

NS_IMETHODIMP_(PRBool)
nsHTMLInputElement::IsPasswordTextControl() const
{
  return mType == NS_FORM_INPUT_PASSWORD;
}

NS_IMETHODIMP_(PRInt32)
nsHTMLInputElement::GetCols()
{
  
  const nsAttrValue* attr = GetParsedAttr(nsGkAtoms::size);
  if (attr && attr->Type() == nsAttrValue::eInteger) {
    PRInt32 cols = attr->GetIntegerValue();
    if (cols > 0) {
      return cols;
    }
  }

  return DEFAULT_COLS;
}

NS_IMETHODIMP_(PRInt32)
nsHTMLInputElement::GetWrapCols()
{
  return -1; 
}

NS_IMETHODIMP_(PRInt32)
nsHTMLInputElement::GetRows()
{
  return DEFAULT_ROWS;
}

NS_IMETHODIMP_(void)
nsHTMLInputElement::GetDefaultValueFromContent(nsAString& aValue)
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    GetDefaultValue(aValue);
    
    
    SanitizeValue(aValue);
  }
}

NS_IMETHODIMP_(PRBool)
nsHTMLInputElement::ValueChanged() const
{
  return GET_BOOLBIT(mBitField, BF_VALUE_CHANGED);
}

NS_IMETHODIMP_(void)
nsHTMLInputElement::GetTextEditorValue(nsAString& aValue,
                                       PRBool aIgnoreWrap) const
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    state->GetValue(aValue, aIgnoreWrap);
  }
}

NS_IMETHODIMP_(void)
nsHTMLInputElement::SetTextEditorValue(const nsAString& aValue,
                                       PRBool aUserInput)
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    state->SetValue(aValue, aUserInput);
  }
}

NS_IMETHODIMP_(void)
nsHTMLInputElement::InitializeKeyboardEventListeners()
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    state->InitializeKeyboardEventListeners();
  }
}
