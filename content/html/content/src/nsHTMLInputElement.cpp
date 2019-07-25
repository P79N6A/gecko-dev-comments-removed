






































#include "nsIDOMHTMLInputElement.h"
#include "nsITextControlElement.h"
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
#include "nsFormSubmissionConstants.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIFormControlFrame.h"
#include "nsITextControlFrame.h"
#include "nsIFrame.h"
#include "nsEventStates.h"
#include "nsIServiceManager.h"
#include "nsIScriptSecurityManager.h"
#include "nsDOMError.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIEditor.h"
#include "nsGUIEvent.h"
#include "nsIIOService.h"

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


#include "nsILocalFile.h"
#include "nsNetUtil.h"
#include "nsDOMFile.h"
#include "nsIFilePicker.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIPrivateBrowsingService.h"
#include "nsIContentURIGrouper.h"
#include "nsIContentPrefService.h"
#include "nsIObserverService.h"
#include "nsIPopupWindowManager.h"
#include "nsGlobalWindow.h"


#include "nsImageLoadingContent.h"
#include "nsIDOMWindowInternal.h"

#include "mozAutoDocUpdate.h"
#include "nsContentCreatorFunctions.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsContentUtils.h"
#include "nsRadioVisitor.h"

#include "nsTextEditRules.h"


#include "jsapi.h"
#include "jscntxt.h"

#include "nsHTMLInputElement.h"

using namespace mozilla::dom;



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
UploadLastDir* nsHTMLInputElement::gUploadLastDir;

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
  { "url", NS_FORM_INPUT_URL },
  { 0 }
};


static const nsAttrValue::EnumTable* kInputDefaultType = &kInputTypeTable[12];

static const PRUint8 NS_INPUT_AUTOCOMPLETE_OFF     = 0;
static const PRUint8 NS_INPUT_AUTOCOMPLETE_ON      = 1;
static const PRUint8 NS_INPUT_AUTOCOMPLETE_DEFAULT = 2;

static const nsAttrValue::EnumTable kInputAutocompleteTable[] = {
  { "", NS_INPUT_AUTOCOMPLETE_DEFAULT },
  { "on", NS_INPUT_AUTOCOMPLETE_ON },
  { "off", NS_INPUT_AUTOCOMPLETE_OFF },
  { 0 }
};


static const nsAttrValue::EnumTable* kInputDefaultAutocomplete = &kInputAutocompleteTable[0];

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

    const nsCOMArray<nsIDOMFile>& GetFiles() {
      return mFiles;
    }

    void SetFiles(const nsCOMArray<nsIDOMFile> &aFiles) {
      mFiles.Clear();
      mFiles.AppendObjects(aFiles);
    }

    nsHTMLInputElementState()
      : mValue()
      , mChecked(PR_FALSE)
      , mCheckedSet(PR_FALSE)
    {};
 
  protected:
    nsString mValue;
    nsCOMArray<nsIDOMFile> mFiles;
    PRPackedBool mChecked;
    PRPackedBool mCheckedSet;
};

NS_IMPL_ISUPPORTS1(nsHTMLInputElementState, nsHTMLInputElementState)
NS_DEFINE_STATIC_IID_ACCESSOR(nsHTMLInputElementState, NS_INPUT_ELEMENT_STATE_IID)

class AsyncClickHandler : public nsRunnable {
public:
  AsyncClickHandler(nsHTMLInputElement* aInput)
   : mInput(aInput) {
    
    nsIDocument* doc = aInput->GetOwnerDoc();
    if (doc) {
      nsPIDOMWindow* win = doc->GetWindow();
      if (win)
        mPopupControlState = win->GetPopupControlState();
    }
  };

  NS_IMETHOD Run();

protected:
  nsRefPtr<nsHTMLInputElement> mInput;
  PopupControlState mPopupControlState;
};

NS_IMETHODIMP
AsyncClickHandler::Run()
{
  nsresult rv;

  
  nsCOMPtr<nsIDocument> doc = mInput->GetOwnerDoc();
  if (!doc)
    return NS_ERROR_FAILURE;

  nsPIDOMWindow* win = doc->GetWindow();
  if (!win) {
    return NS_ERROR_FAILURE;
  }

  
  if (mPopupControlState != openAllowed) {
    nsCOMPtr<nsIPopupWindowManager> pm =
      do_GetService(NS_POPUPWINDOWMANAGER_CONTRACTID);
 
    if (!pm) {
      return NS_OK;
    }

    PRUint32 permission;
    pm->TestPermission(doc->GetDocumentURI(), &permission);
    if (permission == nsIPopupWindowManager::DENY_POPUP) {
      nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(doc);
      nsGlobalWindow::FirePopupBlockedEvent(domDoc, win, nsnull, EmptyString(), EmptyString());
      return NS_OK;
    }
  }

  
  nsXPIDLString title;
  nsContentUtils::GetLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                     "FileUpload", title);

  nsCOMPtr<nsIFilePicker> filePicker = do_CreateInstance("@mozilla.org/filepicker;1");
  if (!filePicker)
    return NS_ERROR_FAILURE;

  PRBool multi = mInput->HasAttr(kNameSpaceID_None, nsGkAtoms::multiple);

  rv = filePicker->Init(win, title, multi ?
                        (PRInt16)nsIFilePicker::modeOpenMultiple :
                        (PRInt16)nsIFilePicker::modeOpen);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mInput->HasAttr(kNameSpaceID_None, nsGkAtoms::accept)) {
    PRInt32 filters = mInput->GetFilterFromAccept();

    if (filters) {
      
      filePicker->AppendFilters(filters | nsIFilePicker::filterAll);

      
      
      
      filePicker->SetFilterIndex(1);
    } else {
      filePicker->AppendFilters(nsIFilePicker::filterAll);
    }
  } else {
    filePicker->AppendFilters(nsIFilePicker::filterAll);
  }

  
  nsAutoString defaultName;

  const nsCOMArray<nsIDOMFile>& oldFiles = mInput->GetFiles();

  if (oldFiles.Count()) {
    nsString path;

    oldFiles[0]->GetMozFullPathInternal(path);

    nsCOMPtr<nsILocalFile> localFile;
    rv = NS_NewLocalFile(path, PR_FALSE, getter_AddRefs(localFile));

    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIFile> parentFile;
      rv = localFile->GetParent(getter_AddRefs(parentFile));
      if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsILocalFile> parentLocalFile = do_QueryInterface(parentFile, &rv);
        if (parentLocalFile) {
          filePicker->SetDisplayDirectory(parentLocalFile);
        }
      }
    }

    
    
    
    if (oldFiles.Count() == 1) {
      nsAutoString leafName;
      oldFiles[0]->GetName(leafName);
      if (!leafName.IsEmpty()) {
        filePicker->SetDefaultString(leafName);
      }
    }
  } else {
    
    nsCOMPtr<nsILocalFile> localFile;
    nsHTMLInputElement::gUploadLastDir->FetchLastUsedDirectory(doc->GetDocumentURI(),
                                                               getter_AddRefs(localFile));
    if (!localFile) {
      
      nsCOMPtr<nsIFile> homeDir;
      NS_GetSpecialDirectory(NS_OS_DESKTOP_DIR, getter_AddRefs(homeDir));
      localFile = do_QueryInterface(homeDir);
    }
    filePicker->SetDisplayDirectory(localFile);
  }

  
  PRInt16 mode;
  rv = filePicker->Show(&mode);
  NS_ENSURE_SUCCESS(rv, rv);
  if (mode == nsIFilePicker::returnCancel) {
    return NS_OK;
  }

  
  nsCOMArray<nsIDOMFile> newFiles;
  if (multi) {
    nsCOMPtr<nsISimpleEnumerator> iter;
    rv = filePicker->GetFiles(getter_AddRefs(iter));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISupports> tmp;
    PRBool prefSaved = PR_FALSE;
    PRBool loop = PR_TRUE;
    while (NS_SUCCEEDED(iter->HasMoreElements(&loop)) && loop) {
      iter->GetNext(getter_AddRefs(tmp));
      nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(tmp);
      if (localFile) {
        nsString unicodePath;
        rv = localFile->GetPath(unicodePath);
        if (!unicodePath.IsEmpty()) {
          nsCOMPtr<nsIDOMFile> domFile =
            do_QueryObject(new nsDOMFile(localFile));
          newFiles.AppendObject(domFile);
        }
        if (!prefSaved) {
          
          nsHTMLInputElement::gUploadLastDir->StoreLastUsedDirectory(doc->GetDocumentURI(),
                                                                     localFile);
          prefSaved = PR_TRUE;
        }
      }
    }
  }
  else {
    nsCOMPtr<nsILocalFile> localFile;
    rv = filePicker->GetFile(getter_AddRefs(localFile));
    if (localFile) {
      nsString unicodePath;
      rv = localFile->GetPath(unicodePath);
      if (!unicodePath.IsEmpty()) {
        nsCOMPtr<nsIDOMFile> domFile=
          do_QueryObject(new nsDOMFile(localFile));
        newFiles.AppendObject(domFile);
      }
      
      nsHTMLInputElement::gUploadLastDir->StoreLastUsedDirectory(doc->GetDocumentURI(),
                                                                 localFile);
    }
  }

  
  if (newFiles.Count()) {
    
    
    
    mInput->SetFiles(newFiles, true);
    nsContentUtils::DispatchTrustedEvent(mInput->GetOwnerDoc(),
                                         static_cast<nsIDOMHTMLInputElement*>(mInput.get()),
                                         NS_LITERAL_STRING("change"), PR_TRUE,
                                         PR_FALSE);
  }

  return NS_OK;
}

