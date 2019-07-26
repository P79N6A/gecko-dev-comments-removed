




#include "mozilla/DebugOnly.h"

#include "nsHTMLInputElement.h"
#include "nsAttrValueInlines.h"

#include "nsIDOMHTMLInputElement.h"
#include "nsITextControlElement.h"
#include "nsIDOMNSEditableElement.h"
#include "nsIRadioVisitor.h"
#include "nsIPhonetic.h"

#include "nsIControllers.h"
#include "nsIStringBundle.h"
#include "nsFocusManager.h"
#include "nsPIDOMWindow.h"
#include "nsContentCID.h"
#include "nsIComponentManager.h"
#include "nsIDOMHTMLFormElement.h"
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
#include "nsError.h"
#include "nsIEditor.h"
#include "nsGUIEvent.h"
#include "nsIIOService.h"
#include "nsDocument.h"
#include "nsAttrValueOrString.h"

#include "nsPresState.h"
#include "nsIDOMEvent.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMHTMLCollection.h"
#include "nsLinebreakConverter.h" 
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsEventDispatcher.h"
#include "nsLayoutUtils.h"

#include "nsIDOMMutationEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsMutationEvent.h"
#include "nsEventListenerManager.h"

#include "nsRuleData.h"
#include <algorithm>


#include "nsIRadioGroupContainer.h"


#include "nsIFile.h"
#include "nsNetUtil.h"
#include "nsDOMFile.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIContentPrefService.h"
#include "nsIMIMEService.h"
#include "nsIObserverService.h"
#include "nsIPopupWindowManager.h"
#include "nsGlobalWindow.h"


#include "nsImageLoadingContent.h"
#include "imgRequestProxy.h"

#include "mozAutoDocUpdate.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentUtils.h"
#include "mozilla/dom/DirectionalityUtils.h"
#include "nsRadioVisitor.h"

#include "mozilla/LookAndFeel.h"
#include "mozilla/Util.h" 
#include "mozilla/Preferences.h"
#include "mozilla/MathAlgorithms.h"

#include "nsIIDNService.h"

#include <limits>


#include "jsapi.h"

using namespace mozilla;
using namespace mozilla::dom;



static NS_DEFINE_CID(kXULControllersCID,  NS_XULCONTROLLERS_CID);


#define NS_OUTER_ACTIVATE_EVENT   (1 << 9)
#define NS_ORIGINAL_CHECKED_VALUE (1 << 10)
#define NS_NO_CONTENT_DISPATCH    (1 << 11)
#define NS_ORIGINAL_INDETERMINATE_VALUE (1 << 12)
#define NS_CONTROL_TYPE(bits)  ((bits) & ~( \
  NS_OUTER_ACTIVATE_EVENT | NS_ORIGINAL_CHECKED_VALUE | NS_NO_CONTENT_DISPATCH | \
  NS_ORIGINAL_INDETERMINATE_VALUE))



static int32_t gSelectTextFieldOnFocus;
UploadLastDir* nsHTMLInputElement::gUploadLastDir;

static const nsAttrValue::EnumTable kInputTypeTable[] = {
  { "button", NS_FORM_INPUT_BUTTON },
  { "checkbox", NS_FORM_INPUT_CHECKBOX },
  { "date", NS_FORM_INPUT_DATE },
  { "email", NS_FORM_INPUT_EMAIL },
  { "file", NS_FORM_INPUT_FILE },
  { "hidden", NS_FORM_INPUT_HIDDEN },
  { "reset", NS_FORM_INPUT_RESET },
  { "image", NS_FORM_INPUT_IMAGE },
  { "number", NS_FORM_INPUT_NUMBER },
  { "password", NS_FORM_INPUT_PASSWORD },
  { "radio", NS_FORM_INPUT_RADIO },
  { "search", NS_FORM_INPUT_SEARCH },
  { "submit", NS_FORM_INPUT_SUBMIT },
  { "tel", NS_FORM_INPUT_TEL },
  { "text", NS_FORM_INPUT_TEXT },
  { "time", NS_FORM_INPUT_TIME },
  { "url", NS_FORM_INPUT_URL },
  { 0 }
};


static const nsAttrValue::EnumTable* kInputDefaultType = &kInputTypeTable[14];

static const uint8_t NS_INPUT_AUTOCOMPLETE_OFF     = 0;
static const uint8_t NS_INPUT_AUTOCOMPLETE_ON      = 1;
static const uint8_t NS_INPUT_AUTOCOMPLETE_DEFAULT = 2;

static const nsAttrValue::EnumTable kInputAutocompleteTable[] = {
  { "", NS_INPUT_AUTOCOMPLETE_DEFAULT },
  { "on", NS_INPUT_AUTOCOMPLETE_ON },
  { "off", NS_INPUT_AUTOCOMPLETE_OFF },
  { 0 }
};


static const nsAttrValue::EnumTable* kInputDefaultAutocomplete = &kInputAutocompleteTable[0];

static const uint8_t NS_INPUT_INPUTMODE_AUTO              = 0;
static const uint8_t NS_INPUT_INPUTMODE_NUMERIC           = 1;
static const uint8_t NS_INPUT_INPUTMODE_DIGIT             = 2;
static const uint8_t NS_INPUT_INPUTMODE_UPPERCASE         = 3;
static const uint8_t NS_INPUT_INPUTMODE_LOWERCASE         = 4;
static const uint8_t NS_INPUT_INPUTMODE_TITLECASE         = 5;
static const uint8_t NS_INPUT_INPUTMODE_AUTOCAPITALIZED   = 6;

static const nsAttrValue::EnumTable kInputInputmodeTable[] = {
  { "auto", NS_INPUT_INPUTMODE_AUTO },
  { "numeric", NS_INPUT_INPUTMODE_NUMERIC },
  { "digit", NS_INPUT_INPUTMODE_DIGIT },
  { "uppercase", NS_INPUT_INPUTMODE_UPPERCASE },
  { "lowercase", NS_INPUT_INPUTMODE_LOWERCASE },
  { "titlecase", NS_INPUT_INPUTMODE_TITLECASE },
  { "autocapitalized", NS_INPUT_INPUTMODE_AUTOCAPITALIZED },
  { 0 }
};


static const nsAttrValue::EnumTable* kInputDefaultInputmode = &kInputInputmodeTable[0];

const double nsHTMLInputElement::kStepScaleFactorDate = 86400000;
const double nsHTMLInputElement::kStepScaleFactorNumber = 1;
const double nsHTMLInputElement::kStepScaleFactorTime = 1000;
const double nsHTMLInputElement::kDefaultStepBase = 0;
const double nsHTMLInputElement::kDefaultStep = 1;
const double nsHTMLInputElement::kDefaultStepTime = 60;
const double nsHTMLInputElement::kStepAny = 0;

#define NS_INPUT_ELEMENT_STATE_IID                 \
{ /* dc3b3d14-23e2-4479-b513-7b369343e3a0 */       \
  0xdc3b3d14,                                      \
  0x23e2,                                          \
  0x4479,                                          \
  {0xb5, 0x13, 0x7b, 0x36, 0x93, 0x43, 0xe3, 0xa0} \
}

class nsHTMLInputElementState MOZ_FINAL : public nsISupports
{
  public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_INPUT_ELEMENT_STATE_IID)
    NS_DECL_ISUPPORTS

    bool IsCheckedSet() {
      return mCheckedSet;
    }

    bool GetChecked() {
      return mChecked;
    }

    void SetChecked(bool aChecked) {
      mChecked = aChecked;
      mCheckedSet = true;
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
      , mChecked(false)
      , mCheckedSet(false)
    {};
 
  protected:
    nsString mValue;
    nsCOMArray<nsIDOMFile> mFiles;
    bool mChecked;
    bool mCheckedSet;
};

NS_IMPL_ISUPPORTS1(nsHTMLInputElementState, nsHTMLInputElementState)
NS_DEFINE_STATIC_IID_ACCESSOR(nsHTMLInputElementState, NS_INPUT_ELEMENT_STATE_IID)

nsHTMLInputElement::nsFilePickerShownCallback::nsFilePickerShownCallback(
  nsHTMLInputElement* aInput, nsIFilePicker* aFilePicker, bool aMulti)
  : mFilePicker(aFilePicker)
  , mInput(aInput)
  , mMulti(aMulti)
{
}

NS_IMETHODIMP
nsHTMLInputElement::nsFilePickerShownCallback::Done(int16_t aResult)
{
  if (aResult == nsIFilePicker::returnCancel) {
    return NS_OK;
  }

  
  nsCOMArray<nsIDOMFile> newFiles;
  if (mMulti) {
    nsCOMPtr<nsISimpleEnumerator> iter;
    nsresult rv = mFilePicker->GetFiles(getter_AddRefs(iter));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISupports> tmp;
    bool prefSaved = false;
    bool loop = true;
    while (NS_SUCCEEDED(iter->HasMoreElements(&loop)) && loop) {
      iter->GetNext(getter_AddRefs(tmp));
      nsCOMPtr<nsIFile> localFile = do_QueryInterface(tmp);
      if (!localFile) {
        continue;
      }
      nsString path;
      localFile->GetPath(path);
      if (path.IsEmpty()) {
        continue;
      }
      nsCOMPtr<nsIDOMFile> domFile =
        do_QueryObject(new nsDOMFileFile(localFile));
      newFiles.AppendObject(domFile);
      if (!prefSaved) {
        
        nsHTMLInputElement::gUploadLastDir->StoreLastUsedDirectory(
          mInput->OwnerDoc(), localFile);
        prefSaved = true;
      }
    }
  }
  else {
    nsCOMPtr<nsIFile> localFile;
    nsresult rv = mFilePicker->GetFile(getter_AddRefs(localFile));
    NS_ENSURE_SUCCESS(rv, rv);
    if (localFile) {
      nsString path;
      rv = localFile->GetPath(path);
      if (!path.IsEmpty()) {
        nsCOMPtr<nsIDOMFile> domFile=
          do_QueryObject(new nsDOMFileFile(localFile));
        newFiles.AppendObject(domFile);
        
        nsHTMLInputElement::gUploadLastDir->StoreLastUsedDirectory(
          mInput->OwnerDoc(), localFile);
      }
    }
  }

  if (!newFiles.Count()) {
    return NS_OK;
  }

  
  
  
  mInput->SetFiles(newFiles, true);
  return nsContentUtils::DispatchTrustedEvent(mInput->OwnerDoc(),
                                              static_cast<nsIDOMHTMLInputElement*>(mInput.get()),
                                              NS_LITERAL_STRING("change"), true,
                                              false);
}

NS_IMPL_ISUPPORTS1(nsHTMLInputElement::nsFilePickerShownCallback,
                   nsIFilePickerShownCallback)

nsHTMLInputElement::AsyncClickHandler::AsyncClickHandler(nsHTMLInputElement* aInput)
  : mInput(aInput)
{
  nsPIDOMWindow* win = aInput->OwnerDoc()->GetWindow();
  if (win) {
    mPopupControlState = win->GetPopupControlState();
  }
}

NS_IMETHODIMP
nsHTMLInputElement::AsyncClickHandler::Run()
{
  
  nsCOMPtr<nsIDocument> doc = mInput->OwnerDoc();

  nsPIDOMWindow* win = doc->GetWindow();
  if (!win) {
    return NS_ERROR_FAILURE;
  }

  
  if (mPopupControlState > openControlled) {
    nsCOMPtr<nsIPopupWindowManager> pm =
      do_GetService(NS_POPUPWINDOWMANAGER_CONTRACTID);

    if (!pm) {
      return NS_OK;
    }

    uint32_t permission;
    pm->TestPermission(doc->NodePrincipal(), &permission);
    if (permission == nsIPopupWindowManager::DENY_POPUP) {
      nsCOMPtr<nsIDOMDocument> domDoc = do_QueryInterface(doc);
      nsGlobalWindow::FirePopupBlockedEvent(domDoc, win, nullptr, EmptyString(), EmptyString());
      return NS_OK;
    }
  }

  
  nsXPIDLString title;
  nsContentUtils::GetLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                     "FileUpload", title);

  nsCOMPtr<nsIFilePicker> filePicker = do_CreateInstance("@mozilla.org/filepicker;1");
  if (!filePicker)
    return NS_ERROR_FAILURE;

  bool multi = mInput->HasAttr(kNameSpaceID_None, nsGkAtoms::multiple);

  nsresult rv = filePicker->Init(win, title,
                                 multi
                                  ? static_cast<int16_t>(nsIFilePicker::modeOpenMultiple)
                                  : static_cast<int16_t>(nsIFilePicker::modeOpen));
  NS_ENSURE_SUCCESS(rv, rv);

  if (mInput->HasAttr(kNameSpaceID_None, nsGkAtoms::accept)) {
    mInput->SetFilePickerFiltersFromAccept(filePicker);
  } else {
    filePicker->AppendFilters(nsIFilePicker::filterAll);
  }

  
  nsAutoString defaultName;

  const nsCOMArray<nsIDOMFile>& oldFiles = mInput->GetFiles();

  if (oldFiles.Count()) {
    nsString path;

    oldFiles[0]->GetMozFullPathInternal(path);

    nsCOMPtr<nsIFile> localFile;
    rv = NS_NewLocalFile(path, false, getter_AddRefs(localFile));

    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIFile> parentFile;
      rv = localFile->GetParent(getter_AddRefs(parentFile));
      if (NS_SUCCEEDED(rv)) {
        filePicker->SetDisplayDirectory(parentFile);
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
    
    nsCOMPtr<nsIFile> localFile;
    nsHTMLInputElement::gUploadLastDir->FetchLastUsedDirectory(doc,
                                                               getter_AddRefs(localFile));
    if (!localFile) {
      
      nsCOMPtr<nsIFile> homeDir;
      NS_GetSpecialDirectory(NS_OS_DESKTOP_DIR, getter_AddRefs(homeDir));
      localFile = do_QueryInterface(homeDir);
    }
    filePicker->SetDisplayDirectory(localFile);
  }

  nsCOMPtr<nsIFilePickerShownCallback> callback =
    new nsHTMLInputElement::nsFilePickerShownCallback(mInput, filePicker, multi);
  return filePicker->Open(callback);
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
    observerService->AddObserver(gUploadLastDir, "browser:purge-session-history", true);
  }
}

void 
nsHTMLInputElement::DestroyUploadLastDir() {
  NS_IF_RELEASE(gUploadLastDir);
}

nsresult
UploadLastDir::FetchLastUsedDirectory(nsIDocument* aDoc, nsIFile** aFile)
{
  NS_PRECONDITION(aDoc, "aDoc is null");
  NS_PRECONDITION(aFile, "aFile is null");

  nsIURI* docURI = aDoc->GetDocumentURI();
  NS_PRECONDITION(docURI, "docURI is null");

  nsCOMPtr<nsISupports> container = aDoc->GetContainer();
  nsCOMPtr<nsILoadContext> loadContext = do_QueryInterface(container);

  
  nsCOMPtr<nsIContentPrefService> contentPrefService =
    do_GetService(NS_CONTENT_PREF_SERVICE_CONTRACTID);
  if (!contentPrefService)
    return NS_ERROR_NOT_AVAILABLE;
  nsCOMPtr<nsIWritableVariant> uri = do_CreateInstance(NS_VARIANT_CONTRACTID);
  if (!uri)
    return NS_ERROR_OUT_OF_MEMORY;
  uri->SetAsISupports(docURI);

  
  bool hasPref;
  if (NS_SUCCEEDED(contentPrefService->HasPref(uri, CPS_PREF_NAME, loadContext, &hasPref)) && hasPref) {
    nsCOMPtr<nsIVariant> pref;
    contentPrefService->GetPref(uri, CPS_PREF_NAME, loadContext, nullptr, getter_AddRefs(pref));
    nsString prefStr;
    pref->GetAsAString(prefStr);

    nsCOMPtr<nsIFile> localFile = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID);
    if (!localFile)
      return NS_ERROR_OUT_OF_MEMORY;
    localFile->InitWithPath(prefStr);
    localFile.forget(aFile);
  }
  return NS_OK;
}

