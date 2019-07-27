





#include "mozilla/dom/HTMLInputElement.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/AsyncEventDispatcher.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/dom/Date.h"
#include "nsAttrValueInlines.h"

#include "nsIDOMHTMLInputElement.h"
#include "nsITextControlElement.h"
#include "nsIDOMNSEditableElement.h"
#include "nsIRadioVisitor.h"
#include "nsIPhonetic.h"

#include "mozilla/Telemetry.h"
#include "nsIControllers.h"
#include "nsIStringBundle.h"
#include "nsFocusManager.h"
#include "nsColorControlFrame.h"
#include "nsNumberControlFrame.h"
#include "nsPIDOMWindow.h"
#include "nsRepeatService.h"
#include "nsContentCID.h"
#include "nsIComponentManager.h"
#include "nsIDOMHTMLFormElement.h"
#include "mozilla/dom/ProgressEvent.h"
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
#include "nsRangeFrame.h"
#include "nsIServiceManager.h"
#include "nsError.h"
#include "nsIEditor.h"
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
#include "nsLayoutUtils.h"

#include "nsIDOMMutationEvent.h"
#include "mozilla/ContentEvents.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/EventStates.h"
#include "mozilla/InternalMutationEvent.h"
#include "mozilla/TextEvents.h"
#include "mozilla/TouchEvents.h"

#include "nsRuleData.h"
#include <algorithm>


#include "nsIRadioGroupContainer.h"


#include "mozilla/dom/File.h"
#include "nsIFile.h"
#include "nsNetUtil.h"
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
#include "nsTextEditorState.h"

#include "mozilla/LookAndFeel.h"
#include "mozilla/Preferences.h"
#include "mozilla/MathAlgorithms.h"

#include "nsIIDNService.h"

#include <limits>

#include "nsIColorPicker.h"
#include "nsIStringEnumerator.h"
#include "HTMLSplitOnSpacesTokenizer.h"
#include "nsIController.h"
#include "nsIMIMEInfo.h"


#include "js/Date.h"

NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(Input)



static NS_DEFINE_CID(kXULControllersCID,  NS_XULCONTROLLERS_CID);



inline mozilla::Decimal
NS_floorModulo(mozilla::Decimal x, mozilla::Decimal y)
{
  return (x - y * (x / y).floor());
}

namespace mozilla {
namespace dom {


#define NS_OUTER_ACTIVATE_EVENT   (1 << 9)
#define NS_ORIGINAL_CHECKED_VALUE (1 << 10)
#define NS_NO_CONTENT_DISPATCH    (1 << 11)
#define NS_ORIGINAL_INDETERMINATE_VALUE (1 << 12)
#define NS_CONTROL_TYPE(bits)  ((bits) & ~( \
  NS_OUTER_ACTIVATE_EVENT | NS_ORIGINAL_CHECKED_VALUE | NS_NO_CONTENT_DISPATCH | \
  NS_ORIGINAL_INDETERMINATE_VALUE))



static int32_t gSelectTextFieldOnFocus;
UploadLastDir* HTMLInputElement::gUploadLastDir;

static const nsAttrValue::EnumTable kInputTypeTable[] = {
  { "button", NS_FORM_INPUT_BUTTON },
  { "checkbox", NS_FORM_INPUT_CHECKBOX },
  { "color", NS_FORM_INPUT_COLOR },
  { "date", NS_FORM_INPUT_DATE },
  { "email", NS_FORM_INPUT_EMAIL },
  { "file", NS_FORM_INPUT_FILE },
  { "hidden", NS_FORM_INPUT_HIDDEN },
  { "reset", NS_FORM_INPUT_RESET },
  { "image", NS_FORM_INPUT_IMAGE },
  { "number", NS_FORM_INPUT_NUMBER },
  { "password", NS_FORM_INPUT_PASSWORD },
  { "radio", NS_FORM_INPUT_RADIO },
  { "range", NS_FORM_INPUT_RANGE },
  { "search", NS_FORM_INPUT_SEARCH },
  { "submit", NS_FORM_INPUT_SUBMIT },
  { "tel", NS_FORM_INPUT_TEL },
  { "text", NS_FORM_INPUT_TEXT },
  { "time", NS_FORM_INPUT_TIME },
  { "url", NS_FORM_INPUT_URL },
  { 0 }
};


static const nsAttrValue::EnumTable* kInputDefaultType = &kInputTypeTable[16];

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

const Decimal HTMLInputElement::kStepScaleFactorDate = Decimal(86400000);
const Decimal HTMLInputElement::kStepScaleFactorNumberRange = Decimal(1);
const Decimal HTMLInputElement::kStepScaleFactorTime = Decimal(1000);
const Decimal HTMLInputElement::kDefaultStepBase = Decimal(0);
const Decimal HTMLInputElement::kDefaultStep = Decimal(1);
const Decimal HTMLInputElement::kDefaultStepTime = Decimal(60);
const Decimal HTMLInputElement::kStepAny = Decimal(0);

#define NS_INPUT_ELEMENT_STATE_IID                 \
{ /* dc3b3d14-23e2-4479-b513-7b369343e3a0 */       \
  0xdc3b3d14,                                      \
  0x23e2,                                          \
  0x4479,                                          \
  {0xb5, 0x13, 0x7b, 0x36, 0x93, 0x43, 0xe3, 0xa0} \
}

#define PROGRESS_STR "progress"
static const uint32_t kProgressEventInterval = 50; 

class HTMLInputElementState final : public nsISupports
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

    void SetValue(const nsAString& aValue) {
      mValue = aValue;
    }

    const nsTArray<nsRefPtr<FileImpl>>& GetFileImpls() {
      return mFileImpls;
    }

    void SetFileImpls(const nsTArray<nsRefPtr<File>>& aFile) {
      mFileImpls.Clear();
      for (uint32_t i = 0, len = aFile.Length(); i < len; ++i) {
        mFileImpls.AppendElement(aFile[i]->Impl());
      }
    }

    HTMLInputElementState()
      : mValue()
      , mChecked(false)
      , mCheckedSet(false)
    {};

  protected:
    ~HTMLInputElementState() {}

    nsString mValue;
    nsTArray<nsRefPtr<FileImpl>> mFileImpls;
    bool mChecked;
    bool mCheckedSet;
};

NS_DEFINE_STATIC_IID_ACCESSOR(HTMLInputElementState, NS_INPUT_ELEMENT_STATE_IID)

NS_IMPL_ISUPPORTS(HTMLInputElementState, HTMLInputElementState)

HTMLInputElement::nsFilePickerShownCallback::nsFilePickerShownCallback(
  HTMLInputElement* aInput, nsIFilePicker* aFilePicker)
  : mFilePicker(aFilePicker)
  , mInput(aInput)
{
}

NS_IMPL_ISUPPORTS(UploadLastDir::ContentPrefCallback, nsIContentPrefCallback2)

NS_IMETHODIMP
UploadLastDir::ContentPrefCallback::HandleCompletion(uint16_t aReason)
{
  nsCOMPtr<nsIFile> localFile = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID);
  NS_ENSURE_STATE(localFile);

  if (aReason == nsIContentPrefCallback2::COMPLETE_ERROR ||
      !mResult) {
    
    nsCOMPtr<nsIFile> homeDir;
    NS_GetSpecialDirectory(NS_OS_DESKTOP_DIR, getter_AddRefs(homeDir));
    localFile = do_QueryInterface(homeDir);
  } else {
    nsAutoString prefStr;
    nsCOMPtr<nsIVariant> pref;
    mResult->GetValue(getter_AddRefs(pref));
    pref->GetAsAString(prefStr);
    localFile->InitWithPath(prefStr);
  }

  mFilePicker->SetDisplayDirectory(localFile);
  mFilePicker->Open(mFpCallback);
  return NS_OK;
}

NS_IMETHODIMP
UploadLastDir::ContentPrefCallback::HandleResult(nsIContentPref* pref)
{
  mResult = pref;
  return NS_OK;
}

NS_IMETHODIMP
UploadLastDir::ContentPrefCallback::HandleError(nsresult error)
{
  
  
  return NS_OK;
}

namespace {















class DirPickerRecursiveFileEnumerator final
  : public nsISimpleEnumerator
{
  ~DirPickerRecursiveFileEnumerator() {}

public:
  NS_DECL_ISUPPORTS

  explicit DirPickerRecursiveFileEnumerator(nsIFile* aTopDir)
    : mTopDir(aTopDir)
  {
    MOZ_ASSERT(!NS_IsMainThread(), "This class blocks on I/O!");

#ifdef DEBUG
    {
      bool isDir;
      aTopDir->IsDirectory(&isDir);
      MOZ_ASSERT(isDir);
    }
#endif

    if (NS_FAILED(aTopDir->GetParent(getter_AddRefs(mTopDirsParent)))) {
      
      
      mTopDirsParent = aTopDir;
    }

    nsCOMPtr<nsISimpleEnumerator> entries;
    if (NS_SUCCEEDED(mTopDir->GetDirectoryEntries(getter_AddRefs(entries))) &&
        entries) {
      mDirEnumeratorStack.AppendElement(entries);
      LookupAndCacheNext();
    }
  }

  NS_IMETHOD
  GetNext(nsISupports** aResult) override
  {
    MOZ_ASSERT(!NS_IsMainThread(),
               "Walking the directory tree involves I/O, so using this "
               "enumerator can block a thread for a long time!");

    if (!mNextFile) {
      return NS_ERROR_FAILURE;
    }

    
    nsRefPtr<File> domFile = File::CreateFromFile(nullptr, mNextFile);
    nsCString relDescriptor;
    nsresult rv =
      mNextFile->GetRelativeDescriptor(mTopDirsParent, relDescriptor);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ConvertUTF8toUTF16 path(relDescriptor);
    nsAutoString leafName;
    mNextFile->GetLeafName(leafName);
    MOZ_ASSERT(leafName.Length() <= path.Length());
    int32_t length = path.Length() - leafName.Length();
    MOZ_ASSERT(length >= 0);
    if (length > 0) {
      
      FileImplFile* fileImpl = static_cast<FileImplFile*>(domFile->Impl());
      MOZ_ASSERT(fileImpl);
      fileImpl->SetPath(Substring(path, 0, uint32_t(length)));
    }
    *aResult = domFile.forget().downcast<nsIDOMFile>().take();
    LookupAndCacheNext();
    return NS_OK;
  }

  NS_IMETHOD
  HasMoreElements(bool* aResult) override
  {
    *aResult = !!mNextFile;
    return NS_OK;
  }

private:

  void
  LookupAndCacheNext()
  {
    for (;;) {
      if (mDirEnumeratorStack.IsEmpty()) {
        mNextFile = nullptr;
        break;
      }

      nsISimpleEnumerator* currentDirEntries =
        mDirEnumeratorStack.LastElement();

      bool hasMore;
      DebugOnly<nsresult> rv = currentDirEntries->HasMoreElements(&hasMore);
      MOZ_ASSERT(NS_SUCCEEDED(rv));

      if (!hasMore) {
        mDirEnumeratorStack.RemoveElementAt(mDirEnumeratorStack.Length() - 1);
        continue;
      }

      nsCOMPtr<nsISupports> entry;
      rv = currentDirEntries->GetNext(getter_AddRefs(entry));
      MOZ_ASSERT(NS_SUCCEEDED(rv));

      nsCOMPtr<nsIFile> file = do_QueryInterface(entry);
      MOZ_ASSERT(file);

      bool isLink, isSpecial;
      file->IsSymlink(&isLink);
      file->IsSpecial(&isSpecial);
      if (isLink || isSpecial) {
        continue;
      }

      bool isDir;
      file->IsDirectory(&isDir);
      if (isDir) {
        nsCOMPtr<nsISimpleEnumerator> subDirEntries;
        rv = file->GetDirectoryEntries(getter_AddRefs(subDirEntries));
        MOZ_ASSERT(NS_SUCCEEDED(rv) && subDirEntries);
        mDirEnumeratorStack.AppendElement(subDirEntries);
        continue;
      }

#ifdef DEBUG
      {
        bool isFile;
        file->IsFile(&isFile);
        MOZ_ASSERT(isFile);
      }
#endif

      mNextFile.swap(file);
      return;
    }
  }

private:
  nsCOMPtr<nsIFile> mTopDir;
  nsCOMPtr<nsIFile> mTopDirsParent; 
  nsCOMPtr<nsIFile> mNextFile;
  nsTArray<nsCOMPtr<nsISimpleEnumerator> > mDirEnumeratorStack;
};

NS_IMPL_ISUPPORTS(DirPickerRecursiveFileEnumerator, nsISimpleEnumerator)







static already_AddRefed<nsIFile>
DOMFileToLocalFile(nsIDOMFile* aDomFile)
{
  nsString path;
  nsresult rv = aDomFile->GetMozFullPathInternal(path);
  if (NS_FAILED(rv) || path.IsEmpty()) {
    return nullptr;
  }

  nsCOMPtr<nsIFile> localFile;
  rv = NS_NewNativeLocalFile(NS_ConvertUTF16toUTF8(path), true,
                             getter_AddRefs(localFile));
  NS_ENSURE_SUCCESS(rv, nullptr);

  return localFile.forget();
}

} 

class DirPickerFileListBuilderTask final
  : public nsRunnable
{
public:
  DirPickerFileListBuilderTask(HTMLInputElement* aInput, nsIFile* aTopDir)
    : mPreviousFileListLength(0)
    , mInput(aInput)
    , mTopDir(aTopDir)
    , mFileListLength(0)
    , mCanceled(false)
  {}

  NS_IMETHOD Run() {
    if (!NS_IsMainThread()) {
      
      nsCOMPtr<nsISimpleEnumerator> iter =
        new DirPickerRecursiveFileEnumerator(mTopDir);
      bool hasMore = true;
      nsCOMPtr<nsISupports> tmp;
      while (NS_SUCCEEDED(iter->HasMoreElements(&hasMore)) && hasMore) {
        iter->GetNext(getter_AddRefs(tmp));
        nsCOMPtr<nsIDOMFile> domFile = do_QueryInterface(tmp);
        MOZ_ASSERT(domFile);
        mFileList.AppendElement(static_cast<File*>(domFile.get()));
        mFileListLength = mFileList.Length();
        if (mCanceled) {
          MOZ_ASSERT(!mInput, "This is bad - how did this happen?");
          
          
          return NS_OK;
        }
      }
      return NS_DispatchToMainThread(this);
    }

    
    if (mCanceled || mFileList.IsEmpty()) {
      return NS_OK;
    }
    MOZ_ASSERT(mInput->mDirPickerFileListBuilderTask,
               "But we aren't canceled!");
    if (mInput->mProgressTimer) {
      mInput->mProgressTimerIsActive = false;
      mInput->mProgressTimer->Cancel();
    }

    mInput->MaybeDispatchProgressEvent(true);        
    mInput->mDirPickerFileListBuilderTask = nullptr; 

    if (mCanceled) { 
      return NS_OK;
    }

    
    nsCOMPtr<nsIGlobalObject> global = mInput->OwnerDoc()->GetScopeObject();
    for (uint32_t i = 0; i < mFileList.Length(); ++i) {
      MOZ_ASSERT(!mFileList[i]->GetParentObject());
      mFileList[i] = new File(global, mFileList[i]->Impl());
    }

    
    
    
    mInput->SetFiles(mFileList, true);
    nsresult rv =
      nsContentUtils::DispatchTrustedEvent(mInput->OwnerDoc(),
                                           static_cast<nsIDOMHTMLInputElement*>(mInput.get()),
                                           NS_LITERAL_STRING("change"), true,
                                           false);
    
    
    mInput = nullptr;
    return rv;
  }

  void Cancel()
  {
    MOZ_ASSERT(NS_IsMainThread() && !mCanceled);
    
    
    mInput = nullptr;
    mCanceled = true;
  }

  uint32_t GetFileListLength() const
  {
    return mFileListLength;
  }

  








  uint32_t mPreviousFileListLength;

private:
  nsRefPtr<HTMLInputElement> mInput;
  nsCOMPtr<nsIFile> mTopDir;
  nsTArray<nsRefPtr<File>> mFileList;

  
  
  mozilla::Atomic<uint32_t> mFileListLength;

  mozilla::Atomic<bool> mCanceled;
};


NS_IMETHODIMP
HTMLInputElement::nsFilePickerShownCallback::Done(int16_t aResult)
{
  mInput->PickerClosed();

  if (aResult == nsIFilePicker::returnCancel) {
    return NS_OK;
  }

  mInput->CancelDirectoryPickerScanIfRunning();

  int16_t mode;
  mFilePicker->GetMode(&mode);

  if (mode == static_cast<int16_t>(nsIFilePicker::modeGetFolder)) {
    
    
    
    

    
    
    nsCOMPtr<nsIFile> pickedDir;
    mFilePicker->GetFile(getter_AddRefs(pickedDir));

#ifdef DEBUG
    {
      bool isDir;
      pickedDir->IsDirectory(&isDir);
      MOZ_ASSERT(isDir);
    }
#endif

    HTMLInputElement::gUploadLastDir->StoreLastUsedDirectory(
      mInput->OwnerDoc(), pickedDir);

    nsCOMPtr<nsIEventTarget> target
      = do_GetService(NS_STREAMTRANSPORTSERVICE_CONTRACTID);
    NS_ASSERTION(target, "Must have stream transport service");

    mInput->StartProgressEventTimer();

    
    
    mInput->mDirPickerFileListBuilderTask =
      new DirPickerFileListBuilderTask(mInput.get(), pickedDir.get());
    return target->Dispatch(mInput->mDirPickerFileListBuilderTask,
                            NS_DISPATCH_NORMAL);
  }

  
  nsTArray<nsRefPtr<File>> newFiles;
  if (mode == static_cast<int16_t>(nsIFilePicker::modeOpenMultiple)) {
    nsCOMPtr<nsISimpleEnumerator> iter;
    nsresult rv = mFilePicker->GetDomfiles(getter_AddRefs(iter));
    NS_ENSURE_SUCCESS(rv, rv);

    if (!iter) {
      return NS_OK;
    }

    nsCOMPtr<nsISupports> tmp;
    bool hasMore = true;

    while (NS_SUCCEEDED(iter->HasMoreElements(&hasMore)) && hasMore) {
      iter->GetNext(getter_AddRefs(tmp));
      nsCOMPtr<nsIDOMFile> domFile = do_QueryInterface(tmp);
      NS_WARN_IF_FALSE(domFile,
                       "Null file object from FilePicker's file enumerator?");
      if (domFile) {
        newFiles.AppendElement(static_cast<File*>(domFile.get()));
      }
    }
  } else {
    MOZ_ASSERT(mode == static_cast<int16_t>(nsIFilePicker::modeOpen));
    nsCOMPtr<nsIDOMFile> domFile;
    nsresult rv = mFilePicker->GetDomfile(getter_AddRefs(domFile));
    NS_ENSURE_SUCCESS(rv, rv);
    if (domFile) {
      newFiles.AppendElement(static_cast<File*>(domFile.get()));
    }
  }

  if (newFiles.IsEmpty()) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIFile> file = DOMFileToLocalFile(newFiles[0]);
  if (file) {
    nsCOMPtr<nsIFile> lastUsedDir;
    file->GetParent(getter_AddRefs(lastUsedDir));
    HTMLInputElement::gUploadLastDir->StoreLastUsedDirectory(
      mInput->OwnerDoc(), lastUsedDir);
  }

  
  
  
  mInput->SetFiles(newFiles, true);
  return nsContentUtils::DispatchTrustedEvent(mInput->OwnerDoc(),
                                              static_cast<nsIDOMHTMLInputElement*>(mInput.get()),
                                              NS_LITERAL_STRING("change"), true,
                                              false);
}

NS_IMPL_ISUPPORTS(HTMLInputElement::nsFilePickerShownCallback,
                  nsIFilePickerShownCallback)

class nsColorPickerShownCallback final
  : public nsIColorPickerShownCallback
{
  ~nsColorPickerShownCallback() {}

public:
  nsColorPickerShownCallback(HTMLInputElement* aInput,
                             nsIColorPicker* aColorPicker)
    : mInput(aInput)
    , mColorPicker(aColorPicker)
    , mValueChanged(false)
  {}

  NS_DECL_ISUPPORTS

  NS_IMETHOD Update(const nsAString& aColor) override;
  NS_IMETHOD Done(const nsAString& aColor) override;

private:
  




  nsresult UpdateInternal(const nsAString& aColor, bool aTrustedUpdate);

  nsRefPtr<HTMLInputElement> mInput;
  nsCOMPtr<nsIColorPicker>   mColorPicker;
  bool                       mValueChanged;
};

