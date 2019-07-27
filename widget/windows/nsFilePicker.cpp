





#include "nsFilePicker.h"

#include <shlobj.h>
#include <shlwapi.h>
#include <cderr.h>

#include "mozilla/WindowsVersion.h"
#include "nsReadableUtils.h"
#include "nsNetUtil.h"
#include "nsWindow.h"
#include "nsILoadContext.h"
#include "nsIServiceManager.h"
#include "nsIURL.h"
#include "nsIStringBundle.h"
#include "nsEnumeratorUtils.h"
#include "nsCRT.h"
#include "nsString.h"
#include "nsToolkit.h"
#include "WinUtils.h"
#include "nsPIDOMWindow.h"

using mozilla::IsVistaOrLater;
using namespace mozilla::widget;

char16_t *nsFilePicker::mLastUsedUnicodeDirectory;
char nsFilePicker::mLastUsedDirectory[MAX_PATH+1] = { 0 };

static const wchar_t kDialogPtrProp[] = L"DialogPtrProperty";
static const DWORD kDialogTimerID = 9999;
static const unsigned long kDialogTimerTimeout = 300;

#define MAX_EXTENSION_LENGTH 10
#define FILE_BUFFER_SIZE     4096 

typedef DWORD FILEOPENDIALOGOPTIONS;





class AutoSuppressEvents
{
public:
  explicit AutoSuppressEvents(nsIWidget* aWidget) :
    mWindow(static_cast<nsWindow *>(aWidget)) {
    SuppressWidgetEvents(true);
  }

  ~AutoSuppressEvents() {
    SuppressWidgetEvents(false);
  }
private:
  void SuppressWidgetEvents(bool aFlag) {
    if (mWindow) {
      mWindow->SuppressBlurEvents(aFlag);
    }
  }
  nsRefPtr<nsWindow> mWindow;
};


class AutoRestoreWorkingPath
{
public:
  AutoRestoreWorkingPath() {
    DWORD bufferLength = GetCurrentDirectoryW(0, nullptr);
    mWorkingPath = new wchar_t[bufferLength];
    if (GetCurrentDirectoryW(bufferLength, mWorkingPath) == 0) {
      mWorkingPath = nullptr;
    }
  }

  ~AutoRestoreWorkingPath() {
    if (HasWorkingPath()) {
      ::SetCurrentDirectoryW(mWorkingPath);
    }
  }

  inline bool HasWorkingPath() const {
    return mWorkingPath != nullptr;
  }
private:
  nsAutoArrayPtr<wchar_t> mWorkingPath;
};




class AutoDestroyTmpWindow
{
public:
  explicit AutoDestroyTmpWindow(HWND aTmpWnd) :
    mWnd(aTmpWnd) {
  }

  ~AutoDestroyTmpWindow() {
    if (mWnd)
      DestroyWindow(mWnd);
  }
  
  inline HWND get() const { return mWnd; }
private:
  HWND mWnd;
};


class AutoWidgetPickerState
{
public:
  explicit AutoWidgetPickerState(nsIWidget* aWidget) :
    mWindow(static_cast<nsWindow *>(aWidget)) {
    PickerState(true);
  }

  ~AutoWidgetPickerState() {
    PickerState(false);
  }
private:
  void PickerState(bool aFlag) {
    if (mWindow) {
      if (aFlag)
        mWindow->PickerOpen();
      else
        mWindow->PickerClosed();
    }
  }
  nsRefPtr<nsWindow> mWindow;
};


class AutoTimerCallbackCancel
{
public:
  AutoTimerCallbackCancel(nsFilePicker* aTarget,
                          nsTimerCallbackFunc aCallbackFunc) {
    Init(aTarget, aCallbackFunc);
  }

  ~AutoTimerCallbackCancel() {
    if (mPickerCallbackTimer) {
      mPickerCallbackTimer->Cancel();
    }
  }

private:
  void Init(nsFilePicker* aTarget,
            nsTimerCallbackFunc aCallbackFunc) {
    mPickerCallbackTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (!mPickerCallbackTimer) {
      NS_WARNING("do_CreateInstance for timer failed??");
      return;
    }
    mPickerCallbackTimer->InitWithFuncCallback(aCallbackFunc,
                                               aTarget,
                                               kDialogTimerTimeout,
                                               nsITimer::TYPE_REPEATING_SLACK);
  }
  nsCOMPtr<nsITimer> mPickerCallbackTimer;
    
};




nsFilePicker::nsFilePicker() :
  mSelectedType(1)
  , mDlgWnd(nullptr)
  , mFDECookie(0)
{
   CoInitialize(nullptr);
}

nsFilePicker::~nsFilePicker()
{
  if (mLastUsedUnicodeDirectory) {
    free(mLastUsedUnicodeDirectory);
    mLastUsedUnicodeDirectory = nullptr;
  }
  CoUninitialize();
}