nsresult
UploadLastDir::StoreLastUsedDirectory(nsIDocument* aDoc, nsIFile* aFile)
{
  NS_PRECONDITION(aDoc, "aDoc is null");
  NS_PRECONDITION(aFile, "aFile is null");

  nsCOMPtr<nsIURI> docURI = aDoc->GetDocumentURI();
  NS_PRECONDITION(docURI, "docURI is null");

  nsCOMPtr<nsIFile> parentFile;
  aFile->GetParent(getter_AddRefs(parentFile));
  if (!parentFile) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIContentPrefService> contentPrefService =
    do_GetService(NS_CONTENT_PREF_SERVICE_CONTRACTID);
  if (!contentPrefService)
    return NS_ERROR_NOT_AVAILABLE;
  nsCOMPtr<nsIWritableVariant> uri = do_CreateInstance(NS_VARIANT_CONTRACTID);
  if (!uri)
    return NS_ERROR_OUT_OF_MEMORY;
  uri->SetAsISupports(docURI);
 
  
  nsString unicodePath;
  parentFile->GetPath(unicodePath);
  if (unicodePath.IsEmpty()) 
    return NS_OK;
  nsCOMPtr<nsIWritableVariant> prefValue = do_CreateInstance(NS_VARIANT_CONTRACTID);
  if (!prefValue)
    return NS_ERROR_OUT_OF_MEMORY;
  prefValue->SetAsAString(unicodePath);

  nsCOMPtr<nsISupports> container = aDoc->GetContainer();
  nsCOMPtr<nsILoadContext> loadContext = do_QueryInterface(container);
  return contentPrefService->SetPref(uri, CPS_PREF_NAME, prefValue, loadContext);
}

NS_IMETHODIMP
UploadLastDir::Observe(nsISupports *aSubject, char const *aTopic, PRUnichar const *aData)
{
  if (strcmp(aTopic, "browser:purge-session-history") == 0) {
    nsCOMPtr<nsIContentPrefService> contentPrefService =
      do_GetService(NS_CONTENT_PREF_SERVICE_CONTRACTID);
    if (contentPrefService)
      contentPrefService->RemovePrefsByName(CPS_PREF_NAME, nullptr);
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
  : nsGenericHTMLFormElement(aNodeInfo)
  , mType(kInputDefaultType->value)
  , mDisabledChanged(false)
  , mValueChanged(false)
  , mCheckedChanged(false)
  , mChecked(false)
  , mHandlingSelectEvent(false)
  , mShouldInitChecked(false)
  , mParserCreating(aFromParser != NOT_FROM_PARSER)
  , mInInternalActivate(false)
  , mCheckedIsToggled(false)
  , mIndeterminate(false)
  , mInhibitRestoration(aFromParser & FROM_PARSER_FRAGMENT)
  , mCanShowValidUI(true)
  , mCanShowInvalidUI(true)
  , mHasRange(false)
{
  
  mInputData.mState = new nsTextEditorState(this);

  if (!gUploadLastDir)
    nsHTMLInputElement::InitUploadLastDir();

  
  
  
  
  
  AddStatesSilently(NS_EVENT_STATE_ENABLED |
                    NS_EVENT_STATE_OPTIONAL |
                    NS_EVENT_STATE_VALID);
}

nsHTMLInputElement::~nsHTMLInputElement()
{
  if (mFileList) {
    mFileList->Disconnect();
  }
  DestroyImageLoadingContent();
  FreeData();
}

void
nsHTMLInputElement::FreeData()
{
  if (!IsSingleLineTextControl(false)) {
    nsMemory::Free(mInputData.mValue);
    mInputData.mValue = nullptr;
  } else {
    UnbindFromFrame(nullptr);
    delete mInputData.mState;
    mInputData.mState = nullptr;
  }
}

nsTextEditorState*
nsHTMLInputElement::GetEditorState() const
{
  if (!IsSingleLineTextControl(false)) {
    return nullptr;
  }

  MOZ_ASSERT(mInputData.mState, "Single line text controls need to have a state"
                                " associated with them");

  return mInputData.mState;
}




NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHTMLInputElement,
                                                  nsGenericHTMLFormElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mValidity)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mControllers)
  if (tmp->IsSingleLineTextControl(false)) {
    tmp->mInputData.mState->Traverse(cb);
  }
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mFiles)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mFileList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsHTMLInputElement,
                                                  nsGenericHTMLFormElement)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mValidity)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mControllers)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mFiles)
  if (tmp->mFileList) {
    tmp->mFileList->Disconnect();
    tmp->mFileList = nullptr;
  }
  if (tmp->IsSingleLineTextControl(false)) {
    tmp->mInputData.mState->Unlink();
  }
  
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
                                                              
NS_IMPL_ADDREF_INHERITED(nsHTMLInputElement, Element)
NS_IMPL_RELEASE_INHERITED(nsHTMLInputElement, Element)


DOMCI_NODE_DATA(HTMLInputElement, nsHTMLInputElement)


NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsHTMLInputElement)
  NS_HTML_CONTENT_INTERFACE_TABLE8(nsHTMLInputElement,
                                   nsIDOMHTMLInputElement,
                                   nsITextControlElement,
                                   nsIPhonetic,
                                   imgINotificationObserver,
                                   nsIImageLoadingContent,
                                   imgIOnloadBlocker,
                                   nsIDOMNSEditableElement,
                                   nsIConstraintValidation)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLInputElement,
                                               nsGenericHTMLFormElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLInputElement)


NS_IMPL_NSICONSTRAINTVALIDATION_EXCEPT_SETCUSTOMVALIDITY(nsHTMLInputElement)



nsresult
nsHTMLInputElement::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
{
  *aResult = nullptr;

  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
  nsRefPtr<nsHTMLInputElement> it =
    new nsHTMLInputElement(ni.forget(), NOT_FROM_PARSER);

  nsresult rv = const_cast<nsHTMLInputElement*>(this)->CopyInnerTo(it);
  NS_ENSURE_SUCCESS(rv, rv);

  switch (mType) {
    case NS_FORM_INPUT_EMAIL:
    case NS_FORM_INPUT_SEARCH:
    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_PASSWORD:
    case NS_FORM_INPUT_TEL:
    case NS_FORM_INPUT_URL:
    case NS_FORM_INPUT_NUMBER:
    case NS_FORM_INPUT_DATE:
    case NS_FORM_INPUT_TIME:
      if (mValueChanged) {
        
        
        nsAutoString value;
        GetValueInternal(value);
        
        it->SetValueInternal(value, false, true);
      }
      break;
    case NS_FORM_INPUT_FILE:
      if (it->OwnerDoc()->IsStaticDocument()) {
        
        
        GetDisplayFileName(it->mStaticDocFileList);
      } else {
        it->mFiles.Clear();
        it->mFiles.AppendObjects(mFiles);
      }
      break;
    case NS_FORM_INPUT_RADIO:
    case NS_FORM_INPUT_CHECKBOX:
      if (mCheckedChanged) {
        
        
        it->DoSetChecked(mChecked, false, true);
      }
      break;
    case NS_FORM_INPUT_IMAGE:
      if (it->OwnerDoc()->IsStaticDocument()) {
        CreateStaticImageClone(it);
      }
      break;
    default:
      break;
  }

  it.forget(aResult);
  return NS_OK;
}

nsresult
nsHTMLInputElement::BeforeSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                  const nsAttrValueOrString* aValue,
                                  bool aNotify)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    
    
    
    
    
    if ((aName == nsGkAtoms::name ||
         (aName == nsGkAtoms::type && !mForm)) &&
        mType == NS_FORM_INPUT_RADIO &&
        (mForm || !mParserCreating)) {
      WillRemoveFromRadioGroup();
    } else if (aNotify && aName == nsGkAtoms::src &&
               mType == NS_FORM_INPUT_IMAGE) {
      if (aValue) {
        LoadImage(aValue->String(), true, aNotify);
      } else {
        
        CancelImageRequests(aNotify);
      }
    } else if (aNotify && aName == nsGkAtoms::disabled) {
      mDisabledChanged = true;
    } else if (aName == nsGkAtoms::dir &&
               AttrValueIs(kNameSpaceID_None, nsGkAtoms::dir,
                           nsGkAtoms::_auto, eIgnoreCase)) {
      SetDirectionIfAuto(false, aNotify);
    }
  }

  return nsGenericHTMLFormElement::BeforeSetAttr(aNameSpaceID, aName,
                                                 aValue, aNotify);
}

nsresult
nsHTMLInputElement::AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                 const nsAttrValue* aValue, bool aNotify)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    
    
    
    
    
    if ((aName == nsGkAtoms::name ||
         (aName == nsGkAtoms::type && !mForm)) &&
        mType == NS_FORM_INPUT_RADIO &&
        (mForm || !mParserCreating)) {
      AddedToRadioGroup();
      UpdateValueMissingValidityStateForRadio(false);
    }

    
    
    
    
    if (aName == nsGkAtoms::value &&
        !mValueChanged && GetValueMode() == VALUE_MODE_VALUE) {
      SetDefaultValueAsValue();
    }

    
    
    
    if (aName == nsGkAtoms::checked && !mCheckedChanged) {
      
      
      if (mParserCreating) {
        mShouldInitChecked = true;
      } else {
        DoSetChecked(DefaultChecked(), true, true);
        SetCheckedChanged(false);
      }
    }

    if (aName == nsGkAtoms::type) {
      if (!aValue) {
        
        
        
        HandleTypeChange(kInputDefaultType->value);
      }

      UpdateBarredFromConstraintValidation();

      if (mType != NS_FORM_INPUT_IMAGE) {
        
        
        
        CancelImageRequests(aNotify);
      } else if (aNotify) {
        
        
        nsAutoString src;
        if (GetAttr(kNameSpaceID_None, nsGkAtoms::src, src)) {
          LoadImage(src, false, aNotify);
        }
      }
    }

    if (mType == NS_FORM_INPUT_RADIO && aName == nsGkAtoms::required) {
      nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();

      if (container) {
        nsAutoString name;
        GetAttr(kNameSpaceID_None, nsGkAtoms::name, name);
        container->RadioRequiredChanged(name, this);
      }
    }

    if (aName == nsGkAtoms::required || aName == nsGkAtoms::disabled ||
        aName == nsGkAtoms::readonly) {
      UpdateValueMissingValidityState();

      
      if (aName == nsGkAtoms::readonly || aName == nsGkAtoms::disabled) {
        UpdateBarredFromConstraintValidation();
      }
    } else if (MaxLengthApplies() && aName == nsGkAtoms::maxlength) {
      UpdateTooLongValidityState();
    } else if (aName == nsGkAtoms::pattern) {
      UpdatePatternMismatchValidityState();
    } else if (aName == nsGkAtoms::multiple) {
      UpdateTypeMismatchValidityState();
    } else if (aName == nsGkAtoms::max) {
      UpdateHasRange();
      UpdateRangeOverflowValidityState();
    } else if (aName == nsGkAtoms::min) {
      UpdateHasRange();
      UpdateRangeUnderflowValidityState();
      UpdateStepMismatchValidityState();
    } else if (aName == nsGkAtoms::step) {
      UpdateStepMismatchValidityState();
    } else if (aName == nsGkAtoms::dir &&
               aValue && aValue->Equals(nsGkAtoms::_auto, eIgnoreCase)) {
      SetDirectionIfAuto(true, aNotify);
    }

    UpdateState(aNotify);
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
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Max, max)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Min, min)
NS_IMPL_ACTION_ATTR(nsHTMLInputElement, FormAction, formaction)
NS_IMPL_ENUM_ATTR_DEFAULT_MISSING_INVALID_VALUES(nsHTMLInputElement, FormEnctype, formenctype,
                                                 "", kFormDefaultEnctype->tag)
NS_IMPL_ENUM_ATTR_DEFAULT_MISSING_INVALID_VALUES(nsHTMLInputElement, FormMethod, formmethod,
                                                 "", kFormDefaultMethod->tag)
NS_IMPL_BOOL_ATTR(nsHTMLInputElement, FormNoValidate, formnovalidate)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, FormTarget, formtarget)
NS_IMPL_ENUM_ATTR_DEFAULT_VALUE(nsHTMLInputElement, Inputmode, inputmode,
                                kInputDefaultInputmode->tag)
NS_IMPL_BOOL_ATTR(nsHTMLInputElement, Multiple, multiple)
NS_IMPL_NON_NEGATIVE_INT_ATTR(nsHTMLInputElement, MaxLength, maxlength)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Name, name)
NS_IMPL_BOOL_ATTR(nsHTMLInputElement, ReadOnly, readonly)
NS_IMPL_BOOL_ATTR(nsHTMLInputElement, Required, required)
NS_IMPL_URI_ATTR(nsHTMLInputElement, Src, src)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Step, step)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, UseMap, usemap)

NS_IMPL_UINT_ATTR_NON_ZERO_DEFAULT_VALUE(nsHTMLInputElement, Size, size, DEFAULT_COLS)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Pattern, pattern)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Placeholder, placeholder)
NS_IMPL_ENUM_ATTR_DEFAULT_VALUE(nsHTMLInputElement, Type, type,
                                kInputDefaultType->tag)

int32_t
nsHTMLInputElement::TabIndexDefault()
{
  return 0;
}