nsresult
nsColorPickerShownCallback::UpdateInternal(const nsAString& aColor,
                                           bool aTrustedUpdate)
{
  bool valueChanged = false;

  nsAutoString oldValue;
  if (aTrustedUpdate) {
    valueChanged = true;
  } else {
    mInput->GetValue(oldValue);
  }

  mInput->SetValue(aColor);

  if (!aTrustedUpdate) {
    nsAutoString newValue;
    mInput->GetValue(newValue);
    if (!oldValue.Equals(newValue)) {
      valueChanged = true;
    }
  }

  if (valueChanged) {
    mValueChanged = true;
    return nsContentUtils::DispatchTrustedEvent(mInput->OwnerDoc(),
                                                static_cast<nsIDOMHTMLInputElement*>(mInput.get()),
                                                NS_LITERAL_STRING("input"), true,
                                                false);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsColorPickerShownCallback::Update(const nsAString& aColor)
{
  return UpdateInternal(aColor, true);
}

NS_IMETHODIMP
nsColorPickerShownCallback::Done(const nsAString& aColor)
{
  






  nsresult rv = NS_OK;

  mInput->PickerClosed();

  if (!aColor.IsEmpty()) {
    UpdateInternal(aColor, false);
  }

  if (mValueChanged) {
    rv = nsContentUtils::DispatchTrustedEvent(mInput->OwnerDoc(),
                                              static_cast<nsIDOMHTMLInputElement*>(mInput.get()),
                                              NS_LITERAL_STRING("change"), true,
                                              false);
  }

  return rv;
}

NS_IMPL_ISUPPORTS(nsColorPickerShownCallback, nsIColorPickerShownCallback)

bool
HTMLInputElement::IsPopupBlocked() const
{
  nsCOMPtr<nsPIDOMWindow> win = OwnerDoc()->GetWindow();
  MOZ_ASSERT(win, "window should not be null");
  if (!win) {
    return true;
  }

  
  if (win->GetPopupControlState() <= openControlled) {
    return false;
  }

  nsCOMPtr<nsIPopupWindowManager> pm = do_GetService(NS_POPUPWINDOWMANAGER_CONTRACTID);
  if (!pm) {
    return true;
  }

  uint32_t permission;
  pm->TestPermission(OwnerDoc()->NodePrincipal(), &permission);
  return permission == nsIPopupWindowManager::DENY_POPUP;
}

nsresult
HTMLInputElement::InitColorPicker()
{
  if (mPickerRunning) {
    NS_WARNING("Just one nsIColorPicker is allowed");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDocument> doc = OwnerDoc();

  nsCOMPtr<nsPIDOMWindow> win = doc->GetWindow();
  if (!win) {
    return NS_ERROR_FAILURE;
  }

  if (IsPopupBlocked()) {
    win->FirePopupBlockedEvent(doc, nullptr, EmptyString(), EmptyString());
    return NS_OK;
  }

  
  nsXPIDLString title;
  nsContentUtils::GetLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                     "ColorPicker", title);

  nsCOMPtr<nsIColorPicker> colorPicker = do_CreateInstance("@mozilla.org/colorpicker;1");
  if (!colorPicker) {
    return NS_ERROR_FAILURE;
  }

  nsAutoString initialValue;
  GetValueInternal(initialValue);
  nsresult rv = colorPicker->Init(win, title, initialValue);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIColorPickerShownCallback> callback =
    new nsColorPickerShownCallback(this, colorPicker);

  rv = colorPicker->Open(callback);
  if (NS_SUCCEEDED(rv)) {
    mPickerRunning = true;
  }

  return rv;
}

nsresult
HTMLInputElement::InitFilePicker(FilePickerType aType)
{
  if (mPickerRunning) {
    NS_WARNING("Just one nsIFilePicker is allowed");
    return NS_ERROR_FAILURE;
  }

  
  nsCOMPtr<nsIDocument> doc = OwnerDoc();

  nsCOMPtr<nsPIDOMWindow> win = doc->GetWindow();
  if (!win) {
    return NS_ERROR_FAILURE;
  }

  if (IsPopupBlocked()) {
    win->FirePopupBlockedEvent(doc, nullptr, EmptyString(), EmptyString());
    return NS_OK;
  }

  
  nsXPIDLString title;
  nsContentUtils::GetLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                     "FileUpload", title);

  nsCOMPtr<nsIFilePicker> filePicker = do_CreateInstance("@mozilla.org/filepicker;1");
  if (!filePicker)
    return NS_ERROR_FAILURE;

  int16_t mode;

  if (aType == FILE_PICKER_DIRECTORY) {
    mode = static_cast<int16_t>(nsIFilePicker::modeGetFolder);
  } else if (HasAttr(kNameSpaceID_None, nsGkAtoms::multiple)) {
    mode = static_cast<int16_t>(nsIFilePicker::modeOpenMultiple);
  } else {
    mode = static_cast<int16_t>(nsIFilePicker::modeOpen);
  }

  nsresult rv = filePicker->Init(win, title, mode);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (HasAttr(kNameSpaceID_None, nsGkAtoms::accept) &&
      aType != FILE_PICKER_DIRECTORY) {
    SetFilePickerFiltersFromAccept(filePicker);
  } else {
    filePicker->AppendFilters(nsIFilePicker::filterAll);
  }

  
  nsAutoString defaultName;

  const nsTArray<nsRefPtr<File>>& oldFiles = GetFilesInternal();

  nsCOMPtr<nsIFilePickerShownCallback> callback =
    new HTMLInputElement::nsFilePickerShownCallback(this, filePicker);

  if (!oldFiles.IsEmpty() &&
      aType != FILE_PICKER_DIRECTORY) {
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

    
    
    
    if (oldFiles.Length() == 1) {
      nsAutoString leafName;
      oldFiles[0]->GetName(leafName);
      if (!leafName.IsEmpty()) {
        filePicker->SetDefaultString(leafName);
      }
    }

    rv = filePicker->Open(callback);
    if (NS_SUCCEEDED(rv)) {
      mPickerRunning = true;
    }

    return rv;
  }

  HTMLInputElement::gUploadLastDir->FetchDirectoryAndDisplayPicker(doc, filePicker, callback);
  mPickerRunning = true;
  return NS_OK;
}

#define CPS_PREF_NAME NS_LITERAL_STRING("browser.upload.lastDir")

NS_IMPL_ISUPPORTS(UploadLastDir, nsIObserver, nsISupportsWeakReference)

void
HTMLInputElement::InitUploadLastDir() {
  gUploadLastDir = new UploadLastDir();
  NS_ADDREF(gUploadLastDir);

  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (observerService && gUploadLastDir) {
    observerService->AddObserver(gUploadLastDir, "browser:purge-session-history", true);
  }
}

void
HTMLInputElement::DestroyUploadLastDir() {
  NS_IF_RELEASE(gUploadLastDir);
}

nsresult
UploadLastDir::FetchDirectoryAndDisplayPicker(nsIDocument* aDoc,
                                              nsIFilePicker* aFilePicker,
                                              nsIFilePickerShownCallback* aFpCallback)
{
  NS_PRECONDITION(aDoc, "aDoc is null");
  NS_PRECONDITION(aFilePicker, "aFilePicker is null");
  NS_PRECONDITION(aFpCallback, "aFpCallback is null");

  nsIURI* docURI = aDoc->GetDocumentURI();
  NS_PRECONDITION(docURI, "docURI is null");

  nsCOMPtr<nsILoadContext> loadContext = aDoc->GetLoadContext();
  nsCOMPtr<nsIContentPrefCallback2> prefCallback = 
    new UploadLastDir::ContentPrefCallback(aFilePicker, aFpCallback);

#ifdef MOZ_B2G
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    prefCallback->HandleCompletion(nsIContentPrefCallback2::COMPLETE_ERROR);
    return NS_OK;
  }
#endif

  
  nsCOMPtr<nsIContentPrefService2> contentPrefService =
    do_GetService(NS_CONTENT_PREF_SERVICE_CONTRACTID);
  if (!contentPrefService) {
    prefCallback->HandleCompletion(nsIContentPrefCallback2::COMPLETE_ERROR);
    return NS_OK;
  }

  nsAutoCString cstrSpec;
  docURI->GetSpec(cstrSpec);
  NS_ConvertUTF8toUTF16 spec(cstrSpec);

  contentPrefService->GetByDomainAndName(spec, CPS_PREF_NAME, loadContext, prefCallback);
  return NS_OK;
}

nsresult
UploadLastDir::StoreLastUsedDirectory(nsIDocument* aDoc, nsIFile* aDir)
{
  NS_PRECONDITION(aDoc, "aDoc is null");
  if (!aDir) {
    return NS_OK;
  }

#ifdef MOZ_B2G
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    return NS_OK;
  }
#endif

  nsCOMPtr<nsIURI> docURI = aDoc->GetDocumentURI();
  NS_PRECONDITION(docURI, "docURI is null");

  
  nsCOMPtr<nsIContentPrefService2> contentPrefService =
    do_GetService(NS_CONTENT_PREF_SERVICE_CONTRACTID);
  if (!contentPrefService)
    return NS_ERROR_NOT_AVAILABLE;

  nsAutoCString cstrSpec;
  docURI->GetSpec(cstrSpec);
  NS_ConvertUTF8toUTF16 spec(cstrSpec);

  
  nsString unicodePath;
  aDir->GetPath(unicodePath);
  if (unicodePath.IsEmpty()) 
    return NS_OK;
  nsCOMPtr<nsIWritableVariant> prefValue = do_CreateInstance(NS_VARIANT_CONTRACTID);
  if (!prefValue)
    return NS_ERROR_OUT_OF_MEMORY;
  prefValue->SetAsAString(unicodePath);

  nsCOMPtr<nsILoadContext> loadContext = aDoc->GetLoadContext();
  return contentPrefService->Set(spec, CPS_PREF_NAME, prefValue, loadContext, nullptr);
}

NS_IMETHODIMP
UploadLastDir::Observe(nsISupports* aSubject, char const* aTopic, char16_t const* aData)
{
  if (strcmp(aTopic, "browser:purge-session-history") == 0) {
    nsCOMPtr<nsIContentPrefService2> contentPrefService =
      do_GetService(NS_CONTENT_PREF_SERVICE_CONTRACTID);
    if (contentPrefService)
      contentPrefService->RemoveByName(CPS_PREF_NAME, nullptr, nullptr);
  }
  return NS_OK;
}

#ifdef ACCESSIBILITY

static nsresult FireEventForAccessibility(nsIDOMHTMLInputElement* aTarget,
                                          nsPresContext* aPresContext,
                                          const nsAString& aEventType);
#endif





HTMLInputElement::HTMLInputElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo,
                                   FromParser aFromParser)
  : nsGenericHTMLFormElementWithState(aNodeInfo)
  , mType(kInputDefaultType->value)
  , mAutocompleteAttrState(nsContentUtils::eAutocompleteAttrState_Unknown)
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
  , mIsDraggingRange(false)
  , mProgressTimerIsActive(false)
  , mNumberControlSpinnerIsSpinning(false)
  , mNumberControlSpinnerSpinsUp(false)
  , mPickerRunning(false)
  , mSelectionCached(true)
{
  
  mInputData.mState = new nsTextEditorState(this);

  if (!gUploadLastDir)
    HTMLInputElement::InitUploadLastDir();

  
  
  
  
  
  AddStatesSilently(NS_EVENT_STATE_ENABLED |
                    NS_EVENT_STATE_OPTIONAL |
                    NS_EVENT_STATE_VALID);
}

HTMLInputElement::~HTMLInputElement()
{
  if (mFileList) {
    mFileList->Disconnect();
  }
  if (mNumberControlSpinnerIsSpinning) {
    StopNumberControlSpinnerSpin();
  }
  DestroyImageLoadingContent();
  FreeData();
}

void
HTMLInputElement::FreeData()
{
  if (!IsSingleLineTextControl(false)) {
    free(mInputData.mValue);
    mInputData.mValue = nullptr;
  } else {
    UnbindFromFrame(nullptr);
    delete mInputData.mState;
    mInputData.mState = nullptr;
  }
}

nsTextEditorState*
HTMLInputElement::GetEditorState() const
{
  if (!IsSingleLineTextControl(false)) {
    return nullptr;
  }

  MOZ_ASSERT(mInputData.mState, "Single line text controls need to have a state"
                                " associated with them");

  return mInputData.mState;
}




NS_IMPL_CYCLE_COLLECTION_CLASS(HTMLInputElement)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(HTMLInputElement,
                                                  nsGenericHTMLFormElementWithState)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mValidity)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mControllers)
  if (tmp->IsSingleLineTextControl(false)) {
    tmp->mInputData.mState->Traverse(cb);
  }
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mFiles)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mFileList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(HTMLInputElement,
                                                nsGenericHTMLFormElementWithState)
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

NS_IMPL_ADDREF_INHERITED(HTMLInputElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLInputElement, Element)


NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(HTMLInputElement)
  NS_INTERFACE_TABLE_INHERITED(HTMLInputElement,
                               nsIDOMHTMLInputElement,
                               nsITextControlElement,
                               nsIPhonetic,
                               imgINotificationObserver,
                               nsIImageLoadingContent,
                               imgIOnloadBlocker,
                               nsIDOMNSEditableElement,
                               nsITimerCallback,
                               nsIConstraintValidation)
NS_INTERFACE_TABLE_TAIL_INHERITING(nsGenericHTMLFormElementWithState)


NS_IMPL_NSICONSTRAINTVALIDATION_EXCEPT_SETCUSTOMVALIDITY(HTMLInputElement)



nsresult
HTMLInputElement::Clone(mozilla::dom::NodeInfo* aNodeInfo, nsINode** aResult) const
{
  *aResult = nullptr;

  already_AddRefed<mozilla::dom::NodeInfo> ni = nsRefPtr<mozilla::dom::NodeInfo>(aNodeInfo).forget();
  nsRefPtr<HTMLInputElement> it = new HTMLInputElement(ni, NOT_FROM_PARSER);

  nsresult rv = const_cast<HTMLInputElement*>(this)->CopyInnerTo(it);
  NS_ENSURE_SUCCESS(rv, rv);

  switch (GetValueMode()) {
    case VALUE_MODE_VALUE:
      if (mValueChanged) {
        
        
        nsAutoString value;
        GetValueInternal(value);
        
        rv = it->SetValueInternal(value, false, true);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      break;
    case VALUE_MODE_FILENAME:
      if (it->OwnerDoc()->IsStaticDocument()) {
        
        
        GetDisplayFileName(it->mStaticDocFileList);
      } else {
        it->mFiles.Clear();
        it->mFiles.AppendElements(mFiles);
      }
      break;
    case VALUE_MODE_DEFAULT_ON:
      if (mCheckedChanged) {
        
        
        it->DoSetChecked(mChecked, false, true);
      }
      break;
    case VALUE_MODE_DEFAULT:
      if (mType == NS_FORM_INPUT_IMAGE && it->OwnerDoc()->IsStaticDocument()) {
        CreateStaticImageClone(it);
      }
      break;
  }

  it.forget(aResult);
  return NS_OK;
}

nsresult
HTMLInputElement::BeforeSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
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
        LoadImage(aValue->String(), true, aNotify, eImageLoadType_Normal);
      } else {
        
        CancelImageRequests(aNotify);
      }
    } else if (aNotify && aName == nsGkAtoms::disabled) {
      mDisabledChanged = true;
    } else if (aName == nsGkAtoms::dir &&
               AttrValueIs(kNameSpaceID_None, nsGkAtoms::dir,
                           nsGkAtoms::_auto, eIgnoreCase)) {
      SetDirectionIfAuto(false, aNotify);
    } else if (mType == NS_FORM_INPUT_RADIO && aName == nsGkAtoms::required) {
      nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();

      if (container &&
          ((aValue && !HasAttr(aNameSpaceID, aName)) ||
           (!aValue && HasAttr(aNameSpaceID, aName)))) {
        nsAutoString name;
        GetAttr(kNameSpaceID_None, nsGkAtoms::name, name);
        container->RadioRequiredWillChange(name, !!aValue);
      }
    }
  }

  return nsGenericHTMLFormElementWithState::BeforeSetAttr(aNameSpaceID, aName,
                                                          aValue, aNotify);
}

nsresult
HTMLInputElement::AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
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
          LoadImage(src, false, aNotify, eImageLoadType_Normal);
        }
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
      if (mType == NS_FORM_INPUT_RANGE) {
        
        
        
        
        
        
        
        
        
        
        
        nsAutoString value;
        GetValue(value);
        nsresult rv = SetValueInternal(value, false, false);
        NS_ENSURE_SUCCESS(rv, rv);
        MOZ_ASSERT(!GetValidityState(VALIDITY_STATE_RANGE_UNDERFLOW),
                   "HTML5 spec does not allow this");
      }
    } else if (aName == nsGkAtoms::min) {
      UpdateHasRange();
      UpdateRangeUnderflowValidityState();
      UpdateStepMismatchValidityState();
      if (mType == NS_FORM_INPUT_RANGE) {
        
        nsAutoString value;
        GetValue(value);
        nsresult rv = SetValueInternal(value, false, false);
        NS_ENSURE_SUCCESS(rv, rv);
        MOZ_ASSERT(!GetValidityState(VALIDITY_STATE_RANGE_UNDERFLOW),
                   "HTML5 spec does not allow this");
      }
    } else if (aName == nsGkAtoms::step) {
      UpdateStepMismatchValidityState();
      if (mType == NS_FORM_INPUT_RANGE) {
        
        nsAutoString value;
        GetValue(value);
        nsresult rv = SetValueInternal(value, false, false);
        NS_ENSURE_SUCCESS(rv, rv);
        MOZ_ASSERT(!GetValidityState(VALIDITY_STATE_RANGE_UNDERFLOW),
                   "HTML5 spec does not allow this");
      }
    } else if (aName == nsGkAtoms::dir &&
               aValue && aValue->Equals(nsGkAtoms::_auto, eIgnoreCase)) {
      SetDirectionIfAuto(true, aNotify);
    } else if (aName == nsGkAtoms::lang) {
      if (mType == NS_FORM_INPUT_NUMBER) {
        
        nsAutoString value;
        GetValueInternal(value);
        nsNumberControlFrame* numberControlFrame =
          do_QueryFrame(GetPrimaryFrame());
        if (numberControlFrame) {
          numberControlFrame->SetValueOfAnonTextControl(value);
        }
      }
    } else if (aName == nsGkAtoms::autocomplete) {
      
      mAutocompleteAttrState = nsContentUtils::eAutocompleteAttrState_Unknown;
    }

    UpdateState(aNotify);
  }

  return nsGenericHTMLFormElementWithState::AfterSetAttr(aNameSpaceID, aName,
                                                         aValue, aNotify);
}



NS_IMETHODIMP
HTMLInputElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElementWithState::GetForm(aForm);
}

NS_IMPL_STRING_ATTR(HTMLInputElement, DefaultValue, value)
NS_IMPL_BOOL_ATTR(HTMLInputElement, DefaultChecked, checked)
NS_IMPL_STRING_ATTR(HTMLInputElement, Accept, accept)
NS_IMPL_STRING_ATTR(HTMLInputElement, Align, align)
NS_IMPL_STRING_ATTR(HTMLInputElement, Alt, alt)
NS_IMPL_BOOL_ATTR(HTMLInputElement, Autofocus, autofocus)

NS_IMPL_BOOL_ATTR(HTMLInputElement, Disabled, disabled)
NS_IMPL_STRING_ATTR(HTMLInputElement, Max, max)
NS_IMPL_STRING_ATTR(HTMLInputElement, Min, min)
NS_IMPL_ACTION_ATTR(HTMLInputElement, FormAction, formaction)
NS_IMPL_ENUM_ATTR_DEFAULT_MISSING_INVALID_VALUES(HTMLInputElement, FormEnctype, formenctype,
                                                 "", kFormDefaultEnctype->tag)
NS_IMPL_ENUM_ATTR_DEFAULT_MISSING_INVALID_VALUES(HTMLInputElement, FormMethod, formmethod,
                                                 "", kFormDefaultMethod->tag)
NS_IMPL_BOOL_ATTR(HTMLInputElement, FormNoValidate, formnovalidate)
NS_IMPL_STRING_ATTR(HTMLInputElement, FormTarget, formtarget)
NS_IMPL_ENUM_ATTR_DEFAULT_VALUE(HTMLInputElement, InputMode, inputmode,
                                kInputDefaultInputmode->tag)
NS_IMPL_BOOL_ATTR(HTMLInputElement, Multiple, multiple)
NS_IMPL_NON_NEGATIVE_INT_ATTR(HTMLInputElement, MaxLength, maxlength)
NS_IMPL_STRING_ATTR(HTMLInputElement, Name, name)
NS_IMPL_BOOL_ATTR(HTMLInputElement, ReadOnly, readonly)
NS_IMPL_BOOL_ATTR(HTMLInputElement, Required, required)
NS_IMPL_URI_ATTR(HTMLInputElement, Src, src)
NS_IMPL_STRING_ATTR(HTMLInputElement, Step, step)
NS_IMPL_STRING_ATTR(HTMLInputElement, UseMap, usemap)