NS_IMPL_ISUPPORTS(nsFilePicker, nsIFilePicker)

NS_IMETHODIMP nsFilePicker::Init(nsIDOMWindow *aParent, const nsAString& aTitle, int16_t aMode)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aParent);
  nsIDocShell* docShell = window ? window->GetDocShell() : nullptr;  
  mLoadContext = do_QueryInterface(docShell);
  
  return nsBaseFilePicker::Init(aParent, aTitle, aMode);
}

STDMETHODIMP nsFilePicker::QueryInterface(REFIID refiid, void** ppvResult)
{
  *ppvResult = nullptr;
  if (IID_IUnknown == refiid ||
      refiid == IID_IFileDialogEvents) {
    *ppvResult = this;
  }

  if (nullptr != *ppvResult) {
    ((LPUNKNOWN)*ppvResult)->AddRef();
    return S_OK;
  }

  return E_NOINTERFACE;
}






int CALLBACK
BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
  if (uMsg == BFFM_INITIALIZED)
  {
    char16_t * filePath = (char16_t *) lpData;
    if (filePath)
      ::SendMessageW(hwnd, BFFM_SETSELECTIONW,
                     TRUE ,
                     lpData);
  }
  return 0;
}

static void
EnsureWindowVisible(HWND hwnd) 
{
  
  
  HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
  if (!monitor) {
    
    HWND parentHwnd = GetParent(hwnd);
    RECT parentRect;
    GetWindowRect(parentHwnd, &parentRect);
    SetWindowPos(hwnd, nullptr, parentRect.left, parentRect.top, 0, 0,
                 SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
  }
}



UINT_PTR CALLBACK
nsFilePicker::FilePickerHook(HWND hwnd,
                             UINT msg,
                             WPARAM wParam,
                             LPARAM lParam) 
{
  switch(msg) {
    case WM_NOTIFY:
      {
        LPOFNOTIFYW lpofn = (LPOFNOTIFYW) lParam;
        if (!lpofn || !lpofn->lpOFN) {
          return 0;
        }
        
        if (CDN_INITDONE == lpofn->hdr.code) {
          
          
          
          PostMessage(hwnd, MOZ_WM_ENSUREVISIBLE, 0, 0);
        }
      }
      break;
    case MOZ_WM_ENSUREVISIBLE:
      EnsureWindowVisible(GetParent(hwnd));
      break;
    case WM_INITDIALOG:
      {
        OPENFILENAMEW* pofn = reinterpret_cast<OPENFILENAMEW*>(lParam);
        SetProp(hwnd, kDialogPtrProp, (HANDLE)pofn->lCustData);
        nsFilePicker* picker = reinterpret_cast<nsFilePicker*>(pofn->lCustData);
        if (picker) {
          picker->SetDialogHandle(hwnd);
          SetTimer(hwnd, kDialogTimerID, kDialogTimerTimeout, nullptr);
        }
      }
      break;
    case WM_TIMER:
      {
        
        if (wParam == kDialogTimerID) {
          nsFilePicker* picker = 
            reinterpret_cast<nsFilePicker*>(GetProp(hwnd, kDialogPtrProp));
          if (picker && picker->ClosePickerIfNeeded(true)) {
            KillTimer(hwnd, kDialogTimerID);
          }
        }
      }
      break;
  }
  return 0;
}




UINT_PTR CALLBACK
nsFilePicker::MultiFilePickerHook(HWND hwnd,
                                  UINT msg,
                                  WPARAM wParam,
                                  LPARAM lParam)
{
  switch (msg) {
    case WM_INITDIALOG:
      {
        
        
        
        HWND comboBox = FindWindowEx(GetParent(hwnd), nullptr, 
                                     L"ComboBoxEx32", nullptr );
        if(comboBox)
          SendMessage(comboBox, CB_LIMITTEXT, 0, 0);
        
        OPENFILENAMEW* pofn = reinterpret_cast<OPENFILENAMEW*>(lParam);
        SetProp(hwnd, kDialogPtrProp, (HANDLE)pofn->lCustData);
        nsFilePicker* picker =
          reinterpret_cast<nsFilePicker*>(pofn->lCustData);
        if (picker) {
          picker->SetDialogHandle(hwnd);
          SetTimer(hwnd, kDialogTimerID, kDialogTimerTimeout, nullptr);
        }
      }
      break;
    case WM_NOTIFY:
      {
        LPOFNOTIFYW lpofn = (LPOFNOTIFYW) lParam;
        if (!lpofn || !lpofn->lpOFN) {
          return 0;
        }
        
        
        if (lpofn->hdr.code == CDN_SELCHANGE) {
          HWND parentHWND = GetParent(hwnd);

          
          UINT newBufLength = 0; 
          int requiredBufLength = CommDlg_OpenSave_GetSpecW(parentHWND, 
                                                            nullptr, 0);
          if(requiredBufLength >= 0)
            newBufLength += requiredBufLength;
          else
            newBufLength += MAX_PATH;

          
          
          
          
          requiredBufLength = CommDlg_OpenSave_GetFolderPathW(parentHWND, 
                                                              nullptr, 0);
          if(requiredBufLength >= 0)
            newBufLength += requiredBufLength;
          else
            newBufLength += MAX_PATH;

          
          if (newBufLength > lpofn->lpOFN->nMaxFile) {
            if (lpofn->lpOFN->lpstrFile)
              delete[] lpofn->lpOFN->lpstrFile;

            
            
            
            newBufLength += FILE_BUFFER_SIZE;

            wchar_t* filesBuffer = new wchar_t[newBufLength];
            ZeroMemory(filesBuffer, newBufLength * sizeof(wchar_t));

            lpofn->lpOFN->lpstrFile = filesBuffer;
            lpofn->lpOFN->nMaxFile  = newBufLength;
          }
        }
      }
      break;
    case WM_TIMER:
      {
        
        if (wParam == kDialogTimerID) {
          nsFilePicker* picker =
            reinterpret_cast<nsFilePicker*>(GetProp(hwnd, kDialogPtrProp));
          if (picker && picker->ClosePickerIfNeeded(true)) {
            KillTimer(hwnd, kDialogTimerID);
          }
        }
      }
      break;
  }

  return FilePickerHook(hwnd, msg, wParam, lParam);
}





HRESULT
nsFilePicker::OnFileOk(IFileDialog *pfd)
{
  return S_OK;
}

HRESULT
nsFilePicker::OnFolderChanging(IFileDialog *pfd,
                               IShellItem *psiFolder)
{
  return S_OK;
}

HRESULT
nsFilePicker::OnFolderChange(IFileDialog *pfd)
{
  return S_OK;
}

HRESULT
nsFilePicker::OnSelectionChange(IFileDialog *pfd)
{
  return S_OK;
}

HRESULT
nsFilePicker::OnShareViolation(IFileDialog *pfd,
                               IShellItem *psi,
                               FDE_SHAREVIOLATION_RESPONSE *pResponse)
{
  return S_OK;
}

HRESULT
nsFilePicker::OnTypeChange(IFileDialog *pfd)
{
  
  nsRefPtr<IOleWindow> win;
  pfd->QueryInterface(IID_IOleWindow, getter_AddRefs(win));
  if (!win) {
    NS_ERROR("Could not retrieve the IOleWindow interface for IFileDialog.");
    return S_OK;
  }
  HWND hwnd = nullptr;
  win->GetWindow(&hwnd);
  if (!hwnd) {
    NS_ERROR("Could not retrieve the HWND for IFileDialog.");
    return S_OK;
  }
  
  SetDialogHandle(hwnd);
  return S_OK;
}

HRESULT
nsFilePicker::OnOverwrite(IFileDialog *pfd,
                          IShellItem *psi,
                          FDE_OVERWRITE_RESPONSE *pResponse)
{
  return S_OK;
}





bool
nsFilePicker::ClosePickerIfNeeded(bool aIsXPDialog)
{
  if (!mParentWidget || !mDlgWnd)
    return false;

  nsWindow *win = static_cast<nsWindow *>(mParentWidget.get());
  
  
  HWND dlgWnd;
  if (aIsXPDialog)
    dlgWnd = GetParent(mDlgWnd);
  else
    dlgWnd = mDlgWnd;
  if (IsWindow(dlgWnd) && IsWindowVisible(dlgWnd) && win->DestroyCalled()) {
    wchar_t className[64];
    
    if (GetClassNameW(dlgWnd, className, mozilla::ArrayLength(className)) &&
        !wcscmp(className, L"#32770") &&
        DestroyWindow(dlgWnd)) {
      mDlgWnd = nullptr;
      return true;
    }
  }
  return false;
}

void
nsFilePicker::PickerCallbackTimerFunc(nsITimer *aTimer, void *aCtx)
{
  nsFilePicker* picker = (nsFilePicker*)aCtx;
  if (picker->ClosePickerIfNeeded(false)) {
    aTimer->Cancel();
  }
}

void
nsFilePicker::SetDialogHandle(HWND aWnd)
{
  if (!aWnd || mDlgWnd)
    return;
  mDlgWnd = aWnd;
}







bool
nsFilePicker::ShowXPFolderPicker(const nsString& aInitialDir)
{
  bool result = false;

  nsAutoArrayPtr<wchar_t> dirBuffer(new wchar_t[FILE_BUFFER_SIZE]);
  wcsncpy(dirBuffer, aInitialDir.get(), FILE_BUFFER_SIZE);
  dirBuffer[FILE_BUFFER_SIZE-1] = '\0';

  AutoDestroyTmpWindow adtw((HWND)(mParentWidget.get() ?
    mParentWidget->GetNativeData(NS_NATIVE_TMP_WINDOW) : nullptr));

  BROWSEINFOW browserInfo = {0};
  browserInfo.pidlRoot       = nullptr;
  browserInfo.pszDisplayName = dirBuffer;
  browserInfo.lpszTitle      = mTitle.get();
  browserInfo.ulFlags        = BIF_USENEWUI | BIF_RETURNONLYFSDIRS;
  browserInfo.hwndOwner      = adtw.get(); 
  browserInfo.iImage         = 0;
  browserInfo.lParam         = reinterpret_cast<LPARAM>(this);

  if (!aInitialDir.IsEmpty()) {
    
    
    browserInfo.lParam = (LPARAM) aInitialDir.get();
    browserInfo.lpfn   = &BrowseCallbackProc;
  } else {
    browserInfo.lParam = 0;
    browserInfo.lpfn   = nullptr;
  }

  LPITEMIDLIST list = ::SHBrowseForFolderW(&browserInfo);
  if (list) {
    result = ::SHGetPathFromIDListW(list, dirBuffer);
    if (result)
      mUnicodeFile.Assign(static_cast<const wchar_t*>(dirBuffer));
    
    CoTaskMemFree(list);
  }

  return result;
}










bool
nsFilePicker::ShowFolderPicker(const nsString& aInitialDir, bool &aWasInitError)
{
  nsRefPtr<IFileOpenDialog> dialog;
  if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC,
                              IID_IFileOpenDialog,
                              getter_AddRefs(dialog)))) {
    aWasInitError = true;
    return false;
  }
  aWasInitError = false;

  
  dialog->Advise(this, &mFDECookie);

  
  FILEOPENDIALOGOPTIONS fos = FOS_PICKFOLDERS;
  dialog->SetOptions(fos);
 
  
  dialog->SetTitle(mTitle.get());
  if (!aInitialDir.IsEmpty()) {
    nsRefPtr<IShellItem> folder;
    if (SUCCEEDED(
          WinUtils::SHCreateItemFromParsingName(aInitialDir.get(), nullptr,
                                                IID_IShellItem,
                                                getter_AddRefs(folder)))) {
      dialog->SetFolder(folder);
    }
  }

  AutoDestroyTmpWindow adtw((HWND)(mParentWidget.get() ?
    mParentWidget->GetNativeData(NS_NATIVE_TMP_WINDOW) : nullptr));
 
  
  nsRefPtr<IShellItem> item;
  if (FAILED(dialog->Show(adtw.get())) ||
      FAILED(dialog->GetResult(getter_AddRefs(item))) ||
      !item) {
    dialog->Unadvise(mFDECookie);
    return false;
  }
  dialog->Unadvise(mFDECookie);

  

  
  
  nsRefPtr<IShellItem> folderPath;
  nsRefPtr<IShellLibrary> shellLib;
  CoCreateInstance(CLSID_ShellLibrary, nullptr, CLSCTX_INPROC,
                   IID_IShellLibrary, getter_AddRefs(shellLib));
  if (shellLib &&
      SUCCEEDED(shellLib->LoadLibraryFromItem(item, STGM_READ)) &&
      SUCCEEDED(shellLib->GetDefaultSaveFolder(DSFT_DETECT, IID_IShellItem,
                                               getter_AddRefs(folderPath)))) {
    item.swap(folderPath);
  }

  
  return WinUtils::GetShellItemPath(item, mUnicodeFile);
}





 bool