#define CPS_PREF_NAME NS_LITERAL_STRING("browser.upload.lastDir")

NS_IMPL_ISUPPORTS2(UploadLastDir, nsIObserver, nsISupportsWeakReference)

void
nsHTMLInputElement::InitUploadLastDir() {
  gUploadLastDir = new UploadLastDir();
  NS_ADDREF(gUploadLastDir);

  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (observerService && gUploadLastDir) {
    observerService->AddObserver(gUploadLastDir, NS_PRIVATE_BROWSING_SWITCH_TOPIC, PR_TRUE);
    observerService->AddObserver(gUploadLastDir, "browser:purge-session-history", PR_TRUE);
  }
}

void 
nsHTMLInputElement::DestroyUploadLastDir() {
  NS_IF_RELEASE(gUploadLastDir);
}

UploadLastDir::UploadLastDir():
  mInPrivateBrowsing(PR_FALSE)
{
  nsCOMPtr<nsIPrivateBrowsingService> pbService =
    do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
  if (pbService) {
    pbService->GetPrivateBrowsingEnabled(&mInPrivateBrowsing);
  }

  mUploadLastDirStore.Init();
}

nsresult
UploadLastDir::FetchLastUsedDirectory(nsIURI* aURI, nsILocalFile** aFile)
{
  NS_PRECONDITION(aURI, "aURI is null");
  NS_PRECONDITION(aFile, "aFile is null");
  
  
  if (mInPrivateBrowsing) {
    nsCOMPtr<nsIContentURIGrouper> hostnameGrouperService =
      do_GetService(NS_HOSTNAME_GROUPER_SERVICE_CONTRACTID);
    if (!hostnameGrouperService)
      return NS_ERROR_NOT_AVAILABLE;
    nsString group;
    hostnameGrouperService->Group(aURI, group);

    if (mUploadLastDirStore.Get(group, aFile)) {
      return NS_OK;
    }
  }

  
  nsCOMPtr<nsIContentPrefService> contentPrefService =
    do_GetService(NS_CONTENT_PREF_SERVICE_CONTRACTID);
  if (!contentPrefService)
    return NS_ERROR_NOT_AVAILABLE;
  nsCOMPtr<nsIWritableVariant> uri = do_CreateInstance(NS_VARIANT_CONTRACTID);
  if (!uri)
    return NS_ERROR_OUT_OF_MEMORY;
  uri->SetAsISupports(aURI);

  
  PRBool hasPref;
  if (NS_SUCCEEDED(contentPrefService->HasPref(uri, CPS_PREF_NAME, &hasPref)) && hasPref) {
    nsCOMPtr<nsIVariant> pref;
    contentPrefService->GetPref(uri, CPS_PREF_NAME, nsnull, getter_AddRefs(pref));
    nsString prefStr;
    pref->GetAsAString(prefStr);

    nsCOMPtr<nsILocalFile> localFile = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID);
    if (!localFile)
      return NS_ERROR_OUT_OF_MEMORY;
    localFile->InitWithPath(prefStr);

    *aFile = localFile;
    NS_ADDREF(*aFile);
  }
  return NS_OK;
}

nsresult
UploadLastDir::StoreLastUsedDirectory(nsIURI* aURI, nsILocalFile* aFile)
{
  NS_PRECONDITION(aURI, "aURI is null");
  NS_PRECONDITION(aFile, "aFile is null");
  nsCOMPtr<nsIFile> parentFile;
  aFile->GetParent(getter_AddRefs(parentFile));
  if (!parentFile) {
    return NS_OK;
  }
  nsCOMPtr<nsILocalFile> localFile = do_QueryInterface(parentFile);

  
  if (mInPrivateBrowsing) {
    nsCOMPtr<nsIContentURIGrouper> hostnameGrouperService =
      do_GetService(NS_HOSTNAME_GROUPER_SERVICE_CONTRACTID);
    if (!hostnameGrouperService)
      return NS_ERROR_NOT_AVAILABLE;
    nsString group;
    hostnameGrouperService->Group(aURI, group);

    return mUploadLastDirStore.Put(group, localFile);
  }

  
  nsCOMPtr<nsIContentPrefService> contentPrefService =
    do_GetService(NS_CONTENT_PREF_SERVICE_CONTRACTID);
  if (!contentPrefService)
    return NS_ERROR_NOT_AVAILABLE;
  nsCOMPtr<nsIWritableVariant> uri = do_CreateInstance(NS_VARIANT_CONTRACTID);
  if (!uri)
    return NS_ERROR_OUT_OF_MEMORY;
  uri->SetAsISupports(aURI);
 
  
  nsString unicodePath;
  parentFile->GetPath(unicodePath);
  if (unicodePath.IsEmpty()) 
    return NS_OK;
  nsCOMPtr<nsIWritableVariant> prefValue = do_CreateInstance(NS_VARIANT_CONTRACTID);
  if (!prefValue)
    return NS_ERROR_OUT_OF_MEMORY;
  prefValue->SetAsAString(unicodePath);
  return contentPrefService->SetPref(uri, CPS_PREF_NAME, prefValue);
}

NS_IMETHODIMP
UploadLastDir::Observe(nsISupports *aSubject, char const *aTopic, PRUnichar const *aData)
{
  if (strcmp(aTopic, NS_PRIVATE_BROWSING_SWITCH_TOPIC) == 0) {
    if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_ENTER).Equals(aData)) {
      mInPrivateBrowsing = PR_TRUE;
    } else if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_LEAVE).Equals(aData)) {
      mInPrivateBrowsing = PR_FALSE;
      if (mUploadLastDirStore.IsInitialized()) {
        mUploadLastDirStore.Clear();
      }
    }
  } else if (strcmp(aTopic, "browser:purge-session-history") == 0) {
    if (mUploadLastDirStore.IsInitialized()) {
      mUploadLastDirStore.Clear();
    }
    nsCOMPtr<nsIContentPrefService> contentPrefService =
      do_GetService(NS_CONTENT_PREF_SERVICE_CONTRACTID);
    if (contentPrefService)
      contentPrefService->RemovePrefsByName(CPS_PREF_NAME);
  }
  return NS_OK;
}

#ifdef ACCESSIBILITY

static nsresult FireEventForAccessibility(nsIDOMHTMLInputElement* aTarget,
                                          nsPresContext* aPresContext,
                                          const nsAString& aEventType);
#endif





NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(Input)