NS_IMPL_UINT_ATTR_NON_ZERO_DEFAULT_VALUE(HTMLInputElement, Size, size, DEFAULT_COLS)
NS_IMPL_STRING_ATTR(HTMLInputElement, Pattern, pattern)
NS_IMPL_STRING_ATTR(HTMLInputElement, Placeholder, placeholder)
NS_IMPL_ENUM_ATTR_DEFAULT_VALUE(HTMLInputElement, Type, type,
                                kInputDefaultType->tag)

NS_IMETHODIMP
HTMLInputElement::GetAutocomplete(nsAString& aValue)
{
  if (!DoesAutocompleteApply()) {
    return NS_OK;
  }

  aValue.Truncate(0);
  const nsAttrValue* attributeVal = GetParsedAttr(nsGkAtoms::autocomplete);

  mAutocompleteAttrState =
    nsContentUtils::SerializeAutocompleteAttribute(attributeVal, aValue,
                                                   mAutocompleteAttrState);
  return NS_OK;
}

NS_IMETHODIMP
HTMLInputElement::SetAutocomplete(const nsAString& aValue)
{
  return SetAttr(kNameSpaceID_None, nsGkAtoms::autocomplete, nullptr, aValue, true);
}

void
HTMLInputElement::GetAutocompleteInfo(Nullable<AutocompleteInfo>& aInfo)
{
  if (!DoesAutocompleteApply()) {
    aInfo.SetNull();
    return;
  }

  const nsAttrValue* attributeVal = GetParsedAttr(nsGkAtoms::autocomplete);
  mAutocompleteAttrState =
    nsContentUtils::SerializeAutocompleteAttribute(attributeVal, aInfo.SetValue(),
                                                   mAutocompleteAttrState);
}

int32_t
HTMLInputElement::TabIndexDefault()
{
  return 0;
}

uint32_t
HTMLInputElement::Height()
{
  if (mType != NS_FORM_INPUT_IMAGE) {
    return 0;
  }
  return GetWidthHeightForImage(mCurrentRequest).height;
}

NS_IMETHODIMP
HTMLInputElement::GetHeight(uint32_t* aHeight)
{
  *aHeight = Height();
  return NS_OK;
}