NS_IMETHODIMP
nsHTMLInputElement::GetHeight(uint32_t *aHeight)
{
  *aHeight = GetWidthHeightForImage(mCurrentRequest).height;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::SetHeight(uint32_t aHeight)
{
  return nsGenericHTMLElement::SetUnsignedIntAttr(nsGkAtoms::height, aHeight);
}

NS_IMETHODIMP
nsHTMLInputElement::GetIndeterminate(bool* aValue)
{
  *aValue = Indeterminate();
  return NS_OK;
}

nsresult
nsHTMLInputElement::SetIndeterminateInternal(bool aValue,
                                             bool aShouldInvalidate)
{
  mIndeterminate = aValue;

  if (aShouldInvalidate) {
    
    nsIFrame* frame = GetPrimaryFrame();
    if (frame)
      frame->InvalidateFrameSubtree();
  }

  UpdateState(true);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::SetIndeterminate(bool aValue)
{
  return SetIndeterminateInternal(aValue, true);
}

NS_IMETHODIMP
nsHTMLInputElement::GetWidth(uint32_t *aWidth)
{
  *aWidth = GetWidthHeightForImage(mCurrentRequest).width;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::SetWidth(uint32_t aWidth)
{
  return nsGenericHTMLElement::SetUnsignedIntAttr(nsGkAtoms::width, aWidth);
}

NS_IMETHODIMP
nsHTMLInputElement::GetValue(nsAString& aValue)
{
  nsresult rv = GetValueInternal(aValue);

  
  if (IsExperimentalMobileType(mType)) {
    SanitizeValue(aValue);
  }

  return rv;
}

nsresult
nsHTMLInputElement::GetValueInternal(nsAString& aValue) const
{
  switch (GetValueMode()) {
    case VALUE_MODE_VALUE:
      if (IsSingleLineTextControl(false)) {
        mInputData.mState->GetValue(aValue, true);
      } else {
        aValue.Assign(mInputData.mValue);
      }
      return NS_OK;

    case VALUE_MODE_FILENAME:
      if (nsContentUtils::IsCallerChrome()) {
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

    case VALUE_MODE_DEFAULT:
      
      GetAttr(kNameSpaceID_None, nsGkAtoms::value, aValue);
      return NS_OK;

    case VALUE_MODE_DEFAULT_ON:
      
      if (!GetAttr(kNameSpaceID_None, nsGkAtoms::value, aValue)) {
        aValue.AssignLiteral("on");
      }
      return NS_OK;
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

bool
nsHTMLInputElement::ConvertStringToNumber(nsAString& aValue,
                                          double& aResultValue) const
{
  MOZ_ASSERT(DoesValueAsNumberApply(),
             "ConvertStringToNumber only applies if .valueAsNumber applies");

  switch (mType) {
    case NS_FORM_INPUT_NUMBER:
      {
        nsresult ec;
        aResultValue = PromiseFlatString(aValue).ToDouble(&ec);
        if (NS_FAILED(ec)) {
          return false;
        }

        return true;
      }
    case NS_FORM_INPUT_DATE:
      {
        SafeAutoJSContext ctx;
        JSAutoRequest ar(ctx);

        uint32_t year, month, day;
        if (!GetValueAsDate(aValue, &year, &month, &day)) {
          return false;
        }

        JSObject* date = JS_NewDateObjectMsec(ctx, 0);
        if (!date) {
          JS_ClearPendingException(ctx);
          return false;
        }

        jsval rval;
        jsval fullYear[3];
        fullYear[0].setInt32(year);
        fullYear[1].setInt32(month - 1);
        fullYear[2].setInt32(day);
        if (!JS::Call(ctx, date, "setUTCFullYear", 3, fullYear, &rval)) {
          JS_ClearPendingException(ctx);
          return false;
        }

        jsval timestamp;
        if (!JS::Call(ctx, date, "getTime", 0, nullptr, &timestamp)) {
          JS_ClearPendingException(ctx);
          return false;
        }

        if (!timestamp.isNumber() || MOZ_DOUBLE_IS_NaN(timestamp.toNumber())) {
          return false;
        }

        aResultValue = timestamp.toNumber();
        return true;
      }
    case NS_FORM_INPUT_TIME:
      uint32_t milliseconds;
      if (!ParseTime(aValue, &milliseconds)) {
        return false;
      }

      aResultValue = static_cast<double>(milliseconds);
      return true;
    default:
      return false;
  }

  MOZ_NOT_REACHED();
  return false;
}

double
nsHTMLInputElement::GetValueAsDouble() const
{
  double doubleValue;
  nsAutoString stringValue;

  GetValueInternal(stringValue);

  return !ConvertStringToNumber(stringValue, doubleValue) ? MOZ_DOUBLE_NaN()
                                                          : doubleValue;
}

NS_IMETHODIMP 
nsHTMLInputElement::SetValue(const nsAString& aValue)
{
  
  
  if (mType == NS_FORM_INPUT_FILE) {
    if (!aValue.IsEmpty()) {
      if (!nsContentUtils::IsCallerChrome()) {
        
        
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
    if (IsSingleLineTextControl(false)) {
      
      
      
      
      
      
      
      nsAutoString currentValue;
      GetValueInternal(currentValue);

      SetValueInternal(aValue, false, true);

      if (mFocusedValue.Equals(currentValue)) {
        GetValueInternal(mFocusedValue);
      }
    } else {
      SetValueInternal(aValue, false, true);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::GetList(nsIDOMHTMLElement** aValue)
{
  *aValue = nullptr;

  nsAutoString dataListId;
  GetAttr(kNameSpaceID_None, nsGkAtoms::list, dataListId);
  if (dataListId.IsEmpty()) {
    return NS_OK;
  }

  nsIDocument* doc = GetCurrentDoc();
  if (!doc) {
    return NS_OK;
  }

  Element* element = doc->GetElementById(dataListId);
  if (!element || !element->IsHTML(nsGkAtoms::datalist)) {
    return NS_OK;
  }

  CallQueryInterface(element, aValue);
  return NS_OK;
}

void
nsHTMLInputElement::SetValue(double aValue)
{
  MOZ_ASSERT(!MOZ_DOUBLE_IS_INFINITE(aValue), "aValue must not be Infinity!");

  if (MOZ_DOUBLE_IS_NaN(aValue)) {
    SetValue(EmptyString());
    return;
  }

  nsAutoString value;
  ConvertNumberToString(aValue, value);
  SetValue(value);
}

bool
nsHTMLInputElement::ConvertNumberToString(double aValue,
                                          nsAString& aResultString) const
{
  MOZ_ASSERT(DoesValueAsNumberApply(),
             "ConvertNumberToString is only implemented for types implementing .valueAsNumber");
  MOZ_ASSERT(!MOZ_DOUBLE_IS_NaN(aValue) && !MOZ_DOUBLE_IS_INFINITE(aValue),
             "aValue must be a valid non-Infinite number.");

  aResultString.Truncate();

  switch (mType) {
    case NS_FORM_INPUT_NUMBER:
      aResultString.AppendFloat(aValue);
      return true;
    case NS_FORM_INPUT_DATE:
      {
        SafeAutoJSContext ctx;
        JSAutoRequest ar(ctx);

        
        aValue = floor(aValue);

        JSObject* date = JS_NewDateObjectMsec(ctx, aValue);
        if (!date) {
          JS_ClearPendingException(ctx);
          return false;
        }

        jsval year, month, day;
        if (!JS::Call(ctx, date, "getUTCFullYear", 0, nullptr, &year) ||
            !JS::Call(ctx, date, "getUTCMonth", 0, nullptr, &month) ||
            !JS::Call(ctx, date, "getUTCDate", 0, nullptr, &day)) {
          JS_ClearPendingException(ctx);
          return false;
        }

        if (!year.isNumber() || !month.isNumber() || !day.isNumber() ||
            MOZ_DOUBLE_IS_NaN(year.toNumber()) ||
            MOZ_DOUBLE_IS_NaN(month.toNumber()) ||
            MOZ_DOUBLE_IS_NaN(day.toNumber())) {
          return false;
        }

        aResultString.AppendPrintf("%04.0f-%02.0f-%02.0f", year.toNumber(),
                                   month.toNumber() + 1, day.toNumber());

        return true;
      }
    case NS_FORM_INPUT_TIME:
      {
        
        
        
        uint32_t value = NS_floorModulo(floor(aValue), 86400000);

        uint16_t milliseconds = value % 1000;
        value /= 1000;

        uint8_t seconds = value % 60;
        value /= 60;

        uint8_t minutes = value % 60;
        value /= 60;

        uint8_t hours = value;

        if (milliseconds != 0) {
          aResultString.AppendPrintf("%02d:%02d:%02d.%03d",
                                     hours, minutes, seconds, milliseconds);
        } else if (seconds != 0) {
          aResultString.AppendPrintf("%02d:%02d:%02d",
                                     hours, minutes, seconds);
        } else {
          aResultString.AppendPrintf("%02d:%02d", hours, minutes);
        }

        return true;
      }
    default:
      MOZ_NOT_REACHED();
      return false;
  }
}

NS_IMETHODIMP
nsHTMLInputElement::GetValueAsDate(JSContext* aCtx, jsval* aDate)
{
  if (mType != NS_FORM_INPUT_DATE && mType != NS_FORM_INPUT_TIME) {
    aDate->setNull();
    return NS_OK;
  }

  switch (mType) {
    case NS_FORM_INPUT_DATE:
    {
      uint32_t year, month, day;
      nsAutoString value;
      GetValueInternal(value);
      if (!GetValueAsDate(value, &year, &month, &day)) {
        aDate->setNull();
        return NS_OK;
      }

      JSObject* date = JS_NewDateObjectMsec(aCtx, 0);
      if (!date) {
        JS_ClearPendingException(aCtx);
        aDate->setNull();
        return NS_OK;
      }

      jsval rval;
      jsval fullYear[3];
      fullYear[0].setInt32(year);
      fullYear[1].setInt32(month - 1);
      fullYear[2].setInt32(day);
      if(!JS::Call(aCtx, date, "setUTCFullYear", 3, fullYear, &rval)) {
        JS_ClearPendingException(aCtx);
        aDate->setNull();
        return NS_OK;
      }

      aDate->setObjectOrNull(date);
      return NS_OK;
    }
    case NS_FORM_INPUT_TIME:
    {
      uint32_t millisecond;
      nsAutoString value;
      GetValueInternal(value);
      if (!ParseTime(value, &millisecond)) {
        aDate->setNull();
        return NS_OK;
      }

      JSObject* date = JS_NewDateObjectMsec(aCtx, millisecond);
      if (!date) {
        JS_ClearPendingException(aCtx);
        aDate->setNull();
        return NS_OK;
      }

      aDate->setObjectOrNull(date);
      return NS_OK;
    }
  }

  MOZ_NOT_REACHED();
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsHTMLInputElement::SetValueAsDate(JSContext* aCtx, const jsval& aDate)
{
  if (mType != NS_FORM_INPUT_DATE && mType != NS_FORM_INPUT_TIME) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  if (aDate.isNullOrUndefined()) {
    return SetValue(EmptyString());
  }

  
  
  if (!aDate.isObject() || !JS_ObjectIsDate(aCtx, &aDate.toObject())) {
    SetValue(EmptyString());
    return NS_ERROR_INVALID_ARG;
  }

  JSObject& date = aDate.toObject();
  jsval timestamp;
  if (!JS::Call(aCtx, &date, "getTime", 0, nullptr, &timestamp) ||
      !timestamp.isNumber() || MOZ_DOUBLE_IS_NaN(timestamp.toNumber())) {
    JS_ClearPendingException(aCtx);
    SetValue(EmptyString());
    return NS_OK;
  }

  SetValue(timestamp.toNumber());
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::GetValueAsNumber(double* aValueAsNumber)
{
  *aValueAsNumber = DoesValueAsNumberApply() ? GetValueAsDouble()
                                             : MOZ_DOUBLE_NaN();
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::SetValueAsNumber(double aValueAsNumber)
{
  
  
  if (MOZ_DOUBLE_IS_INFINITE(aValueAsNumber)) {
    return NS_ERROR_INVALID_ARG;
  }

  if (!DoesValueAsNumberApply()) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  SetValue(aValueAsNumber);
  return NS_OK;
}

double
nsHTMLInputElement::GetMinimum() const
{
  MOZ_ASSERT(DoesValueAsNumberApply(),
             "GetMinAsDouble() should only be used for types that allow .valueAsNumber");

  
  

  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::min)) {
    return MOZ_DOUBLE_NaN();
  }

  nsAutoString minStr;
  GetAttr(kNameSpaceID_None, nsGkAtoms::min, minStr);

  double min;
  return ConvertStringToNumber(minStr, min) ? min : MOZ_DOUBLE_NaN();
}

double
nsHTMLInputElement::GetMaximum() const
{
  MOZ_ASSERT(DoesValueAsNumberApply(),
             "GetMaxAsDouble() should only be used for types that allow .valueAsNumber");

  
  

  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::max)) {
    return MOZ_DOUBLE_NaN();
  }

  nsAutoString maxStr;
  GetAttr(kNameSpaceID_None, nsGkAtoms::max, maxStr);

  double max;
  return ConvertStringToNumber(maxStr, max) ? max : MOZ_DOUBLE_NaN();
}

double
nsHTMLInputElement::GetStepBase() const
{
  MOZ_ASSERT(mType == NS_FORM_INPUT_NUMBER ||
             mType == NS_FORM_INPUT_DATE ||
             mType == NS_FORM_INPUT_TIME,
             "Check that kDefaultStepBase is correct for this new type");

  double stepBase;

  
  
  nsAutoString minStr;
  if (GetAttr(kNameSpaceID_None, nsGkAtoms::min, minStr) &&
      ConvertStringToNumber(minStr, stepBase)) {
    return stepBase;
  }

  
  nsAutoString valueStr;
  if (GetAttr(kNameSpaceID_None, nsGkAtoms::value, valueStr) &&
      ConvertStringToNumber(valueStr, stepBase)) {
    return stepBase;
  }

  return kDefaultStepBase;
}

nsresult
nsHTMLInputElement::ApplyStep(int32_t aStep)
{
  if (!DoStepDownStepUpApply()) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  double step = GetStep();
  if (step == kStepAny) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  double value = GetValueAsDouble();
  if (MOZ_DOUBLE_IS_NaN(value)) {
    return NS_OK;
  }

  double minimum = GetMinimum();

  double maximum = GetMaximum();
  if (!MOZ_DOUBLE_IS_NaN(maximum)) {
    
    maximum = maximum - NS_floorModulo(maximum - GetStepBase(), step);
  }

  
  
  
  
  if ((value <= minimum && aStep < 0) ||
      (value >= maximum && aStep > 0)) {
    return NS_OK;
  }

  if (GetValidityState(VALIDITY_STATE_STEP_MISMATCH) &&
      value != minimum && value != maximum) {
    if (aStep > 0) {
      value -= NS_floorModulo(value - GetStepBase(), step);
    } else if (aStep < 0) {
      value -= NS_floorModulo(value - GetStepBase(), step);
      value += step;
    }
  }

  value += aStep * step;

  
  
  
  if (mType == NS_FORM_INPUT_DATE &&
      NS_floorModulo(value - GetStepBase(), GetStepScaleFactor()) != 0) {
    double validStep = EuclidLCM<uint64_t>(static_cast<uint64_t>(step),
                                           static_cast<uint64_t>(GetStepScaleFactor()));
    if (aStep > 0) {
      value -= NS_floorModulo(value - GetStepBase(), validStep);
      value += validStep;
    } else if (aStep < 0) {
      value -= NS_floorModulo(value - GetStepBase(), validStep);
    }
  }

  
  
  if (GetValidityState(VALIDITY_STATE_RANGE_UNDERFLOW) && aStep > 0 &&
      value <= minimum) {
    MOZ_ASSERT(!MOZ_DOUBLE_IS_NaN(minimum), "Can't be NaN if we are here");
    value = minimum;
  
  } else if (GetValidityState(VALIDITY_STATE_RANGE_OVERFLOW) && aStep < 0 &&
             value >= maximum) {
    MOZ_ASSERT(!MOZ_DOUBLE_IS_NaN(maximum), "Can't be NaN if we are here");
    value = maximum;
  
  } else if (aStep < 0 && minimum == minimum) {
    value = std::max(value, minimum);
  
  } else if (aStep > 0 && maximum == maximum) {
    value = std::min(value, maximum);
  }

  SetValue(value);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::StepDown(int32_t n, uint8_t optional_argc)
{
  return ApplyStep(optional_argc ? -n : -1);
}

NS_IMETHODIMP
nsHTMLInputElement::StepUp(int32_t n, uint8_t optional_argc)
{
  return ApplyStep(optional_argc ? n : 1);
}

NS_IMETHODIMP 
nsHTMLInputElement::MozGetFileNameArray(uint32_t *aLength, PRUnichar ***aFileNames)
{
  if (!nsContentUtils::IsCallerChrome()) {
    
    
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  *aLength = mFiles.Count();
  PRUnichar **ret =
    static_cast<PRUnichar **>(NS_Alloc(mFiles.Count() * sizeof(PRUnichar*)));
  if (!ret) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  for (int32_t i = 0; i < mFiles.Count(); i++) {
    nsString str;
    mFiles[i]->GetMozFullPathInternal(str);
    ret[i] = NS_strdup(str.get());
  }

  *aFileNames = ret;

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLInputElement::MozSetFileNameArray(const PRUnichar **aFileNames, uint32_t aLength)
{
  if (!nsContentUtils::IsCallerChrome()) {
    
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsCOMArray<nsIDOMFile> files;
  for (uint32_t i = 0; i < aLength; ++i) {
    nsCOMPtr<nsIFile> file;
    if (StringBeginsWith(nsDependentString(aFileNames[i]),
                         NS_LITERAL_STRING("file:"),
                         nsASCIICaseInsensitiveStringComparator())) {
      
      
      NS_GetFileFromURLSpec(NS_ConvertUTF16toUTF8(aFileNames[i]),
                            getter_AddRefs(file));
    }

    if (!file) {
      
      NS_NewLocalFile(nsDependentString(aFileNames[i]),
                      false, getter_AddRefs(file));
    }

    if (file) {
      nsCOMPtr<nsIDOMFile> domFile = new nsDOMFileFile(file);
      files.AppendObject(domFile);
    } else {
      continue; 
    }

  }

  SetFiles(files, true);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::MozIsTextField(bool aExcludePassword, bool* aResult)
{
  
  if (IsExperimentalMobileType(mType)) {
    *aResult = false;
    return NS_OK;
  }

  *aResult = IsSingleLineTextControl(aExcludePassword);

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLInputElement::SetUserInput(const nsAString& aValue)
{
  if (!nsContentUtils::IsCallerChrome()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  if (mType == NS_FORM_INPUT_FILE)
  {
    const PRUnichar* name = PromiseFlatString(aValue).get();
    return MozSetFileNameArray(&name, 1);
  } else {
    SetValueInternal(aValue, true, true);
  }

  return nsContentUtils::DispatchTrustedEvent(OwnerDoc(),
                                              static_cast<nsIDOMHTMLInputElement*>(this),
                                              NS_LITERAL_STRING("input"), true,
                                              true);
}

NS_IMETHODIMP_(nsIEditor*)
nsHTMLInputElement::GetTextEditor()
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    return state->GetEditor();
  }
  return nullptr;
}

NS_IMETHODIMP_(nsISelectionController*)
nsHTMLInputElement::GetSelectionController()
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    return state->GetSelectionController();
  }
  return nullptr;
}

nsFrameSelection*
nsHTMLInputElement::GetConstFrameSelection()
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    return state->GetConstFrameSelection();
  }
  return nullptr;
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
  return nullptr;
}

NS_IMETHODIMP_(nsIContent*)
nsHTMLInputElement::CreatePlaceholderNode()
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    NS_ENSURE_SUCCESS(state->CreatePlaceholderNode(), nullptr);
    return state->GetPlaceholderNode();
  }
  return nullptr;
}

NS_IMETHODIMP_(nsIContent*)
nsHTMLInputElement::GetPlaceholderNode()
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    return state->GetPlaceholderNode();
  }
  return nullptr;
}

NS_IMETHODIMP_(void)
nsHTMLInputElement::UpdatePlaceholderVisibility(bool aNotify)
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    state->UpdatePlaceholderVisibility(aNotify);
  }
}

NS_IMETHODIMP_(bool)
nsHTMLInputElement::GetPlaceholderVisibility()
{
  nsTextEditorState* state = GetEditorState();
  if (!state) {
    return false;
  }

  return state->GetPlaceholderVisibility();
}

void
nsHTMLInputElement::GetDisplayFileName(nsAString& aValue) const
{
  if (OwnerDoc()->IsStaticDocument()) {
    aValue = mStaticDocFileList;
    return;
  }

  aValue.Truncate();
  for (int32_t i = 0; i < mFiles.Count(); ++i) {
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

  AfterSetFiles(aSetValueChanged);
}

void
nsHTMLInputElement::SetFiles(nsIDOMFileList* aFiles,
                             bool aSetValueChanged)
{
  mFiles.Clear();

  if (aFiles) {
    uint32_t listLength;
    aFiles->GetLength(&listLength);
    for (uint32_t i = 0; i < listLength; i++) {
      nsCOMPtr<nsIDOMFile> file;
      aFiles->Item(i, getter_AddRefs(file));
      mFiles.AppendObject(file);
    }
  }

  AfterSetFiles(aSetValueChanged);
}

void
nsHTMLInputElement::AfterSetFiles(bool aSetValueChanged)
{
  
  
  
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(false);
  if (formControlFrame) {
    nsAutoString readableValue;
    GetDisplayFileName(readableValue);
    formControlFrame->SetFormProperty(nsGkAtoms::value, readableValue);
  }

  UpdateFileList();

  if (aSetValueChanged) {
    SetValueChanged(true);
  }

  UpdateAllValidityStates(true);
}

void
nsHTMLInputElement::FireChangeEventIfNeeded()
{
  nsString value;
  GetValueInternal(value);

  if (!IsSingleLineTextControl(false) || mFocusedValue.Equals(value)) {
    return;
  }

  
  mFocusedValue = value;
  nsContentUtils::DispatchTrustedEvent(OwnerDoc(),
                                       static_cast<nsIContent*>(this), 
                                       NS_LITERAL_STRING("change"), true,
                                       false);
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
    for (int32_t i = 0; i < files.Count(); ++i) {
      if (!mFileList->Append(files[i])) {
        return NS_ERROR_FAILURE;
      }
    }
  }

  return NS_OK;
}

nsresult
nsHTMLInputElement::SetValueInternal(const nsAString& aValue,
                                     bool aUserInput,
                                     bool aSetValueChanged)
{
  NS_PRECONDITION(GetValueMode() != VALUE_MODE_FILENAME,
                  "Don't call SetValueInternal for file inputs");

  switch (GetValueMode()) {
    case VALUE_MODE_VALUE:
    {
      
      
      
      nsAutoString value(aValue);

      if (!mParserCreating) {
        SanitizeValue(value);
      }

      if (aSetValueChanged) {
        SetValueChanged(true);
      }

      if (IsSingleLineTextControl(false)) {
        mInputData.mState->SetValue(value, aUserInput, aSetValueChanged);
      } else {
        mInputData.mValue = ToNewUnicode(value);
        if (aSetValueChanged) {
          SetValueChanged(true);
        }
        OnValueChanged(!mParserCreating);
      }

      return NS_OK;
    }

    case VALUE_MODE_DEFAULT:
    case VALUE_MODE_DEFAULT_ON:
      
      
      
      
      
      if (mType == NS_FORM_INPUT_HIDDEN) {
        SetValueChanged(true);
      }

      
      return nsGenericHTMLFormElement::SetAttr(kNameSpaceID_None,
                                               nsGkAtoms::value, aValue,
                                               true);

    case VALUE_MODE_FILENAME:
      return NS_ERROR_UNEXPECTED;
  }

  
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::SetValueChanged(bool aValueChanged)
{
  bool valueChangedBefore = mValueChanged;

  mValueChanged = aValueChanged;

  if (valueChangedBefore != aValueChanged) {
    UpdateState(true);
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLInputElement::GetChecked(bool* aChecked)
{
  *aChecked = Checked();
  return NS_OK;
}

void
nsHTMLInputElement::SetCheckedChanged(bool aCheckedChanged)
{
  DoSetCheckedChanged(aCheckedChanged, true);
}

void
nsHTMLInputElement::DoSetCheckedChanged(bool aCheckedChanged,
                                        bool aNotify)
{
  if (mType == NS_FORM_INPUT_RADIO) {
    if (mCheckedChanged != aCheckedChanged) {
      nsCOMPtr<nsIRadioVisitor> visitor =
        new nsRadioSetCheckedChangedVisitor(aCheckedChanged);
      VisitGroup(visitor, aNotify);
    }
  } else {
    SetCheckedChangedInternal(aCheckedChanged);
  }
}

void
nsHTMLInputElement::SetCheckedChangedInternal(bool aCheckedChanged)
{
  bool checkedChangedBefore = mCheckedChanged;

  mCheckedChanged = aCheckedChanged;

  
  
  if (checkedChangedBefore != aCheckedChanged) {
    UpdateState(true);
  }
}

NS_IMETHODIMP
nsHTMLInputElement::SetChecked(bool aChecked)
{
  DoSetChecked(aChecked, true, true);
  return NS_OK;
}

void
nsHTMLInputElement::DoSetChecked(bool aChecked, bool aNotify,
                                 bool aSetValueChanged)
{
  
  
  
  if (aSetValueChanged) {
    DoSetCheckedChanged(true, aNotify);
  }

  
  
  
  if (mChecked == aChecked) {
    return;
  }

  
  if (mType != NS_FORM_INPUT_RADIO) {
    SetCheckedInternal(aChecked, aNotify);
    return;
  }

  
  if (aChecked) {
    RadioSetChecked(aNotify);
    return;
  }

  nsIRadioGroupContainer* container = GetRadioGroupContainer();
  if (container) {
    nsAutoString name;
    GetAttr(kNameSpaceID_None, nsGkAtoms::name, name);
    container->SetCurrentRadioButton(name, nullptr);
  }
  
  
  
  SetCheckedInternal(false, aNotify);
}

void
nsHTMLInputElement::RadioSetChecked(bool aNotify)
{
  
  nsCOMPtr<nsIDOMHTMLInputElement> currentlySelected = GetSelectedRadioButton();

  
  if (currentlySelected) {
    
    
    static_cast<nsHTMLInputElement*>(currentlySelected.get())
      ->SetCheckedInternal(false, true);
  }

  
  nsIRadioGroupContainer* container = GetRadioGroupContainer();
  if (container) {
    nsAutoString name;
    GetAttr(kNameSpaceID_None, nsGkAtoms::name, name);
    container->SetCurrentRadioButton(name, this);
  }

  
  
  SetCheckedInternal(true, aNotify);
}

nsIRadioGroupContainer*
nsHTMLInputElement::GetRadioGroupContainer() const
{
  NS_ASSERTION(mType == NS_FORM_INPUT_RADIO,
               "GetRadioGroupContainer should only be called when type='radio'");

  nsAutoString name;
  GetAttr(kNameSpaceID_None, nsGkAtoms::name, name);

  if (name.IsEmpty()) {
    return nullptr;
  }

  if (mForm) {
    return mForm;
  }

  return static_cast<nsDocument*>(GetCurrentDoc());
}

already_AddRefed<nsIDOMHTMLInputElement>
nsHTMLInputElement::GetSelectedRadioButton()
{
  nsIRadioGroupContainer* container = GetRadioGroupContainer();
  if (!container) {
    return nullptr;
  }

  nsAutoString name;
  GetAttr(kNameSpaceID_None, nsGkAtoms::name, name);

  nsCOMPtr<nsIDOMHTMLInputElement> selected = container->GetCurrentRadioButton(name);
  return selected.forget();
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
    nsCOMPtr<nsIContent> submitContent = do_QueryInterface(submitControl);
    NS_ASSERTION(submitContent, "Form control not implementing nsIContent?!");
    
    
    nsMouseEvent event(true, NS_MOUSE_CLICK, nullptr, nsMouseEvent::eReal);
    nsEventStatus status = nsEventStatus_eIgnore;
    shell->HandleDOMEventWithTarget(submitContent, &event, &status);
  } else if (mForm->HasSingleTextControl() &&
             (mForm->HasAttr(kNameSpaceID_None, nsGkAtoms::novalidate) ||
              mForm->CheckValidFormSubmission())) {
    
    
    
    
    nsRefPtr<nsHTMLFormElement> form = mForm;
    nsFormEvent event(true, NS_FORM_SUBMIT);
    nsEventStatus status = nsEventStatus_eIgnore;
    shell->HandleDOMEventWithTarget(mForm, &event, &status);
  }

  return NS_OK;
}

void
nsHTMLInputElement::SetCheckedInternal(bool aChecked, bool aNotify)
{
  
  mChecked = aChecked;

  
  if (mType == NS_FORM_INPUT_CHECKBOX || mType == NS_FORM_INPUT_RADIO) {
    nsIFrame* frame = GetPrimaryFrame();
    if (frame) {
      frame->InvalidateFrameSubtree();
    }
  }

  UpdateAllValidityStates(aNotify);

  
  
  UpdateState(aNotify);
}

void
nsHTMLInputElement::Focus(ErrorResult& aError)
{
  if (mType != NS_FORM_INPUT_FILE) {
    nsGenericHTMLElement::Focus(aError);
    return;
  }

  
  nsIFrame* frame = GetPrimaryFrame();
  if (frame) {
    for (nsIFrame* childFrame = frame->GetFirstPrincipalChild();
         childFrame;
         childFrame = childFrame->GetNextSibling()) {
      
      nsCOMPtr<nsIFormControl> formCtrl =
        do_QueryInterface(childFrame->GetContent());
      if (formCtrl && formCtrl->GetType() == NS_FORM_INPUT_BUTTON) {
        nsCOMPtr<nsIDOMElement> element = do_QueryInterface(formCtrl);
        nsIFocusManager* fm = nsFocusManager::GetFocusManager();
        if (fm && element) {
          fm->SetFocus(element, 0);
        }
        break;
      }
    }
  }

  return;
}

NS_IMETHODIMP
nsHTMLInputElement::Select()
{
  if (!IsSingleLineTextControl(false)) {
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

bool
nsHTMLInputElement::DispatchSelectEvent(nsPresContext* aPresContext)
{
  nsEventStatus status = nsEventStatus_eIgnore;

  
  if (!mHandlingSelectEvent) {
    nsEvent event(nsContentUtils::IsCallerChrome(), NS_FORM_SELECTED);

    mHandlingSelectEvent = true;
    nsEventDispatcher::Dispatch(static_cast<nsIContent*>(this),
                                aPresContext, &event, nullptr, &status);
    mHandlingSelectEvent = false;
  }

  
  
  return (status == nsEventStatus_eIgnore);
}
    
void
nsHTMLInputElement::SelectAll(nsPresContext* aPresContext)
{
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(true);

  if (formControlFrame) {
    formControlFrame->SetFormProperty(nsGkAtoms::select, EmptyString());
  }
}

void
nsHTMLInputElement::Click()
{
  if (mType == NS_FORM_INPUT_FILE) {
    FireAsyncClickHandler();
  }

  nsGenericHTMLElement::Click();
}

NS_IMETHODIMP
nsHTMLInputElement::FireAsyncClickHandler()
{
  nsCOMPtr<nsIRunnable> event = new AsyncClickHandler(this);
  return NS_DispatchToMainThread(event);
}

bool
nsHTMLInputElement::NeedToInitializeEditorForEvent(nsEventChainPreVisitor& aVisitor) const
{
  
  
  
  
  
  if (!IsSingleLineTextControl(false) ||
      aVisitor.mEvent->eventStructType == NS_MUTATION_EVENT) {
    return false;
  }

  switch (aVisitor.mEvent->message) {
  case NS_MOUSE_MOVE:
  case NS_MOUSE_ENTER:
  case NS_MOUSE_EXIT:
  case NS_MOUSE_ENTER_SYNTH:
  case NS_MOUSE_EXIT_SYNTH:
    return false;
  default:
    return true;
  }
}

bool
nsHTMLInputElement::IsDisabledForEvents(uint32_t aMessage)
{
  return IsElementDisabledForEvents(aMessage, GetPrimaryFrame());
}

nsresult
nsHTMLInputElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  
  aVisitor.mCanHandle = false;
  if (IsDisabledForEvents(aVisitor.mEvent->message)) {
    return NS_OK;
  }

  
  if (NeedToInitializeEditorForEvent(aVisitor)) {
    nsITextControlFrame* textControlFrame = do_QueryFrame(GetPrimaryFrame());
    if (textControlFrame)
      textControlFrame->EnsureEditorInitialized();
  }

  
  if (!aVisitor.mPresContext) {
    return nsGenericHTMLElement::PreHandleEvent(aVisitor);
  } 
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  bool outerActivateEvent =
    (NS_IS_MOUSE_LEFT_CLICK(aVisitor.mEvent) ||
     (aVisitor.mEvent->message == NS_UI_ACTIVATE && !mInInternalActivate));

  if (outerActivateEvent) {
    aVisitor.mItemFlags |= NS_OUTER_ACTIVATE_EVENT;
  }

  bool originalCheckedValue = false;

  if (outerActivateEvent) {
    mCheckedIsToggled = false;

    switch(mType) {
      case NS_FORM_INPUT_CHECKBOX:
        {
          if (mIndeterminate) {
            
            SetIndeterminateInternal(false, false);
            aVisitor.mItemFlags |= NS_ORIGINAL_INDETERMINATE_VALUE;
          }

          GetChecked(&originalCheckedValue);
          DoSetChecked(!originalCheckedValue, true, true);
          mCheckedIsToggled = true;
        }
        break;

      case NS_FORM_INPUT_RADIO:
        {
          nsCOMPtr<nsIDOMHTMLInputElement> selectedRadioButton = GetSelectedRadioButton();
          aVisitor.mItemData = selectedRadioButton;

          originalCheckedValue = mChecked;
          if (!originalCheckedValue) {
            DoSetChecked(true, true, true);
            mCheckedIsToggled = true;
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

  
  
  
  if (aVisitor.mEvent->mFlags.mNoContentDispatch) {
    aVisitor.mItemFlags |= NS_NO_CONTENT_DISPATCH;
  }
  if (IsSingleLineTextControl(false) &&
      aVisitor.mEvent->message == NS_MOUSE_CLICK &&
      aVisitor.mEvent->eventStructType == NS_MOUSE_EVENT &&
      static_cast<nsMouseEvent*>(aVisitor.mEvent)->button ==
        nsMouseEvent::eMiddleButton) {
    aVisitor.mEvent->mFlags.mNoContentDispatch = false;
  }

  
  aVisitor.mItemFlags |= mType;

  
  if (aVisitor.mEvent->message == NS_BLUR_CONTENT) {
    
    
    
    if (IsExperimentalMobileType(mType)) {
      nsAutoString aValue;
      GetValueInternal(aValue);
      SetValueInternal(aValue, false, false);
    }
    FireChangeEventIfNeeded();
  }

  return nsGenericHTMLFormElement::PreHandleEvent(aVisitor);
}

static bool
SelectTextFieldOnFocus()
{
  if (!gSelectTextFieldOnFocus) {
    int32_t selectTextfieldsOnKeyFocus = -1;
    nsresult rv =
      LookAndFeel::GetInt(LookAndFeel::eIntID_SelectTextfieldsOnKeyFocus,
                          &selectTextfieldsOnKeyFocus);
    if (NS_FAILED(rv)) {
      gSelectTextFieldOnFocus = -1;
    } else {
      gSelectTextFieldOnFocus = selectTextfieldsOnKeyFocus != 0 ? 1 : -1;
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
    if (aVisitor.mEvent->message == NS_FOCUS_CONTENT && 
        IsSingleLineTextControl(false)) {
      GetValueInternal(mFocusedValue);
    }

    UpdateValidityUIBits(aVisitor.mEvent->message == NS_FOCUS_CONTENT);

    UpdateState(true);
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
  bool outerActivateEvent = !!(aVisitor.mItemFlags & NS_OUTER_ACTIVATE_EVENT);
  bool originalCheckedValue =
    !!(aVisitor.mItemFlags & NS_ORIGINAL_CHECKED_VALUE);
  bool noContentDispatch = !!(aVisitor.mItemFlags & NS_NO_CONTENT_DISPATCH);
  uint8_t oldType = NS_CONTROL_TYPE(aVisitor.mItemFlags);
  
  
  
  
  
  
  
  if (aVisitor.mEventStatus != nsEventStatus_eConsumeNoDefault &&
      !IsSingleLineTextControl(true) &&
      NS_IS_MOUSE_LEFT_CLICK(aVisitor.mEvent)) {
    nsUIEvent actEvent(aVisitor.mEvent->mFlags.mIsTrusted, NS_UI_ACTIVATE, 1);

    nsCOMPtr<nsIPresShell> shell = aVisitor.mPresContext->GetPresShell();
    if (shell) {
      nsEventStatus status = nsEventStatus_eIgnore;
      mInInternalActivate = true;
      rv = shell->HandleDOMEventWithTarget(this, &actEvent, &status);
      mInInternalActivate = false;

      
      
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

  
  aVisitor.mEvent->mFlags.mNoContentDispatch = noContentDispatch;

  
  if (mCheckedIsToggled && outerActivateEvent) {
    if (aVisitor.mEventStatus == nsEventStatus_eConsumeNoDefault) {
      
      
      
      if (oldType == NS_FORM_INPUT_RADIO) {
        nsCOMPtr<nsIDOMHTMLInputElement> selectedRadioButton =
          do_QueryInterface(aVisitor.mItemData);
        if (selectedRadioButton) {
          selectedRadioButton->SetChecked(true);
        }
        
        
        
        if (!selectedRadioButton || mType != NS_FORM_INPUT_RADIO) {
          DoSetChecked(false, true, true);
        }
      } else if (oldType == NS_FORM_INPUT_CHECKBOX) {
        bool originalIndeterminateValue =
          !!(aVisitor.mItemFlags & NS_ORIGINAL_INDETERMINATE_VALUE);
        SetIndeterminateInternal(originalIndeterminateValue, false);
        DoSetChecked(originalCheckedValue, true, true);
      }
    } else {
      nsContentUtils::DispatchTrustedEvent(OwnerDoc(),
                                           static_cast<nsIDOMHTMLInputElement*>(this),
                                           NS_LITERAL_STRING("change"), true,
                                           false);
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
          if (fm && IsSingleLineTextControl(false) &&
              !(static_cast<nsFocusEvent *>(aVisitor.mEvent))->fromRaise &&
              SelectTextFieldOnFocus()) {
            nsIDocument* document = GetCurrentDoc();
            if (document) {
              uint32_t lastFocusMethod;
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
                nsMouseEvent event(aVisitor.mEvent->mFlags.mIsTrusted,
                                   NS_MOUSE_CLICK, nullptr, nsMouseEvent::eReal);
                event.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_KEYBOARD;
                nsEventStatus status = nsEventStatus_eIgnore;

                nsEventDispatcher::Dispatch(static_cast<nsIContent*>(this),
                                            aVisitor.mPresContext, &event,
                                            nullptr, &status);
                aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
              } 
            } 
          }
          if (aVisitor.mEvent->message == NS_KEY_PRESS &&
              mType == NS_FORM_INPUT_RADIO && !keyEvent->IsAlt() &&
              !keyEvent->IsControl() && !keyEvent->IsMeta()) {
            bool isMovingBack = false;
            switch (keyEvent->keyCode) {
              case NS_VK_UP: 
              case NS_VK_LEFT:
                isMovingBack = true;
                
              case NS_VK_DOWN:
              case NS_VK_RIGHT:
              
              nsIRadioGroupContainer* container = GetRadioGroupContainer();
              if (container) {
                nsAutoString name;
                GetAttr(kNameSpaceID_None, nsGkAtoms::name, name);
                nsCOMPtr<nsIDOMHTMLInputElement> selectedRadioButton;
                container->GetNextRadioButton(name, isMovingBack, this,
                                              getter_AddRefs(selectedRadioButton));
                nsCOMPtr<nsIContent> radioContent =
                  do_QueryInterface(selectedRadioButton);
                if (radioContent) {
                  rv = selectedRadioButton->Focus();
                  if (NS_SUCCEEDED(rv)) {
                    nsEventStatus status = nsEventStatus_eIgnore;
                    nsMouseEvent event(aVisitor.mEvent->mFlags.mIsTrusted,
                                       NS_MOUSE_CLICK, nullptr,
                                       nsMouseEvent::eReal);
                    event.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_KEYBOARD;
                    rv = nsEventDispatcher::Dispatch(radioContent,
                                                     aVisitor.mPresContext,
                                                     &event, nullptr, &status);
                    if (NS_SUCCEEDED(rv)) {
                      aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
                    }
                  }
                }
              }
            }
          }

          











          if (aVisitor.mEvent->message == NS_KEY_PRESS &&
              (keyEvent->keyCode == NS_VK_RETURN ||
               keyEvent->keyCode == NS_VK_ENTER) &&
               (IsSingleLineTextControl(false, mType) ||
                IsExperimentalMobileType(mType))) {
            FireChangeEventIfNeeded();
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
            nsFormEvent event(true, (mType == NS_FORM_INPUT_RESET) ?
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
      (NS_FAILED(LoadImage(uri, false, true)) ||
       !LoadingEnabled())) {
    CancelImageRequests(true);
  }
}

nsresult
nsHTMLInputElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                               nsIContent* aBindingParent,
                               bool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLFormElement::BindToTree(aDocument, aParent,
                                                     aBindingParent,
                                                     aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  nsImageLoadingContent::BindToTree(aDocument, aParent, aBindingParent,
                                    aCompileEventHandlers);

  if (mType == NS_FORM_INPUT_IMAGE) {
    
    
    if (HasAttr(kNameSpaceID_None, nsGkAtoms::src)) {
      
      
      ClearBrokenState();
      RemoveStatesSilently(NS_EVENT_STATE_BROKEN);
      nsContentUtils::AddScriptRunner(
        NS_NewRunnableMethod(this, &nsHTMLInputElement::MaybeLoadImage));
    }
  }

  
  
  if (aDocument && !mForm && mType == NS_FORM_INPUT_RADIO) {
    AddedToRadioGroup();
  }

  
  SetDirectionIfAuto(HasDirAuto(), false);

  
  
  UpdateValueMissingValidityState();

  
  
  
  UpdateBarredFromConstraintValidation();

  
  UpdateState(false);

  return rv;
}

void
nsHTMLInputElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  
  
  
  
  
  if (!mForm && mType == NS_FORM_INPUT_RADIO) {
    WillRemoveFromRadioGroup();
  }

  nsImageLoadingContent::UnbindFromTree(aDeep, aNullParent);
  nsGenericHTMLFormElement::UnbindFromTree(aDeep, aNullParent);

  
  
  UpdateValueMissingValidityState();
  
  UpdateBarredFromConstraintValidation();

  
  UpdateState(false);
}

void
nsHTMLInputElement::HandleTypeChange(uint8_t aNewType)
{
  ValueModeType aOldValueMode = GetValueMode();
  uint8_t oldType = mType;
  nsAutoString aOldValue;

  if (aOldValueMode == VALUE_MODE_VALUE) {
    GetValue(aOldValue);
  }

  
  FreeData();
  mType = aNewType;

  if (IsSingleLineTextControl()) {
    mInputData.mState = new nsTextEditorState(this);
  }

  



  switch (GetValueMode()) {
    case VALUE_MODE_DEFAULT:
    case VALUE_MODE_DEFAULT_ON:
      
      
      
      if (aOldValueMode == VALUE_MODE_VALUE && !aOldValue.IsEmpty()) {
        SetAttr(kNameSpaceID_None, nsGkAtoms::value, aOldValue, true);
      }
      break;
    case VALUE_MODE_VALUE:
      
      
      
      {
        nsAutoString value;
        if (aOldValueMode != VALUE_MODE_VALUE) {
          GetAttr(kNameSpaceID_None, nsGkAtoms::value, value);
        } else {
          value = aOldValue;
        }
        SetValueInternal(value, false, false);
      }
      break;
    case VALUE_MODE_FILENAME:
    default:
      
      
      break;
  }

  
  
  
  
  
  if (IsSingleLineTextControl(mType, false) &&
      !IsSingleLineTextControl(oldType, false)) {
    GetValueInternal(mFocusedValue);
  } else if (!IsSingleLineTextControl(mType, false) &&
             IsSingleLineTextControl(oldType, false)) {
    mFocusedValue.Truncate();
  }

  UpdateHasRange();

  
  UpdateAllValidityStates(false);
}

void
nsHTMLInputElement::SanitizeValue(nsAString& aValue)
{
  NS_ASSERTION(!mParserCreating, "The element parsing should be finished!");

  switch (mType) {
    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_SEARCH:
    case NS_FORM_INPUT_TEL:
    case NS_FORM_INPUT_PASSWORD:
      {
        PRUnichar crlf[] = { PRUnichar('\r'), PRUnichar('\n'), 0 };
        aValue.StripChars(crlf);
      }
      break;
    case NS_FORM_INPUT_EMAIL:
    case NS_FORM_INPUT_URL:
      {
        PRUnichar crlf[] = { PRUnichar('\r'), PRUnichar('\n'), 0 };
        aValue.StripChars(crlf);

        aValue = nsContentUtils::TrimWhitespace<nsContentUtils::IsHTMLWhitespace>(aValue);
      }
      break;
    case NS_FORM_INPUT_NUMBER:
      {
        nsresult ec;
        PromiseFlatString(aValue).ToDouble(&ec);
        if (NS_FAILED(ec)) {
          aValue.Truncate();
        }
      }
      break;
    case NS_FORM_INPUT_DATE:
      {
        if (!aValue.IsEmpty() && !IsValidDate(aValue)) {
          aValue.Truncate();
        }
      }
      break;
    case NS_FORM_INPUT_TIME:
      {
        if (!aValue.IsEmpty() && !IsValidTime(aValue)) {
          aValue.Truncate();
        }
      }
      break;
  }
}

bool
nsHTMLInputElement::IsValidDate(const nsAString& aValue) const
{
  uint32_t year, month, day;
  return GetValueAsDate(aValue, &year, &month, &day);
}

bool
nsHTMLInputElement::GetValueAsDate(const nsAString& aValue,
                                   uint32_t* aYear,
                                   uint32_t* aMonth,
                                   uint32_t* aDay) const
{









  if (aValue.Length() < 10) {
    return false;
  }

  uint32_t endOfYearOffset = 0;
  for (; NS_IsAsciiDigit(aValue[endOfYearOffset]); ++endOfYearOffset);

  
  if (aValue[endOfYearOffset] != '-' || endOfYearOffset < 4) {
    return false;
  }

  
  
  if (aValue[endOfYearOffset + 3] != '-' ||
      aValue.Length() != 10 + (endOfYearOffset - 4)) {
    return false;
  }

  nsresult ec;
  *aYear = PromiseFlatString(StringHead(aValue, endOfYearOffset)).ToInteger(&ec);
  if (NS_FAILED(ec) || *aYear == 0) {
    return false;
  }

  if (!DigitSubStringToNumber(aValue, endOfYearOffset + 1, 2, aMonth) ||
      *aMonth < 1 || *aMonth > 12) {
    return false;
  }

  return DigitSubStringToNumber(aValue, endOfYearOffset + 4, 2, aDay) &&
         *aDay > 0 && *aDay <= NumberOfDaysInMonth(*aMonth, *aYear);
}

uint32_t
nsHTMLInputElement::NumberOfDaysInMonth(uint32_t aMonth, uint32_t aYear) const
{








  static const bool longMonths[] = { true, false, true, false, true, false,
                                     true, true, false, true, false, true };
  MOZ_ASSERT(aMonth <= 12 && aMonth > 0);

  if (longMonths[aMonth-1]) {
    return 31;
  }

  if (aMonth != 2) {
    return 30;
  }

  return (aYear % 400 == 0 || (aYear % 100 != 0 && aYear % 4 == 0))
          ? 29 : 28;
}

 bool
nsHTMLInputElement::DigitSubStringToNumber(const nsAString& aStr,
                                           uint32_t aStart, uint32_t aLen,
                                           uint32_t* aRetVal)
{
  MOZ_ASSERT(aStr.Length() > (aStart + aLen - 1));

  for (uint32_t offset = 0; offset < aLen; ++offset) {
    if (!NS_IsAsciiDigit(aStr[aStart + offset])) {
      return false;
    }
  }

  nsresult ec;
  *aRetVal = static_cast<uint32_t>(PromiseFlatString(Substring(aStr, aStart, aLen)).ToInteger(&ec));

  return NS_SUCCEEDED(ec);
}

bool
nsHTMLInputElement::IsValidTime(const nsAString& aValue) const
{
  return ParseTime(aValue, nullptr);
}

 bool
nsHTMLInputElement::ParseTime(const nsAString& aValue, uint32_t* aResult)
{
  











  
  if (aValue.Length() < 5) {
    return false;
  }

  uint32_t hours;
  if (!DigitSubStringToNumber(aValue, 0, 2, &hours) || hours > 23) {
    return false;
  }

  
  if (aValue[2] != ':') {
    return false;
  }

  uint32_t minutes;
  if (!DigitSubStringToNumber(aValue, 3, 2, &minutes) || minutes > 59) {
    return false;
  }

  if (aValue.Length() == 5) {
    if (aResult) {
      *aResult = ((hours * 60) + minutes) * 60000;
    }
    return true;
  }

  
  if (aValue.Length() < 8 || aValue[5] != ':') {
    return false;
  }

  uint32_t seconds;
  if (!DigitSubStringToNumber(aValue, 6, 2, &seconds) || seconds > 59) {
    return false;
  }

  if (aValue.Length() == 8) {
    if (aResult) {
      *aResult = (((hours * 60) + minutes) * 60 + seconds) * 1000;
    }
    return true;
  }

  
  
  if (aValue.Length() == 9 || aValue.Length() > 12 || aValue[8] != '.') {
    return false;
  }

  uint32_t fractionsSeconds;
  if (!DigitSubStringToNumber(aValue, 9, aValue.Length() - 9, &fractionsSeconds)) {
    return false;
  }

  if (aResult) {
    *aResult = (((hours * 60) + minutes) * 60 + seconds) * 1000 +
               
               
               fractionsSeconds * pow(10.0, static_cast<int>(3 - (aValue.Length() - 9)));
  }

  return true;
}
 
bool
nsHTMLInputElement::ParseAttribute(int32_t aNamespaceID,
                                   nsIAtom* aAttribute,
                                   const nsAString& aValue,
                                   nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::type) {
      
      
      int32_t newType;
      bool success = aResult.ParseEnumValue(aValue, kInputTypeTable, false);
      if (success) {
        newType = aResult.GetEnumValue();
        if (IsExperimentalMobileType(newType) &&
            !Preferences::GetBool("dom.experimental_forms", false)) {
          newType = kInputDefaultType->value;
          aResult.SetTo(newType, &aValue);
        }
        if (newType == NS_FORM_INPUT_FILE &&
            Preferences::GetBool("dom.disable_input_file", false)) {
          newType = kInputDefaultType->value;
          aResult.SetTo(newType, &aValue);
        }
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
      return aResult.ParseEnumValue(aValue, kFormMethodTable, false);
    }
    if (aAttribute == nsGkAtoms::formenctype) {
      return aResult.ParseEnumValue(aValue, kFormEnctypeTable, false);
    }
    if (aAttribute == nsGkAtoms::autocomplete) {
      return aResult.ParseEnumValue(aValue, kInputAutocompleteTable, false);
    }
    if (aAttribute == nsGkAtoms::inputmode) {
      return aResult.ParseEnumValue(aValue, kInputInputmodeTable, false);
    }
    if (ParseImageAttribute(aAttribute, aValue, aResult)) {
      
      
      
      
      return true;
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
                                           int32_t aModType) const
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
             IsSingleLineTextControl(false)) {
    NS_UpdateHint(retval, NS_STYLE_HINT_REFLOW);
  } else if (PlaceholderApplies() && aAttribute == nsGkAtoms::placeholder) {
    NS_UpdateHint(retval, NS_STYLE_HINT_FRAMECHANGE);
  }
  return retval;
}

NS_IMETHODIMP_(bool)
nsHTMLInputElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::align },
    { &nsGkAtoms::type },
    { nullptr },
  };

  static const MappedAttributeEntry* const map[] = {
    attributes,
    sCommonAttributeMap,
    sImageMarginSizeAttributeMap,
    sImageBorderAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map);
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

  
  if (IsSingleLineTextControl(false))
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

      controller = do_CreateInstance("@mozilla.org/editor/editingcontroller;1",
                                     &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      mControllers->AppendController(controller);
    }
  }

  *aResult = mControllers;
  NS_IF_ADDREF(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::GetTextLength(int32_t* aTextLength)
{
  nsAutoString val;

  nsresult rv = GetValue(val);

  *aTextLength = val.Length();

  return rv;
}

NS_IMETHODIMP
nsHTMLInputElement::SetSelectionRange(int32_t aSelectionStart,
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

NS_IMETHODIMP
nsHTMLInputElement::GetSelectionStart(int32_t* aSelectionStart)
{
  NS_ENSURE_ARG_POINTER(aSelectionStart);

  int32_t selEnd;
  nsresult rv = GetSelectionRange(aSelectionStart, &selEnd);

  if (NS_FAILED(rv)) {
    nsTextEditorState *state = GetEditorState();
    if (state && state->IsSelectionCached()) {
      *aSelectionStart = state->GetSelectionProperties().mStart;
      return NS_OK;
    }
  }
  return rv;
}

NS_IMETHODIMP
nsHTMLInputElement::SetSelectionStart(int32_t aSelectionStart)
{
  nsTextEditorState *state = GetEditorState();
  if (state && state->IsSelectionCached()) {
    state->GetSelectionProperties().mStart = aSelectionStart;
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
nsHTMLInputElement::GetSelectionEnd(int32_t* aSelectionEnd)
{
  NS_ENSURE_ARG_POINTER(aSelectionEnd);

  int32_t selStart;
  nsresult rv = GetSelectionRange(&selStart, aSelectionEnd);

  if (NS_FAILED(rv)) {
    nsTextEditorState *state = GetEditorState();
    if (state && state->IsSelectionCached()) {
      *aSelectionEnd = state->GetSelectionProperties().mEnd;
      return NS_OK;
    }
  }
  return rv;
}


NS_IMETHODIMP
nsHTMLInputElement::SetSelectionEnd(int32_t aSelectionEnd)
{
  nsTextEditorState *state = GetEditorState();
  if (state && state->IsSelectionCached()) {
    state->GetSelectionProperties().mEnd = aSelectionEnd;
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

NS_IMETHODIMP
nsHTMLInputElement::GetFiles(nsIDOMFileList** aFileList)
{
  *aFileList = nullptr;

  if (mType != NS_FORM_INPUT_FILE) {
    return NS_OK;
  }

  if (!mFileList) {
    mFileList = new nsDOMFileList(static_cast<nsIContent*>(this));
    if (!mFileList) return NS_ERROR_OUT_OF_MEMORY;

    UpdateFileList();
  }

  NS_ADDREF(*aFileList = mFileList);

  return NS_OK;
}

nsresult
nsHTMLInputElement::GetSelectionRange(int32_t* aSelectionStart,
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

NS_IMETHODIMP
nsHTMLInputElement::GetSelectionDirection(nsAString& aDirection)
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
    nsTextEditorState *state = GetEditorState();
    if (state && state->IsSelectionCached()) {
      DirectionToName(state->GetSelectionProperties().mDirection, aDirection);
      return NS_OK;
    }
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLInputElement::SetSelectionDirection(const nsAString& aDirection) {
  nsTextEditorState *state = GetEditorState();
  if (state && state->IsSelectionCached()) {
    nsITextControlFrame::SelectionDirection dir = nsITextControlFrame::eNone;
    if (aDirection.EqualsLiteral("forward")) {
      dir = nsITextControlFrame::eForward;
    } else if (aDirection.EqualsLiteral("backward")) {
      dir = nsITextControlFrame::eBackward;
    }
    state->GetSelectionProperties().mDirection = dir;
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
nsHTMLInputElement::GetPhonetic(nsAString& aPhonetic)
{
  aPhonetic.Truncate();
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(true);

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
  if (NS_SUCCEEDED(nsEventDispatcher::CreateEvent(aPresContext, nullptr,
                                                  NS_LITERAL_STRING("Events"),
                                                  getter_AddRefs(event)))) {
    event->InitEvent(aEventType, true, true);
    event->SetTrusted(true);

    nsEventDispatcher::DispatchDOMEvent(aTarget, nullptr, event, aPresContext, nullptr);
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

  
  return SetValueInternal(resetVal, false, false);
}

void
nsHTMLInputElement::SetDirectionIfAuto(bool aAuto, bool aNotify)
{
  if (aAuto) {
    SetHasDirAuto();
    if (IsSingleLineTextControl(true)) {
      nsAutoString value;
      GetValue(value);
      SetDirectionalityFromValue(this, value, aNotify);
    }
  } else {
    ClearHasDirAuto();
  }
}

NS_IMETHODIMP
nsHTMLInputElement::Reset()
{
  
  SetCheckedChanged(false);
  SetValueChanged(false);

  switch (GetValueMode()) {
    case VALUE_MODE_VALUE:
      return SetDefaultValueAsValue();
    case VALUE_MODE_DEFAULT_ON:
      DoSetChecked(DefaultChecked(), true, false);
      return NS_OK;
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
  
  
  
  
  
  if (IsDisabled() || mType == NS_FORM_INPUT_RESET ||
      mType == NS_FORM_INPUT_BUTTON ||
      ((mType == NS_FORM_INPUT_SUBMIT || mType == NS_FORM_INPUT_IMAGE) &&
       aFormSubmission->GetOriginatingElement() != this) ||
      ((mType == NS_FORM_INPUT_RADIO || mType == NS_FORM_INPUT_CHECKBOX) &&
       !mChecked)) {
    return NS_OK;
  }

  
  nsAutoString name;
  GetAttr(kNameSpaceID_None, nsGkAtoms::name, name);

  
  if (mType == NS_FORM_INPUT_IMAGE) {
    
    nsIntPoint* lastClickedPoint =
      static_cast<nsIntPoint*>(GetProperty(nsGkAtoms::imageClickedPoint));
    int32_t x, y;
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

  
  
  

  
  if (name.IsEmpty()) {
    return NS_OK;
  }

  
  nsAutoString value;
  nsresult rv = GetValue(value);
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

    for (int32_t i = 0; i < files.Count(); ++i) {
      aFormSubmission->AddNameFilePair(name, files[i]);
    }

    if (files.Count() == 0) {
      
      
      aFormSubmission->AddNameFilePair(name, nullptr);

    }

    return NS_OK;
  }

  if (mType == NS_FORM_INPUT_HIDDEN && name.EqualsLiteral("_charset_")) {
    nsCString charset;
    aFormSubmission->GetCharset(charset);
    return aFormSubmission->AddNameValuePair(name,
                                             NS_ConvertASCIItoUTF16(charset));
  }
  if (IsSingleLineTextControl(true) &&
      name.EqualsLiteral("isindex") &&
      aFormSubmission->SupportsIsindexSubmission()) {
    return aFormSubmission->AddIsindex(value);
  }
  return aFormSubmission->AddNameValuePair(name, value);
}


NS_IMETHODIMP
nsHTMLInputElement::SaveState()
{
  nsRefPtr<nsHTMLInputElementState> inputState;
  switch (mType) {
    case NS_FORM_INPUT_CHECKBOX:
    case NS_FORM_INPUT_RADIO:
      {
        if (mCheckedChanged) {
          inputState = new nsHTMLInputElementState();
          inputState->SetChecked(mChecked);
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
    case NS_FORM_INPUT_NUMBER:
    case NS_FORM_INPUT_DATE:
    case NS_FORM_INPUT_TIME:
      {
        if (mValueChanged) {
          inputState = new nsHTMLInputElementState();
          nsAutoString value;
          GetValue(value);
          DebugOnly<nsresult> rv =
            nsLinebreakConverter::ConvertStringLineBreaks(
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
          inputState->SetFiles(mFiles);
        }
        break;
      }
  }
  
  nsresult rv = NS_OK;
  nsPresState* state = nullptr;
  if (inputState) {
    rv = GetPrimaryPresState(this, &state);
    if (state) {
      state->SetStateProperty(inputState);
    }
  }

  if (mDisabledChanged) {
    nsresult tmp = GetPrimaryPresState(this, &state);
    if (NS_FAILED(tmp)) {
      rv = tmp;
    }
    if (state) {
      
      
      state->SetDisabled(HasAttr(kNameSpaceID_None, nsGkAtoms::disabled));
    }
  }

  return rv;
}

void
nsHTMLInputElement::DoneCreatingElement()
{
  mParserCreating = false;

  
  
  
  
  bool restoredCheckedState =
    !mInhibitRestoration && RestoreFormControlState(this, this);

  
  
  
  
  if (!restoredCheckedState && mShouldInitChecked) {
    DoSetChecked(DefaultChecked(), false, true);
    DoSetCheckedChanged(false, false);
  }

  
  if (GetValueMode() == VALUE_MODE_VALUE) {
    nsAutoString aValue;
    GetValue(aValue);
    SetValueInternal(aValue, false, false);
  }

  mShouldInitChecked = false;
}

nsEventStates
nsHTMLInputElement::IntrinsicState() const
{
  
  
  
  nsEventStates state = nsGenericHTMLFormElement::IntrinsicState();
  if (mType == NS_FORM_INPUT_CHECKBOX || mType == NS_FORM_INPUT_RADIO) {
    
    if (mChecked) {
      state |= NS_EVENT_STATE_CHECKED;
    }

    
    if (mType == NS_FORM_INPUT_CHECKBOX && mIndeterminate) {
      state |= NS_EVENT_STATE_INDETERMINATE;
    }

    
    if (DefaultChecked()) {
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
           (mCanShowInvalidUI && ShouldShowValidityUI()))) {
        state |= NS_EVENT_STATE_MOZ_UI_INVALID;
      }
    }

    
    
    
    
    
    
    
    
    
    if ((!mForm || !mForm->HasAttr(kNameSpaceID_None, nsGkAtoms::novalidate)) &&
        (mCanShowValidUI && ShouldShowValidityUI() &&
         (IsValid() || (!state.HasState(NS_EVENT_STATE_MOZ_UI_INVALID) &&
                        !mCanShowInvalidUI)))) {
      state |= NS_EVENT_STATE_MOZ_UI_VALID;
    }
  }

  if (mForm && !mForm->GetValidity() && IsSubmitControl()) {
    state |= NS_EVENT_STATE_MOZ_SUBMITINVALID;
  }

  
  if (mHasRange) {
    state |= (GetValidityState(VALIDITY_STATE_RANGE_OVERFLOW) ||
              GetValidityState(VALIDITY_STATE_RANGE_UNDERFLOW))
               ? NS_EVENT_STATE_OUTOFRANGE
               : NS_EVENT_STATE_INRANGE;
  }

  return state;
}

bool
nsHTMLInputElement::RestoreState(nsPresState* aState)
{
  bool restoredCheckedState = false;

  nsCOMPtr<nsHTMLInputElementState> inputState
    (do_QueryInterface(aState->GetStateProperty()));

  if (inputState) {
    switch (mType) {
      case NS_FORM_INPUT_CHECKBOX:
      case NS_FORM_INPUT_RADIO:
        {
          if (inputState->IsCheckedSet()) {
            restoredCheckedState = true;
            DoSetChecked(inputState->GetChecked(), true, true);
          }
          break;
        }

      case NS_FORM_INPUT_EMAIL:
      case NS_FORM_INPUT_SEARCH:
      case NS_FORM_INPUT_TEXT:
      case NS_FORM_INPUT_TEL:
      case NS_FORM_INPUT_URL:
      case NS_FORM_INPUT_HIDDEN:
      case NS_FORM_INPUT_NUMBER:
      case NS_FORM_INPUT_DATE:
      case NS_FORM_INPUT_TIME:
        {
          SetValueInternal(inputState->GetValue(), false, true);
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

bool
nsHTMLInputElement::AllowDrop()
{
  

  return mType != NS_FORM_INPUT_FILE;
}





void
nsHTMLInputElement::AddedToRadioGroup()
{
  
  
  if (!mForm && !IsInDoc()) {
    return;
  }

  
  bool notify = !mParserCreating;

  
  
  
  
  if (mChecked) {
    
    
    
    
    
    
    
    RadioSetChecked(notify);
  }

  
  
  
  
  bool checkedChanged = mCheckedChanged;

  nsCOMPtr<nsIRadioVisitor> visitor =
    new nsRadioGetCheckedChangedVisitor(&checkedChanged, this);
  VisitGroup(visitor, notify);

  SetCheckedChangedInternal(checkedChanged);

  
  
  
  nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
  if (container) {
    nsAutoString name;
    GetAttr(kNameSpaceID_None, nsGkAtoms::name, name);
    container->AddToRadioGroup(name, static_cast<nsIFormControl*>(this));

    
    
    SetValidityState(VALIDITY_STATE_VALUE_MISSING,
                     container->GetValueMissingState(name));
  }
}

void
nsHTMLInputElement::WillRemoveFromRadioGroup()
{
  nsIRadioGroupContainer* container = GetRadioGroupContainer();
  if (!container) {
    return;
  }

  nsAutoString name;
  GetAttr(kNameSpaceID_None, nsGkAtoms::name, name);

  
  
  if (mChecked) {
    container->SetCurrentRadioButton(name, nullptr);
  }

  
  
  
  UpdateValueMissingValidityStateForRadio(true);
  container->RemoveFromRadioGroup(name, static_cast<nsIFormControl*>(this));
}

bool
nsHTMLInputElement::IsHTMLFocusable(bool aWithMouse, bool *aIsFocusable, int32_t *aTabIndex)
{
  if (nsGenericHTMLFormElement::IsHTMLFocusable(aWithMouse, aIsFocusable, aTabIndex)) {
    return true;
  }

  if (IsDisabled()) {
    *aIsFocusable = false;
    return true;
  }

  if (IsSingleLineTextControl(false)) {
    *aIsFocusable = true;
    return false;
  }

#ifdef XP_MACOSX
  const bool defaultFocusable = !aWithMouse || nsFocusManager::sMouseFocusesFormControl;
#else
  const bool defaultFocusable = true;
#endif

  if (mType == NS_FORM_INPUT_FILE) {
    if (aTabIndex) {
      *aTabIndex = -1;
    }
    *aIsFocusable = defaultFocusable;
    return true;
  }

  if (mType == NS_FORM_INPUT_HIDDEN) {
    if (aTabIndex) {
      *aTabIndex = -1;
    }
    *aIsFocusable = false;
    return false;
  }

  if (!aTabIndex) {
    
    *aIsFocusable = defaultFocusable;
    return false;
  }

  if (mType != NS_FORM_INPUT_RADIO) {
    *aIsFocusable = defaultFocusable;
    return false;
  }

  if (mChecked) {
    
    *aIsFocusable = defaultFocusable;
    return false;
  }

  
  
  nsIRadioGroupContainer* container = GetRadioGroupContainer();
  if (!container) {
    *aIsFocusable = defaultFocusable;
    return false;
  }

  nsAutoString name;
  GetAttr(kNameSpaceID_None, nsGkAtoms::name, name);

  nsCOMPtr<nsIDOMHTMLInputElement> currentRadio = container->GetCurrentRadioButton(name);
  if (currentRadio) {
    *aTabIndex = -1;
  }
  *aIsFocusable = defaultFocusable;
  return false;
}

nsresult
nsHTMLInputElement::VisitGroup(nsIRadioVisitor* aVisitor, bool aFlushContent)
{
  nsIRadioGroupContainer* container = GetRadioGroupContainer();
  if (container) {
    nsAutoString name;
    GetAttr(kNameSpaceID_None, nsGkAtoms::name, name);
    return container->WalkRadioGroup(name, aVisitor, aFlushContent);
  }

  aVisitor->Visit(this);
  return NS_OK;
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
    case NS_FORM_INPUT_NUMBER:
    case NS_FORM_INPUT_DATE:
    case NS_FORM_INPUT_TIME:
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

bool
nsHTMLInputElement::IsMutable() const
{
  return !IsDisabled() && GetCurrentDoc() &&
         !(DoesReadOnlyApply() &&
           HasAttr(kNameSpaceID_None, nsGkAtoms::readonly));
}

bool
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
    
    
    
      return false;
#ifdef DEBUG
    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_PASSWORD:
    case NS_FORM_INPUT_SEARCH:
    case NS_FORM_INPUT_TEL:
    case NS_FORM_INPUT_EMAIL:
    case NS_FORM_INPUT_URL:
    case NS_FORM_INPUT_NUMBER:
    case NS_FORM_INPUT_DATE:
    case NS_FORM_INPUT_TIME:
      return true;
    default:
      NS_NOTYETIMPLEMENTED("Unexpected input type in DoesReadOnlyApply()");
      return true;
#else 
    default:
      return true;
#endif 
  }
}

bool
nsHTMLInputElement::DoesRequiredApply() const
{
  switch (mType)
  {
    case NS_FORM_INPUT_HIDDEN:
    case NS_FORM_INPUT_BUTTON:
    case NS_FORM_INPUT_IMAGE:
    case NS_FORM_INPUT_RESET:
    case NS_FORM_INPUT_SUBMIT:
    
    
    
      return false;
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
    case NS_FORM_INPUT_NUMBER:
    case NS_FORM_INPUT_DATE:
    case NS_FORM_INPUT_TIME:
      return true;
    default:
      NS_NOTYETIMPLEMENTED("Unexpected input type in DoesRequiredApply()");
      return true;
#else 
    default:
      return true;
#endif 
  }
}

bool
nsHTMLInputElement::PlaceholderApplies() const
{
  if (mType == NS_FORM_INPUT_DATE ||
      mType == NS_FORM_INPUT_TIME) {
    return false;
  }

  return IsSingleLineTextControl(false);
}

bool
nsHTMLInputElement::DoesPatternApply() const
{
  
  if (IsExperimentalMobileType(mType)) {
    return false;
  }

  return IsSingleLineTextControl(false);
}

bool
nsHTMLInputElement::DoesMinMaxApply() const
{
  switch (mType)
  {
    case NS_FORM_INPUT_NUMBER:
    case NS_FORM_INPUT_DATE:
    case NS_FORM_INPUT_TIME:
    
    
    
      return true;
#ifdef DEBUG
    case NS_FORM_INPUT_RESET:
    case NS_FORM_INPUT_SUBMIT:
    case NS_FORM_INPUT_IMAGE:
    case NS_FORM_INPUT_BUTTON:
    case NS_FORM_INPUT_HIDDEN:
    case NS_FORM_INPUT_RADIO:
    case NS_FORM_INPUT_CHECKBOX:
    case NS_FORM_INPUT_FILE:
    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_PASSWORD:
    case NS_FORM_INPUT_SEARCH:
    case NS_FORM_INPUT_TEL:
    case NS_FORM_INPUT_EMAIL:
    case NS_FORM_INPUT_URL:
      return false;
    default:
      NS_NOTYETIMPLEMENTED("Unexpected input type in DoesRequiredApply()");
      return false;
#else 
    default:
      return false;
#endif 
  }
}

double
nsHTMLInputElement::GetStep() const
{
  MOZ_ASSERT(DoesStepApply(), "GetStep() can only be called if @step applies");

  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::step)) {
    return GetDefaultStep() * GetStepScaleFactor();
  }

  nsAutoString stepStr;
  GetAttr(kNameSpaceID_None, nsGkAtoms::step, stepStr);

  if (stepStr.LowerCaseEqualsLiteral("any")) {
    
    return kStepAny;
  }

  nsresult ec;
  double step = stepStr.ToDouble(&ec);
  if (NS_FAILED(ec) || step <= 0) {
    step = GetDefaultStep();
  }

  
  
  return step * GetStepScaleFactor();
}



NS_IMETHODIMP
nsHTMLInputElement::SetCustomValidity(const nsAString& aError)
{
  nsIConstraintValidation::SetCustomValidity(aError);

  UpdateState(true);

  return NS_OK;
}

bool
nsHTMLInputElement::IsTooLong()
{
  if (!MaxLengthApplies() ||
      !HasAttr(kNameSpaceID_None, nsGkAtoms::maxlength) ||
      !mValueChanged) {
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
nsHTMLInputElement::IsValueMissing() const
{
  
  MOZ_ASSERT(mType != NS_FORM_INPUT_RADIO);

  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::required) ||
      !DoesRequiredApply()) {
    return false;
  }

  if (!IsMutable()) {
    return false;
  }

  switch (GetValueMode()) {
    case VALUE_MODE_VALUE:
      return IsValueEmpty();
    case VALUE_MODE_FILENAME:
    {
      const nsCOMArray<nsIDOMFile>& files = GetFiles();
      return !files.Count();
    }
    case VALUE_MODE_DEFAULT_ON:
      
      
      return !mChecked;
    case VALUE_MODE_DEFAULT:
    default:
      return false;
  }
}

bool
nsHTMLInputElement::HasTypeMismatch() const
{
  if (mType != NS_FORM_INPUT_EMAIL && mType != NS_FORM_INPUT_URL) {
    return false;
  }

  nsAutoString value;
  NS_ENSURE_SUCCESS(GetValueInternal(value), false);

  if (value.IsEmpty()) {
    return false;
  }

  if (mType == NS_FORM_INPUT_EMAIL) {
    return HasAttr(kNameSpaceID_None, nsGkAtoms::multiple)
             ? !IsValidEmailAddressList(value) : !IsValidEmailAddress(value);
  } else if (mType == NS_FORM_INPUT_URL) {
    










    nsCOMPtr<nsIIOService> ioService = do_GetIOService();
    nsCOMPtr<nsIURI> uri;

    return !NS_SUCCEEDED(ioService->NewURI(NS_ConvertUTF16toUTF8(value), nullptr,
                                           nullptr, getter_AddRefs(uri)));
  }

  return false;
}

bool
nsHTMLInputElement::HasPatternMismatch() const
{
  if (!DoesPatternApply() ||
      !HasAttr(kNameSpaceID_None, nsGkAtoms::pattern)) {
    return false;
  }

  nsAutoString pattern;
  GetAttr(kNameSpaceID_None, nsGkAtoms::pattern, pattern);

  nsAutoString value;
  NS_ENSURE_SUCCESS(GetValueInternal(value), false);

  if (value.IsEmpty()) {
    return false;
  }

  nsIDocument* doc = OwnerDoc();

  return !nsContentUtils::IsPatternMatching(value, pattern, doc);
}

bool
nsHTMLInputElement::IsRangeOverflow() const
{
  if (!DoesMinMaxApply()) {
    return false;
  }

  double maximum = GetMaximum();
  if (MOZ_DOUBLE_IS_NaN(maximum)) {
    return false;
  }

  double value = GetValueAsDouble();
  if (MOZ_DOUBLE_IS_NaN(value)) {
    return false;
  }

  return value > maximum;
}

bool
nsHTMLInputElement::IsRangeUnderflow() const
{
  if (!DoesMinMaxApply()) {
    return false;
  }

  double minimum = GetMinimum();
  if (MOZ_DOUBLE_IS_NaN(minimum)) {
    return false;
  }

  double value = GetValueAsDouble();
  if (MOZ_DOUBLE_IS_NaN(value)) {
    return false;
  }

  return value < minimum;
}

bool
nsHTMLInputElement::HasStepMismatch() const
{
  if (!DoesStepApply()) {
    return false;
  }

  double value = GetValueAsDouble();
  if (MOZ_DOUBLE_IS_NaN(value)) {
    
    return false;
  }

  double step = GetStep();
  if (step == kStepAny) {
    return false;
  }

  if (mType == NS_FORM_INPUT_DATE) {
    
    
    
    
    
    step = NS_round(step);
  }

  
  return NS_floorModulo(value - GetStepBase(), step) != 0;
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
  bool notify = !mParserCreating;
  nsCOMPtr<nsIDOMHTMLInputElement> selection = GetSelectedRadioButton();

  
  
  bool selected = selection || (!aIgnoreSelf && mChecked);
  bool required = !aIgnoreSelf && HasAttr(kNameSpaceID_None, nsGkAtoms::required);
  bool valueMissing = false;

  nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();

  if (!container) {
    SetValidityState(VALIDITY_STATE_VALUE_MISSING,
                     IsMutable() && required && !selected);
    return;
  }

  nsAutoString name;
  GetAttr(kNameSpaceID_None, nsGkAtoms::name, name);

  
  
  if (!required) {
    required = (aIgnoreSelf && HasAttr(kNameSpaceID_None, nsGkAtoms::required))
                 ? container->GetRequiredRadioCount(name) - 1
                 : container->GetRequiredRadioCount(name);
  }

  valueMissing = IsMutable() && required && !selected;

  if (container->GetValueMissingState(name) != valueMissing) {
    container->SetValueMissingState(name, valueMissing);

    SetValidityState(VALIDITY_STATE_VALUE_MISSING, valueMissing);

    
    nsAutoScriptBlocker scriptBlocker;
    nsCOMPtr<nsIRadioVisitor> visitor =
      new nsRadioSetValueMissingState(this, valueMissing, notify);
    VisitGroup(visitor, notify);
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
nsHTMLInputElement::UpdateRangeOverflowValidityState()
{
  SetValidityState(VALIDITY_STATE_RANGE_OVERFLOW, IsRangeOverflow());
}

void
nsHTMLInputElement::UpdateRangeUnderflowValidityState()
{
  SetValidityState(VALIDITY_STATE_RANGE_UNDERFLOW, IsRangeUnderflow());
}

void
nsHTMLInputElement::UpdateStepMismatchValidityState()
{
  SetValidityState(VALIDITY_STATE_STEP_MISMATCH, HasStepMismatch());
}

void
nsHTMLInputElement::UpdateAllValidityStates(bool aNotify)
{
  bool validBefore = IsValid();
  UpdateTooLongValidityState();
  UpdateValueMissingValidityState();
  UpdateTypeMismatchValidityState();
  UpdatePatternMismatchValidityState();
  UpdateRangeOverflowValidityState();
  UpdateRangeUnderflowValidityState();
  UpdateStepMismatchValidityState();

  if (validBefore != IsValid()) {
    UpdateState(aNotify);
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
      break;
    }
    case VALIDITY_STATE_VALUE_MISSING:
    {
      nsXPIDLString message;
      nsAutoCString key;
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
      nsAutoCString key;
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
                                                   params, message);
      }
      aValidationMessage = message;
      break;
    }
    case VALIDITY_STATE_RANGE_OVERFLOW:
    {
      nsXPIDLString message;

      nsAutoString maxStr;
      if (mType == NS_FORM_INPUT_NUMBER) {
        
        double maximum = GetMaximum();
        MOZ_ASSERT(!MOZ_DOUBLE_IS_NaN(maximum));

        maxStr.AppendFloat(maximum);
      } else if (mType == NS_FORM_INPUT_DATE || mType == NS_FORM_INPUT_TIME) {
        GetAttr(kNameSpaceID_None, nsGkAtoms::max, maxStr);
      } else {
        NS_NOTREACHED("Unexpected input type");
      }

      const PRUnichar* params[] = { maxStr.get() };
      rv = nsContentUtils::FormatLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                 "FormValidationRangeOverflow",
                                                 params, message);
      aValidationMessage = message;
      break;
    }
    case VALIDITY_STATE_RANGE_UNDERFLOW:
    {
      nsXPIDLString message;

      nsAutoString minStr;
      if (mType == NS_FORM_INPUT_NUMBER) {
        double minimum = GetMinimum();
        MOZ_ASSERT(!MOZ_DOUBLE_IS_NaN(minimum));

        minStr.AppendFloat(minimum);
      } else if (mType == NS_FORM_INPUT_DATE || mType == NS_FORM_INPUT_TIME) {
        GetAttr(kNameSpaceID_None, nsGkAtoms::min, minStr);
      } else {
        NS_NOTREACHED("Unexpected input type");
      }

      const PRUnichar* params[] = { minStr.get() };
      rv = nsContentUtils::FormatLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                 "FormValidationRangeUnderflow",
                                                 params, message);
      aValidationMessage = message;
      break;
    }
    case VALIDITY_STATE_STEP_MISMATCH:
    {
      nsXPIDLString message;

      double value = GetValueAsDouble();
      MOZ_ASSERT(!MOZ_DOUBLE_IS_NaN(value));

      double step = GetStep();
      MOZ_ASSERT(step != kStepAny);

      
      
      
      
      
      if (mType == NS_FORM_INPUT_DATE) {
        step = EuclidLCM<uint64_t>(static_cast<uint64_t>(step),
                                   static_cast<uint64_t>(GetStepScaleFactor()));
      }

      double stepBase = GetStepBase();

      double valueLow = value - NS_floorModulo(value - stepBase, step);
      double valueHigh = value + step - NS_floorModulo(value - stepBase, step);

      double maximum = GetMaximum();

      if (MOZ_DOUBLE_IS_NaN(maximum) || valueHigh <= maximum) {
        nsAutoString valueLowStr, valueHighStr;
        ConvertNumberToString(valueLow, valueLowStr);
        ConvertNumberToString(valueHigh, valueHighStr);

        if (valueLowStr.Equals(valueHighStr)) {
          const PRUnichar* params[] = { valueLowStr.get() };
          rv = nsContentUtils::FormatLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                     "FormValidationStepMismatchOneValue",
                                                     params, message);
        } else {
          const PRUnichar* params[] = { valueLowStr.get(), valueHighStr.get() };
          rv = nsContentUtils::FormatLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                     "FormValidationStepMismatch",
                                                     params, message);
        }
      } else {
        nsAutoString valueLowStr;
        ConvertNumberToString(valueLow, valueLowStr);

        const PRUnichar* params[] = { valueLowStr.get() };
        rv = nsContentUtils::FormatLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                   "FormValidationStepMismatchOneValue",
                                                   params, message);
      }

      aValidationMessage = message;
      break;
    }
    default:
      rv = nsIConstraintValidation::GetValidationMessage(aValidationMessage, aType);
  }

  return rv;
}


bool
nsHTMLInputElement::IsValidEmailAddressList(const nsAString& aValue)
{
  HTMLSplitOnSpacesTokenizer tokenizer(aValue, ',');

  while (tokenizer.hasMoreTokens()) {
    if (!IsValidEmailAddress(tokenizer.nextToken())) {
      return false;
    }
  }

  return !tokenizer.lastTokenEndedWithSeparator();
}


bool
nsHTMLInputElement::IsValidEmailAddress(const nsAString& aValue)
{
  nsAutoCString value = NS_ConvertUTF16toUTF8(aValue);
  uint32_t i = 0;
  uint32_t length = value.Length();

  
  nsCOMPtr<nsIIDNService> idnSrv = do_GetService(NS_IDNSERVICE_CONTRACTID);
  if (idnSrv) {
    bool ace;
    if (NS_SUCCEEDED(idnSrv->IsACE(value, &ace)) && !ace) {
      nsAutoCString punyCodedValue;
      if (NS_SUCCEEDED(idnSrv->ConvertUTF8toACE(value, punyCodedValue))) {
        value = punyCodedValue;
        length = value.Length();
      }
    }
  } else {
    NS_ERROR("nsIIDNService isn't present!");
  }

  
  
  if (length == 0 || value[0] == '@' || value[length-1] == '.') {
    return false;
  }

  
  for (; i < length && value[i] != '@'; ++i) {
    PRUnichar c = value[i];

    
    if (!(nsCRT::IsAsciiAlpha(c) || nsCRT::IsAsciiDigit(c) ||
          c == '.' || c == '!' || c == '#' || c == '$' || c == '%' ||
          c == '&' || c == '\''|| c == '*' || c == '+' || c == '-' ||
          c == '/' || c == '=' || c == '?' || c == '^' || c == '_' ||
          c == '`' || c == '{' || c == '|' || c == '}' || c == '~' )) {
      return false;
    }
  }

  
  
  if (++i >= length) {
    return false;
  }

  
  if (value[i] == '.') {
    return false;
  }

  
  for (; i < length; ++i) {
    PRUnichar c = value[i];

    if (c == '.') {
      
      if (value[i-1] == '.') {
        return false;
      }
    } else if (!(nsCRT::IsAsciiAlpha(c) || nsCRT::IsAsciiDigit(c) ||
                 c == '-')) {
      
      return false;
    }
  }

  return true;
}

NS_IMETHODIMP_(bool)
nsHTMLInputElement::IsSingleLineTextControl() const
{
  return IsSingleLineTextControl(false);
}

NS_IMETHODIMP_(bool)
nsHTMLInputElement::IsTextArea() const
{
  return false;
}

NS_IMETHODIMP_(bool)
nsHTMLInputElement::IsPlainTextControl() const
{
  
  return true;
}

NS_IMETHODIMP_(bool)
nsHTMLInputElement::IsPasswordTextControl() const
{
  return mType == NS_FORM_INPUT_PASSWORD;
}

NS_IMETHODIMP_(int32_t)
nsHTMLInputElement::GetCols()
{
  
  const nsAttrValue* attr = GetParsedAttr(nsGkAtoms::size);
  if (attr && attr->Type() == nsAttrValue::eInteger) {
    int32_t cols = attr->GetIntegerValue();
    if (cols > 0) {
      return cols;
    }
  }

  return DEFAULT_COLS;
}

NS_IMETHODIMP_(int32_t)
nsHTMLInputElement::GetWrapCols()
{
  return -1; 
}

NS_IMETHODIMP_(int32_t)
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
    
    
    if (!mParserCreating) {
      SanitizeValue(aValue);
    }
  }
}

NS_IMETHODIMP_(bool)
nsHTMLInputElement::ValueChanged() const
{
  return mValueChanged;
}

NS_IMETHODIMP_(void)
nsHTMLInputElement::GetTextEditorValue(nsAString& aValue,
                                       bool aIgnoreWrap) const
{
  nsTextEditorState *state = GetEditorState();
  if (state) {
    state->GetValue(aValue, aIgnoreWrap);
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
nsHTMLInputElement::OnValueChanged(bool aNotify)
{
  UpdateAllValidityStates(aNotify);

  if (HasDirAuto()) {
    SetDirectionIfAuto(true, aNotify);
  }
}

NS_IMETHODIMP_(bool)
nsHTMLInputElement::HasCachedSelection()
{
  bool isCached = false;
  nsTextEditorState *state = GetEditorState();
  if (state) {
    isCached = state->IsSelectionCached() &&
               state->HasNeverInitializedBefore() &&
               !state->GetSelectionProperties().IsDefault();
    if (isCached) {
      state->WillInitEagerly();
    }
  }
  return isCached;
}

void
nsHTMLInputElement::FieldSetDisabledChanged(bool aNotify)
{
  UpdateValueMissingValidityState();
  UpdateBarredFromConstraintValidation();

  nsGenericHTMLFormElement::FieldSetDisabledChanged(aNotify);
}

void
nsHTMLInputElement::SetFilePickerFiltersFromAccept(nsIFilePicker* filePicker)
{
  
  filePicker->AppendFilters(nsIFilePicker::filterAll);

  NS_ASSERTION(HasAttr(kNameSpaceID_None, nsGkAtoms::accept),
               "You should not call SetFilePickerFiltersFromAccept if the"
               " element has no accept attribute!");

  
  nsCOMPtr<nsIStringBundleService> stringService =
    mozilla::services::GetStringBundleService();
  if (!stringService) {
    return;
  }
  nsCOMPtr<nsIStringBundle> filterBundle;
  if (NS_FAILED(stringService->CreateBundle("chrome://global/content/filepicker.properties",
                                            getter_AddRefs(filterBundle)))) {
    return;
  }

  
  nsCOMPtr<nsIMIMEService> mimeService = do_GetService("@mozilla.org/mime;1");
  if (!mimeService) {
    return;
  }

  nsAutoString accept;
  GetAttr(kNameSpaceID_None, nsGkAtoms::accept, accept);

  HTMLSplitOnSpacesTokenizer tokenizer(accept, ',');

  nsTArray<nsFilePickerFilter> filters;
  nsString allExtensionsList;

  
  while (tokenizer.hasMoreTokens()) {
    const nsDependentSubstring& token = tokenizer.nextToken();

    if (token.IsEmpty()) {
      continue;
    }

    int32_t filterMask = 0;
    nsString filterName;
    nsString extensionListStr;

    
    if (token.EqualsLiteral("image/*")) {
      filterMask = nsIFilePicker::filterImages;
      filterBundle->GetStringFromName(NS_LITERAL_STRING("imageFilter").get(),
                                      getter_Copies(extensionListStr));
    } else if (token.EqualsLiteral("audio/*")) {
      filterMask = nsIFilePicker::filterAudio;
      filterBundle->GetStringFromName(NS_LITERAL_STRING("audioFilter").get(),
                                      getter_Copies(extensionListStr));
    } else if (token.EqualsLiteral("video/*")) {
      filterMask = nsIFilePicker::filterVideo;
      filterBundle->GetStringFromName(NS_LITERAL_STRING("videoFilter").get(),
                                      getter_Copies(extensionListStr));
    } else {
      
      nsCOMPtr<nsIMIMEInfo> mimeInfo;
      if (NS_FAILED(mimeService->GetFromTypeAndExtension(
                      NS_ConvertUTF16toUTF8(token),
                      EmptyCString(), 
                      getter_AddRefs(mimeInfo))) ||
          !mimeInfo) {
        continue;
      }

      
      nsCString mimeTypeName;
      mimeInfo->GetType(mimeTypeName);
      CopyUTF8toUTF16(mimeTypeName, filterName);

      
      nsCOMPtr<nsIUTF8StringEnumerator> extensions;
      mimeInfo->GetFileExtensions(getter_AddRefs(extensions));

      bool hasMore;
      while (NS_SUCCEEDED(extensions->HasMore(&hasMore)) && hasMore) {
        nsCString extension;
        if (NS_FAILED(extensions->GetNext(extension))) {
          continue;
        }
        if (!extensionListStr.IsEmpty()) {
          extensionListStr.AppendLiteral("; ");
        }
        extensionListStr += NS_LITERAL_STRING("*.") +
                            NS_ConvertUTF8toUTF16(extension);
      }
    }

    if (!filterMask && (extensionListStr.IsEmpty() || filterName.IsEmpty())) {
      
      continue;
    }

    
    
    nsFilePickerFilter filter;
    if (filterMask) {
      filter = nsFilePickerFilter(filterMask);
    } else {
      filter = nsFilePickerFilter(filterName, extensionListStr);
    }

    if (!filters.Contains(filter)) {
      if (!allExtensionsList.IsEmpty()) {
        allExtensionsList.AppendLiteral("; ");
      }
      allExtensionsList += extensionListStr;
      filters.AppendElement(filter);
    }
  }

  
  if (filters.Length() > 1) {
    nsXPIDLString title;
    nsContentUtils::GetLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                       "AllSupportedTypes", title);
    filePicker->AppendFilter(title, allExtensionsList);
  }

  
  bool allFilterAreTrusted = true;
  for (uint32_t i = 0; i < filters.Length(); ++i) {
    const nsFilePickerFilter& filter = filters[i];
    if (filter.mFilterMask) {
      filePicker->AppendFilters(filter.mFilterMask);
    } else {
      filePicker->AppendFilter(filter.mTitle, filter.mFilter);
    }
    allFilterAreTrusted &= filter.mIsTrusted;
  }

  
  
  if (filters.Length() >= 1 && allFilterAreTrusted) {
    
    
    filePicker->SetFilterIndex(1);
  }
}

int32_t
nsHTMLInputElement::GetFilterFromAccept()
{
  NS_ASSERTION(HasAttr(kNameSpaceID_None, nsGkAtoms::accept),
               "You should not call GetFileFiltersFromAccept if the element"
               " has no accept attribute!");

  int32_t filter = 0;
  nsAutoString accept;
  GetAttr(kNameSpaceID_None, nsGkAtoms::accept, accept);

  HTMLSplitOnSpacesTokenizer tokenizer(accept, ',');

  while (tokenizer.hasMoreTokens()) {
    const nsDependentSubstring token = tokenizer.nextToken();

    int32_t tokenFilter = 0;
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

double
nsHTMLInputElement::GetStepScaleFactor() const
{
  MOZ_ASSERT(DoesStepApply());

  switch (mType) {
    case NS_FORM_INPUT_DATE:
      return kStepScaleFactorDate;
    case NS_FORM_INPUT_NUMBER:
      return kStepScaleFactorNumber;
    case NS_FORM_INPUT_TIME:
      return kStepScaleFactorTime;
    default:
      MOZ_NOT_REACHED();
      return MOZ_DOUBLE_NaN();
  }
}

double
nsHTMLInputElement::GetDefaultStep() const
{
  MOZ_ASSERT(DoesStepApply());

  switch (mType) {
    case NS_FORM_INPUT_DATE:
    case NS_FORM_INPUT_NUMBER:
      return kDefaultStep;
    case NS_FORM_INPUT_TIME:
      return kDefaultStepTime;
    default:
      MOZ_NOT_REACHED();
      return MOZ_DOUBLE_NaN();
  }
}

void
nsHTMLInputElement::UpdateValidityUIBits(bool aIsFocused)
{
  if (aIsFocused) {
    
    
    mCanShowInvalidUI = !IsValid() && ShouldShowValidityUI();

    
    
    mCanShowValidUI = ShouldShowValidityUI();
  } else {
    mCanShowInvalidUI = true;
    mCanShowValidUI = true;
  }
}

void
nsHTMLInputElement::UpdateHasRange()
{
  




  mHasRange = false;

  if (!DoesMinMaxApply()) {
    return;
  }

  double minimum = GetMinimum();
  if (!MOZ_DOUBLE_IS_NaN(minimum)) {
    mHasRange = true;
    return;
  }

  double maximum = GetMaximum();
  if (!MOZ_DOUBLE_IS_NaN(maximum)) {
    mHasRange = true;
    return;
  }
}