nsFilePicker::GetFileNameWrapper(OPENFILENAMEW* ofn, PickerType aType)
{
  MOZ_SEH_TRY {
    if (aType == PICKER_TYPE_OPEN)
      return ::GetOpenFileNameW(ofn);
    else if (aType == PICKER_TYPE_SAVE)
      return ::GetSaveFileNameW(ofn);
  } MOZ_SEH_EXCEPT(true) {
    NS_ERROR("nsFilePicker GetFileName win32 call generated an exception! This is bad!");
  }
  return false;
}

bool
nsFilePicker::FilePickerWrapper(OPENFILENAMEW* ofn, PickerType aType)
{
  if (!ofn)
    return false;
  AutoWidgetPickerState awps(mParentWidget);
  return GetFileNameWrapper(ofn, aType);
}

bool
nsFilePicker::ShowXPFilePicker(const nsString& aInitialDir)
{
  OPENFILENAMEW ofn = {0};
  ofn.lStructSize = sizeof(ofn);
  nsString filterBuffer = mFilterList;
                                
  nsAutoArrayPtr<wchar_t> fileBuffer(new wchar_t[FILE_BUFFER_SIZE]);
  wcsncpy(fileBuffer,  mDefaultFilePath.get(), FILE_BUFFER_SIZE);
  fileBuffer[FILE_BUFFER_SIZE-1] = '\0'; 

  if (!aInitialDir.IsEmpty()) {
    ofn.lpstrInitialDir = aInitialDir.get();
  }

  AutoDestroyTmpWindow adtw((HWND) (mParentWidget.get() ?
    mParentWidget->GetNativeData(NS_NATIVE_TMP_WINDOW) : nullptr));

  ofn.lpstrTitle   = (LPCWSTR)mTitle.get();
  ofn.lpstrFilter  = (LPCWSTR)filterBuffer.get();
  ofn.nFilterIndex = mSelectedType;
  ofn.lpstrFile    = fileBuffer;
  ofn.nMaxFile     = FILE_BUFFER_SIZE;
  ofn.hwndOwner    = adtw.get();
  ofn.lCustData    = reinterpret_cast<LPARAM>(this);
  ofn.Flags = OFN_SHAREAWARE | OFN_LONGNAMES | OFN_OVERWRITEPROMPT |
              OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_ENABLESIZING | 
              OFN_EXPLORER;

  
  
  
  
  if (!IsVistaOrLater()) {
    ofn.lpfnHook = FilePickerHook;
    ofn.Flags |= OFN_ENABLEHOOK;
  }

  
  if (IsPrivacyModeEnabled() || !mAddToRecentDocs) {
    ofn.Flags |= OFN_DONTADDTORECENT;
  }

  NS_NAMED_LITERAL_STRING(htmExt, "html");

  if (!mDefaultExtension.IsEmpty()) {
    ofn.lpstrDefExt = mDefaultExtension.get();
  } else if (IsDefaultPathHtml()) {
    
    
    
    
    
    
    ofn.lpstrDefExt = htmExt.get();
  }

  
  
  
  
  
  
  AutoRestoreWorkingPath restoreWorkingPath;
  
  
  if (!restoreWorkingPath.HasWorkingPath()) {
    ofn.Flags |= OFN_NOCHANGEDIR;
  }

  bool result = false;

  switch(mMode) {
    case modeOpen:
      
      ofn.Flags |= OFN_FILEMUSTEXIST;
      result = FilePickerWrapper(&ofn, PICKER_TYPE_OPEN);
      break;

    case modeOpenMultiple:
      ofn.Flags |= OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;

      
      
      
      
      
      
      
      
      
      
      if (!IsVistaOrLater()) {
        ofn.lpfnHook = MultiFilePickerHook;
        fileBuffer.forget();
        result = FilePickerWrapper(&ofn, PICKER_TYPE_OPEN);
        fileBuffer = ofn.lpstrFile;
      } else {
        result = FilePickerWrapper(&ofn, PICKER_TYPE_OPEN);
      }
      break;

    case modeSave:
      {
        ofn.Flags |= OFN_NOREADONLYRETURN;

        
        
        if (IsDefaultPathLink())
          ofn.Flags |= OFN_NODEREFERENCELINKS;

        result = FilePickerWrapper(&ofn, PICKER_TYPE_SAVE);
        if (!result) {
          
          if (GetLastError() == ERROR_INVALID_PARAMETER ||
              CommDlgExtendedError() == FNERR_INVALIDFILENAME) {
            
            
            ofn.lpstrFile[0] = L'\0';
            result = FilePickerWrapper(&ofn, PICKER_TYPE_SAVE);
          }
        }
      }
      break;

    default:
      NS_NOTREACHED("unsupported file picker mode");
      return false;
  }

  if (!result)
    return false;

  
  mSelectedType = (int16_t)ofn.nFilterIndex;

  
  if (mMode != modeOpenMultiple) {
    GetQualifiedPath(fileBuffer, mUnicodeFile);
    return true;
  }

  
  
  
  
  
  
  wchar_t *current = fileBuffer;
  
  nsAutoString dirName(current);
  
  if (current[dirName.Length() - 1] != '\\')
    dirName.Append((char16_t)'\\');
  
  while (current && *current && *(current + wcslen(current) + 1)) {
    current = current + wcslen(current) + 1;
    
    nsCOMPtr<nsIFile> file = do_CreateInstance("@mozilla.org/file/local;1");
    NS_ENSURE_TRUE(file, false);

    
    nsAutoString path;
    if (PathIsRelativeW(current)) {
      path = dirName + nsDependentString(current);
    } else {
      path = current;
    }

    nsAutoString canonicalizedPath;
    GetQualifiedPath(path.get(), canonicalizedPath);
    if (NS_FAILED(file->InitWithPath(canonicalizedPath)) ||
        !mFiles.AppendObject(file))
      return false;
  }
  
  
  
  
  if (current && *current && (current == fileBuffer)) {
    nsCOMPtr<nsIFile> file = do_CreateInstance("@mozilla.org/file/local;1");
    NS_ENSURE_TRUE(file, false);
    
    nsAutoString canonicalizedPath;
    GetQualifiedPath(current, canonicalizedPath);
    if (NS_FAILED(file->InitWithPath(canonicalizedPath)) ||
        !mFiles.AppendObject(file))
      return false;
  }

  return true;
}