NS_IMETHODIMP
HTMLInputElement::SetHeight(uint32_t aHeight)
{
  ErrorResult rv;
  SetHeight(aHeight, rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP
HTMLInputElement::GetIndeterminate(bool* aValue)
{
  *aValue = Indeterminate();
  return NS_OK;
}

void
HTMLInputElement::SetIndeterminateInternal(bool aValue,
                                           bool aShouldInvalidate)
{
  mIndeterminate = aValue;

  if (aShouldInvalidate) {
    
    nsIFrame* frame = GetPrimaryFrame();
    if (frame)
      frame->InvalidateFrameSubtree();
  }

  UpdateState(true);
}

NS_IMETHODIMP
HTMLInputElement::SetIndeterminate(bool aValue)
{
  SetIndeterminateInternal(aValue, true);
  return NS_OK;
}

uint32_t
HTMLInputElement::Width()
{
  if (mType != NS_FORM_INPUT_IMAGE) {
    return 0;
  }
  return GetWidthHeightForImage(mCurrentRequest).width;
}

NS_IMETHODIMP
HTMLInputElement::GetWidth(uint32_t* aWidth)
{
  *aWidth = Width();
  return NS_OK;
}

NS_IMETHODIMP
HTMLInputElement::SetWidth(uint32_t aWidth)
{
  ErrorResult rv;
  SetWidth(aWidth, rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP
HTMLInputElement::GetValue(nsAString& aValue)
{
  GetValueInternal(aValue);

  
  if (IsExperimentalMobileType(mType)) {
    SanitizeValue(aValue);
  }

  return NS_OK;
}

nsresult
HTMLInputElement::GetValueInternal(nsAString& aValue) const
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
#ifndef MOZ_CHILD_PERMISSIONS
        aValue.Assign(mFirstFilePath);
#else
        
        
        if (!mFiles.IsEmpty()) {
          return mFiles[0]->GetMozFullPath(aValue);
        }
        else {
          aValue.Truncate();
        }
#endif
      } else {
        
        if (mFiles.IsEmpty() || NS_FAILED(mFiles[0]->GetName(aValue))) {
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
HTMLInputElement::IsValueEmpty() const
{
  nsAutoString value;
  GetValueInternal(value);

  return value.IsEmpty();
}

void
HTMLInputElement::ClearFiles(bool aSetValueChanged)
{
  nsTArray<nsRefPtr<File>> files;
  SetFiles(files, aSetValueChanged);
}

 Decimal
HTMLInputElement::StringToDecimal(const nsAString& aValue)
{
  if (!IsASCII(aValue)) {
    return Decimal::nan();
  }
  NS_LossyConvertUTF16toASCII asciiString(aValue);
  std::string stdString = asciiString.get();
  return Decimal::fromString(stdString);
}

bool
HTMLInputElement::ConvertStringToNumber(nsAString& aValue,
                                        Decimal& aResultValue) const
{
  MOZ_ASSERT(DoesValueAsNumberApply(),
             "ConvertStringToNumber only applies if .valueAsNumber applies");

  switch (mType) {
    case NS_FORM_INPUT_NUMBER:
    case NS_FORM_INPUT_RANGE:
      {
        aResultValue = StringToDecimal(aValue);
        if (!aResultValue.isFinite()) {
          return false;
        }
        return true;
      }
    case NS_FORM_INPUT_DATE:
      {
        uint32_t year, month, day;
        if (!GetValueAsDate(aValue, &year, &month, &day)) {
          return false;
        }

        double date = JS::MakeDate(year, month - 1, day);
        if (IsNaN(date)) {
          return false;
        }

        aResultValue = Decimal::fromDouble(date);
        return true;
      }
    case NS_FORM_INPUT_TIME:
      uint32_t milliseconds;
      if (!ParseTime(aValue, &milliseconds)) {
        return false;
      }

      aResultValue = Decimal(int32_t(milliseconds));
      return true;
    default:
      MOZ_ASSERT(false, "Unrecognized input type");
      return false;
  }
}

Decimal
HTMLInputElement::GetValueAsDecimal() const
{
  Decimal decimalValue;
  nsAutoString stringValue;

  GetValueInternal(stringValue);

  return !ConvertStringToNumber(stringValue, decimalValue) ? Decimal::nan()
                                                           : decimalValue;
}

void
HTMLInputElement::SetValue(const nsAString& aValue, ErrorResult& aRv)
{
  
  
  if (mType == NS_FORM_INPUT_FILE) {
    if (!aValue.IsEmpty()) {
      if (!nsContentUtils::IsCallerChrome()) {
        
        
        aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);
        return;
      }
      Sequence<nsString> list;
      list.AppendElement(aValue);
      MozSetFileNameArray(list, aRv);
      return;
    }
    else {
      ClearFiles(true);
    }
  }
  else {
    if (MayFireChangeOnBlur()) {
      
      
      
      
      
      
      
      nsAutoString currentValue;
      GetValue(currentValue);

      nsresult rv = SetValueInternal(aValue, false, true);
      if (NS_FAILED(rv)) {
        aRv.Throw(rv);
        return;
      }

      if (mFocusedValue.Equals(currentValue)) {
        GetValue(mFocusedValue);
      }
    } else {
      nsresult rv = SetValueInternal(aValue, false, true);
      if (NS_FAILED(rv)) {
        aRv.Throw(rv);
        return;
      }
    }
  }
}

NS_IMETHODIMP
HTMLInputElement::SetValue(const nsAString& aValue)
{
  ErrorResult rv;
  SetValue(aValue, rv);
  return rv.ErrorCode();
}

nsGenericHTMLElement*
HTMLInputElement::GetList() const
{
  nsAutoString dataListId;
  GetAttr(kNameSpaceID_None, nsGkAtoms::list, dataListId);
  if (dataListId.IsEmpty()) {
    return nullptr;
  }

  
  nsIDocument* doc = GetUncomposedDoc();
  if (!doc) {
    return nullptr;
  }

  Element* element = doc->GetElementById(dataListId);
  if (!element || !element->IsHTMLElement(nsGkAtoms::datalist)) {
    return nullptr;
  }

  return static_cast<nsGenericHTMLElement*>(element);
}

NS_IMETHODIMP
HTMLInputElement::GetList(nsIDOMHTMLElement** aValue)
{
  *aValue = nullptr;

  nsRefPtr<nsGenericHTMLElement> element = GetList();
  if (!element) {
    return NS_OK;
  }

  element.forget(aValue);
  return NS_OK;
}

void
HTMLInputElement::SetValue(Decimal aValue)
{
  MOZ_ASSERT(!aValue.isInfinity(), "aValue must not be Infinity!");

  if (aValue.isNaN()) {
    SetValue(EmptyString());
    return;
  }

  nsAutoString value;
  ConvertNumberToString(aValue, value);
  SetValue(value);
}

bool
HTMLInputElement::ConvertNumberToString(Decimal aValue,
                                        nsAString& aResultString) const
{
  MOZ_ASSERT(DoesValueAsNumberApply(),
             "ConvertNumberToString is only implemented for types implementing .valueAsNumber");
  MOZ_ASSERT(aValue.isFinite(),
             "aValue must be a valid non-Infinite number.");

  aResultString.Truncate();

  switch (mType) {
    case NS_FORM_INPUT_NUMBER:
    case NS_FORM_INPUT_RANGE:
      {
        char buf[32];
        bool ok = aValue.toString(buf, ArrayLength(buf));
        aResultString.AssignASCII(buf);
        MOZ_ASSERT(ok, "buf not big enough");
        return ok;
      }
    case NS_FORM_INPUT_DATE:
      {
        
        aValue = aValue.floor();

        double year = JS::YearFromTime(aValue.toDouble());
        double month = JS::MonthFromTime(aValue.toDouble());
        double day = JS::DayFromTime(aValue.toDouble());

        if (IsNaN(year) || IsNaN(month) || IsNaN(day)) {
          return false;
        }

        aResultString.AppendPrintf("%04.0f-%02.0f-%02.0f", year,
                                   month + 1, day);

        return true;
      }
    case NS_FORM_INPUT_TIME:
      {
        
        
        
        uint32_t value = NS_floorModulo(aValue.floor(), Decimal(86400000)).toDouble();

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
      MOZ_ASSERT(false, "Unrecognized input type");
      return false;
  }
}


Nullable<Date>
HTMLInputElement::GetValueAsDate(ErrorResult& aRv)
{
  if (mType != NS_FORM_INPUT_DATE && mType != NS_FORM_INPUT_TIME) {
    return Nullable<Date>();
  }

  switch (mType) {
    case NS_FORM_INPUT_DATE:
    {
      uint32_t year, month, day;
      nsAutoString value;
      GetValueInternal(value);
      if (!GetValueAsDate(value, &year, &month, &day)) {
        return Nullable<Date>();
      }

      return Nullable<Date>(Date(JS::MakeDate(year, month - 1, day)));
    }
    case NS_FORM_INPUT_TIME:
    {
      uint32_t millisecond;
      nsAutoString value;
      GetValueInternal(value);
      if (!ParseTime(value, &millisecond)) {
        return Nullable<Date>();
      }

      return Nullable<Date>(Date(millisecond));
    }
  }

  MOZ_ASSERT(false, "Unrecognized input type");
  aRv.Throw(NS_ERROR_UNEXPECTED);
  return Nullable<Date>();
}

void
HTMLInputElement::SetValueAsDate(Nullable<Date> aDate, ErrorResult& aRv)
{
  if (mType != NS_FORM_INPUT_DATE && mType != NS_FORM_INPUT_TIME) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  if (aDate.IsNull() || aDate.Value().IsUndefined()) {
    aRv = SetValue(EmptyString());
    return;
  }

  SetValue(Decimal::fromDouble(aDate.Value().TimeStamp()));
}

NS_IMETHODIMP
HTMLInputElement::GetValueAsNumber(double* aValueAsNumber)
{
  *aValueAsNumber = ValueAsNumber();
  return NS_OK;
}

void
HTMLInputElement::SetValueAsNumber(double aValueAsNumber, ErrorResult& aRv)
{
  
  
  if (IsInfinite(aValueAsNumber)) {
    aRv.Throw(NS_ERROR_INVALID_ARG);
    return;
  }

  if (!DoesValueAsNumberApply()) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  SetValue(Decimal::fromDouble(aValueAsNumber));
}

NS_IMETHODIMP
HTMLInputElement::SetValueAsNumber(double aValueAsNumber)
{
  ErrorResult rv;
  SetValueAsNumber(aValueAsNumber, rv);
  return rv.ErrorCode();
}

Decimal
HTMLInputElement::GetMinimum() const
{
  MOZ_ASSERT(DoesValueAsNumberApply(),
             "GetMinimum() should only be used for types that allow .valueAsNumber");

  
  Decimal defaultMinimum =
    mType == NS_FORM_INPUT_RANGE ? Decimal(0) : Decimal::nan();

  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::min)) {
    return defaultMinimum;
  }

  nsAutoString minStr;
  GetAttr(kNameSpaceID_None, nsGkAtoms::min, minStr);

  Decimal min;
  return ConvertStringToNumber(minStr, min) ? min : defaultMinimum;
}

Decimal
HTMLInputElement::GetMaximum() const
{
  MOZ_ASSERT(DoesValueAsNumberApply(),
             "GetMaximum() should only be used for types that allow .valueAsNumber");

  
  Decimal defaultMaximum =
    mType == NS_FORM_INPUT_RANGE ? Decimal(100) : Decimal::nan();

  if (!HasAttr(kNameSpaceID_None, nsGkAtoms::max)) {
    return defaultMaximum;
  }

  nsAutoString maxStr;
  GetAttr(kNameSpaceID_None, nsGkAtoms::max, maxStr);

  Decimal max;
  return ConvertStringToNumber(maxStr, max) ? max : defaultMaximum;
}

Decimal
HTMLInputElement::GetStepBase() const
{
  MOZ_ASSERT(mType == NS_FORM_INPUT_NUMBER ||
             mType == NS_FORM_INPUT_DATE ||
             mType == NS_FORM_INPUT_TIME ||
             mType == NS_FORM_INPUT_RANGE,
             "Check that kDefaultStepBase is correct for this new type");

  Decimal stepBase;

  
  
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
HTMLInputElement::GetValueIfStepped(int32_t aStep,
                                    StepCallerType aCallerType,
                                    Decimal* aNextStep)
{
  if (!DoStepDownStepUpApply()) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  Decimal stepBase = GetStepBase();
  Decimal step = GetStep();
  if (step == kStepAny) {
    if (aCallerType != CALLED_FOR_USER_EVENT) {
      return NS_ERROR_DOM_INVALID_STATE_ERR;
    }
    
    step = GetDefaultStep();
  }

  Decimal minimum = GetMinimum();
  Decimal maximum = GetMaximum();

  if (!maximum.isNaN()) {
    
    maximum = maximum - NS_floorModulo(maximum - stepBase, step);
    if (!minimum.isNaN()) {
      if (minimum > maximum) {
        
        
        
        
        return NS_OK;
      }
    }
  }

  Decimal value = GetValueAsDecimal();
  bool valueWasNaN = false;
  if (value.isNaN()) {
    value = Decimal(0);
    valueWasNaN = true;
  }
  Decimal valueBeforeStepping = value;

  Decimal deltaFromStep = NS_floorModulo(value - stepBase, step);

  if (deltaFromStep != Decimal(0)) {
    if (aStep > 0) {
      value += step - deltaFromStep;      
      value += step * Decimal(aStep - 1); 
    } else if (aStep < 0) {
      value -= deltaFromStep;             
      value += step * Decimal(aStep + 1); 
    }
  } else {
    value += step * Decimal(aStep);
  }

  
  
  
  if (mType == NS_FORM_INPUT_DATE &&
      NS_floorModulo(Decimal(value - GetStepBase()), GetStepScaleFactor()) != Decimal(0)) {
    MOZ_ASSERT(GetStep() > Decimal(0));
    Decimal validStep = EuclidLCM<Decimal>(GetStep().floor(),
                                           GetStepScaleFactor().floor());
    if (aStep > 0) {
      value -= NS_floorModulo(value - GetStepBase(), validStep);
      value += validStep;
    } else if (aStep < 0) {
      value -= NS_floorModulo(value - GetStepBase(), validStep);
    }
  }

  if (value < minimum) {
    value = minimum;
    deltaFromStep = NS_floorModulo(value - stepBase, step);
    if (deltaFromStep != Decimal(0)) {
      value += step - deltaFromStep;
    }
  }
  if (value > maximum) {
    value = maximum;
    deltaFromStep = NS_floorModulo(value - stepBase, step);
    if (deltaFromStep != Decimal(0)) {
      value -= deltaFromStep;
    }
  }

  if (!valueWasNaN && 
      ((aStep > 0 && value < valueBeforeStepping) ||
       (aStep < 0 && value > valueBeforeStepping))) {
    
    
    return NS_OK;
  }

  *aNextStep = value;
  return NS_OK;
}

nsresult
HTMLInputElement::ApplyStep(int32_t aStep)
{
  Decimal nextStep = Decimal::nan(); 

  nsresult rv = GetValueIfStepped(aStep, CALLED_FOR_SCRIPT, &nextStep);

  if (NS_SUCCEEDED(rv) && nextStep.isFinite()) {
    SetValue(nextStep);
  }

  return rv;
}

NS_IMETHODIMP
HTMLInputElement::StepDown(int32_t n, uint8_t optional_argc)
{
  return ApplyStep(optional_argc ? -n : -1);
}

NS_IMETHODIMP
HTMLInputElement::StepUp(int32_t n, uint8_t optional_argc)
{
  return ApplyStep(optional_argc ? n : 1);
}

void
HTMLInputElement::FlushFrames()
{
  if (GetComposedDoc()) {
    GetComposedDoc()->FlushPendingNotifications(Flush_Frames);
  }
}

void
HTMLInputElement::MozGetFileNameArray(nsTArray< nsString >& aArray)
{
  for (uint32_t i = 0; i < mFiles.Length(); i++) {
    nsString str;
    mFiles[i]->GetMozFullPathInternal(str);
    aArray.AppendElement(str);
  }
}


NS_IMETHODIMP
HTMLInputElement::MozGetFileNameArray(uint32_t* aLength, char16_t*** aFileNames)
{
  if (!nsContentUtils::IsCallerChrome()) {
    
    
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsTArray<nsString> array;
  MozGetFileNameArray(array);

  *aLength = array.Length();
  char16_t** ret =
    static_cast<char16_t**>(NS_Alloc(*aLength * sizeof(char16_t*)));
  if (!ret) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  for (uint32_t i = 0; i < *aLength; ++i) {
    ret[i] = NS_strdup(array[i].get());
  }

  *aFileNames = ret;

  return NS_OK;
}

void
HTMLInputElement::MozSetFileArray(const Sequence<OwningNonNull<File>>& aFiles)
{
  nsCOMPtr<nsIGlobalObject> global = OwnerDoc()->GetScopeObject();
  MOZ_ASSERT(global);
  if (!global) {
    return;
  }
  nsTArray<nsRefPtr<File>> files;
  for (uint32_t i = 0; i < aFiles.Length(); ++i) {
    files.AppendElement(new File(global, aFiles[i].get()->Impl()));
  }
  SetFiles(files, true);
}

void
HTMLInputElement::MozSetFileNameArray(const Sequence< nsString >& aFileNames, ErrorResult& aRv)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return;
  }

  nsTArray<nsRefPtr<File>> files;
  for (uint32_t i = 0; i < aFileNames.Length(); ++i) {
    nsCOMPtr<nsIFile> file;

    if (StringBeginsWith(aFileNames[i], NS_LITERAL_STRING("file:"),
                         nsASCIICaseInsensitiveStringComparator())) {
      
      
      NS_GetFileFromURLSpec(NS_ConvertUTF16toUTF8(aFileNames[i]),
                            getter_AddRefs(file));
    }

    if (!file) {
      
      NS_NewLocalFile(aFileNames[i], false, getter_AddRefs(file));
    }

    if (file) {
      nsCOMPtr<nsIGlobalObject> global = OwnerDoc()->GetScopeObject();
      nsRefPtr<File> domFile = File::CreateFromFile(global, file);
      files.AppendElement(domFile);
    } else {
      continue; 
    }

  }

  SetFiles(files, true);
}

NS_IMETHODIMP
HTMLInputElement::MozSetFileNameArray(const char16_t** aFileNames, uint32_t aLength)
{
  if (!nsContentUtils::IsCallerChrome()) {
    
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  Sequence<nsString> list;
  for (uint32_t i = 0; i < aLength; ++i) {
    list.AppendElement(nsDependentString(aFileNames[i]));
  }

  ErrorResult rv;
  MozSetFileNameArray(list, rv);
  return rv.ErrorCode();
}

bool
HTMLInputElement::MozIsTextField(bool aExcludePassword)
{
  
  if (IsExperimentalMobileType(mType)) {
    return false;
  }

  return IsSingleLineTextControl(aExcludePassword);
}

HTMLInputElement*
HTMLInputElement::GetOwnerNumberControl()
{
  if (IsInNativeAnonymousSubtree() &&
      mType == NS_FORM_INPUT_TEXT &&
      GetParent() && GetParent()->GetParent()) {
    HTMLInputElement* grandparent =
      HTMLInputElement::FromContentOrNull(GetParent()->GetParent());
    if (grandparent && grandparent->mType == NS_FORM_INPUT_NUMBER) {
      return grandparent;
    }
  }
  return nullptr;
}

NS_IMETHODIMP
HTMLInputElement::MozIsTextField(bool aExcludePassword, bool* aResult)
{
  *aResult = MozIsTextField(aExcludePassword);
  return NS_OK;
}

NS_IMETHODIMP
HTMLInputElement::SetUserInput(const nsAString& aValue)
{
  if (!nsContentUtils::IsCallerChrome()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  if (mType == NS_FORM_INPUT_FILE)
  {
    Sequence<nsString> list;
    list.AppendElement(aValue);
    ErrorResult rv;
    MozSetFileNameArray(list, rv);
    return rv.ErrorCode();
  } else {
    nsresult rv = SetValueInternal(aValue, true, true);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsContentUtils::DispatchTrustedEvent(OwnerDoc(),
                                       static_cast<nsIDOMHTMLInputElement*>(this),
                                       NS_LITERAL_STRING("input"), true,
                                       true);

  
  
  if (!ShouldBlur(this)) {
    FireChangeEventIfNeeded();
  }

  return NS_OK;
}

nsIEditor*
HTMLInputElement::GetEditor()
{
  nsTextEditorState* state = GetEditorState();
  if (state) {
    return state->GetEditor();
  }
  return nullptr;
}

NS_IMETHODIMP_(nsIEditor*)
HTMLInputElement::GetTextEditor()
{
  return GetEditor();
}

NS_IMETHODIMP_(nsISelectionController*)
HTMLInputElement::GetSelectionController()
{
  nsTextEditorState* state = GetEditorState();
  if (state) {
    return state->GetSelectionController();
  }
  return nullptr;
}

nsFrameSelection*
HTMLInputElement::GetConstFrameSelection()
{
  nsTextEditorState* state = GetEditorState();
  if (state) {
    return state->GetConstFrameSelection();
  }
  return nullptr;
}

NS_IMETHODIMP
HTMLInputElement::BindToFrame(nsTextControlFrame* aFrame)
{
  nsTextEditorState* state = GetEditorState();
  if (state) {
    return state->BindToFrame(aFrame);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP_(void)
HTMLInputElement::UnbindFromFrame(nsTextControlFrame* aFrame)
{
  nsTextEditorState* state = GetEditorState();
  if (state && aFrame) {
    state->UnbindFromFrame(aFrame);
  }
}

NS_IMETHODIMP
HTMLInputElement::CreateEditor()
{
  nsTextEditorState* state = GetEditorState();
  if (state) {
    return state->PrepareEditor();
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP_(nsIContent*)
HTMLInputElement::GetRootEditorNode()
{
  nsTextEditorState* state = GetEditorState();
  if (state) {
    return state->GetRootNode();
  }
  return nullptr;
}

NS_IMETHODIMP_(Element*)
HTMLInputElement::CreatePlaceholderNode()
{
  nsTextEditorState* state = GetEditorState();
  if (state) {
    NS_ENSURE_SUCCESS(state->CreatePlaceholderNode(), nullptr);
    return state->GetPlaceholderNode();
  }
  return nullptr;
}

NS_IMETHODIMP_(Element*)
HTMLInputElement::GetPlaceholderNode()
{
  nsTextEditorState* state = GetEditorState();
  if (state) {
    return state->GetPlaceholderNode();
  }
  return nullptr;
}

NS_IMETHODIMP_(void)
HTMLInputElement::UpdatePlaceholderVisibility(bool aNotify)
{
  nsTextEditorState* state = GetEditorState();
  if (state) {
    state->UpdatePlaceholderVisibility(aNotify);
  }
}

NS_IMETHODIMP_(bool)
HTMLInputElement::GetPlaceholderVisibility()
{
  nsTextEditorState* state = GetEditorState();
  if (!state) {
    return false;
  }

  return state->GetPlaceholderVisibility();
}

void
HTMLInputElement::GetDisplayFileName(nsAString& aValue) const
{
  if (OwnerDoc()->IsStaticDocument()) {
    aValue = mStaticDocFileList;
    return;
  }

  if (mFiles.Length() == 1) {
    mFiles[0]->GetName(aValue);
    return;
  }

  nsXPIDLString value;

  if (mFiles.IsEmpty()) {
    if (HasAttr(kNameSpaceID_None, nsGkAtoms::multiple)) {
      nsContentUtils::GetLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                         "NoFilesSelected", value);
    } else {
      nsContentUtils::GetLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                         "NoFileSelected", value);
    }
  } else {
    nsString count;
    count.AppendInt(int(mFiles.Length()));

    const char16_t* params[] = { count.get() };
    nsContentUtils::FormatLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                          "XFilesSelected", params, value);
  }

  aValue = value;
}

void
HTMLInputElement::SetFiles(const nsTArray<nsRefPtr<File>>& aFiles,
                           bool aSetValueChanged)
{
  mFiles.Clear();
  mFiles.AppendElements(aFiles);

  AfterSetFiles(aSetValueChanged);
}

void
HTMLInputElement::SetFiles(nsIDOMFileList* aFiles,
                           bool aSetValueChanged)
{
  nsRefPtr<FileList> files = static_cast<FileList*>(aFiles);
  mFiles.Clear();

  if (aFiles) {
    uint32_t listLength;
    aFiles->GetLength(&listLength);
    for (uint32_t i = 0; i < listLength; i++) {
      nsRefPtr<File> file = files->Item(i);
      mFiles.AppendElement(file);
    }
  }

  AfterSetFiles(aSetValueChanged);
}

void
HTMLInputElement::AfterSetFiles(bool aSetValueChanged)
{
  
  
  
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(false);
  if (formControlFrame) {
    nsAutoString readableValue;
    GetDisplayFileName(readableValue);
    formControlFrame->SetFormProperty(nsGkAtoms::value, readableValue);
  }

#ifndef MOZ_CHILD_PERMISSIONS
  
  
  
  
  
  if (mFiles.IsEmpty()) {
    mFirstFilePath.Truncate();
  } else {
    mFiles[0]->GetMozFullPath(mFirstFilePath);
  }
#endif

  UpdateFileList();

  if (aSetValueChanged) {
    SetValueChanged(true);
  }

  UpdateAllValidityStates(true);
}

void
HTMLInputElement::FireChangeEventIfNeeded()
{
  nsAutoString value;
  GetValue(value);

  if (!MayFireChangeOnBlur() || mFocusedValue.Equals(value)) {
    return;
  }

  
  mFocusedValue = value;
  nsContentUtils::DispatchTrustedEvent(OwnerDoc(),
                                       static_cast<nsIContent*>(this),
                                       NS_LITERAL_STRING("change"), true,
                                       false);
}

FileList*
HTMLInputElement::GetFiles()
{
  if (mType != NS_FORM_INPUT_FILE) {
    return nullptr;
  }

  if (!mFileList) {
    mFileList = new FileList(static_cast<nsIContent*>(this));
    UpdateFileList();
  }

  return mFileList;
}

void
HTMLInputElement::OpenDirectoryPicker(ErrorResult& aRv)
{
  if (mType != NS_FORM_INPUT_FILE) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
  }
  InitFilePicker(FILE_PICKER_DIRECTORY);
}

void
HTMLInputElement::CancelDirectoryPickerScanIfRunning()
{
  if (!mDirPickerFileListBuilderTask) {
    return;
  }
  if (mProgressTimer) {
    mProgressTimerIsActive = false;
    mProgressTimer->Cancel();
  }
  mDirPickerFileListBuilderTask->Cancel();
  mDirPickerFileListBuilderTask = nullptr;
}

void
HTMLInputElement::StartProgressEventTimer()
{
  if (!mProgressTimer) {
    mProgressTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  }
  if (mProgressTimer) {
    mProgressTimerIsActive = true;
    mProgressTimer->Cancel();
    mProgressTimer->InitWithCallback(this, kProgressEventInterval,
                                           nsITimer::TYPE_ONE_SHOT);
  }
}


NS_IMETHODIMP
HTMLInputElement::Notify(nsITimer* aTimer)
{
  if (mProgressTimer == aTimer) {
    mProgressTimerIsActive = false;
    MaybeDispatchProgressEvent(false);
    return NS_OK;
  }

  
  NS_WARNING("Unexpected timer!");
  return NS_ERROR_INVALID_POINTER;
}

 void
HTMLInputElement::HandleNumberControlSpin(void* aData)
{
  HTMLInputElement* input = static_cast<HTMLInputElement*>(aData);

  NS_ASSERTION(input->mNumberControlSpinnerIsSpinning,
               "Should have called nsRepeatService::Stop()");

  nsNumberControlFrame* numberControlFrame =
    do_QueryFrame(input->GetPrimaryFrame());
  if (input->mType != NS_FORM_INPUT_NUMBER || !numberControlFrame) {
    
    
    
    input->StopNumberControlSpinnerSpin();
  } else {
    input->StepNumberControlForUserEvent(input->mNumberControlSpinnerSpinsUp ? 1 : -1);
  }
}

void
HTMLInputElement::MaybeDispatchProgressEvent(bool aFinalProgress)
{
  nsRefPtr<HTMLInputElement> kungFuDeathGrip;

  if (aFinalProgress && mProgressTimerIsActive) {
    
    
    
    kungFuDeathGrip = this;

    mProgressTimerIsActive = false;
    mProgressTimer->Cancel();
  }

  uint32_t fileListLength = mDirPickerFileListBuilderTask->GetFileListLength();

  if (mProgressTimerIsActive ||
      fileListLength == mDirPickerFileListBuilderTask->mPreviousFileListLength) {
    return;
  }

  if (!aFinalProgress) {
    StartProgressEventTimer();
  }

  mDirPickerFileListBuilderTask->mPreviousFileListLength = fileListLength;

  DispatchProgressEvent(NS_LITERAL_STRING(PROGRESS_STR),
                        false,
                        mDirPickerFileListBuilderTask->mPreviousFileListLength,
                        0);
}

void
HTMLInputElement::DispatchProgressEvent(const nsAString& aType,
                                        bool aLengthComputable,
                                        uint64_t aLoaded, uint64_t aTotal)
{
  NS_ASSERTION(!aType.IsEmpty(), "missing event type");

  ProgressEventInit init;
  init.mBubbles = false;
  init.mCancelable = true; 
  init.mLengthComputable = aLengthComputable;
  init.mLoaded = aLoaded;
  init.mTotal = (aTotal == UINT64_MAX) ? 0 : aTotal;

  nsRefPtr<ProgressEvent> event =
    ProgressEvent::Constructor(this, aType, init);
  event->SetTrusted(true);

  bool doDefaultAction;
  nsresult rv = DispatchEvent(event, &doDefaultAction);
  if (NS_SUCCEEDED(rv) && !doDefaultAction) {
    CancelDirectoryPickerScanIfRunning();
  }
}

nsresult
HTMLInputElement::UpdateFileList()
{
  if (mFileList) {
    mFileList->Clear();

    const nsTArray<nsRefPtr<File>>& files = GetFilesInternal();
    for (uint32_t i = 0; i < files.Length(); ++i) {
      if (!mFileList->Append(files[i])) {
        return NS_ERROR_FAILURE;
      }
    }
  }

  return NS_OK;
}

nsresult
HTMLInputElement::SetValueInternal(const nsAString& aValue,
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
        if (!mInputData.mState->SetValue(value, aUserInput, aSetValueChanged)) {
          return NS_ERROR_OUT_OF_MEMORY;
        }
        if (mType == NS_FORM_INPUT_EMAIL) {
          UpdateAllValidityStates(mParserCreating);
        }
      } else {
        free(mInputData.mValue);
        mInputData.mValue = ToNewUnicode(value);
        if (aSetValueChanged) {
          SetValueChanged(true);
        }
        if (mType == NS_FORM_INPUT_NUMBER) {
          
          
          nsNumberControlFrame* numberControlFrame =
            do_QueryFrame(GetPrimaryFrame());
          if (numberControlFrame) {
            numberControlFrame->SetValueOfAnonTextControl(value);
          }
        } else if (mType == NS_FORM_INPUT_RANGE) {
          nsRangeFrame* frame = do_QueryFrame(GetPrimaryFrame());
          if (frame) {
            frame->UpdateForValueChange();
          }
        }
        if (!mParserCreating) {
          OnValueChanged(true);
        }
        
      }

      if (mType == NS_FORM_INPUT_COLOR) {
        
        nsColorControlFrame* colorControlFrame = do_QueryFrame(GetPrimaryFrame());
        if (colorControlFrame) {
          colorControlFrame->UpdateColor();
        }
      }

      return NS_OK;
    }

    case VALUE_MODE_DEFAULT:
    case VALUE_MODE_DEFAULT_ON:
      
      
      
      
      
      if (mType == NS_FORM_INPUT_HIDDEN) {
        SetValueChanged(true);
      }

      
      return nsGenericHTMLFormElementWithState::SetAttr(kNameSpaceID_None,
                                                        nsGkAtoms::value, aValue,
                                                        true);

    case VALUE_MODE_FILENAME:
      return NS_ERROR_UNEXPECTED;
  }

  
  return NS_OK;
}

NS_IMETHODIMP
HTMLInputElement::SetValueChanged(bool aValueChanged)
{
  bool valueChangedBefore = mValueChanged;

  mValueChanged = aValueChanged;

  if (valueChangedBefore != aValueChanged) {
    UpdateState(true);
  }

  return NS_OK;
}

NS_IMETHODIMP
HTMLInputElement::GetChecked(bool* aChecked)
{
  *aChecked = Checked();
  return NS_OK;
}

void
HTMLInputElement::SetCheckedChanged(bool aCheckedChanged)
{
  DoSetCheckedChanged(aCheckedChanged, true);
}

void
HTMLInputElement::DoSetCheckedChanged(bool aCheckedChanged,
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
HTMLInputElement::SetCheckedChangedInternal(bool aCheckedChanged)
{
  bool checkedChangedBefore = mCheckedChanged;

  mCheckedChanged = aCheckedChanged;

  
  
  if (checkedChangedBefore != aCheckedChanged) {
    UpdateState(true);
  }
}

NS_IMETHODIMP
HTMLInputElement::SetChecked(bool aChecked)
{
  DoSetChecked(aChecked, true, true);
  return NS_OK;
}

void
HTMLInputElement::DoSetChecked(bool aChecked, bool aNotify,
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
HTMLInputElement::RadioSetChecked(bool aNotify)
{
  
  nsCOMPtr<nsIDOMHTMLInputElement> currentlySelected = GetSelectedRadioButton();

  
  if (currentlySelected) {
    
    
    static_cast<HTMLInputElement*>(currentlySelected.get())
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
HTMLInputElement::GetRadioGroupContainer() const
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

  
  return static_cast<nsDocument*>(GetUncomposedDoc());
}

already_AddRefed<nsIDOMHTMLInputElement>
HTMLInputElement::GetSelectedRadioButton()
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
HTMLInputElement::MaybeSubmitForm(nsPresContext* aPresContext)
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
    
    
    WidgetMouseEvent event(true, NS_MOUSE_CLICK, nullptr,
                           WidgetMouseEvent::eReal);
    nsEventStatus status = nsEventStatus_eIgnore;
    shell->HandleDOMEventWithTarget(submitContent, &event, &status);
  } else if (!mForm->ImplicitSubmissionIsDisabled() &&
             (mForm->HasAttr(kNameSpaceID_None, nsGkAtoms::novalidate) ||
              mForm->CheckValidFormSubmission())) {
    
    
    
    
    nsRefPtr<mozilla::dom::HTMLFormElement> form = mForm;
    InternalFormEvent event(true, NS_FORM_SUBMIT);
    nsEventStatus status = nsEventStatus_eIgnore;
    shell->HandleDOMEventWithTarget(mForm, &event, &status);
  }

  return NS_OK;
}

void
HTMLInputElement::SetCheckedInternal(bool aChecked, bool aNotify)
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
HTMLInputElement::Blur(ErrorResult& aError)
{
  if (mType == NS_FORM_INPUT_NUMBER) {
    
    
    nsNumberControlFrame* numberControlFrame =
      do_QueryFrame(GetPrimaryFrame());
    if (numberControlFrame) {
      HTMLInputElement* textControl = numberControlFrame->GetAnonTextControl();
      if (textControl) {
        textControl->Blur(aError);
        return;
      }
    }
  }
  nsGenericHTMLElement::Blur(aError);
}

void
HTMLInputElement::Focus(ErrorResult& aError)
{
  if (mType == NS_FORM_INPUT_NUMBER) {
    
    nsNumberControlFrame* numberControlFrame =
      do_QueryFrame(GetPrimaryFrame());
    if (numberControlFrame) {
      HTMLInputElement* textControl = numberControlFrame->GetAnonTextControl();
      if (textControl) {
        textControl->Focus(aError);
        return;
      }
    }
  }

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
      if (formCtrl && formCtrl->GetType() == NS_FORM_BUTTON_BUTTON) {
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

bool
HTMLInputElement::IsInteractiveHTMLContent(bool aIgnoreTabindex) const
{
  return mType != NS_FORM_INPUT_HIDDEN ||
         nsGenericHTMLFormElementWithState::IsInteractiveHTMLContent(aIgnoreTabindex);
}

NS_IMETHODIMP
HTMLInputElement::Select()
{
  if (mType == NS_FORM_INPUT_NUMBER) {
    nsNumberControlFrame* numberControlFrame =
      do_QueryFrame(GetPrimaryFrame());
    if (numberControlFrame) {
      return numberControlFrame->HandleSelectCall();
    }
    return NS_OK;
  }

  if (!IsSingleLineTextControl(false)) {
    return NS_OK;
  }

  
  

  FocusTristate state = FocusState();
  if (state == eUnfocusable) {
    return NS_OK;
  }

  nsIFocusManager* fm = nsFocusManager::GetFocusManager();

  nsRefPtr<nsPresContext> presContext = GetPresContext(eForComposedDoc);
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
    if (SameCOMIdentity(static_cast<nsIDOMNode*>(this), focusedElement)) {
      
      SelectAll(presContext);
    }
  }

  return NS_OK;
}

bool
HTMLInputElement::DispatchSelectEvent(nsPresContext* aPresContext)
{
  nsEventStatus status = nsEventStatus_eIgnore;

  
  if (!mHandlingSelectEvent) {
    WidgetEvent event(nsContentUtils::IsCallerChrome(), NS_FORM_SELECTED);

    mHandlingSelectEvent = true;
    EventDispatcher::Dispatch(static_cast<nsIContent*>(this),
                              aPresContext, &event, nullptr, &status);
    mHandlingSelectEvent = false;
  }

  
  
  return (status == nsEventStatus_eIgnore);
}

void
HTMLInputElement::SelectAll(nsPresContext* aPresContext)
{
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(true);

  if (formControlFrame) {
    formControlFrame->SetFormProperty(nsGkAtoms::select, EmptyString());
  }
}

bool
HTMLInputElement::NeedToInitializeEditorForEvent(
                    EventChainPreVisitor& aVisitor) const
{
  
  
  
  
  
  if (!IsSingleLineTextControl(false) ||
      aVisitor.mEvent->mClass == eMutationEventClass) {
    return false;
  }

  switch (aVisitor.mEvent->message) {
  case NS_MOUSE_MOVE:
  case NS_MOUSE_ENTER:
  case NS_MOUSE_EXIT:
  case NS_MOUSE_ENTER_SYNTH:
  case NS_MOUSE_EXIT_SYNTH:
  case NS_SCROLLPORT_UNDERFLOW:
  case NS_SCROLLPORT_OVERFLOW:
    return false;
  default:
    return true;
  }
}

bool
HTMLInputElement::IsDisabledForEvents(uint32_t aMessage)
{
  return IsElementDisabledForEvents(aMessage, GetPrimaryFrame());
}

nsresult
HTMLInputElement::PreHandleEvent(EventChainPreVisitor& aVisitor)
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
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  WidgetMouseEvent* mouseEvent = aVisitor.mEvent->AsMouseEvent();
  bool outerActivateEvent =
    ((mouseEvent && mouseEvent->IsLeftClickEvent()) ||
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
        if (mForm) {
          
          
          
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
      aVisitor.mEvent->AsMouseEvent()->button ==
        WidgetMouseEvent::eMiddleButton) {
    aVisitor.mEvent->mFlags.mNoContentDispatch = false;
  }

  
  aVisitor.mItemFlags |= mType;

  
  if (aVisitor.mEvent->message == NS_BLUR_CONTENT) {
    
    
    
    if (IsExperimentalMobileType(mType)) {
      nsAutoString aValue;
      GetValueInternal(aValue);
      nsresult rv = SetValueInternal(aValue, false, false);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    FireChangeEventIfNeeded();
  }

  if (mType == NS_FORM_INPUT_RANGE &&
      (aVisitor.mEvent->message == NS_FOCUS_CONTENT ||
       aVisitor.mEvent->message == NS_BLUR_CONTENT)) {
    
    
    nsIFrame* frame = GetPrimaryFrame();
    if (frame) {
      frame->InvalidateFrameSubtree();
    }
  }

  if (mType == NS_FORM_INPUT_NUMBER &&
      aVisitor.mEvent->mFlags.mIsTrusted) {
    if (mNumberControlSpinnerIsSpinning) {
      
      
      
      
      
      
      if (aVisitor.mEvent->message == NS_MOUSE_MOVE) {
        
        bool stopSpin = true;
        nsNumberControlFrame* numberControlFrame =
          do_QueryFrame(GetPrimaryFrame());
        if (numberControlFrame) {
          bool oldNumberControlSpinTimerSpinsUpValue =
                 mNumberControlSpinnerSpinsUp;
          switch (numberControlFrame->GetSpinButtonForPointerEvent(
                    aVisitor.mEvent->AsMouseEvent())) {
          case nsNumberControlFrame::eSpinButtonUp:
            mNumberControlSpinnerSpinsUp = true;
            stopSpin = false;
            break;
          case nsNumberControlFrame::eSpinButtonDown:
            mNumberControlSpinnerSpinsUp = false;
            stopSpin = false;
            break;
          }
          if (mNumberControlSpinnerSpinsUp !=
                oldNumberControlSpinTimerSpinsUpValue) {
            nsNumberControlFrame* numberControlFrame =
              do_QueryFrame(GetPrimaryFrame());
            if (numberControlFrame) {
              numberControlFrame->SpinnerStateChanged();
            }
          }
        }
        if (stopSpin) {
          StopNumberControlSpinnerSpin();
        }
      } else if (aVisitor.mEvent->message == NS_MOUSE_BUTTON_UP) {
        StopNumberControlSpinnerSpin();
      }
    }
    if (aVisitor.mEvent->message == NS_FOCUS_CONTENT ||
        aVisitor.mEvent->message == NS_BLUR_CONTENT) {
      if (aVisitor.mEvent->message == NS_FOCUS_CONTENT) {
        
        
        nsNumberControlFrame* numberControlFrame =
          do_QueryFrame(GetPrimaryFrame());
        if (numberControlFrame) {
          
          numberControlFrame->HandleFocusEvent(aVisitor.mEvent);
        }
      }
      nsIFrame* frame = GetPrimaryFrame();
      if (frame && frame->IsThemed()) {
        
        
        
        
        
        
        frame->InvalidateFrame();
      }
    } else if (aVisitor.mEvent->message == NS_KEY_UP) {
      WidgetKeyboardEvent* keyEvent = aVisitor.mEvent->AsKeyboardEvent();
      if ((keyEvent->keyCode == NS_VK_UP || keyEvent->keyCode == NS_VK_DOWN) &&
          !(keyEvent->IsShift() || keyEvent->IsControl() ||
            keyEvent->IsAlt() || keyEvent->IsMeta() ||
            keyEvent->IsAltGraph() || keyEvent->IsFn() ||
            keyEvent->IsOS())) {
        
        
        
        FireChangeEventIfNeeded();
      }
    }
  }

  nsresult rv = nsGenericHTMLFormElementWithState::PreHandleEvent(aVisitor);

  
  
  if (mType == NS_FORM_INPUT_NUMBER &&
      aVisitor.mEvent->mFlags.mIsTrusted  &&
      aVisitor.mEvent->originalTarget != this) {
    
    
    
    HTMLInputElement* textControl = nullptr;
    nsNumberControlFrame* numberControlFrame =
      do_QueryFrame(GetPrimaryFrame());
    if (numberControlFrame) {
      textControl = numberControlFrame->GetAnonTextControl();
    }
    if (textControl && aVisitor.mEvent->originalTarget == textControl) {
      if (aVisitor.mEvent->message == NS_EDITOR_INPUT) {
        
        nsAutoString value;
        numberControlFrame->GetValueOfAnonTextControl(value);
        numberControlFrame->HandlingInputEvent(true);
        nsWeakFrame weakNumberControlFrame(numberControlFrame);
        rv = SetValueInternal(value, true, true);
        NS_ENSURE_SUCCESS(rv, rv);
        if (weakNumberControlFrame.IsAlive()) {
          numberControlFrame->HandlingInputEvent(false);
        }
      }
      else if (aVisitor.mEvent->message == NS_FORM_CHANGE) {
        
        
        
        
        
        
        
        aVisitor.mCanHandle = false;
      }
    }
  }

  return rv;
}

void
HTMLInputElement::StartRangeThumbDrag(WidgetGUIEvent* aEvent)
{
  mIsDraggingRange = true;
  mRangeThumbDragStartValue = GetValueAsDecimal();
  
  
  nsIPresShell::SetCapturingContent(this, CAPTURE_IGNOREALLOWED);
  nsRangeFrame* rangeFrame = do_QueryFrame(GetPrimaryFrame());

  
  
  
  
  
  GetValue(mFocusedValue);

  SetValueOfRangeForUserEvent(rangeFrame->GetValueAtEventPoint(aEvent));
}

void
HTMLInputElement::FinishRangeThumbDrag(WidgetGUIEvent* aEvent)
{
  MOZ_ASSERT(mIsDraggingRange);

  if (nsIPresShell::GetCapturingContent() == this) {
    nsIPresShell::SetCapturingContent(nullptr, 0); 
  }
  if (aEvent) {
    nsRangeFrame* rangeFrame = do_QueryFrame(GetPrimaryFrame());
    SetValueOfRangeForUserEvent(rangeFrame->GetValueAtEventPoint(aEvent));
  }
  mIsDraggingRange = false;
  FireChangeEventIfNeeded();
}

void
HTMLInputElement::CancelRangeThumbDrag(bool aIsForUserEvent)
{
  MOZ_ASSERT(mIsDraggingRange);

  mIsDraggingRange = false;
  if (nsIPresShell::GetCapturingContent() == this) {
    nsIPresShell::SetCapturingContent(nullptr, 0); 
  }
  if (aIsForUserEvent) {
    SetValueOfRangeForUserEvent(mRangeThumbDragStartValue);
  } else {
    
    
    
    nsAutoString val;
    ConvertNumberToString(mRangeThumbDragStartValue, val);
    
    
    SetValueInternal(val, true, true);
    nsRangeFrame* frame = do_QueryFrame(GetPrimaryFrame());
    if (frame) {
      frame->UpdateForValueChange();
    }
    nsRefPtr<AsyncEventDispatcher> asyncDispatcher =
      new AsyncEventDispatcher(this, NS_LITERAL_STRING("input"), true, false);
    asyncDispatcher->RunDOMEventWhenSafe();
  }
}

void
HTMLInputElement::SetValueOfRangeForUserEvent(Decimal aValue)
{
  MOZ_ASSERT(aValue.isFinite());

  nsAutoString val;
  ConvertNumberToString(aValue, val);
  
  
  SetValueInternal(val, true, true);
  nsRangeFrame* frame = do_QueryFrame(GetPrimaryFrame());
  if (frame) {
    frame->UpdateForValueChange();
  }
  nsContentUtils::DispatchTrustedEvent(OwnerDoc(),
                                       static_cast<nsIDOMHTMLInputElement*>(this),
                                       NS_LITERAL_STRING("input"), true,
                                       false);
}

void
HTMLInputElement::StartNumberControlSpinnerSpin()
{
  MOZ_ASSERT(!mNumberControlSpinnerIsSpinning);

  mNumberControlSpinnerIsSpinning = true;

  nsRepeatService::GetInstance()->Start(HandleNumberControlSpin, this);

  
  
  nsIPresShell::SetCapturingContent(this, CAPTURE_IGNOREALLOWED);

  nsNumberControlFrame* numberControlFrame =
    do_QueryFrame(GetPrimaryFrame());
  if (numberControlFrame) {
    numberControlFrame->SpinnerStateChanged();
  }
}

void
HTMLInputElement::StopNumberControlSpinnerSpin()
{
  if (mNumberControlSpinnerIsSpinning) {
    if (nsIPresShell::GetCapturingContent() == this) {
      nsIPresShell::SetCapturingContent(nullptr, 0); 
    }

    nsRepeatService::GetInstance()->Stop(HandleNumberControlSpin, this);

    mNumberControlSpinnerIsSpinning = false;

    FireChangeEventIfNeeded();

    nsNumberControlFrame* numberControlFrame =
      do_QueryFrame(GetPrimaryFrame());
    if (numberControlFrame) {
      numberControlFrame->SpinnerStateChanged();
    }
  }
}

void
HTMLInputElement::StepNumberControlForUserEvent(int32_t aDirection)
{
  
  
  
  if (HasBadInput()) {
    
    
    
    
    
    
    
    
    nsNumberControlFrame* numberControlFrame =
      do_QueryFrame(GetPrimaryFrame());
    if (numberControlFrame &&
        !numberControlFrame->AnonTextControlIsEmpty()) {
      
      
      
      UpdateValidityUIBits(true);
      UpdateState(true);
      return;
    }
  }

  Decimal newValue = Decimal::nan(); 

  nsresult rv = GetValueIfStepped(aDirection, CALLED_FOR_USER_EVENT, &newValue);

  if (NS_FAILED(rv) || !newValue.isFinite()) {
    return; 
  }

  nsAutoString newVal;
  ConvertNumberToString(newValue, newVal);
  
  
  SetValueInternal(newVal, true, true);

  nsContentUtils::DispatchTrustedEvent(OwnerDoc(),
                                       static_cast<nsIDOMHTMLInputElement*>(this),
                                       NS_LITERAL_STRING("input"), true,
                                       false);
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

bool
HTMLInputElement::ShouldPreventDOMActivateDispatch(EventTarget* aOriginalTarget)
{
  







  if (mType != NS_FORM_INPUT_FILE) {
    return false;
  }

  nsCOMPtr<nsIContent> target = do_QueryInterface(aOriginalTarget);
  if (!target) {
    return false;
  }

  return target->GetParent() == this &&
         target->IsRootOfNativeAnonymousSubtree() &&
         target->AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                             nsGkAtoms::button, eCaseMatters);
}

nsresult
HTMLInputElement::MaybeInitPickers(EventChainPostVisitor& aVisitor)
{
  
  
  
  
  
  
  
  if (aVisitor.mEvent->mFlags.mDefaultPrevented) {
    return NS_OK;
  }
  WidgetMouseEvent* mouseEvent = aVisitor.mEvent->AsMouseEvent();
  if (!(mouseEvent && mouseEvent->IsLeftClickEvent())) {
    return NS_OK;
  }
  if (mType == NS_FORM_INPUT_FILE) {
    return InitFilePicker(FILE_PICKER_FILE);
  }
  if (mType == NS_FORM_INPUT_COLOR) {
    return InitColorPicker();
  }
  return NS_OK;
}

nsresult
HTMLInputElement::PostHandleEvent(EventChainPostVisitor& aVisitor)
{
  if (!aVisitor.mPresContext) {
    
    
    return MaybeInitPickers(aVisitor);
  }

  if (aVisitor.mEvent->message == NS_FOCUS_CONTENT ||
      aVisitor.mEvent->message == NS_BLUR_CONTENT) {
    if (aVisitor.mEvent->message == NS_FOCUS_CONTENT &&
        MayFireChangeOnBlur() &&
        !mIsDraggingRange) { 
      GetValue(mFocusedValue);
    }

    if (aVisitor.mEvent->message == NS_BLUR_CONTENT) {
      if (mIsDraggingRange) {
        FinishRangeThumbDrag();
      } else if (mNumberControlSpinnerIsSpinning) {
        StopNumberControlSpinnerSpin();
      }
    }

    UpdateValidityUIBits(aVisitor.mEvent->message == NS_FOCUS_CONTENT);

    UpdateState(true);
  }

  nsresult rv = NS_OK;
  bool outerActivateEvent = !!(aVisitor.mItemFlags & NS_OUTER_ACTIVATE_EVENT);
  bool originalCheckedValue =
    !!(aVisitor.mItemFlags & NS_ORIGINAL_CHECKED_VALUE);
  bool noContentDispatch = !!(aVisitor.mItemFlags & NS_NO_CONTENT_DISPATCH);
  uint8_t oldType = NS_CONTROL_TYPE(aVisitor.mItemFlags);

  
  
  
  
  
  
  
  if (aVisitor.mEventStatus != nsEventStatus_eConsumeNoDefault &&
      !IsSingleLineTextControl(true) &&
      mType != NS_FORM_INPUT_NUMBER) {
    WidgetMouseEvent* mouseEvent = aVisitor.mEvent->AsMouseEvent();
    if (mouseEvent && mouseEvent->IsLeftClickEvent() &&
        !ShouldPreventDOMActivateDispatch(aVisitor.mEvent->originalTarget)) {
      
      
      InternalUIEvent actEvent(aVisitor.mEvent->mFlags.mIsTrusted,
                               NS_UI_ACTIVATE);
      actEvent.detail = 1;

      nsCOMPtr<nsIPresShell> shell = aVisitor.mPresContext->GetPresShell();
      if (shell) {
        nsEventStatus status = nsEventStatus_eIgnore;
        mInInternalActivate = true;
        rv = shell->HandleDOMEventWithTarget(this, &actEvent, &status);
        mInInternalActivate = false;

        
        
        if (status == nsEventStatus_eConsumeNoDefault) {
          aVisitor.mEventStatus = status;
        }
      }
    }
  }

  if (outerActivateEvent) {
    switch(oldType) {
      case NS_FORM_INPUT_SUBMIT:
      case NS_FORM_INPUT_IMAGE:
        if (mForm) {
          
          
          
          
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
        if (previous) {
          FireEventForAccessibility(previous, aVisitor.mPresContext,
                                    NS_LITERAL_STRING("RadioStateChange"));
        }
      }
#endif
    }
  }

  if (NS_SUCCEEDED(rv)) {
    WidgetKeyboardEvent* keyEvent = aVisitor.mEvent->AsKeyboardEvent();
    if (mType ==  NS_FORM_INPUT_NUMBER &&
        keyEvent && keyEvent->message == NS_KEY_PRESS &&
        aVisitor.mEvent->mFlags.mIsTrusted &&
        (keyEvent->keyCode == NS_VK_UP || keyEvent->keyCode == NS_VK_DOWN) &&
        !(keyEvent->IsShift() || keyEvent->IsControl() ||
          keyEvent->IsAlt() || keyEvent->IsMeta() ||
          keyEvent->IsAltGraph() || keyEvent->IsFn() ||
          keyEvent->IsOS())) {
      
      
      
      
      
      
      
      
      
      
      
      if (!aVisitor.mEvent->mFlags.mDefaultPreventedByContent && IsMutable()) {
        StepNumberControlForUserEvent(keyEvent->keyCode == NS_VK_UP ? 1 : -1);
        aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
      }
    } else if (nsEventStatus_eIgnore == aVisitor.mEventStatus) {
      switch (aVisitor.mEvent->message) {

        case NS_FOCUS_CONTENT:
        {
          
          
          
          
          nsIFocusManager* fm = nsFocusManager::GetFocusManager();
          if (fm && IsSingleLineTextControl(false) &&
              !aVisitor.mEvent->AsFocusEvent()->fromRaise &&
              SelectTextFieldOnFocus()) {
            nsIDocument* document = GetComposedDoc();
            if (document) {
              uint32_t lastFocusMethod;
              fm->GetLastFocusMethod(document->GetWindow(), &lastFocusMethod);
              if (lastFocusMethod &
                  (nsIFocusManager::FLAG_BYKEY | nsIFocusManager::FLAG_BYMOVEFOCUS)) {
                nsRefPtr<nsPresContext> presContext =
                  GetPresContext(eForComposedDoc);
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
          
          
          WidgetKeyboardEvent* keyEvent = aVisitor.mEvent->AsKeyboardEvent();
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
              case NS_FORM_INPUT_COLOR:
              {
                WidgetMouseEvent event(aVisitor.mEvent->mFlags.mIsTrusted,
                                       NS_MOUSE_CLICK, nullptr,
                                       WidgetMouseEvent::eReal);
                event.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_KEYBOARD;
                nsEventStatus status = nsEventStatus_eIgnore;

                EventDispatcher::Dispatch(static_cast<nsIContent*>(this),
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
                nsRefPtr<HTMLInputElement> selectedRadioButton;
                container->GetNextRadioButton(name, isMovingBack, this,
                                              getter_AddRefs(selectedRadioButton));
                if (selectedRadioButton) {
                  rv = selectedRadioButton->Focus();
                  if (NS_SUCCEEDED(rv)) {
                    nsEventStatus status = nsEventStatus_eIgnore;
                    WidgetMouseEvent event(aVisitor.mEvent->mFlags.mIsTrusted,
                                           NS_MOUSE_CLICK, nullptr,
                                           WidgetMouseEvent::eReal);
                    event.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_KEYBOARD;
                    rv =
                      EventDispatcher::Dispatch(ToSupports(selectedRadioButton),
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
              keyEvent->keyCode == NS_VK_RETURN &&
               (IsSingleLineTextControl(false, mType) ||
                mType == NS_FORM_INPUT_NUMBER ||
                IsExperimentalMobileType(mType))) {
            FireChangeEventIfNeeded();
            rv = MaybeSubmitForm(aVisitor.mPresContext);
            NS_ENSURE_SUCCESS(rv, rv);
          }

          if (aVisitor.mEvent->message == NS_KEY_PRESS &&
              mType == NS_FORM_INPUT_RANGE && !keyEvent->IsAlt() &&
              !keyEvent->IsControl() && !keyEvent->IsMeta() &&
              (keyEvent->keyCode == NS_VK_LEFT ||
               keyEvent->keyCode == NS_VK_RIGHT ||
               keyEvent->keyCode == NS_VK_UP ||
               keyEvent->keyCode == NS_VK_DOWN ||
               keyEvent->keyCode == NS_VK_PAGE_UP ||
               keyEvent->keyCode == NS_VK_PAGE_DOWN ||
               keyEvent->keyCode == NS_VK_HOME ||
               keyEvent->keyCode == NS_VK_END)) {
            Decimal minimum = GetMinimum();
            Decimal maximum = GetMaximum();
            MOZ_ASSERT(minimum.isFinite() && maximum.isFinite());
            if (minimum < maximum) { 
              Decimal value = GetValueAsDecimal();
              Decimal step = GetStep();
              if (step == kStepAny) {
                step = GetDefaultStep();
              }
              MOZ_ASSERT(value.isFinite() && step.isFinite());
              Decimal newValue;
              switch (keyEvent->keyCode) {
                case  NS_VK_LEFT:
                  newValue = value + (GetComputedDirectionality() == eDir_RTL
                                        ? step : -step);
                  break;
                case  NS_VK_RIGHT:
                  newValue = value + (GetComputedDirectionality() == eDir_RTL
                                        ? -step : step);
                  break;
                case  NS_VK_UP:
                  
                  newValue = value + step;
                  break;
                case  NS_VK_DOWN:
                  
                  newValue = value - step;
                  break;
                case  NS_VK_HOME:
                  newValue = minimum;
                  break;
                case  NS_VK_END:
                  newValue = maximum;
                  break;
                case  NS_VK_PAGE_UP:
                  
                  
                  newValue = value + std::max(step, (maximum - minimum) / Decimal(10));
                  break;
                case  NS_VK_PAGE_DOWN:
                  newValue = value - std::max(step, (maximum - minimum) / Decimal(10));
                  break;
              }
              SetValueOfRangeForUserEvent(newValue);
              aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
            }
          }

        } break; 

        case NS_MOUSE_BUTTON_DOWN:
        case NS_MOUSE_BUTTON_UP:
        case NS_MOUSE_DOUBLECLICK:
        {
          
          
          WidgetMouseEvent* mouseEvent = aVisitor.mEvent->AsMouseEvent();
          if (mouseEvent->button == WidgetMouseEvent::eMiddleButton ||
              mouseEvent->button == WidgetMouseEvent::eRightButton) {
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
          if (mType == NS_FORM_INPUT_NUMBER &&
              aVisitor.mEvent->mFlags.mIsTrusted) {
            if (mouseEvent->button == WidgetMouseEvent::eLeftButton &&
                !(mouseEvent->IsShift() || mouseEvent->IsControl() ||
                  mouseEvent->IsAlt() || mouseEvent->IsMeta() ||
                  mouseEvent->IsAltGraph() || mouseEvent->IsFn() ||
                  mouseEvent->IsOS())) {
              nsNumberControlFrame* numberControlFrame =
                do_QueryFrame(GetPrimaryFrame());
              if (numberControlFrame) {
                if (aVisitor.mEvent->message == NS_MOUSE_BUTTON_DOWN && 
                    IsMutable()) {
                  switch (numberControlFrame->GetSpinButtonForPointerEvent(
                            aVisitor.mEvent->AsMouseEvent())) {
                  case nsNumberControlFrame::eSpinButtonUp:
                    StepNumberControlForUserEvent(1);
                    mNumberControlSpinnerSpinsUp = true;
                    StartNumberControlSpinnerSpin();
                    aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
                    break;
                  case nsNumberControlFrame::eSpinButtonDown:
                    StepNumberControlForUserEvent(-1);
                    mNumberControlSpinnerSpinsUp = false;
                    StartNumberControlSpinnerSpin();
                    aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
                    break;
                  }
                }
              }
            }
            if (aVisitor.mEventStatus != nsEventStatus_eConsumeNoDefault) {
              
              
              
              
              StopNumberControlSpinnerSpin();
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
            InternalFormEvent event(true,
              (mType == NS_FORM_INPUT_RESET) ? NS_FORM_RESET : NS_FORM_SUBMIT);
            event.originator      = this;
            nsEventStatus status  = nsEventStatus_eIgnore;

            nsCOMPtr<nsIPresShell> presShell =
              aVisitor.mPresContext->GetPresShell();

            
            
            
            
            
            if (presShell && (event.message != NS_FORM_SUBMIT ||
                              mForm->HasAttr(kNameSpaceID_None, nsGkAtoms::novalidate) ||
                              
                              
                              HasAttr(kNameSpaceID_None, nsGkAtoms::formnovalidate) ||
                              mForm->CheckValidFormSubmission())) {
              
              nsRefPtr<mozilla::dom::HTMLFormElement> form(mForm);
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

  if (NS_SUCCEEDED(rv) && mType == NS_FORM_INPUT_RANGE) {
    PostHandleEventForRangeThumb(aVisitor);
  }

  return MaybeInitPickers(aVisitor);
}

void
HTMLInputElement::PostHandleEventForRangeThumb(EventChainPostVisitor& aVisitor)
{
  MOZ_ASSERT(mType == NS_FORM_INPUT_RANGE);

  if (nsEventStatus_eConsumeNoDefault == aVisitor.mEventStatus ||
      !(aVisitor.mEvent->mClass == eMouseEventClass ||
        aVisitor.mEvent->mClass == eTouchEventClass ||
        aVisitor.mEvent->mClass == eKeyboardEventClass)) {
    return;
  }

  nsRangeFrame* rangeFrame = do_QueryFrame(GetPrimaryFrame());
  if (!rangeFrame && mIsDraggingRange) {
    CancelRangeThumbDrag();
    return;
  }

  switch (aVisitor.mEvent->message)
  {
    case NS_MOUSE_BUTTON_DOWN:
    case NS_TOUCH_START: {
      if (mIsDraggingRange) {
        break;
      }
      if (nsIPresShell::GetCapturingContent()) {
        break; 
      }
      WidgetInputEvent* inputEvent = aVisitor.mEvent->AsInputEvent();
      if (inputEvent->IsShift() || inputEvent->IsControl() ||
          inputEvent->IsAlt() || inputEvent->IsMeta() ||
          inputEvent->IsAltGraph() || inputEvent->IsFn() ||
          inputEvent->IsOS()) {
        break; 
      }
      if (aVisitor.mEvent->message == NS_MOUSE_BUTTON_DOWN) {
        if (aVisitor.mEvent->AsMouseEvent()->buttons ==
              WidgetMouseEvent::eLeftButtonFlag) {
          StartRangeThumbDrag(inputEvent);
        } else if (mIsDraggingRange) {
          CancelRangeThumbDrag();
        }
      } else {
        if (aVisitor.mEvent->AsTouchEvent()->touches.Length() == 1) {
          StartRangeThumbDrag(inputEvent);
        } else if (mIsDraggingRange) {
          CancelRangeThumbDrag();
        }
      }
      aVisitor.mEvent->mFlags.mMultipleActionsPrevented = true;
    } break;

    case NS_MOUSE_MOVE:
    case NS_TOUCH_MOVE:
      if (!mIsDraggingRange) {
        break;
      }
      if (nsIPresShell::GetCapturingContent() != this) {
        
        CancelRangeThumbDrag();
        break;
      }
      SetValueOfRangeForUserEvent(
        rangeFrame->GetValueAtEventPoint(aVisitor.mEvent->AsInputEvent()));
      aVisitor.mEvent->mFlags.mMultipleActionsPrevented = true;
      break;

    case NS_MOUSE_BUTTON_UP:
    case NS_TOUCH_END:
      if (!mIsDraggingRange) {
        break;
      }
      
      
      
      
      FinishRangeThumbDrag(aVisitor.mEvent->AsInputEvent());
      aVisitor.mEvent->mFlags.mMultipleActionsPrevented = true;
      break;

    case NS_KEY_PRESS:
      if (mIsDraggingRange &&
          aVisitor.mEvent->AsKeyboardEvent()->keyCode == NS_VK_ESCAPE) {
        CancelRangeThumbDrag();
      }
      break;

    case NS_TOUCH_CANCEL:
      if (mIsDraggingRange) {
        CancelRangeThumbDrag();
      }
      break;
  }
}

void
HTMLInputElement::MaybeLoadImage()
{
  
  
  nsAutoString uri;
  if (mType == NS_FORM_INPUT_IMAGE &&
      GetAttr(kNameSpaceID_None, nsGkAtoms::src, uri) &&
      (NS_FAILED(LoadImage(uri, false, true, eImageLoadType_Normal)) ||
       !LoadingEnabled())) {
    CancelImageRequests(true);
  }
}

nsresult
HTMLInputElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                             nsIContent* aBindingParent,
                             bool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLFormElementWithState::BindToTree(aDocument, aParent,
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
        NS_NewRunnableMethod(this, &HTMLInputElement::MaybeLoadImage));
    }
  }

  
  
  if (aDocument && !mForm && mType == NS_FORM_INPUT_RADIO) {
    AddedToRadioGroup();
  }

  
  SetDirectionIfAuto(HasDirAuto(), false);

  
  
  UpdateValueMissingValidityState();

  
  
  
  UpdateBarredFromConstraintValidation();

  
  UpdateState(false);

#ifdef EARLY_BETA_OR_EARLIER
  if (mType == NS_FORM_INPUT_PASSWORD) {
    Telemetry::Accumulate(Telemetry::PWMGR_PASSWORD_INPUT_IN_FORM, !!mForm);
  }
#endif

  return rv;
}

void
HTMLInputElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  
  
  
  
  
  if (!mForm && mType == NS_FORM_INPUT_RADIO) {
    WillRemoveFromRadioGroup();
  }

  nsImageLoadingContent::UnbindFromTree(aDeep, aNullParent);
  nsGenericHTMLFormElementWithState::UnbindFromTree(aDeep, aNullParent);

  
  
  UpdateValueMissingValidityState();
  
  UpdateBarredFromConstraintValidation();

  
  UpdateState(false);
}

void
HTMLInputElement::HandleTypeChange(uint8_t aNewType)
{
  if (mType == NS_FORM_INPUT_RANGE && mIsDraggingRange) {
    CancelRangeThumbDrag(false);
  }

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

  
  
  
  
  
  if (MayFireChangeOnBlur(mType) && !MayFireChangeOnBlur(oldType)) {
    GetValue(mFocusedValue);
  } else if (!IsSingleLineTextControl(mType, false) &&
             IsSingleLineTextControl(oldType, false)) {
    mFocusedValue.Truncate();
  }

  UpdateHasRange();

  
  UpdateAllValidityStates(false);
}

void
HTMLInputElement::SanitizeValue(nsAString& aValue)
{
  NS_ASSERTION(!mParserCreating, "The element parsing should be finished!");

  switch (mType) {
    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_SEARCH:
    case NS_FORM_INPUT_TEL:
    case NS_FORM_INPUT_PASSWORD:
      {
        char16_t crlf[] = { char16_t('\r'), char16_t('\n'), 0 };
        aValue.StripChars(crlf);
      }
      break;
    case NS_FORM_INPUT_EMAIL:
    case NS_FORM_INPUT_URL:
      {
        char16_t crlf[] = { char16_t('\r'), char16_t('\n'), 0 };
        aValue.StripChars(crlf);

        aValue = nsContentUtils::TrimWhitespace<nsContentUtils::IsHTMLWhitespace>(aValue);
      }
      break;
    case NS_FORM_INPUT_NUMBER:
      {
        Decimal value;
        bool ok = ConvertStringToNumber(aValue, value);
        if (!ok) {
          aValue.Truncate();
        }
      }
      break;
    case NS_FORM_INPUT_RANGE:
      {
        Decimal minimum = GetMinimum();
        Decimal maximum = GetMaximum();
        MOZ_ASSERT(minimum.isFinite() && maximum.isFinite(),
                   "type=range should have a default maximum/minimum");

        
        
        
        bool needSanitization = false;

        Decimal value;
        bool ok = ConvertStringToNumber(aValue, value);
        if (!ok) {
          needSanitization = true;
          
          value = maximum <= minimum ? minimum : minimum + (maximum - minimum)/Decimal(2);
        } else if (value < minimum || maximum < minimum) {
          needSanitization = true;
          value = minimum;
        } else if (value > maximum) {
          needSanitization = true;
          value = maximum;
        }

        Decimal step = GetStep();
        if (step != kStepAny) {
          Decimal stepBase = GetStepBase();
          
          
          
          Decimal deltaToStep = NS_floorModulo(value - stepBase, step);
          if (deltaToStep != Decimal(0)) {
            
            
            
            
            
            
            MOZ_ASSERT(deltaToStep > Decimal(0), "stepBelow/stepAbove will be wrong");
            Decimal stepBelow = value - deltaToStep;
            Decimal stepAbove = value - deltaToStep + step;
            Decimal halfStep = step / Decimal(2);
            bool stepAboveIsClosest = (stepAbove - value) <= halfStep;
            bool stepAboveInRange = stepAbove >= minimum &&
                                    stepAbove <= maximum;
            bool stepBelowInRange = stepBelow >= minimum &&
                                    stepBelow <= maximum;

            if ((stepAboveIsClosest || !stepBelowInRange) && stepAboveInRange) {
              needSanitization = true;
              value = stepAbove;
            } else if ((!stepAboveIsClosest || !stepAboveInRange) && stepBelowInRange) {
              needSanitization = true;
              value = stepBelow;
            }
          }
        }

        if (needSanitization) {
          char buf[32];
          DebugOnly<bool> ok = value.toString(buf, ArrayLength(buf));
          aValue.AssignASCII(buf);
          MOZ_ASSERT(ok, "buf not big enough");
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
    case NS_FORM_INPUT_COLOR:
      {
        if (IsValidSimpleColor(aValue)) {
          ToLowerCase(aValue);
        } else {
          
          aValue.AssignLiteral("#000000");
        }
      }
      break;
  }
}

bool HTMLInputElement::IsValidSimpleColor(const nsAString& aValue) const
{
  if (aValue.Length() != 7 || aValue.First() != '#') {
    return false;
  }

  for (int i = 1; i < 7; ++i) {
    if (!nsCRT::IsAsciiDigit(aValue[i]) &&
        !(aValue[i] >= 'a' && aValue[i] <= 'f') &&
        !(aValue[i] >= 'A' && aValue[i] <= 'F')) {
      return false;
    }
  }
  return true;
}

bool
HTMLInputElement::IsValidDate(const nsAString& aValue) const
{
  uint32_t year, month, day;
  return GetValueAsDate(aValue, &year, &month, &day);
}

bool
HTMLInputElement::GetValueAsDate(const nsAString& aValue,
                                 uint32_t* aYear,
                                 uint32_t* aMonth,
                                 uint32_t* aDay) const
{









  if (aValue.Length() < 10) {
    return false;
  }

  uint32_t endOfYearOffset = aValue.Length() - 6;
  
  if (aValue[endOfYearOffset]     != '-' ||
      aValue[endOfYearOffset + 3] != '-') {
    return false;
  }

  if (!DigitSubStringToNumber(aValue, 0, endOfYearOffset, aYear) ||
      *aYear < 1) {
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
HTMLInputElement::NumberOfDaysInMonth(uint32_t aMonth, uint32_t aYear) const
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
HTMLInputElement::DigitSubStringToNumber(const nsAString& aStr,
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
HTMLInputElement::IsValidTime(const nsAString& aValue) const
{
  return ParseTime(aValue, nullptr);
}

 bool
HTMLInputElement::ParseTime(const nsAString& aValue, uint32_t* aResult)
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
HTMLInputElement::ParseAttribute(int32_t aNamespaceID,
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
        if ((IsExperimentalMobileType(newType) &&
             !Preferences::GetBool("dom.experimental_forms", false)) ||
            (newType == NS_FORM_INPUT_NUMBER &&
             !Preferences::GetBool("dom.forms.number", false)) ||
            (newType == NS_FORM_INPUT_COLOR &&
             !Preferences::GetBool("dom.forms.color", false))) {
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
      aResult.ParseAtomArray(aValue);
      return true;
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

void
HTMLInputElement::MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                        nsRuleData* aData)
{
  const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::type);
  if (value && value->Type() == nsAttrValue::eEnum &&
      value->GetEnumValue() == NS_FORM_INPUT_IMAGE) {
    nsGenericHTMLFormElementWithState::MapImageBorderAttributeInto(aAttributes, aData);
    nsGenericHTMLFormElementWithState::MapImageMarginAttributeInto(aAttributes, aData);
    nsGenericHTMLFormElementWithState::MapImageSizeAttributesInto(aAttributes, aData);
    
    nsGenericHTMLFormElementWithState::MapImageAlignAttributeInto(aAttributes, aData);
  }

  nsGenericHTMLFormElementWithState::MapCommonAttributesInto(aAttributes, aData);
}

nsChangeHint
HTMLInputElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                         int32_t aModType) const
{
  nsChangeHint retval =
    nsGenericHTMLFormElementWithState::GetAttributeChangeHint(aAttribute, aModType);
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
HTMLInputElement::IsAttributeMapped(const nsIAtom* aAttribute) const
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
HTMLInputElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}




nsIControllers*
HTMLInputElement::GetControllers(ErrorResult& aRv)
{
  
  if (IsSingleLineTextControl(false))
  {
    if (!mControllers)
    {
      nsresult rv;
      mControllers = do_CreateInstance(kXULControllersCID, &rv);
      if (NS_FAILED(rv)) {
        aRv.Throw(rv);
        return nullptr;
      }

      nsCOMPtr<nsIController>
        controller(do_CreateInstance("@mozilla.org/editor/editorcontroller;1",
                                     &rv));
      if (NS_FAILED(rv)) {
        aRv.Throw(rv);
        return nullptr;
      }

      mControllers->AppendController(controller);

      controller = do_CreateInstance("@mozilla.org/editor/editingcontroller;1",
                                     &rv);
      if (NS_FAILED(rv)) {
        aRv.Throw(rv);
        return nullptr;
      }

      mControllers->AppendController(controller);
    }
  }

  return mControllers;
}

NS_IMETHODIMP
HTMLInputElement::GetControllers(nsIControllers** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  ErrorResult rv;
  nsRefPtr<nsIControllers> controller = GetControllers(rv);
  controller.forget(aResult);
  return rv.ErrorCode();
}

int32_t
HTMLInputElement::GetTextLength(ErrorResult& aRv)
{
  nsAutoString val;
  GetValue(val);
  return val.Length();
}

NS_IMETHODIMP
HTMLInputElement::GetTextLength(int32_t* aTextLength)
{
  ErrorResult rv;
  *aTextLength = GetTextLength(rv);
  return rv.ErrorCode();
}

void
HTMLInputElement::SetSelectionRange(int32_t aSelectionStart,
                                    int32_t aSelectionEnd,
                                    const Optional<nsAString>& aDirection,
                                    ErrorResult& aRv)
{
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(true);
  nsITextControlFrame* textControlFrame = do_QueryFrame(formControlFrame);
  if (textControlFrame) {
    
    
    
    nsITextControlFrame::SelectionDirection dir = nsITextControlFrame::eForward;
    if (aDirection.WasPassed() && aDirection.Value().EqualsLiteral("backward")) {
      dir = nsITextControlFrame::eBackward;
    }

    aRv = textControlFrame->SetSelectionRange(aSelectionStart, aSelectionEnd, dir);
    if (!aRv.Failed()) {
      aRv = textControlFrame->ScrollSelectionIntoView();
      nsRefPtr<AsyncEventDispatcher> asyncDispatcher =
        new AsyncEventDispatcher(this, NS_LITERAL_STRING("select"),
                                 true, false);
      asyncDispatcher->PostDOMEvent();
    }
  }
}

NS_IMETHODIMP
HTMLInputElement::SetSelectionRange(int32_t aSelectionStart,
                                    int32_t aSelectionEnd,
                                    const nsAString& aDirection)
{
  ErrorResult rv;
  Optional<nsAString> direction;
  direction = &aDirection;

  SetSelectionRange(aSelectionStart, aSelectionEnd, direction, rv);
  return rv.ErrorCode();
}

void
HTMLInputElement::SetRangeText(const nsAString& aReplacement, ErrorResult& aRv)
{
  if (!SupportsSetRangeText()) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return;
  }

  int32_t start, end;
  aRv = GetSelectionRange(&start, &end);
  if (aRv.Failed()) {
    nsTextEditorState* state = GetEditorState();
    if (state && state->IsSelectionCached()) {
      start = state->GetSelectionProperties().mStart;
      end = state->GetSelectionProperties().mEnd;
      aRv = NS_OK;
    }
  }

  SetRangeText(aReplacement, start, end, mozilla::dom::SelectionMode::Preserve,
               aRv, start, end);
}

void
HTMLInputElement::SetRangeText(const nsAString& aReplacement, uint32_t aStart,
                               uint32_t aEnd, const SelectionMode& aSelectMode,
                               ErrorResult& aRv, int32_t aSelectionStart,
                               int32_t aSelectionEnd)
{
  if (!SupportsSetRangeText()) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return;
  }

  if (aStart > aEnd) {
    aRv.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  nsAutoString value;
  GetValueInternal(value);
  uint32_t inputValueLength = value.Length();

  if (aStart > inputValueLength) {
    aStart = inputValueLength;
  }

  if (aEnd > inputValueLength) {
    aEnd = inputValueLength;
  }

  if (aSelectionStart == -1 && aSelectionEnd == -1) {
    aRv = GetSelectionRange(&aSelectionStart, &aSelectionEnd);
    if (aRv.Failed()) {
      nsTextEditorState* state = GetEditorState();
      if (state && state->IsSelectionCached()) {
        aSelectionStart = state->GetSelectionProperties().mStart;
        aSelectionEnd = state->GetSelectionProperties().mEnd;
        aRv = NS_OK;
      }
    }
  }

  if (aStart <= aEnd) {
    value.Replace(aStart, aEnd - aStart, aReplacement);
    nsresult rv = SetValueInternal(value, false, false);
    if (NS_FAILED(rv)) {
      aRv.Throw(rv);
      return;
    }
  }

  uint32_t newEnd = aStart + aReplacement.Length();
  int32_t delta =  aReplacement.Length() - (aEnd - aStart);

  switch (aSelectMode) {
    case mozilla::dom::SelectionMode::Select:
    {
      aSelectionStart = aStart;
      aSelectionEnd = newEnd;
    }
    break;
    case mozilla::dom::SelectionMode::Start:
    {
      aSelectionStart = aSelectionEnd = aStart;
    }
    break;
    case mozilla::dom::SelectionMode::End:
    {
      aSelectionStart = aSelectionEnd = newEnd;
    }
    break;
    case mozilla::dom::SelectionMode::Preserve:
    {
      if ((uint32_t)aSelectionStart > aEnd) {
        aSelectionStart += delta;
      } else if ((uint32_t)aSelectionStart > aStart) {
        aSelectionStart = aStart;
      }

      if ((uint32_t)aSelectionEnd > aEnd) {
        aSelectionEnd += delta;
      } else if ((uint32_t)aSelectionEnd > aStart) {
        aSelectionEnd = newEnd;
      }
    }
    break;
    default:
      MOZ_CRASH("Unknown mode!");
  }

  Optional<nsAString> direction;
  SetSelectionRange(aSelectionStart, aSelectionEnd, direction, aRv);
}

int32_t
HTMLInputElement::GetSelectionStart(ErrorResult& aRv)
{
  int32_t selEnd, selStart;
  aRv = GetSelectionRange(&selStart, &selEnd);

  if (aRv.Failed()) {
    nsTextEditorState* state = GetEditorState();
    if (state && state->IsSelectionCached()) {
      aRv = NS_OK;
      return state->GetSelectionProperties().mStart;
    }
  }

  return selStart;
}

NS_IMETHODIMP
HTMLInputElement::GetSelectionStart(int32_t* aSelectionStart)
{
  NS_ENSURE_ARG_POINTER(aSelectionStart);

  ErrorResult rv;
  *aSelectionStart = GetSelectionStart(rv);
  return rv.ErrorCode();
}

void
HTMLInputElement::SetSelectionStart(int32_t aSelectionStart, ErrorResult& aRv)
{
  nsTextEditorState* state = GetEditorState();
  if (state && state->IsSelectionCached()) {
    state->GetSelectionProperties().mStart = aSelectionStart;
    return;
  }

  nsAutoString direction;
  aRv = GetSelectionDirection(direction);
  if (aRv.Failed()) {
    return;
  }

  int32_t start, end;
  aRv = GetSelectionRange(&start, &end);
  if (aRv.Failed()) {
    return;
  }

  start = aSelectionStart;
  if (end < start) {
    end = start;
  }

  aRv = SetSelectionRange(start, end, direction);
}

NS_IMETHODIMP
HTMLInputElement::SetSelectionStart(int32_t aSelectionStart)
{
  ErrorResult rv;
  SetSelectionStart(aSelectionStart, rv);
  return rv.ErrorCode();
}

int32_t
HTMLInputElement::GetSelectionEnd(ErrorResult& aRv)
{
  int32_t selStart, selEnd;
  aRv = GetSelectionRange(&selStart, &selEnd);

  if (aRv.Failed()) {
    nsTextEditorState* state = GetEditorState();
    if (state && state->IsSelectionCached()) {
      aRv = NS_OK;
      return state->GetSelectionProperties().mEnd;
    }
  }

  return selEnd;
}

NS_IMETHODIMP
HTMLInputElement::GetSelectionEnd(int32_t* aSelectionEnd)
{
  NS_ENSURE_ARG_POINTER(aSelectionEnd);

  ErrorResult rv;
  *aSelectionEnd = GetSelectionEnd(rv);
  return rv.ErrorCode();
}

void
HTMLInputElement::SetSelectionEnd(int32_t aSelectionEnd, ErrorResult& aRv)
{
  nsTextEditorState* state = GetEditorState();
  if (state && state->IsSelectionCached()) {
    state->GetSelectionProperties().mEnd = aSelectionEnd;
    return;
  }

  nsAutoString direction;
  aRv = GetSelectionDirection(direction);
  if (aRv.Failed()) {
    return;
  }

  int32_t start, end;
  aRv = GetSelectionRange(&start, &end);
  if (aRv.Failed()) {
    return;
  }

  end = aSelectionEnd;
  if (start > end) {
    start = end;
  }

  aRv = SetSelectionRange(start, end, direction);
}

NS_IMETHODIMP
HTMLInputElement::SetSelectionEnd(int32_t aSelectionEnd)
{
  ErrorResult rv;
  SetSelectionEnd(aSelectionEnd, rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP
HTMLInputElement::GetFiles(nsIDOMFileList** aFileList)
{
  nsRefPtr<FileList> list = GetFiles();
  list.forget(aFileList);
  return NS_OK;
}

nsresult
HTMLInputElement::GetSelectionRange(int32_t* aSelectionStart,
                                    int32_t* aSelectionEnd)
{
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(true);
  nsITextControlFrame* textControlFrame = do_QueryFrame(formControlFrame);
  if (textControlFrame) {
    return textControlFrame->GetSelectionRange(aSelectionStart, aSelectionEnd);
  }

  return NS_ERROR_FAILURE;
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

void
HTMLInputElement::GetSelectionDirection(nsAString& aDirection, ErrorResult& aRv)
{
  nsresult rv = NS_ERROR_FAILURE;
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(true);
  nsITextControlFrame* textControlFrame = do_QueryFrame(formControlFrame);
  if (textControlFrame) {
    nsITextControlFrame::SelectionDirection dir;
    rv = textControlFrame->GetSelectionRange(nullptr, nullptr, &dir);
    if (NS_SUCCEEDED(rv)) {
      DirectionToName(dir, aDirection);
    }
  }

  if (NS_FAILED(rv)) {
    nsTextEditorState* state = GetEditorState();
    if (state && state->IsSelectionCached()) {
      DirectionToName(state->GetSelectionProperties().mDirection, aDirection);
      return;
    }
  }

  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
  }
}

NS_IMETHODIMP
HTMLInputElement::GetSelectionDirection(nsAString& aDirection)
{
  ErrorResult rv;
  GetSelectionDirection(aDirection, rv);
  return rv.ErrorCode();
}

void
HTMLInputElement::SetSelectionDirection(const nsAString& aDirection, ErrorResult& aRv)
{
  nsTextEditorState* state = GetEditorState();
  if (state && state->IsSelectionCached()) {
    nsITextControlFrame::SelectionDirection dir = nsITextControlFrame::eNone;
    if (aDirection.EqualsLiteral("forward")) {
      dir = nsITextControlFrame::eForward;
    } else if (aDirection.EqualsLiteral("backward")) {
      dir = nsITextControlFrame::eBackward;
    }
    state->GetSelectionProperties().mDirection = dir;
    return;
  }

  int32_t start, end;
  aRv = GetSelectionRange(&start, &end);
  if (!aRv.Failed()) {
    aRv = SetSelectionRange(start, end, aDirection);
  }
}

NS_IMETHODIMP
HTMLInputElement::SetSelectionDirection(const nsAString& aDirection)
{
  ErrorResult rv;
  SetSelectionDirection(aDirection, rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP
HTMLInputElement::GetPhonetic(nsAString& aPhonetic)
{
  aPhonetic.Truncate();
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(true);
  nsITextControlFrame* textControlFrame = do_QueryFrame(formControlFrame);
  if (textControlFrame) {
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
  nsCOMPtr<mozilla::dom::Element> element = do_QueryInterface(aTarget);
  if (NS_SUCCEEDED(EventDispatcher::CreateEvent(element, aPresContext, nullptr,
                                                NS_LITERAL_STRING("Events"),
                                                getter_AddRefs(event)))) {
    event->InitEvent(aEventType, true, true);
    event->SetTrusted(true);

    EventDispatcher::DispatchDOMEvent(aTarget, nullptr, event, aPresContext,
                                      nullptr);
  }

  return NS_OK;
}
#endif

nsresult
HTMLInputElement::SetDefaultValueAsValue()
{
  NS_ASSERTION(GetValueMode() == VALUE_MODE_VALUE,
               "GetValueMode() should return VALUE_MODE_VALUE!");

  
  
  nsAutoString resetVal;
  GetDefaultValue(resetVal);

  
  return SetValueInternal(resetVal, false, false);
}

void
HTMLInputElement::SetDirectionIfAuto(bool aAuto, bool aNotify)
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
HTMLInputElement::Reset()
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
HTMLInputElement::SubmitNamesValues(nsFormSubmission* aFormSubmission)
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
    

    const nsTArray<nsRefPtr<File>>& files = GetFilesInternal();

    for (uint32_t i = 0; i < files.Length(); ++i) {
      aFormSubmission->AddNameFilePair(name, files[i]);
    }

    if (files.IsEmpty()) {
      
      
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
HTMLInputElement::SaveState()
{
  nsRefPtr<HTMLInputElementState> inputState;
  switch (GetValueMode()) {
    case VALUE_MODE_DEFAULT_ON:
      if (mCheckedChanged) {
        inputState = new HTMLInputElementState();
        inputState->SetChecked(mChecked);
      }
      break;
    case VALUE_MODE_FILENAME:
      if (!mFiles.IsEmpty()) {
        inputState = new HTMLInputElementState();
        inputState->SetFileImpls(mFiles);
      }
      break;
    case VALUE_MODE_VALUE:
    case VALUE_MODE_DEFAULT:
      
      
      if ((GetValueMode() == VALUE_MODE_DEFAULT &&
           mType != NS_FORM_INPUT_HIDDEN) ||
          mType == NS_FORM_INPUT_PASSWORD || !mValueChanged) {
        break;
      }

      inputState = new HTMLInputElementState();
      nsAutoString value;
      GetValue(value);
      DebugOnly<nsresult> rv =
        nsLinebreakConverter::ConvertStringLineBreaks(
             value,
             nsLinebreakConverter::eLinebreakPlatform,
             nsLinebreakConverter::eLinebreakContent);
      NS_ASSERTION(NS_SUCCEEDED(rv), "Converting linebreaks failed!");
      inputState->SetValue(value);
      break;
  }

  if (inputState) {
    nsPresState* state = GetPrimaryPresState();
    if (state) {
      state->SetStateProperty(inputState);
    }
  }

  if (mDisabledChanged) {
    nsPresState* state = GetPrimaryPresState();
    if (state) {
      
      
      state->SetDisabled(HasAttr(kNameSpaceID_None, nsGkAtoms::disabled));
    }
  }

  return NS_OK;
}

void
HTMLInputElement::DoneCreatingElement()
{
  mParserCreating = false;

  
  
  
  
  bool restoredCheckedState =
    !mInhibitRestoration && NS_SUCCEEDED(GenerateStateKey()) && RestoreFormControlState();

  
  
  
  
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

EventStates
HTMLInputElement::IntrinsicState() const
{
  
  

  EventStates state = nsGenericHTMLFormElementWithState::IntrinsicState();
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

void
HTMLInputElement::AddStates(EventStates aStates)
{
  if (mType == NS_FORM_INPUT_TEXT) {
    EventStates focusStates(aStates & (NS_EVENT_STATE_FOCUS |
                                       NS_EVENT_STATE_FOCUSRING));
    if (!focusStates.IsEmpty()) {
      HTMLInputElement* ownerNumberControl = GetOwnerNumberControl();
      if (ownerNumberControl) {
        ownerNumberControl->AddStates(focusStates);
      }
    }
  }
  nsGenericHTMLFormElementWithState::AddStates(aStates);                          
}

void
HTMLInputElement::RemoveStates(EventStates aStates)
{
  if (mType == NS_FORM_INPUT_TEXT) {
    EventStates focusStates(aStates & (NS_EVENT_STATE_FOCUS |
                                       NS_EVENT_STATE_FOCUSRING));
    if (!focusStates.IsEmpty()) {
      HTMLInputElement* ownerNumberControl = GetOwnerNumberControl();
      if (ownerNumberControl) {
        ownerNumberControl->RemoveStates(focusStates);
      }
    }
  }
  nsGenericHTMLFormElementWithState::RemoveStates(aStates);
}

bool
HTMLInputElement::RestoreState(nsPresState* aState)
{
  bool restoredCheckedState = false;

  nsCOMPtr<HTMLInputElementState> inputState
    (do_QueryInterface(aState->GetStateProperty()));

  if (inputState) {
    switch (GetValueMode()) {
      case VALUE_MODE_DEFAULT_ON:
        if (inputState->IsCheckedSet()) {
          restoredCheckedState = true;
          DoSetChecked(inputState->GetChecked(), true, true);
        }
        break;
      case VALUE_MODE_FILENAME:
        {
          const nsTArray<nsRefPtr<FileImpl>>& fileImpls = inputState->GetFileImpls();

          nsCOMPtr<nsIGlobalObject> global = OwnerDoc()->GetScopeObject();
          MOZ_ASSERT(global);

          nsTArray<nsRefPtr<File>> files;
          for (uint32_t i = 0, len = fileImpls.Length(); i < len; ++i) {
            nsRefPtr<File> file = new File(global, fileImpls[i]);
            files.AppendElement(file);
          }

          SetFiles(files, true);
        }
        break;
      case VALUE_MODE_VALUE:
      case VALUE_MODE_DEFAULT:
        if (GetValueMode() == VALUE_MODE_DEFAULT &&
            mType != NS_FORM_INPUT_HIDDEN) {
          break;
        }

        
        
        
        SetValueInternal(inputState->GetValue(), false, true);
        break;
    }
  }

  if (aState->IsDisabledSet()) {
    SetDisabled(aState->GetDisabled());
  }

  return restoredCheckedState;
}

bool
HTMLInputElement::AllowDrop()
{
  

  return mType != NS_FORM_INPUT_FILE;
}





void
HTMLInputElement::AddedToRadioGroup()
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
HTMLInputElement::WillRemoveFromRadioGroup()
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
HTMLInputElement::IsHTMLFocusable(bool aWithMouse, bool* aIsFocusable, int32_t* aTabIndex)
{
  if (nsGenericHTMLFormElementWithState::IsHTMLFocusable(aWithMouse, aIsFocusable,
      aTabIndex))
  {
    return true;
  }

  if (IsDisabled()) {
    *aIsFocusable = false;
    return true;
  }

  if (IsSingleLineTextControl(false) ||
      mType == NS_FORM_INPUT_RANGE) {
    *aIsFocusable = true;
    return false;
  }

#ifdef XP_MACOSX
  const bool defaultFocusable = !aWithMouse || nsFocusManager::sMouseFocusesFormControl;
#else
  const bool defaultFocusable = true;
#endif

  if (mType == NS_FORM_INPUT_FILE ||
      mType == NS_FORM_INPUT_NUMBER) {
    if (aTabIndex) {
      
      *aTabIndex = -1;
    }
    if (mType == NS_FORM_INPUT_NUMBER) {
      *aIsFocusable = true;
    } else {
      *aIsFocusable = defaultFocusable;
    }
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

  if (container->GetCurrentRadioButton(name)) {
    *aTabIndex = -1;
  }
  *aIsFocusable = defaultFocusable;
  return false;
}

nsresult
HTMLInputElement::VisitGroup(nsIRadioVisitor* aVisitor, bool aFlushContent)
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

HTMLInputElement::ValueModeType
HTMLInputElement::GetValueMode() const
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
    case NS_FORM_INPUT_RANGE:
    case NS_FORM_INPUT_DATE:
    case NS_FORM_INPUT_TIME:
    case NS_FORM_INPUT_COLOR:
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
HTMLInputElement::IsMutable() const
{
  return !IsDisabled() &&
         !(DoesReadOnlyApply() &&
           HasAttr(kNameSpaceID_None, nsGkAtoms::readonly));
}

bool
HTMLInputElement::DoesReadOnlyApply() const
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
    case NS_FORM_INPUT_RANGE:
    case NS_FORM_INPUT_COLOR:
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
HTMLInputElement::DoesRequiredApply() const
{
  switch (mType)
  {
    case NS_FORM_INPUT_HIDDEN:
    case NS_FORM_INPUT_BUTTON:
    case NS_FORM_INPUT_IMAGE:
    case NS_FORM_INPUT_RESET:
    case NS_FORM_INPUT_SUBMIT:
    case NS_FORM_INPUT_RANGE:
    case NS_FORM_INPUT_COLOR:
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
HTMLInputElement::PlaceholderApplies() const
{
  if (mType == NS_FORM_INPUT_DATE ||
      mType == NS_FORM_INPUT_TIME) {
    return false;
  }

  return IsSingleLineTextControl(false);
}

bool
HTMLInputElement::DoesPatternApply() const
{
  
  if (IsExperimentalMobileType(mType)) {
    return false;
  }

  return IsSingleLineTextControl(false);
}

bool
HTMLInputElement::DoesMinMaxApply() const
{
  switch (mType)
  {
    case NS_FORM_INPUT_NUMBER:
    case NS_FORM_INPUT_DATE:
    case NS_FORM_INPUT_TIME:
    case NS_FORM_INPUT_RANGE:
    
    
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
    case NS_FORM_INPUT_COLOR:
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

bool
HTMLInputElement::DoesAutocompleteApply() const
{
  switch (mType)
  {
    case NS_FORM_INPUT_HIDDEN:
    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_SEARCH:
    case NS_FORM_INPUT_URL:
    case NS_FORM_INPUT_TEL:
    case NS_FORM_INPUT_EMAIL:
    case NS_FORM_INPUT_PASSWORD:
    case NS_FORM_INPUT_DATE:
    case NS_FORM_INPUT_TIME:
    case NS_FORM_INPUT_NUMBER:
    case NS_FORM_INPUT_RANGE:
    case NS_FORM_INPUT_COLOR:
      return true;
#ifdef DEBUG
    case NS_FORM_INPUT_RESET:
    case NS_FORM_INPUT_SUBMIT:
    case NS_FORM_INPUT_IMAGE:
    case NS_FORM_INPUT_BUTTON:
    case NS_FORM_INPUT_RADIO:
    case NS_FORM_INPUT_CHECKBOX:
    case NS_FORM_INPUT_FILE:
      return false;
    default:
      NS_NOTYETIMPLEMENTED("Unexpected input type in DoesAutocompleteApply()");
      return false;
#else 
    default:
      return false;
#endif 
  }
}

Decimal
HTMLInputElement::GetStep() const
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

  Decimal step = StringToDecimal(stepStr);
  if (!step.isFinite() || step <= Decimal(0)) {
    step = GetDefaultStep();
  }

  return step * GetStepScaleFactor();
}



NS_IMETHODIMP
HTMLInputElement::SetCustomValidity(const nsAString& aError)
{
  nsIConstraintValidation::SetCustomValidity(aError);

  UpdateState(true);

  return NS_OK;
}

bool
HTMLInputElement::IsTooLong()
{
  if (!MaxLengthApplies() ||
      !HasAttr(kNameSpaceID_None, nsGkAtoms::maxlength) ||
      !mValueChanged) {
    return false;
  }

  int32_t maxLength = MaxLength();

  
  if (maxLength == -1) {
    return false;
  }

  int32_t textLength = -1;
  GetTextLength(&textLength);

  return textLength > maxLength;
}

bool
HTMLInputElement::IsValueMissing() const
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
      const nsTArray<nsRefPtr<File>>& files = GetFilesInternal();
      return files.IsEmpty();
    }
    case VALUE_MODE_DEFAULT_ON:
      
      
      return !mChecked;
    case VALUE_MODE_DEFAULT:
    default:
      return false;
  }
}

bool
HTMLInputElement::HasTypeMismatch() const
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
HTMLInputElement::HasPatternMismatch() const
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
HTMLInputElement::IsRangeOverflow() const
{
  if (!DoesMinMaxApply()) {
    return false;
  }

  Decimal maximum = GetMaximum();
  if (maximum.isNaN()) {
    return false;
  }

  Decimal value = GetValueAsDecimal();
  if (value.isNaN()) {
    return false;
  }

  return value > maximum;
}

bool
HTMLInputElement::IsRangeUnderflow() const
{
  if (!DoesMinMaxApply()) {
    return false;
  }

  Decimal minimum = GetMinimum();
  if (minimum.isNaN()) {
    return false;
  }

  Decimal value = GetValueAsDecimal();
  if (value.isNaN()) {
    return false;
  }

  return value < minimum;
}

bool
HTMLInputElement::HasStepMismatch(bool aUseZeroIfValueNaN) const
{
  if (!DoesStepApply()) {
    return false;
  }

  Decimal value = GetValueAsDecimal();
  if (value.isNaN()) {
    if (aUseZeroIfValueNaN) {
      value = Decimal(0);
    } else {
      
      return false;
    }
  }

  Decimal step = GetStep();
  if (step == kStepAny) {
    return false;
  }

  
  return NS_floorModulo(value - GetStepBase(), step) != Decimal(0);
}

















static bool PunycodeEncodeEmailAddress(const nsAString& aEmail,
                                       nsAutoCString& aEncodedEmail,
                                       uint32_t* aIndexOfAt)
{
  nsAutoCString value = NS_ConvertUTF16toUTF8(aEmail);
  *aIndexOfAt = (uint32_t)value.FindChar('@');

  if (*aIndexOfAt == (uint32_t)kNotFound ||
      *aIndexOfAt == value.Length() - 1) {
    aEncodedEmail = value;
    return true;
  }

  nsCOMPtr<nsIIDNService> idnSrv = do_GetService(NS_IDNSERVICE_CONTRACTID);
  if (!idnSrv) {
    NS_ERROR("nsIIDNService isn't present!");
    return false;
  }

  uint32_t indexOfDomain = *aIndexOfAt + 1;

  const nsDependentCSubstring domain = Substring(value, indexOfDomain);
  bool ace;
  if (NS_SUCCEEDED(idnSrv->IsACE(domain, &ace)) && !ace) {
    nsAutoCString domainACE;
    if (NS_FAILED(idnSrv->ConvertUTF8toACE(domain, domainACE))) {
      return false;
    }
    value.Replace(indexOfDomain, domain.Length(), domainACE);
  }

  aEncodedEmail = value;
  return true;
}

bool
HTMLInputElement::HasBadInput() const
{
  if (mType == NS_FORM_INPUT_NUMBER) {
    nsAutoString value;
    GetValueInternal(value);
    if (!value.IsEmpty()) {
      
      
      NS_ASSERTION(!GetValueAsDecimal().isNaN(), "Should have sanitized");
      return false;
    }
    nsNumberControlFrame* numberControlFrame =
      do_QueryFrame(GetPrimaryFrame());
    if (numberControlFrame &&
        !numberControlFrame->AnonTextControlIsEmpty()) {
      
      return true;
    }
    return false;
  }
  if (mType == NS_FORM_INPUT_EMAIL) {
    
    
    
    
    nsAutoString value;
    nsAutoCString unused;
    uint32_t unused2;
    NS_ENSURE_SUCCESS(GetValueInternal(value), false);
    HTMLSplitOnSpacesTokenizer tokenizer(value, ',');
    while (tokenizer.hasMoreTokens()) {
      if (!PunycodeEncodeEmailAddress(tokenizer.nextToken(), unused, &unused2)) {
        return true;
      }
    }
    return false;
  }
  return false;
}

void
HTMLInputElement::UpdateTooLongValidityState()
{
  
#if 0
  SetValidityState(VALIDITY_STATE_TOO_LONG, IsTooLong());
#endif
}

void
HTMLInputElement::UpdateValueMissingValidityStateForRadio(bool aIgnoreSelf)
{
  bool notify = !mParserCreating;
  nsCOMPtr<nsIDOMHTMLInputElement> selection = GetSelectedRadioButton();

  aIgnoreSelf = aIgnoreSelf || !IsMutable();

  
  
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

  valueMissing = required && !selected;

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
HTMLInputElement::UpdateValueMissingValidityState()
{
  if (mType == NS_FORM_INPUT_RADIO) {
    UpdateValueMissingValidityStateForRadio(false);
    return;
  }

  SetValidityState(VALIDITY_STATE_VALUE_MISSING, IsValueMissing());
}

void
HTMLInputElement::UpdateTypeMismatchValidityState()
{
    SetValidityState(VALIDITY_STATE_TYPE_MISMATCH, HasTypeMismatch());
}

void
HTMLInputElement::UpdatePatternMismatchValidityState()
{
  SetValidityState(VALIDITY_STATE_PATTERN_MISMATCH, HasPatternMismatch());
}

void
HTMLInputElement::UpdateRangeOverflowValidityState()
{
  SetValidityState(VALIDITY_STATE_RANGE_OVERFLOW, IsRangeOverflow());
}

void
HTMLInputElement::UpdateRangeUnderflowValidityState()
{
  SetValidityState(VALIDITY_STATE_RANGE_UNDERFLOW, IsRangeUnderflow());
}

void
HTMLInputElement::UpdateStepMismatchValidityState()
{
  SetValidityState(VALIDITY_STATE_STEP_MISMATCH, HasStepMismatch());
}

void
HTMLInputElement::UpdateBadInputValidityState()
{
  SetValidityState(VALIDITY_STATE_BAD_INPUT, HasBadInput());
}

void
HTMLInputElement::UpdateAllValidityStates(bool aNotify)
{
  bool validBefore = IsValid();
  UpdateTooLongValidityState();
  UpdateValueMissingValidityState();
  UpdateTypeMismatchValidityState();
  UpdatePatternMismatchValidityState();
  UpdateRangeOverflowValidityState();
  UpdateRangeUnderflowValidityState();
  UpdateStepMismatchValidityState();
  UpdateBadInputValidityState();

  if (validBefore != IsValid()) {
    UpdateState(aNotify);
  }
}

void
HTMLInputElement::UpdateBarredFromConstraintValidation()
{
  SetBarredFromConstraintValidation(mType == NS_FORM_INPUT_HIDDEN ||
                                    mType == NS_FORM_INPUT_BUTTON ||
                                    mType == NS_FORM_INPUT_RESET ||
                                    HasAttr(kNameSpaceID_None, nsGkAtoms::readonly) ||
                                    IsDisabled());
}

void
HTMLInputElement::GetValidationMessage(nsAString& aValidationMessage,
                                       ErrorResult& aRv)
{
  aRv = GetValidationMessage(aValidationMessage);
}

nsresult
HTMLInputElement::GetValidationMessage(nsAString& aValidationMessage,
                                       ValidityStateType aType)
{
  nsresult rv = NS_OK;

  switch (aType)
  {
    case VALIDITY_STATE_TOO_LONG:
    {
      nsXPIDLString message;
      int32_t maxLength = MaxLength();
      int32_t textLength = -1;
      nsAutoString strMaxLength;
      nsAutoString strTextLength;

      GetTextLength(&textLength);

      strMaxLength.AppendInt(maxLength);
      strTextLength.AppendInt(textLength);

      const char16_t* params[] = { strMaxLength.get(), strTextLength.get() };
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
          key.AssignLiteral("FormValidationFileMissing");
          break;
        case NS_FORM_INPUT_CHECKBOX:
          key.AssignLiteral("FormValidationCheckboxMissing");
          break;
        case NS_FORM_INPUT_RADIO:
          key.AssignLiteral("FormValidationRadioMissing");
          break;
        case NS_FORM_INPUT_NUMBER:
          key.AssignLiteral("FormValidationBadInputNumber");
          break;
        default:
          key.AssignLiteral("FormValidationValueMissing");
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
        const char16_t* params[] = { title.get() };
        rv = nsContentUtils::FormatLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                   "FormValidationPatternMismatchWithTitle",
                                                   params, message);
      }
      aValidationMessage = message;
      break;
    }
    case VALIDITY_STATE_RANGE_OVERFLOW:
    {
      static const char kNumberOverTemplate[] = "FormValidationNumberRangeOverflow";
      static const char kDateOverTemplate[] = "FormValidationDateRangeOverflow";
      static const char kTimeOverTemplate[] = "FormValidationTimeRangeOverflow";

      const char* msgTemplate;
      nsXPIDLString message;

      nsAutoString maxStr;
      if (mType == NS_FORM_INPUT_NUMBER ||
          mType == NS_FORM_INPUT_RANGE) {
        msgTemplate = kNumberOverTemplate;

        
        Decimal maximum = GetMaximum();
        MOZ_ASSERT(!maximum.isNaN());

        char buf[32];
        DebugOnly<bool> ok = maximum.toString(buf, ArrayLength(buf));
        maxStr.AssignASCII(buf);
        MOZ_ASSERT(ok, "buf not big enough");
      } else if (mType == NS_FORM_INPUT_DATE || mType == NS_FORM_INPUT_TIME) {
        msgTemplate = mType == NS_FORM_INPUT_DATE ? kDateOverTemplate : kTimeOverTemplate;
        GetAttr(kNameSpaceID_None, nsGkAtoms::max, maxStr);
      } else {
        msgTemplate = kNumberOverTemplate;
        NS_NOTREACHED("Unexpected input type");
      }

      const char16_t* params[] = { maxStr.get() };
      rv = nsContentUtils::FormatLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                 msgTemplate,
                                                 params, message);
      aValidationMessage = message;
      break;
    }
    case VALIDITY_STATE_RANGE_UNDERFLOW:
    {
      static const char kNumberUnderTemplate[] = "FormValidationNumberRangeUnderflow";
      static const char kDateUnderTemplate[] = "FormValidationDateRangeUnderflow";
      static const char kTimeUnderTemplate[] = "FormValidationTimeRangeUnderflow";

      const char* msgTemplate;
      nsXPIDLString message;

      nsAutoString minStr;
      if (mType == NS_FORM_INPUT_NUMBER ||
          mType == NS_FORM_INPUT_RANGE) {
        msgTemplate = kNumberUnderTemplate;

        Decimal minimum = GetMinimum();
        MOZ_ASSERT(!minimum.isNaN());

        char buf[32];
        DebugOnly<bool> ok = minimum.toString(buf, ArrayLength(buf));
        minStr.AssignASCII(buf);
        MOZ_ASSERT(ok, "buf not big enough");
      } else if (mType == NS_FORM_INPUT_DATE || mType == NS_FORM_INPUT_TIME) {
        msgTemplate = mType == NS_FORM_INPUT_DATE ? kDateUnderTemplate : kTimeUnderTemplate;
        GetAttr(kNameSpaceID_None, nsGkAtoms::min, minStr);
      } else {
        msgTemplate = kNumberUnderTemplate;
        NS_NOTREACHED("Unexpected input type");
      }

      const char16_t* params[] = { minStr.get() };
      rv = nsContentUtils::FormatLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                 msgTemplate,
                                                 params, message);
      aValidationMessage = message;
      break;
    }
    case VALIDITY_STATE_STEP_MISMATCH:
    {
      nsXPIDLString message;

      Decimal value = GetValueAsDecimal();
      MOZ_ASSERT(!value.isNaN());

      Decimal step = GetStep();
      MOZ_ASSERT(step != kStepAny && step > Decimal(0));

      
      
      
      
      
      if (mType == NS_FORM_INPUT_DATE) {
        step = EuclidLCM<Decimal>(step.floor(),
                                  GetStepScaleFactor().floor());
      }

      Decimal stepBase = GetStepBase();

      Decimal valueLow = value - NS_floorModulo(value - stepBase, step);
      Decimal valueHigh = value + step - NS_floorModulo(value - stepBase, step);

      Decimal maximum = GetMaximum();

      if (maximum.isNaN() || valueHigh <= maximum) {
        nsAutoString valueLowStr, valueHighStr;
        ConvertNumberToString(valueLow, valueLowStr);
        ConvertNumberToString(valueHigh, valueHighStr);

        if (valueLowStr.Equals(valueHighStr)) {
          const char16_t* params[] = { valueLowStr.get() };
          rv = nsContentUtils::FormatLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                     "FormValidationStepMismatchOneValue",
                                                     params, message);
        } else {
          const char16_t* params[] = { valueLowStr.get(), valueHighStr.get() };
          rv = nsContentUtils::FormatLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                     "FormValidationStepMismatch",
                                                     params, message);
        }
      } else {
        nsAutoString valueLowStr;
        ConvertNumberToString(valueLow, valueLowStr);

        const char16_t* params[] = { valueLowStr.get() };
        rv = nsContentUtils::FormatLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                                   "FormValidationStepMismatchOneValue",
                                                   params, message);
      }

      aValidationMessage = message;
      break;
    }
    case VALIDITY_STATE_BAD_INPUT:
    {
      nsXPIDLString message;
      nsAutoCString key;
      if (mType == NS_FORM_INPUT_NUMBER) {
        key.AssignLiteral("FormValidationBadInputNumber");
      } else if (mType == NS_FORM_INPUT_EMAIL) {
        key.AssignLiteral("FormValidationInvalidEmail");
      } else {
        return NS_ERROR_UNEXPECTED;
      }
      rv = nsContentUtils::GetLocalizedString(nsContentUtils::eDOM_PROPERTIES,
                                              key.get(), message);
      aValidationMessage = message;
      break;
    }
    default:
      rv = nsIConstraintValidation::GetValidationMessage(aValidationMessage, aType);
  }

  return rv;
}


bool
HTMLInputElement::IsValidEmailAddressList(const nsAString& aValue)
{
  HTMLSplitOnSpacesTokenizer tokenizer(aValue, ',');

  while (tokenizer.hasMoreTokens()) {
    if (!IsValidEmailAddress(tokenizer.nextToken())) {
      return false;
    }
  }

  return !tokenizer.separatorAfterCurrentToken();
}


bool
HTMLInputElement::IsValidEmailAddress(const nsAString& aValue)
{
  
  if (aValue.IsEmpty() || aValue.Last() == '.' || aValue.Last() == '-') {
    return false;
  }

  uint32_t atPos;
  nsAutoCString value;
  if (!PunycodeEncodeEmailAddress(aValue, value, &atPos) ||
      atPos == (uint32_t)kNotFound || atPos == 0 || atPos == value.Length() - 1) {
    
    
    return false;
  }

  uint32_t length = value.Length();
  uint32_t i = 0;

  
  for (; i < atPos; ++i) {
    char16_t c = value[i];

    
    if (!(nsCRT::IsAsciiAlpha(c) || nsCRT::IsAsciiDigit(c) ||
          c == '.' || c == '!' || c == '#' || c == '$' || c == '%' ||
          c == '&' || c == '\''|| c == '*' || c == '+' || c == '-' ||
          c == '/' || c == '=' || c == '?' || c == '^' || c == '_' ||
          c == '`' || c == '{' || c == '|' || c == '}' || c == '~' )) {
      return false;
    }
  }

  
  ++i;

  
  if (value[i] == '.' || value[i] == '-') {
    return false;
  }

  
  for (; i < length; ++i) {
    char16_t c = value[i];

    if (c == '.') {
      
      if (value[i-1] == '.' || value[i-1] == '-') {
        return false;
      }
    } else if (c == '-'){
      
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
HTMLInputElement::IsSingleLineTextControl() const
{
  return IsSingleLineTextControl(false);
}

NS_IMETHODIMP_(bool)
HTMLInputElement::IsTextArea() const
{
  return false;
}

NS_IMETHODIMP_(bool)
HTMLInputElement::IsPlainTextControl() const
{
  
  return true;
}

NS_IMETHODIMP_(bool)
HTMLInputElement::IsPasswordTextControl() const
{
  return mType == NS_FORM_INPUT_PASSWORD;
}

NS_IMETHODIMP_(int32_t)
HTMLInputElement::GetCols()
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
HTMLInputElement::GetWrapCols()
{
  return 0; 
}

NS_IMETHODIMP_(int32_t)
HTMLInputElement::GetRows()
{
  return DEFAULT_ROWS;
}

NS_IMETHODIMP_(void)
HTMLInputElement::GetDefaultValueFromContent(nsAString& aValue)
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
HTMLInputElement::ValueChanged() const
{
  return mValueChanged;
}

NS_IMETHODIMP_(void)
HTMLInputElement::GetTextEditorValue(nsAString& aValue,
                                     bool aIgnoreWrap) const
{
  nsTextEditorState* state = GetEditorState();
  if (state) {
    state->GetValue(aValue, aIgnoreWrap);
  }
}

NS_IMETHODIMP_(void)
HTMLInputElement::InitializeKeyboardEventListeners()
{
  nsTextEditorState* state = GetEditorState();
  if (state) {
    state->InitializeKeyboardEventListeners();
  }
}

NS_IMETHODIMP_(void)
HTMLInputElement::OnValueChanged(bool aNotify)
{
  UpdateAllValidityStates(aNotify);

  if (HasDirAuto()) {
    SetDirectionIfAuto(true, aNotify);
  }
}

NS_IMETHODIMP_(bool)
HTMLInputElement::HasCachedSelection()
{
  bool isCached = false;
  nsTextEditorState* state = GetEditorState();
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
HTMLInputElement::FieldSetDisabledChanged(bool aNotify)
{
  UpdateValueMissingValidityState();
  UpdateBarredFromConstraintValidation();

  nsGenericHTMLFormElementWithState::FieldSetDisabledChanged(aNotify);
}

void
HTMLInputElement::SetFilePickerFiltersFromAccept(nsIFilePicker* filePicker)
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

  bool allMimeTypeFiltersAreValid = true;
  bool atLeastOneFileExtensionFilter = false;

  
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
      filterBundle->GetStringFromName(MOZ_UTF16("imageFilter"),
                                      getter_Copies(extensionListStr));
    } else if (token.EqualsLiteral("audio/*")) {
      filterMask = nsIFilePicker::filterAudio;
      filterBundle->GetStringFromName(MOZ_UTF16("audioFilter"),
                                      getter_Copies(extensionListStr));
    } else if (token.EqualsLiteral("video/*")) {
      filterMask = nsIFilePicker::filterVideo;
      filterBundle->GetStringFromName(MOZ_UTF16("videoFilter"),
                                      getter_Copies(extensionListStr));
    } else if (token.First() == '.') {
      extensionListStr = NS_LITERAL_STRING("*") + token;
      filterName = extensionListStr + NS_LITERAL_STRING("; ");
      atLeastOneFileExtensionFilter = true;
    } else {
      
      nsCOMPtr<nsIMIMEInfo> mimeInfo;
      if (NS_FAILED(mimeService->GetFromTypeAndExtension(
                      NS_ConvertUTF16toUTF8(token),
                      EmptyCString(), 
                      getter_AddRefs(mimeInfo))) ||
          !mimeInfo) {
        allMimeTypeFiltersAreValid =  false;
        continue;
      }

      
      
      mimeInfo->GetDescription(filterName);
      if (filterName.IsEmpty()) {
        nsCString mimeTypeName;
        mimeInfo->GetType(mimeTypeName);
        CopyUTF8toUTF16(mimeTypeName, filterName);
      }

      
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
      
      allMimeTypeFiltersAreValid = false;
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

  
  
  nsTArray<nsFilePickerFilter> filtersCopy;
  filtersCopy = filters;
  for (uint32_t i = 0; i < filtersCopy.Length(); ++i) {
    const nsFilePickerFilter& filterToCheck = filtersCopy[i];
    if (filterToCheck.mFilterMask) {
      continue;
    }
    for (uint32_t j = 0; j < filtersCopy.Length(); ++j) {
      if (i == j) {
        continue;
      }
      if (FindInReadable(filterToCheck.mFilter, filtersCopy[j].mFilter)) {
        
        
        
        filters.RemoveElement(filterToCheck);
      }
    }
  }

  
  if (filters.Length() > 1) {
    nsXPIDLString title;
    nsContentUtils::GetLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                       "AllSupportedTypes", title);
    filePicker->AppendFilter(title, allExtensionsList);
  }

  
  for (uint32_t i = 0; i < filters.Length(); ++i) {
    const nsFilePickerFilter& filter = filters[i];
    if (filter.mFilterMask) {
      filePicker->AppendFilters(filter.mFilterMask);
    } else {
      filePicker->AppendFilter(filter.mTitle, filter.mFilter);
    }
  }

  if (filters.Length() >= 1 &&
      (allMimeTypeFiltersAreValid || atLeastOneFileExtensionFilter)) {
    
    
    filePicker->SetFilterIndex(1);
  }
}

Decimal
HTMLInputElement::GetStepScaleFactor() const
{
  MOZ_ASSERT(DoesStepApply());

  switch (mType) {
    case NS_FORM_INPUT_DATE:
      return kStepScaleFactorDate;
    case NS_FORM_INPUT_NUMBER:
    case NS_FORM_INPUT_RANGE:
      return kStepScaleFactorNumberRange;
    case NS_FORM_INPUT_TIME:
      return kStepScaleFactorTime;
    default:
      MOZ_ASSERT(false, "Unrecognized input type");
      return Decimal::nan();
  }
}

Decimal
HTMLInputElement::GetDefaultStep() const
{
  MOZ_ASSERT(DoesStepApply());

  switch (mType) {
    case NS_FORM_INPUT_DATE:
    case NS_FORM_INPUT_NUMBER:
    case NS_FORM_INPUT_RANGE:
      return kDefaultStep;
    case NS_FORM_INPUT_TIME:
      return kDefaultStepTime;
    default:
      MOZ_ASSERT(false, "Unrecognized input type");
      return Decimal::nan();
  }
}

void
HTMLInputElement::UpdateValidityUIBits(bool aIsFocused)
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
HTMLInputElement::UpdateHasRange()
{
  




  mHasRange = false;

  if (!DoesMinMaxApply()) {
    return;
  }

  Decimal minimum = GetMinimum();
  if (!minimum.isNaN()) {
    mHasRange = true;
    return;
  }

  Decimal maximum = GetMaximum();
  if (!maximum.isNaN()) {
    mHasRange = true;
    return;
  }
}

void
HTMLInputElement::PickerClosed()
{
  mPickerRunning = false;
}

JSObject*
HTMLInputElement::WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return HTMLInputElementBinding::Wrap(aCx, this, aGivenProto);
}

} 
} 

#undef NS_ORIGINAL_CHECKED_VALUE