nsHTMLInputElement::nsHTMLInputElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                                       FromParser aFromParser)
  : nsGenericHTMLFormElement(aNodeInfo),
    mType(kInputDefaultType->value),
    mBitField(0)
{
  SET_BOOLBIT(mBitField, BF_PARSER_CREATING, aFromParser);
  SET_BOOLBIT(mBitField, BF_INHIBIT_RESTORATION,
      aFromParser & mozilla::dom::FROM_PARSER_FRAGMENT);
  SET_BOOLBIT(mBitField, BF_CAN_SHOW_INVALID_UI, PR_TRUE);
  SET_BOOLBIT(mBitField, BF_CAN_SHOW_VALID_UI, PR_TRUE);
  mInputData.mState = new nsTextEditorState(this);
  NS_ADDREF(mInputData.mState);
  
  if (!gUploadLastDir)
    nsHTMLInputElement::InitUploadLastDir();
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
                                   nsIPhonetic,
                                   imgIDecoderObserver,
                                   nsIImageLoadingContent,
                                   imgIContainerObserver,
                                   nsIDOMNSEditableElement,
                                   nsIConstraintValidation)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLInputElement,
                                               nsGenericHTMLFormElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLInputElement)


NS_IMPL_NSICONSTRAINTVALIDATION_EXCEPT_SETCUSTOMVALIDITY(nsHTMLInputElement)