bool
nsFilePicker::ShowFilePicker(const nsString& aInitialDir, bool &aWasInitError)
{
  nsRefPtr<IFileDialog> dialog;
  if (mMode != modeSave) {
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC,
                                IID_IFileOpenDialog,
                                getter_AddRefs(dialog)))) {
      aWasInitError = true;
      return false;
    }
  } else {
    if (FAILED(CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC,
                                IID_IFileSaveDialog,
                                getter_AddRefs(dialog)))) {
      aWasInitError = true;
      return false;
    }
  }
  aWasInitError = false;

  
  dialog->Advise(this, &mFDECookie);

  

  FILEOPENDIALOGOPTIONS fos = 0;
  fos |= FOS_SHAREAWARE | FOS_OVERWRITEPROMPT |
         FOS_FORCEFILESYSTEM;

  
  if (IsPrivacyModeEnabled() || !mAddToRecentDocs) {
    fos |= FOS_DONTADDTORECENT;
  }

  
  
  AutoRestoreWorkingPath arw;

  
  switch(mMode) {
    case modeOpen:
      fos |= FOS_FILEMUSTEXIST;
      break;

    case modeOpenMultiple:
      fos |= FOS_FILEMUSTEXIST | FOS_ALLOWMULTISELECT;
      break;

    case modeSave:
      fos |= FOS_NOREADONLYRETURN;
      
      
      if (IsDefaultPathLink())
        fos |= FOS_NODEREFERENCELINKS;
      break;
  }

  dialog->SetOptions(fos);

  

  
  dialog->SetTitle(mTitle.get());

  
  if (!mDefaultFilename.IsEmpty()) {
    dialog->SetFileName(mDefaultFilename.get());
  }
  
  NS_NAMED_LITERAL_STRING(htmExt, "html");

  
  if (!mDefaultExtension.IsEmpty()) {
    dialog->SetDefaultExtension(mDefaultExtension.get());
  } else if (IsDefaultPathHtml()) {
    dialog->SetDefaultExtension(htmExt.get());
  }

  
  if (!aInitialDir.IsEmpty()) {
    nsRefPtr<IShellItem> folder;
    if (SUCCEEDED(
          WinUtils::SHCreateItemFromParsingName(aInitialDir.get(), nullptr,
                                                IID_IShellItem,
                                                getter_AddRefs(folder)))) {
      dialog->SetFolder(folder);
    }
  }

  
  if (!mComFilterList.IsEmpty()) {
    dialog->SetFileTypes(mComFilterList.Length(), mComFilterList.get());
    dialog->SetFileTypeIndex(mSelectedType);
  }

  

  {
    AutoDestroyTmpWindow adtw((HWND)(mParentWidget.get() ?
      mParentWidget->GetNativeData(NS_NATIVE_TMP_WINDOW) : nullptr));
    AutoTimerCallbackCancel atcc(this, PickerCallbackTimerFunc);
    AutoWidgetPickerState awps(mParentWidget);

    if (FAILED(dialog->Show(adtw.get()))) {
      dialog->Unadvise(mFDECookie);
      return false;
    }
    dialog->Unadvise(mFDECookie);
  }

  

  
  UINT filterIdxResult;
  if (SUCCEEDED(dialog->GetFileTypeIndex(&filterIdxResult))) {
    mSelectedType = (int16_t)filterIdxResult;
  }

  
  if (mMode != modeOpenMultiple) {
    nsRefPtr<IShellItem> item;
    if (FAILED(dialog->GetResult(getter_AddRefs(item))) || !item)
      return false;
    return WinUtils::GetShellItemPath(item, mUnicodeFile);
  }

  
  nsRefPtr<IFileOpenDialog> openDlg;
  dialog->QueryInterface(IID_IFileOpenDialog, getter_AddRefs(openDlg));
  if (!openDlg) {
    
    return false;
  }

  nsRefPtr<IShellItemArray> items;
  if (FAILED(openDlg->GetResults(getter_AddRefs(items))) || !items) {
    return false;
  }

  DWORD count = 0;
  items->GetCount(&count);
  for (unsigned int idx = 0; idx < count; idx++) {
    nsRefPtr<IShellItem> item;
    nsAutoString str;
    if (SUCCEEDED(items->GetItemAt(idx, getter_AddRefs(item)))) {
      if (!WinUtils::GetShellItemPath(item, str))
        continue;
      nsCOMPtr<nsIFile> file = do_CreateInstance("@mozilla.org/file/local;1");
      if (file && NS_SUCCEEDED(file->InitWithPath(str)))
        mFiles.AppendObject(file);
    }
  }
  return true;
}




NS_IMETHODIMP
nsFilePicker::ShowW(int16_t *aReturnVal)
{
  NS_ENSURE_ARG_POINTER(aReturnVal);

  *aReturnVal = returnCancel;

  AutoSuppressEvents supress(mParentWidget);

  nsAutoString initialDir;
  if (mDisplayDirectory)
    mDisplayDirectory->GetPath(initialDir);

  
  if(initialDir.IsEmpty()) {
    
    initialDir = mLastUsedUnicodeDirectory;
  }

  
  mUnicodeFile.Truncate();
  mFiles.Clear();

  
  
  
  
  
  bool result = false, wasInitError = true;
  if (mMode == modeGetFolder) {
    if (IsVistaOrLater())
      result = ShowFolderPicker(initialDir, wasInitError);
    if (!result && wasInitError)
      result = ShowXPFolderPicker(initialDir);
  } else {
    if (IsVistaOrLater())
      result = ShowFilePicker(initialDir, wasInitError);
    if (!result && wasInitError)
      result = ShowXPFilePicker(initialDir);
  }

  
  if (!result)
    return NS_OK;

  RememberLastUsedDirectory();

  int16_t retValue = returnOK;
  if (mMode == modeSave) {
    
    
    nsCOMPtr<nsIFile> file(do_CreateInstance("@mozilla.org/file/local;1"));
    bool flag = false;
    if (file && NS_SUCCEEDED(file->InitWithPath(mUnicodeFile)) &&
        NS_SUCCEEDED(file->Exists(&flag)) && flag) {
      retValue = returnReplace;
    }
  }

  *aReturnVal = retValue;
  return NS_OK;
}