nsresult
nsHTMLInputElement::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
{
  *aResult = nsnull;

  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
  nsHTMLInputElement *it = new nsHTMLInputElement(ni.forget(), NOT_FROM_PARSER);

  nsCOMPtr<nsINode> kungFuDeathGrip = it;
  nsresult rv = CopyInnerTo(it);
  NS_ENSURE_SUCCESS(rv, rv);

  switch (mType) {
    case NS_FORM_INPUT_EMAIL:
    case NS_FORM_INPUT_SEARCH:
    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_PASSWORD:
    case NS_FORM_INPUT_TEL:
    case NS_FORM_INPUT_URL:
      if (GetValueChanged()) {
        
        
        nsAutoString value;
        GetValueInternal(value);
        
        it->SetValueInternal(value, PR_FALSE, PR_TRUE);
      }
      break;
    case NS_FORM_INPUT_FILE:
      if (it->GetOwnerDoc()->IsStaticDocument()) {
        
        
        GetDisplayFileName(it->mStaticDocFileList);
      } else {
        it->mFiles.Clear();
        it->mFiles.AppendObjects(mFiles);
      }
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
  
  nsEventStates states;

  if (aNameSpaceID == kNameSpaceID_None) {
    
    
    
    
    
    if ((aName == nsGkAtoms::name ||
         (aName == nsGkAtoms::type && !mForm)) &&
        mType == NS_FORM_INPUT_RADIO &&
        (mForm || !(GET_BOOLBIT(mBitField, BF_PARSER_CREATING)))) {
      AddedToRadioGroup();
      UpdateValueMissingValidityStateForRadio(false);
      states |= NS_EVENT_STATE_VALID | NS_EVENT_STATE_INVALID |
                NS_EVENT_STATE_REQUIRED | NS_EVENT_STATE_OPTIONAL;
    }

    
    
    
    
    if (aName == nsGkAtoms::value &&
        !GetValueChanged() && GetValueMode() == VALUE_MODE_VALUE) {
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
      if (!aValue) {
        
        
        
        HandleTypeChange(kInputDefaultType->value);
      }

      UpdateBarredFromConstraintValidation();

      
      
      if (mInputData.mValue &&
          mType != NS_FORM_INPUT_EMAIL &&
          mType != NS_FORM_INPUT_TEXT &&
          mType != NS_FORM_INPUT_SEARCH &&
          mType != NS_FORM_INPUT_PASSWORD &&
          mType != NS_FORM_INPUT_TEL &&
          mType != NS_FORM_INPUT_URL &&
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

      
      
      
      
      
      
      states |= NS_EVENT_STATE_CHECKED |
                NS_EVENT_STATE_DEFAULT |
                NS_EVENT_STATE_BROKEN |
                NS_EVENT_STATE_USERDISABLED |
                NS_EVENT_STATE_SUPPRESSED |
                NS_EVENT_STATE_LOADING |
                NS_EVENT_STATE_MOZ_READONLY |
                NS_EVENT_STATE_MOZ_READWRITE |
                NS_EVENT_STATE_REQUIRED |
                NS_EVENT_STATE_OPTIONAL |
                NS_EVENT_STATE_VALID |
                NS_EVENT_STATE_INVALID |
                NS_EVENT_STATE_MOZ_UI_VALID |
                NS_EVENT_STATE_MOZ_UI_INVALID |
                NS_EVENT_STATE_INDETERMINATE |
                NS_EVENT_STATE_MOZ_PLACEHOLDER |
                NS_EVENT_STATE_MOZ_SUBMITINVALID;
    }

    if (mType == NS_FORM_INPUT_RADIO && aName == nsGkAtoms::required) {
      nsCOMPtr<nsIRadioGroupContainer> c = GetRadioGroupContainer();
      nsCOMPtr<nsIRadioGroupContainer_MOZILLA_2_0_BRANCH> container =
        do_QueryInterface(c);
      nsAutoString name;

      if (container && GetNameIfExists(name)) {
        container->RadioRequiredChanged(name, this);
      }
    }

    if (aName == nsGkAtoms::required || aName == nsGkAtoms::disabled ||
        aName == nsGkAtoms::readonly) {
      UpdateValueMissingValidityState();

      
      if (aName == nsGkAtoms::readonly || aName == nsGkAtoms::disabled) {
        UpdateBarredFromConstraintValidation();
      }

      states |= NS_EVENT_STATE_REQUIRED | NS_EVENT_STATE_OPTIONAL |
                NS_EVENT_STATE_VALID | NS_EVENT_STATE_INVALID |
                NS_EVENT_STATE_MOZ_UI_VALID | NS_EVENT_STATE_MOZ_UI_INVALID;
    } else if (MaxLengthApplies() && aName == nsGkAtoms::maxlength) {
      UpdateTooLongValidityState();
      states |= NS_EVENT_STATE_VALID | NS_EVENT_STATE_INVALID |
                NS_EVENT_STATE_MOZ_UI_VALID | NS_EVENT_STATE_MOZ_UI_INVALID;
    } else if (aName == nsGkAtoms::pattern) {
      UpdatePatternMismatchValidityState();
      states |= NS_EVENT_STATE_VALID | NS_EVENT_STATE_INVALID |
                NS_EVENT_STATE_MOZ_UI_VALID | NS_EVENT_STATE_MOZ_UI_INVALID;
    }

    if (aNotify) {
      nsIDocument* doc = GetCurrentDoc();

      if (aName == nsGkAtoms::type) {
        UpdateEditableState();
      } else if (IsSingleLineTextControl(PR_FALSE) && aName == nsGkAtoms::readonly) {
        UpdateEditableState();
        states |= NS_EVENT_STATE_MOZ_READONLY | NS_EVENT_STATE_MOZ_READWRITE;
      }

      if (doc && !states.IsEmpty()) {
        MOZ_AUTO_DOC_UPDATE(doc, UPDATE_CONTENT_STATE, PR_TRUE);
        doc->ContentStateChanged(this, states);
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
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Align, align)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Alt, alt)
NS_IMPL_ENUM_ATTR_DEFAULT_VALUE(nsHTMLInputElement, Autocomplete, autocomplete,
                                kInputDefaultAutocomplete->tag)
NS_IMPL_BOOL_ATTR(nsHTMLInputElement, Autofocus, autofocus)

NS_IMPL_BOOL_ATTR(nsHTMLInputElement, Disabled, disabled)
NS_IMPL_ACTION_ATTR(nsHTMLInputElement, FormAction, formaction)
NS_IMPL_ENUM_ATTR_DEFAULT_VALUE(nsHTMLInputElement, FormEnctype, formenctype,
                                kFormDefaultEnctype->tag)
NS_IMPL_ENUM_ATTR_DEFAULT_VALUE(nsHTMLInputElement, FormMethod, formmethod,
                                kFormDefaultMethod->tag)
NS_IMPL_BOOL_ATTR(nsHTMLInputElement, FormNoValidate, formnovalidate)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, FormTarget, formtarget)
NS_IMPL_BOOL_ATTR(nsHTMLInputElement, Multiple, multiple)
NS_IMPL_NON_NEGATIVE_INT_ATTR(nsHTMLInputElement, MaxLength, maxlength)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Name, name)
NS_IMPL_BOOL_ATTR(nsHTMLInputElement, ReadOnly, readonly)
NS_IMPL_BOOL_ATTR(nsHTMLInputElement, Required, required)
NS_IMPL_URI_ATTR(nsHTMLInputElement, Src, src)
NS_IMPL_INT_ATTR(nsHTMLInputElement, TabIndex, tabindex)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, UseMap, usemap)

NS_IMPL_UINT_ATTR_NON_ZERO_DEFAULT_VALUE(nsHTMLInputElement, Size, size, DEFAULT_COLS)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Pattern, pattern)
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
      frame->InvalidateFrameSubtree();
  }

  
  nsIDocument* document = GetCurrentDoc();
  if (document) {
    mozAutoDocUpdate upd(document, UPDATE_CONTENT_STATE, PR_TRUE);
    document->ContentStateChanged(this, NS_EVENT_STATE_INDETERMINATE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::SetIndeterminate(PRBool aValue)
{
  return SetIndeterminateInternal(aValue, PR_TRUE);
}

NS_IMETHODIMP
nsHTMLInputElement::GetValue(nsAString& aValue)
{
  return GetValueInternal(aValue);
}

nsresult
nsHTMLInputElement::GetValueInternal(nsAString& aValue) const
{
  nsTextEditorState* state = GetEditorState();
  if (state) {
    state->GetValue(aValue, PR_TRUE);
    return NS_OK;
  }

  if (mType == NS_FORM_INPUT_FILE) {
    if (nsContentUtils::IsCallerTrustedForCapability("UniversalFileRead")) {
      if (mFiles.Count()) {
        return mFiles[0]->GetMozFullPath(aValue);
      }
      else {
        aValue.Truncate();
      }
    } else {
      
      if (mFiles.Count() == 0 || NS_FAILED(mFiles[0]->GetName(aValue))) {
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

bool
nsHTMLInputElement::IsValueEmpty() const
{
  nsAutoString value;
  GetValueInternal(value);

  return value.IsEmpty();
}

NS_IMETHODIMP 
nsHTMLInputElement::SetValue(const nsAString& aValue)
{
  
  
  if (mType == NS_FORM_INPUT_FILE) {
    if (!aValue.IsEmpty()) {
      if (!nsContentUtils::IsCallerTrustedForCapability("UniversalFileRead")) {
        
        
        return NS_ERROR_DOM_SECURITY_ERR;
      }
      const PRUnichar *name = PromiseFlatString(aValue).get();
      return MozSetFileNameArray(&name, 1);
    }
    else {
      ClearFiles(true);
    }
  }
  else {
    SetValueInternal(aValue, PR_FALSE, PR_TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::GetList(nsIDOMHTMLElement** aValue)
{
  nsAutoString dataListId;
  GetAttr(kNameSpaceID_None, nsGkAtoms::list, dataListId);
  if (!dataListId.IsEmpty()) {
    nsIDocument* doc = GetCurrentDoc();

    if (doc) {
      Element* elem = doc->GetElementById(dataListId);

      if (elem && elem->IsHTML(nsGkAtoms::datalist)) {
        CallQueryInterface(elem, aValue);
        return NS_OK;
      }
    }
  }

  *aValue = nsnull;
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLInputElement::MozGetFileNameArray(PRUint32 *aLength, PRUnichar ***aFileNames)
{
  if (!nsContentUtils::IsCallerTrustedForCapability("UniversalFileRead")) {
    
    
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  *aLength = mFiles.Count();
  PRUnichar **ret =
    static_cast<PRUnichar **>(NS_Alloc(mFiles.Count() * sizeof(PRUnichar*)));
  
  for (PRInt32 i = 0; i <  mFiles.Count(); i++) {
    nsString str;
    mFiles[i]->GetMozFullPathInternal(str);
    ret[i] = NS_strdup(str.get());
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

  nsCOMArray<nsIDOMFile> files;
  for (PRUint32 i = 0; i < aLength; ++i) {
    nsCOMPtr<nsIFile> file;
    if (StringBeginsWith(nsDependentString(aFileNames[i]),
                         NS_LITERAL_STRING("file:"),
                         nsASCIICaseInsensitiveStringComparator())) {
      
      
      NS_GetFileFromURLSpec(NS_ConvertUTF16toUTF8(aFileNames[i]),
                            getter_AddRefs(file));
    }

    if (!file) {
      
      nsCOMPtr<nsILocalFile> localFile;
      NS_NewLocalFile(nsDependentString(aFileNames[i]),
                      PR_FALSE, getter_AddRefs(localFile));
      file = do_QueryInterface(localFile);
    }

    if (file) {
      nsCOMPtr<nsIDOMFile> domFile = new nsDOMFile(file);
      files.AppendObject(domFile);
    } else {
      continue; 
    }

  }

  SetFiles(files, true);

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
    const PRUnichar* name = PromiseFlatString(aValue).get();
    return MozSetFileNameArray(&name, 1);
  } else {
    SetValueInternal(aValue, PR_TRUE, PR_TRUE);
  }

  return nsContentUtils::DispatchTrustedEvent(GetOwnerDoc(),
                                              static_cast<nsIDOMHTMLInputElement*>(this),
                                              NS_LITERAL_STRING("input"), PR_TRUE,
                                              PR_TRUE);
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
nsHTMLInputElement::CreatePlaceholderNode()
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    NS_ENSURE_SUCCESS(state->CreatePlaceholderNode(), nsnull);
    return state->GetPlaceholderNode();
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
nsHTMLInputElement::GetDisplayFileName(nsAString& aValue) const
{
  if (GetOwnerDoc()->IsStaticDocument()) {
    aValue = mStaticDocFileList;
    return;
  }

  aValue.Truncate();
  for (PRUint32 i = 0; i < (PRUint32)mFiles.Count(); ++i) {
    nsString str;
    mFiles[i]->GetMozFullPathInternal(str);
    if (i == 0) {
      aValue.Append(str);
    }
    else {
      aValue.Append(NS_LITERAL_STRING(", ") + str);
    }
  }
}

void
nsHTMLInputElement::SetFiles(const nsCOMArray<nsIDOMFile>& aFiles,
                             bool aSetValueChanged)
{
  mFiles.Clear();
  mFiles.AppendObjects(aFiles);

  
  
  
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_FALSE);
  if (formControlFrame) {
    nsAutoString readableValue;
    GetDisplayFileName(readableValue);
    formControlFrame->SetFormProperty(nsGkAtoms::value, readableValue);
  }

  UpdateFileList();

  if (aSetValueChanged) {
    SetValueChanged(PR_TRUE);
  }

  UpdateAllValidityStates(PR_TRUE);
}

const nsCOMArray<nsIDOMFile>&
nsHTMLInputElement::GetFiles() const
{
  return mFiles;
}

nsresult
nsHTMLInputElement::UpdateFileList()
{
  if (mFileList) {
    mFileList->Clear();

    const nsCOMArray<nsIDOMFile>& files = GetFiles();
    for (PRUint32 i = 0; i < (PRUint32)files.Count(); ++i) {
      if (!mFileList->Append(files[i])) {
        return NS_ERROR_FAILURE;
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
    if (!GET_BOOLBIT(mBitField, BF_PARSER_CREATING)) {
      SanitizeValue(value);
    }

    if (aSetValueChanged) {
      SetValueChanged(PR_TRUE);
    }
    mInputData.mState->SetValue(value, aUserInput);

    if (PlaceholderApplies() &&
        HasAttr(kNameSpaceID_None, nsGkAtoms::placeholder)) {
      nsIDocument* doc = GetCurrentDoc();
      if (doc) {
        mozAutoDocUpdate upd(doc, UPDATE_CONTENT_STATE, PR_TRUE);
        doc->ContentStateChanged(this, NS_EVENT_STATE_MOZ_PLACEHOLDER);
      }
    }

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
  PRBool valueChangedBefore = GetValueChanged();

  SET_BOOLBIT(mBitField, BF_VALUE_CHANGED, aValueChanged);

  if (!aValueChanged) {
    if (!IsSingleLineTextControl(PR_FALSE)) {
      FreeData();
    }
  }

  if (valueChangedBefore != aValueChanged) {
    nsIDocument* doc = GetCurrentDoc();
    if (doc) {
      mozAutoDocUpdate upd(doc, UPDATE_CONTENT_STATE, PR_TRUE);
      doc->ContentStateChanged(this, NS_EVENT_STATE_MOZ_UI_VALID |
                                     NS_EVENT_STATE_MOZ_UI_INVALID);
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
      nsCOMPtr<nsIRadioVisitor> visitor =
        new nsRadioSetCheckedChangedVisitor(aCheckedChanged);
      VisitGroup(visitor, aNotify);
    }
  } else {
    SetCheckedChangedInternal(aCheckedChanged);
  }
}

void
nsHTMLInputElement::SetCheckedChangedInternal(PRBool aCheckedChanged)
{
  PRBool checkedChangedBefore = GetCheckedChanged();

  SET_BOOLBIT(mBitField, BF_CHECKED_CHANGED, aCheckedChanged);

  
  
  if (checkedChangedBefore != aCheckedChanged) {
    nsIDocument* document = GetCurrentDoc();
    if (document) {
      mozAutoDocUpdate upd(document, UPDATE_CONTENT_STATE, PR_TRUE);
      document->ContentStateChanged(this,
                                    NS_EVENT_STATE_MOZ_UI_VALID |
                                    NS_EVENT_STATE_MOZ_UI_INVALID);
    }
  }
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
      nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
      if (container) {
        nsAutoString name;
        if (GetNameIfExists(name)) {
          container->SetCurrentRadioButton(name, nsnull);
        }
      }
      
      
      
      SetCheckedInternal(PR_FALSE, aNotify);
    }
  } else {
    SetCheckedInternal(aChecked, aNotify);
  }

  return rv;
}

nsresult
nsHTMLInputElement::RadioSetChecked(PRBool aNotify)
{
  nsresult rv = NS_OK;

  
  
  
  nsCOMPtr<nsIDOMHTMLInputElement> currentlySelected = GetSelectedRadioButton();

  
  
  
  if (currentlySelected) {
    
    
    static_cast<nsHTMLInputElement*>
               (static_cast<nsIDOMHTMLInputElement*>(currentlySelected))->SetCheckedInternal(PR_FALSE, PR_TRUE);
  }

  
  
  
  nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
  nsAutoString name;
  if (container && GetNameIfExists(name)) {
    rv = container->SetCurrentRadioButton(name, this);
  }

  
  
  
  if (NS_SUCCEEDED(rv)) {
    SetCheckedInternal(PR_TRUE, aNotify);
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
  } else if (mForm->HasSingleTextControl() &&
             (mForm->HasAttr(kNameSpaceID_None, nsGkAtoms::novalidate) ||
              mForm->CheckValidFormSubmission())) {
    
    
    
    
    nsRefPtr<nsHTMLFormElement> form(mForm);
    nsFormEvent event(PR_TRUE, NS_FORM_SUBMIT);
    nsEventStatus status  = nsEventStatus_eIgnore;
    shell->HandleDOMEventWithTarget(mForm, &event, &status);
  }

  return NS_OK;
}

void
nsHTMLInputElement::SetCheckedInternal(PRBool aChecked, PRBool aNotify)
{
  
  
  
  SET_BOOLBIT(mBitField, BF_CHECKED, aChecked);

  
  
  
  if (mType == NS_FORM_INPUT_CHECKBOX || mType == NS_FORM_INPUT_RADIO) {
    nsIFrame* frame = GetPrimaryFrame();
    if (frame) {
      frame->InvalidateFrameSubtree();
    }
  }

  
  
  if (aNotify) {
    nsIDocument* document = GetCurrentDoc();
    if (document) {
      mozAutoDocUpdate upd(document, UPDATE_CONTENT_STATE, aNotify);
      document->ContentStateChanged(this, NS_EVENT_STATE_CHECKED);
    }
  }

  if (mType == NS_FORM_INPUT_CHECKBOX) {
    UpdateAllValidityStates(aNotify);
  }

  if (mType == NS_FORM_INPUT_RADIO) {
    UpdateValueMissingValidityState();
  }
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
  if (mType == NS_FORM_INPUT_FILE)
    FireAsyncClickHandler();

  return nsGenericHTMLElement::Click();
}

NS_IMETHODIMP
nsHTMLInputElement::FireAsyncClickHandler()
{
  nsCOMPtr<nsIRunnable> event = new AsyncClickHandler(this);
  return NS_DispatchToMainThread(event);
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
  if (IsDisabled()) {
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
          
          
          
          mForm->OnSubmitClickBegin(this);
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

  if (aVisitor.mEvent->message == NS_FOCUS_CONTENT ||
      aVisitor.mEvent->message == NS_BLUR_CONTENT) {
    nsEventStates states;

    if (aVisitor.mEvent->message == NS_FOCUS_CONTENT) {
      UpdateValidityUIBits(true);

      
      
    } else { 
      UpdateValidityUIBits(false);
      states |= NS_EVENT_STATE_MOZ_UI_VALID | NS_EVENT_STATE_MOZ_UI_INVALID;
    }

    if (PlaceholderApplies() &&
        HasAttr(kNameSpaceID_None, nsGkAtoms::placeholder)) {
        
      
      states |= NS_EVENT_STATE_MOZ_PLACEHOLDER;
    }

    nsIDocument* doc = GetCurrentDoc();
    if (doc) {
      MOZ_AUTO_DOC_UPDATE(doc, UPDATE_CONTENT_STATE, PR_TRUE);
      doc->ContentStateChanged(this, states);
    }
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
      nsContentUtils::DispatchTrustedEvent(GetOwnerDoc(),
                                           static_cast<nsIDOMHTMLInputElement*>(this),
                                           NS_LITERAL_STRING("change"), PR_TRUE,
                                           PR_FALSE);
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
                aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
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
               IsSingleLineTextControl(PR_FALSE, mType)) {
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

            
            
            
            
            
            if (presShell && (event.message != NS_FORM_SUBMIT ||
                              mForm->HasAttr(kNameSpaceID_None, nsGkAtoms::novalidate) ||
                              
                              
                              HasAttr(kNameSpaceID_None, nsGkAtoms::formnovalidate) ||
                              mForm->CheckValidFormSubmission())) {
              
              nsRefPtr<nsHTMLFormElement> form(mForm);
              presShell->HandleDOMEventWithTarget(mForm, &event, &status);
              aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
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

  
  
  UpdateValueMissingValidityState();

  
  
  
  UpdateBarredFromConstraintValidation();

  return rv;
}

void
nsHTMLInputElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  
  
  
  
  
  if (!mForm && mType == NS_FORM_INPUT_RADIO) {
    WillRemoveFromRadioGroup();
  }

  nsGenericHTMLFormElement::UnbindFromTree(aDeep, aNullParent);

  
  
  UpdateValueMissingValidityState();
  
  UpdateBarredFromConstraintValidation();
}

void
nsHTMLInputElement::HandleTypeChange(PRUint8 aNewType)
{
  ValueModeType aOldValueMode = GetValueMode();
  nsAutoString aOldValue;

  if (aOldValueMode == VALUE_MODE_VALUE && !GET_BOOLBIT(mBitField, BF_PARSER_CREATING)) {
    GetValue(aOldValue);
  }

  
  bool isNewTypeSingleLine = IsSingleLineTextControl(PR_FALSE, aNewType);
  bool isCurrentTypeSingleLine = IsSingleLineTextControl(PR_FALSE, mType);

  if (isNewTypeSingleLine && !isCurrentTypeSingleLine) {
    FreeData();
    mInputData.mState = new nsTextEditorState(this);
    NS_ADDREF(mInputData.mState);
  } else if (isCurrentTypeSingleLine && !isNewTypeSingleLine) {
    FreeData();
  }

  mType = aNewType;

  if (!GET_BOOLBIT(mBitField, BF_PARSER_CREATING)) {
    



    switch (GetValueMode()) {
      case VALUE_MODE_DEFAULT:
      case VALUE_MODE_DEFAULT_ON:
        
        
        
        if (aOldValueMode == VALUE_MODE_VALUE && !aOldValue.IsEmpty()) {
          SetAttr(kNameSpaceID_None, nsGkAtoms::value, aOldValue, PR_TRUE);
        }
        break;
      case VALUE_MODE_VALUE:
        
        
        
        {
          nsAutoString value;
          if (aOldValueMode != VALUE_MODE_VALUE) {
            GetAttr(kNameSpaceID_None, nsGkAtoms::value, value);
          } else {
            
            GetValue(value);
          }
          SetValueInternal(value, PR_FALSE, PR_FALSE);
        }
        break;
      case VALUE_MODE_FILENAME:
      default:
        
        
        break;
    }
  }

  
  UpdateAllValidityStates(PR_FALSE);
}

void
nsHTMLInputElement::SanitizeValue(nsAString& aValue)
{
  NS_ASSERTION(!GET_BOOLBIT(mBitField, BF_PARSER_CREATING),
               "The element parsing should be finished!");

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
    case NS_FORM_INPUT_URL:
      {
        PRUnichar crlf[] = { PRUnichar('\r'), PRUnichar('\n'), 0 };
        aValue.StripChars(crlf);

        aValue = nsContentUtils::TrimWhitespace<nsContentUtils::IsHTMLWhitespace>(aValue);
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
          
          
          
          ClearFiles(false);
        }

        HandleTypeChange(newType);
      }

      return success;
    }
    if (aAttribute == nsGkAtoms::width) {
      return aResult.ParseSpecialIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::height) {
      return aResult.ParseSpecialIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::maxlength) {
      return aResult.ParseNonNegativeIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::size) {
      return aResult.ParsePositiveIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::border) {
      return aResult.ParseIntWithBounds(aValue, 0);
    }
    if (aAttribute == nsGkAtoms::align) {
      return ParseAlignValue(aValue, aResult);
    }
    if (aAttribute == nsGkAtoms::formmethod) {
      return aResult.ParseEnumValue(aValue, kFormMethodTable, PR_FALSE);
    }
    if (aAttribute == nsGkAtoms::formenctype) {
      return aResult.ParseEnumValue(aValue, kFormEnctypeTable, PR_FALSE);
    }
    if (aAttribute == nsGkAtoms::autocomplete) {
      return aResult.ParseEnumValue(aValue, kInputAutocompleteTable, PR_FALSE);
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
  } else if (PlaceholderApplies() && aAttribute == nsGkAtoms::placeholder) {
    NS_UpdateHint(retval, NS_STYLE_HINT_FRAMECHANGE);
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
    if (textControlFrame) {
      rv = textControlFrame->SetSelectionRange(aSelectionStart, aSelectionEnd);
      if (NS_SUCCEEDED(rv)) {
        rv = textControlFrame->ScrollSelectionIntoView();
      }
    }
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
    if (textControlFrame) {
      rv = textControlFrame->SetSelectionEnd(aSelectionEnd);
      if (NS_SUCCEEDED(rv)) {
        rv = textControlFrame->ScrollSelectionIntoView();
      }
    }
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
  NS_ASSERTION(GetValueMode() == VALUE_MODE_VALUE,
               "GetValueMode() should return VALUE_MODE_VALUE!");

  
  
  nsAutoString resetVal;
  GetDefaultValue(resetVal);

  
  return SetValueInternal(resetVal, PR_FALSE, PR_FALSE);
}

NS_IMETHODIMP
nsHTMLInputElement::Reset()
{
  
  SetCheckedChanged(PR_FALSE);
  SetValueChanged(PR_FALSE);

  switch (GetValueMode()) {
    case VALUE_MODE_VALUE:
      return SetDefaultValueAsValue();
    case VALUE_MODE_DEFAULT_ON:
      PRBool resetVal;
      GetDefaultChecked(&resetVal);
      return DoSetChecked(resetVal, PR_TRUE, PR_FALSE);
    case VALUE_MODE_FILENAME:
      ClearFiles(false);
      return NS_OK;
    case VALUE_MODE_DEFAULT:
    default:
      return NS_OK;
  }
}

NS_IMETHODIMP
nsHTMLInputElement::SubmitNamesValues(nsFormSubmission* aFormSubmission)
{
  nsresult rv = NS_OK;

  
  
  
  
  
  if (IsDisabled() || mType == NS_FORM_INPUT_RESET ||
      mType == NS_FORM_INPUT_BUTTON ||
      ((mType == NS_FORM_INPUT_SUBMIT || mType == NS_FORM_INPUT_IMAGE) &&
       aFormSubmission->GetOriginatingElement() != this) ||
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
    

    const nsCOMArray<nsIDOMFile>& files = GetFiles();

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
    case NS_FORM_INPUT_URL:
    case NS_FORM_INPUT_HIDDEN:
      {
        if (GetValueChanged()) {
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
        if (mFiles.Count()) {
          inputState = new nsHTMLInputElementState();
          if (!inputState) {
            return NS_ERROR_OUT_OF_MEMORY;
          }

          inputState->SetFiles(mFiles);
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
      
      
      state->SetDisabled(HasAttr(kNameSpaceID_None, nsGkAtoms::disabled));
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

  
  if (GetValueMode() == VALUE_MODE_VALUE) {
    nsAutoString aValue;
    GetValue(aValue);
    SetValueInternal(aValue, PR_FALSE, PR_FALSE);
  }

  SET_BOOLBIT(mBitField, BF_SHOULD_INIT_CHECKED, PR_FALSE);
}

nsEventStates
nsHTMLInputElement::IntrinsicState() const
{
  
  
  
  nsEventStates state = nsGenericHTMLFormElement::IntrinsicState();
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

  if (DoesRequiredApply() && HasAttr(kNameSpaceID_None, nsGkAtoms::required)) {
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
           (GET_BOOLBIT(mBitField, BF_CAN_SHOW_INVALID_UI) &&
            ShouldShowValidityUI()))) {
        state |= NS_EVENT_STATE_MOZ_UI_INVALID;
      }
    }

    
    
    
    
    
    
    
    
    
    if ((!mForm || !mForm->HasAttr(kNameSpaceID_None, nsGkAtoms::novalidate)) &&
        (GET_BOOLBIT(mBitField, BF_CAN_SHOW_VALID_UI) && ShouldShowValidityUI() &&
         (IsValid() || (!state.HasState(NS_EVENT_STATE_MOZ_UI_INVALID) &&
                        !GET_BOOLBIT(mBitField, BF_CAN_SHOW_INVALID_UI))))) {
      state |= NS_EVENT_STATE_MOZ_UI_VALID;
    }
  }

  if (PlaceholderApplies() && HasAttr(kNameSpaceID_None, nsGkAtoms::placeholder) &&
      !nsContentUtils::IsFocusedContent((nsIContent*)(this)) &&
      IsValueEmpty()) {
    state |= NS_EVENT_STATE_MOZ_PLACEHOLDER;
  }

  if (mForm && !mForm->GetValidity() && IsSubmitControl()) {
    state |= NS_EVENT_STATE_MOZ_SUBMITINVALID;
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
      case NS_FORM_INPUT_URL:
      case NS_FORM_INPUT_HIDDEN:
        {
          SetValueInternal(inputState->GetValue(), PR_FALSE, PR_TRUE);
          break;
        }
      case NS_FORM_INPUT_FILE:
        {
          const nsCOMArray<nsIDOMFile>& files = inputState->GetFiles();
          SetFiles(files, true);
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
nsHTMLInputElement::AddedToRadioGroup()
{
  
  PRBool notify = !GET_BOOLBIT(mBitField, BF_PARSER_CREATING);

  
  
  
  
  if (!mForm && !(IsInDoc() && GetParent())) {
    return;
  }

  
  
  
  
  if (GetChecked()) {
    
    
    
    
    
    
    
    RadioSetChecked(notify);
  }

  
  
  
  
  bool checkedChanged = GET_BOOLBIT(mBitField, BF_CHECKED_CHANGED);

  nsCOMPtr<nsIRadioVisitor> visitor =
    new nsRadioGetCheckedChangedVisitor(&checkedChanged, this);
  VisitGroup(visitor, notify);

  SetCheckedChangedInternal(checkedChanged);

  
  
  
  nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
  if (container) {
    nsAutoString name;
    if (GetNameIfExists(name)) {
      container->AddToRadioGroup(name, static_cast<nsIFormControl*>(this));

      
      
      nsCOMPtr<nsIRadioGroupContainer_MOZILLA_2_0_BRANCH> container2 =
        do_QueryInterface(container);
      SetValidityState(VALIDITY_STATE_VALUE_MISSING,
                       container2->GetValueMissingState(name));
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

    UpdateValueMissingValidityStateForRadio(true);

    container->RemoveFromRadioGroup(name,
                                    static_cast<nsIFormControl*>(this));
  }
}

PRBool
nsHTMLInputElement::IsHTMLFocusable(PRBool aWithMouse, PRBool *aIsFocusable, PRInt32 *aTabIndex)
{
  if (nsGenericHTMLFormElement::IsHTMLFocusable(aWithMouse, aIsFocusable, aTabIndex)) {
    return PR_TRUE;
  }

  if (IsDisabled()) {
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
      aVisitor->Visit(this);
    }
  } else {
    aVisitor->Visit(this);
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
    case NS_FORM_INPUT_URL:
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
  return !IsDisabled() && GetCurrentDoc() &&
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
    case NS_FORM_INPUT_URL:
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
    case NS_FORM_INPUT_URL:
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
nsHTMLInputElement::DoesPatternApply() const
{
  return IsSingleLineTextControl(PR_FALSE);
}



NS_IMETHODIMP
nsHTMLInputElement::SetCustomValidity(const nsAString& aError)
{
  nsIConstraintValidation::SetCustomValidity(aError);

  nsIDocument* doc = GetCurrentDoc();
  if (doc) {
    MOZ_AUTO_DOC_UPDATE(doc, UPDATE_CONTENT_STATE, PR_TRUE);
    doc->ContentStateChanged(this, NS_EVENT_STATE_INVALID |
                                   NS_EVENT_STATE_VALID |
                                   NS_EVENT_STATE_MOZ_UI_INVALID |
                                   NS_EVENT_STATE_MOZ_UI_VALID);
  }

  return NS_OK;
}

PRBool
nsHTMLInputElement::IsTooLong()
{
  if (!MaxLengthApplies() ||
      !HasAttr(kNameSpaceID_None, nsGkAtoms::maxlength) ||
      !GetValueChanged()) {
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
nsHTMLInputElement::IsValueMissing() const
{
  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::required) ||
      !DoesRequiredApply()) {
    return PR_FALSE;
  }

  if (GetValueMode() == VALUE_MODE_VALUE) {
    if (!IsMutable()) {
      return PR_FALSE;
    }

    return IsValueEmpty();
  }

  switch (mType)
  {
    case NS_FORM_INPUT_CHECKBOX:
      return !GetChecked();
    case NS_FORM_INPUT_FILE:
      {
        const nsCOMArray<nsIDOMFile>& files = GetFiles();
        return !files.Count();
      }
    default:
      return PR_FALSE;
  }
}

PRBool
nsHTMLInputElement::HasTypeMismatch() const
{
  if (mType != NS_FORM_INPUT_EMAIL && mType != NS_FORM_INPUT_URL) {
    return PR_FALSE;
  }

  nsAutoString value;
  NS_ENSURE_SUCCESS(GetValueInternal(value), PR_FALSE);

  if (value.IsEmpty()) {
    return PR_FALSE;
  }

  if (mType == NS_FORM_INPUT_EMAIL) {
    return HasAttr(kNameSpaceID_None, nsGkAtoms::multiple)
             ? !IsValidEmailAddressList(value) : !IsValidEmailAddress(value);
  } else if (mType == NS_FORM_INPUT_URL) {
    










    nsCOMPtr<nsIIOService> ioService = do_GetIOService();
    nsCOMPtr<nsIURI> uri;

    return !NS_SUCCEEDED(ioService->NewURI(NS_ConvertUTF16toUTF8(value), nsnull,
                                           nsnull, getter_AddRefs(uri)));
  }

  return PR_FALSE;
}

PRBool
nsHTMLInputElement::HasPatternMismatch() const
{
  nsAutoString pattern;
  if (!DoesPatternApply() ||
      !GetAttr(kNameSpaceID_None, nsGkAtoms::pattern, pattern)) {
    return PR_FALSE;
  }

  nsAutoString value;
  NS_ENSURE_SUCCESS(GetValueInternal(value), PR_FALSE);

  if (value.IsEmpty()) {
    return PR_FALSE;
  }

  nsIDocument* doc = GetOwnerDoc();
  if (!doc) {
    return PR_FALSE;
  }

  return !IsPatternMatching(value, pattern, doc);
}

void
nsHTMLInputElement::UpdateTooLongValidityState()
{
  
#if 0
  SetValidityState(VALIDITY_STATE_TOO_LONG, IsTooLong());
#endif
}

void
nsHTMLInputElement::UpdateValueMissingValidityStateForRadio(bool aIgnoreSelf)
{
  PRBool notify = !GET_BOOLBIT(mBitField, BF_PARSER_CREATING);
  nsCOMPtr<nsIDOMHTMLInputElement> selection = GetSelectedRadioButton();

  
  
  bool selected = selection ? true
                            : aIgnoreSelf ? false : GetChecked();
  bool required = aIgnoreSelf ? false
                              : HasAttr(kNameSpaceID_None, nsGkAtoms::required);
  bool valueMissing = false;

  nsCOMPtr<nsIRadioGroupContainer> c = GetRadioGroupContainer();
  nsCOMPtr<nsIRadioGroupContainer_MOZILLA_2_0_BRANCH> container =
    do_QueryInterface(c);
  nsAutoString name;
  GetNameIfExists(name);

  
  
  if (!required && container && !name.IsEmpty()) {
    required = (aIgnoreSelf && HasAttr(kNameSpaceID_None, nsGkAtoms::required))
                 ? container->GetRequiredRadioCount(name) - 1
                 : container->GetRequiredRadioCount(name);
  }

  valueMissing = required && !selected;

  if (container && !name.IsEmpty()) {
    if (container->GetValueMissingState(name) != valueMissing) {
      container->SetValueMissingState(name, valueMissing);

      SetValidityState(VALIDITY_STATE_VALUE_MISSING, valueMissing);

      nsCOMPtr<nsIRadioVisitor> visitor =
        new nsRadioSetValueMissingState(this, valueMissing, notify);
      VisitGroup(visitor, notify);
    }
  } else {
    SetValidityState(VALIDITY_STATE_VALUE_MISSING, valueMissing);
  }
}

void
nsHTMLInputElement::UpdateValueMissingValidityState()
{
  if (mType == NS_FORM_INPUT_RADIO) {
    UpdateValueMissingValidityStateForRadio(false);
    return;
  }

  SetValidityState(VALIDITY_STATE_VALUE_MISSING, IsValueMissing());
}

void
nsHTMLInputElement::UpdateTypeMismatchValidityState()
{
    SetValidityState(VALIDITY_STATE_TYPE_MISMATCH, HasTypeMismatch());
}

void
nsHTMLInputElement::UpdatePatternMismatchValidityState()
{
  SetValidityState(VALIDITY_STATE_PATTERN_MISMATCH, HasPatternMismatch());
}

void
nsHTMLInputElement::UpdateAllValidityStates(PRBool aNotify)
{
  PRBool validBefore = IsValid();
  UpdateTooLongValidityState();
  UpdateValueMissingValidityState();
  UpdateTypeMismatchValidityState();
  UpdatePatternMismatchValidityState();

  if (validBefore != IsValid() && aNotify) {
    nsIDocument* doc = GetCurrentDoc();
    if (doc) {
      MOZ_AUTO_DOC_UPDATE(doc, UPDATE_CONTENT_STATE, PR_TRUE);
      doc->ContentStateChanged(this,
                               NS_EVENT_STATE_VALID | NS_EVENT_STATE_INVALID |
                               NS_EVENT_STATE_MOZ_UI_VALID |
                               NS_EVENT_STATE_MOZ_UI_INVALID);
    }
  }
}

void
nsHTMLInputElement::UpdateBarredFromConstraintValidation()
{
  SetBarredFromConstraintValidation(mType == NS_FORM_INPUT_HIDDEN ||
                                    mType == NS_FORM_INPUT_BUTTON ||
                                    mType == NS_FORM_INPUT_RESET ||
                                    mType == NS_FORM_INPUT_SUBMIT ||
                                    mType == NS_FORM_INPUT_IMAGE ||
                                    HasAttr(kNameSpaceID_None, nsGkAtoms::readonly) ||
                                    IsDisabled());
}

nsresult
nsHTMLInputElement::GetValidationMessage(nsAString& aValidationMessage,
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
      break;
    }
    case VALIDITY_STATE_VALUE_MISSING:
    {
      nsXPIDLString message;
      nsCAutoString key;
      switch (mType)
      {
        case NS_FORM_INPUT_FILE:
          key.Assign("FormValidationFileMissing");
          break;
        case NS_FORM_INPUT_CHECKBOX:
          key.Assign("FormValidationCheckboxMissing");
          break;
        case NS_FORM_INPUT_RADIO:
          key.Assign("FormValidationRadioMissing");
          break;
        default:
          key.Assign("FormValidationValueMissing");
      }
      rv = nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                              key.get(), message);
      aValidationMessage = message;
      break;
    }
    case VALIDITY_STATE_TYPE_MISMATCH:
    {
      nsXPIDLString message;
      nsCAutoString key;
      if (mType == NS_FORM_INPUT_EMAIL) {
        key.AssignLiteral("FormValidationInvalidEmail");
      } else if (mType == NS_FORM_INPUT_URL) {
        key.AssignLiteral("FormValidationInvalidURL");
      } else {
        return NS_ERROR_UNEXPECTED;
      }
      rv = nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                              key.get(), message);
      aValidationMessage = message;
      break;
    }
    case VALIDITY_STATE_PATTERN_MISMATCH:
    {
      nsXPIDLString message;
      nsAutoString title;
      GetAttr(kNameSpaceID_None, nsGkAtoms::title, title);
      if (title.IsEmpty()) {
        rv = nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                "FormValidationPatternMismatch",
                                                message);
      } else {
        if (title.Length() > nsIConstraintValidation::sContentSpecifiedMaxLengthMessage) {
          title.Truncate(nsIConstraintValidation::sContentSpecifiedMaxLengthMessage);
        }
        const PRUnichar* params[] = { title.get() };
        rv = nsContentUtils::FormatLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                   "FormValidationPatternMismatchWithTitle",
                                                   params, 1, message);
      }
      aValidationMessage = message;
      break;
    }
    default:
      rv = nsIConstraintValidation::GetValidationMessage(aValidationMessage, aType);
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

  
  for (; i < length; ++i) {
    PRUnichar c = aValue[i];

    if (c == '.') {
      
      if (aValue[i-1] == '.') {
        return PR_FALSE;
      }
    } else if (!(nsCRT::IsAsciiAlpha(c) || nsCRT::IsAsciiDigit(c) ||
                 c == '-')) {
      
      return PR_FALSE;
    }
  }

  return PR_TRUE;
}


PRBool
nsHTMLInputElement::IsPatternMatching(nsAString& aValue, nsAString& aPattern,
                                      nsIDocument* aDocument)
{
  NS_ASSERTION(aDocument, "aDocument should be a valid pointer (not null)");
  NS_ENSURE_TRUE(aDocument->GetScriptGlobalObject(), PR_TRUE);

  JSContext* ctx = (JSContext*) aDocument->GetScriptGlobalObject()->
                                  GetContext()->GetNativeContext();
  NS_ENSURE_TRUE(ctx, PR_TRUE);

  JSAutoRequest ar(ctx);

  
  aPattern.Insert(NS_LITERAL_STRING("^(?:"), 0);
  aPattern.Append(NS_LITERAL_STRING(")$"));

  JSObject* re = JS_NewUCRegExpObjectNoStatics(ctx, reinterpret_cast<jschar*>
                                                 (aPattern.BeginWriting()),
                                                aPattern.Length(), 0);
  NS_ENSURE_TRUE(re, PR_TRUE);

  jsval rval = JSVAL_NULL;
  size_t idx = 0;
  JSBool res;

  res = JS_ExecuteRegExpNoStatics(ctx, re, reinterpret_cast<jschar*>
                                    (aValue.BeginWriting()),
                                  aValue.Length(), &idx, JS_TRUE, &rval);

  return res == JS_FALSE || rval != JSVAL_NULL;
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
    
    
    if (!GET_BOOLBIT(mBitField, BF_PARSER_CREATING)) {
      SanitizeValue(aValue);
    }
  }
}

NS_IMETHODIMP_(PRBool)
nsHTMLInputElement::ValueChanged() const
{
  return GetValueChanged();
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

NS_IMETHODIMP_(void)
nsHTMLInputElement::OnValueChanged(PRBool aNotify)
{
  UpdateAllValidityStates(aNotify);

  
  
  if (aNotify && PlaceholderApplies()
      && HasAttr(kNameSpaceID_None, nsGkAtoms::placeholder)
      && !nsContentUtils::IsFocusedContent((nsIContent*)(this))) {
    nsIDocument* doc = GetCurrentDoc();
    if (doc) {
      MOZ_AUTO_DOC_UPDATE(doc, UPDATE_CONTENT_STATE, PR_TRUE);
      doc->ContentStateChanged(this, NS_EVENT_STATE_MOZ_PLACEHOLDER);
    }
  }
}

void
nsHTMLInputElement::FieldSetDisabledChanged(nsEventStates aStates, PRBool aNotify)
{
  UpdateValueMissingValidityState();
  UpdateBarredFromConstraintValidation();

  aStates |= NS_EVENT_STATE_VALID | NS_EVENT_STATE_INVALID |
             NS_EVENT_STATE_MOZ_UI_VALID | NS_EVENT_STATE_MOZ_UI_INVALID;
  nsGenericHTMLFormElement::FieldSetDisabledChanged(aStates, aNotify);
}

PRInt32
nsHTMLInputElement::GetFilterFromAccept()
{
  NS_ASSERTION(HasAttr(kNameSpaceID_None, nsGkAtoms::accept),
               "You should not call GetFileFiltersFromAccept if the element"
               " has no accept attribute!");

  PRInt32 filter = 0;
  nsAutoString accept;
  GetAttr(kNameSpaceID_None, nsGkAtoms::accept, accept);

  nsCharSeparatedTokenizerTemplate<nsContentUtils::IsHTMLWhitespace>
    tokenizer(accept, ',');

  while (tokenizer.hasMoreTokens()) {
    const nsDependentSubstring token = tokenizer.nextToken();

    PRInt32 tokenFilter = 0;
    if (token.EqualsLiteral("image/*")) {
      tokenFilter = nsIFilePicker::filterImages;
    } else if (token.EqualsLiteral("audio/*")) {
      tokenFilter = nsIFilePicker::filterAudio;
    } else if (token.EqualsLiteral("video/*")) {
      tokenFilter = nsIFilePicker::filterVideo;
    }

    if (tokenFilter) {
      
      
      if (filter && filter != tokenFilter) {
        return 0;
      }
      filter = tokenFilter;
    }
  }

  return filter;
}

void
nsHTMLInputElement::UpdateValidityUIBits(bool aIsFocused)
{
  if (aIsFocused) {
    
    
    SET_BOOLBIT(mBitField, BF_CAN_SHOW_INVALID_UI,
                !IsValid() && ShouldShowValidityUI());

    
    
    SET_BOOLBIT(mBitField, BF_CAN_SHOW_VALID_UI, ShouldShowValidityUI());
  } else {
    SET_BOOLBIT(mBitField, BF_CAN_SHOW_INVALID_UI, PR_TRUE);
    SET_BOOLBIT(mBitField, BF_CAN_SHOW_VALID_UI, PR_TRUE);
  }
}