NS_IMETHODIMP
nsFilePicker::Show(int16_t *aReturnVal)
{
  return ShowW(aReturnVal);
}

NS_IMETHODIMP
nsFilePicker::GetFile(nsIFile **aFile)
{
  NS_ENSURE_ARG_POINTER(aFile);
  *aFile = nullptr;

  if (mUnicodeFile.IsEmpty())
      return NS_OK;

  nsCOMPtr<nsIFile> file(do_CreateInstance("@mozilla.org/file/local;1"));
    
  NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

  file->InitWithPath(mUnicodeFile);

  NS_ADDREF(*aFile = file);

  return NS_OK;
}

NS_IMETHODIMP
nsFilePicker::GetFileURL(nsIURI **aFileURL)
{
  *aFileURL = nullptr;
  nsCOMPtr<nsIFile> file;
  nsresult rv = GetFile(getter_AddRefs(file));
  if (!file)
    return rv;

  return NS_NewFileURI(aFileURL, file);
}

NS_IMETHODIMP
nsFilePicker::GetFiles(nsISimpleEnumerator **aFiles)
{
  NS_ENSURE_ARG_POINTER(aFiles);
  return NS_NewArrayEnumerator(aFiles, mFiles);
}


NS_IMETHODIMP
nsBaseWinFilePicker::SetDefaultString(const nsAString& aString)
{
  mDefaultFilePath = aString;

  
  int32_t nameLength;
  int32_t nameIndex = mDefaultFilePath.RFind("\\");
  if (nameIndex == kNotFound)
    nameIndex = 0;
  else
    nameIndex ++;
  nameLength = mDefaultFilePath.Length() - nameIndex;
  mDefaultFilename.Assign(Substring(mDefaultFilePath, nameIndex));
  
  if (nameLength > MAX_PATH) {
    int32_t extIndex = mDefaultFilePath.RFind(".");
    if (extIndex == kNotFound)
      extIndex = mDefaultFilePath.Length();

    
    int32_t charsToRemove = nameLength - MAX_PATH;
    if (extIndex - nameIndex >= charsToRemove) {
      mDefaultFilePath.Cut(extIndex - charsToRemove, charsToRemove);
    }
  }

  
  
  mDefaultFilePath.ReplaceChar(FILE_ILLEGAL_CHARACTERS, '-');
  mDefaultFilename.ReplaceChar(FILE_ILLEGAL_CHARACTERS, '-');

  return NS_OK;
}

NS_IMETHODIMP
nsBaseWinFilePicker::GetDefaultString(nsAString& aString)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsBaseWinFilePicker::GetDefaultExtension(nsAString& aExtension)
{
  aExtension = mDefaultExtension;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseWinFilePicker::SetDefaultExtension(const nsAString& aExtension)
{
  mDefaultExtension = aExtension;
  return NS_OK;
}


NS_IMETHODIMP
nsFilePicker::GetFilterIndex(int32_t *aFilterIndex)
{
  
  *aFilterIndex = mSelectedType - 1;
  return NS_OK;
}

NS_IMETHODIMP
nsFilePicker::SetFilterIndex(int32_t aFilterIndex)
{
  
  mSelectedType = aFilterIndex + 1;
  return NS_OK;
}

void
nsFilePicker::InitNative(nsIWidget *aParent,
                         const nsAString& aTitle)
{
  mParentWidget = aParent;
  mTitle.Assign(aTitle);
}

void 
nsFilePicker::GetQualifiedPath(const wchar_t *aInPath, nsString &aOutPath)
{
  
  
  
  wchar_t qualifiedFileBuffer[MAX_PATH];
  if (PathSearchAndQualifyW(aInPath, qualifiedFileBuffer, MAX_PATH)) {
    aOutPath.Assign(qualifiedFileBuffer);
  } else {
    aOutPath.Assign(aInPath);
  }
}

void
nsFilePicker::AppendXPFilter(const nsAString& aTitle, const nsAString& aFilter)
{
  mFilterList.Append(aTitle);
  mFilterList.Append(char16_t('\0'));

  if (aFilter.EqualsLiteral("..apps"))
    mFilterList.AppendLiteral("*.exe;*.com");
  else
  {
    nsAutoString filter(aFilter);
    filter.StripWhitespace();
    if (filter.EqualsLiteral("*"))
      filter.AppendLiteral(".*");
    mFilterList.Append(filter);
  }

  mFilterList.Append(char16_t('\0'));
}

NS_IMETHODIMP
nsFilePicker::AppendFilter(const nsAString& aTitle, const nsAString& aFilter)
{
  if (IsVistaOrLater()) {
    mComFilterList.Append(aTitle, aFilter);
  } else {
    AppendXPFilter(aTitle, aFilter);
  }
  return NS_OK;
}

void
nsFilePicker::RememberLastUsedDirectory()
{
  nsCOMPtr<nsIFile> file(do_CreateInstance("@mozilla.org/file/local;1"));
  if (!file || NS_FAILED(file->InitWithPath(mUnicodeFile))) {
    NS_WARNING("RememberLastUsedDirectory failed to init file path.");
    return;
  }

  nsCOMPtr<nsIFile> dir;
  nsAutoString newDir;
  if (NS_FAILED(file->GetParent(getter_AddRefs(dir))) ||
      !(mDisplayDirectory = do_QueryInterface(dir)) ||
      NS_FAILED(mDisplayDirectory->GetPath(newDir)) ||
      newDir.IsEmpty()) {
    NS_WARNING("RememberLastUsedDirectory failed to get parent directory.");
    return;
  }

  if (mLastUsedUnicodeDirectory) {
    free(mLastUsedUnicodeDirectory);
    mLastUsedUnicodeDirectory = nullptr;
  }
  mLastUsedUnicodeDirectory = ToNewUnicode(newDir);
}

bool
nsFilePicker::IsPrivacyModeEnabled()
{
  return mLoadContext && mLoadContext->UsePrivateBrowsing();
}

bool
nsFilePicker::IsDefaultPathLink()
{
  NS_ConvertUTF16toUTF8 ext(mDefaultFilePath);
  ext.Trim(" .", false, true); 
  ToLowerCase(ext);
  if (StringEndsWith(ext, NS_LITERAL_CSTRING(".lnk")) ||
      StringEndsWith(ext, NS_LITERAL_CSTRING(".pif")) ||
      StringEndsWith(ext, NS_LITERAL_CSTRING(".url")))
    return true;
  return false;
}

bool
nsFilePicker::IsDefaultPathHtml()
{
  int32_t extIndex = mDefaultFilePath.RFind(".");
  if (extIndex >= 0) {
    nsAutoString ext;
    mDefaultFilePath.Right(ext, mDefaultFilePath.Length() - extIndex);
    if (ext.LowerCaseEqualsLiteral(".htm")  ||
        ext.LowerCaseEqualsLiteral(".html") ||
        ext.LowerCaseEqualsLiteral(".shtml"))
      return true;
  }
  return false;
}

void
nsFilePicker::ComDlgFilterSpec::Append(const nsAString& aTitle, const nsAString& aFilter)
{
  COMDLG_FILTERSPEC* pSpecForward = mSpecList.AppendElement();
  if (!pSpecForward) {
    NS_WARNING("mSpecList realloc failed.");
    return;
  }
  memset(pSpecForward, 0, sizeof(*pSpecForward));
  nsString* pStr = mStrings.AppendElement(aTitle);
  if (!pStr) {
    NS_WARNING("mStrings.AppendElement failed.");
    return;
  }
  pSpecForward->pszName = pStr->get();
  pStr = mStrings.AppendElement(aFilter);
  if (!pStr) {
    NS_WARNING("mStrings.AppendElement failed.");
    return;
  }
  if (aFilter.EqualsLiteral("..apps"))
    pStr->AssignLiteral("*.exe;*.com");
  else {
    pStr->StripWhitespace();
    if (pStr->EqualsLiteral("*"))
      pStr->AppendLiteral(".*");
  }
  pSpecForward->pszSpec = pStr->get();
}
