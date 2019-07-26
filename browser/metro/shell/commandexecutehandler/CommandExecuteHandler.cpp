





#include "CEHHelper.h"

#include <objbase.h>
#include <combaseapi.h>
#include <atlcore.h>
#include <atlstr.h>
#include <wininet.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <propkey.h>
#include <propvarutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>
#include <io.h>
#include <shellapi.h>
#include <wininet.h>

#ifdef SHOW_CONSOLE
#define DEBUG_DELAY_SHUTDOWN 1
#endif


#define HEARTBEAT_MSEC 1000

#define REQUEST_WAIT_TIMEOUT 30

#define APP_REG_NAME L"Firefox"





#define RESTART_WAIT_PER_RETRY 50
#define RESTART_WAIT_TIMEOUT 38000

static const WCHAR* kFirefoxExe = L"firefox.exe";
static const WCHAR* kMetroFirefoxExe = L"firefox.exe";
static const WCHAR* kDefaultMetroBrowserIDPathKey = L"FirefoxURL";
static const WCHAR* kMetroRestartCmdLine = L"--metro-restart";
static const WCHAR* kDesktopRestartCmdLine = L"--desktop-restart";

static bool GetDefaultBrowserPath(CStringW& aPathBuffer);






static bool GetModulePath(CStringW& aPathBuffer)
{
  WCHAR buffer[MAX_PATH];
  memset(buffer, 0, sizeof(buffer));

  if (!GetModuleFileName(nullptr, buffer, MAX_PATH)) {
    Log(L"GetModuleFileName failed.");
    return false;
  }

  WCHAR* slash = wcsrchr(buffer, '\\');
  if (!slash)
    return false;
  *slash = '\0';

  aPathBuffer = buffer;
  return true;
}


template <class T>void SafeRelease(T **ppT)
{
  if (*ppT) {
    (*ppT)->Release();
    *ppT = nullptr;
  }
}

template <class T> HRESULT SetInterface(T **ppT, IUnknown *punk)
{
  SafeRelease(ppT);
  return punk ? punk->QueryInterface(ppT) : E_NOINTERFACE;
}

class __declspec(uuid("5100FEC1-212B-4BF5-9BF8-3E650FD794A3"))
  CExecuteCommandVerb : public IExecuteCommand,
                        public IObjectWithSelection,
                        public IInitializeCommand,
                        public IObjectWithSite,
                        public IExecuteCommandApplicationHostEnvironment
{
public:

  CExecuteCommandVerb() :
    mRef(1),
    mShellItemArray(nullptr),
    mUnkSite(nullptr),
    mTargetIsFileSystemLink(false),
    mTargetIsDefaultBrowser(false),
    mTargetIsBrowser(false),
    mIsDesktopRequest(true),
    mIsRestartMetroRequest(false),
    mIsRestartDesktopRequest(false),
    mRequestMet(false),
    mVerb(L"open")
  {
  }

  bool RequestMet() { return mRequestMet; }
  long RefCount() { return mRef; }

  
  IFACEMETHODIMP QueryInterface(REFIID aRefID, void **aInt)
  {
    static const QITAB qit[] = {
      QITABENT(CExecuteCommandVerb, IExecuteCommand),
      QITABENT(CExecuteCommandVerb, IObjectWithSelection),
      QITABENT(CExecuteCommandVerb, IInitializeCommand),
      QITABENT(CExecuteCommandVerb, IObjectWithSite),
      QITABENT(CExecuteCommandVerb, IExecuteCommandApplicationHostEnvironment),
      { 0 },
    };
    return QISearch(this, qit, aRefID, aInt);
  }

  IFACEMETHODIMP_(ULONG) AddRef()
  {
    return InterlockedIncrement(&mRef);
  }

  IFACEMETHODIMP_(ULONG) Release()
  {
    long cRef = InterlockedDecrement(&mRef);
    if (!cRef) {
      delete this;
    }
    return cRef;
  }

  
  IFACEMETHODIMP SetKeyState(DWORD aKeyState)
  {
    mKeyState = aKeyState;
    return S_OK;
  }

  IFACEMETHODIMP SetParameters(PCWSTR aParameters)
  {
    Log(L"SetParameters: '%s'", aParameters);

    if (_wcsicmp(aParameters, kMetroRestartCmdLine) == 0) {
      mIsRestartMetroRequest = true;
    } else if (_wcsicmp(aParameters, kDesktopRestartCmdLine) == 0) {
      mIsRestartDesktopRequest = true;
    } else {
      mParameters = aParameters;
    }
    return S_OK;
  }

  IFACEMETHODIMP SetPosition(POINT aPoint)
  { return S_OK; }

  IFACEMETHODIMP SetShowWindow(int aShowFlag)
  { return S_OK; }

  IFACEMETHODIMP SetNoShowUI(BOOL aNoUI)
  { return S_OK; }

  IFACEMETHODIMP SetDirectory(PCWSTR aDirPath)
  { return S_OK; }

  IFACEMETHODIMP Execute();

  
  IFACEMETHODIMP SetSelection(IShellItemArray *aArray)
  {
    if (!aArray) {
      return E_FAIL;
    }

    SetInterface(&mShellItemArray, aArray);

    DWORD count = 0;
    aArray->GetCount(&count);
    if (!count) {
      return E_FAIL;
    }

#ifdef SHOW_CONSOLE
    Log(L"SetSelection param count: %d", count);
    for (DWORD idx = 0; idx < count; idx++) {
      IShellItem* item = nullptr;
      if (SUCCEEDED(aArray->GetItemAt(idx, &item))) {
        LPWSTR str = nullptr;
        if (FAILED(item->GetDisplayName(SIGDN_FILESYSPATH, &str))) {
          if (FAILED(item->GetDisplayName(SIGDN_URL, &str))) {
            Log(L"Failed to get a shell item array item.");
            item->Release();
            continue;
          }
        }
        item->Release();
        Log(L"SetSelection param: '%s'", str);
        CoTaskMemFree(str);
      }
    }
#endif

    IShellItem* item = nullptr;
    if (FAILED(aArray->GetItemAt(0, &item))) {
      return E_FAIL;
    }

    bool isFileSystem = false;
    if (!SetTargetPath(item) || !mTarget.GetLength()) {
      Log(L"SetTargetPath failed.");
      return E_FAIL;
    }
    item->Release();

    Log(L"SetSelection target: %s", mTarget);
    return S_OK;
  }

  IFACEMETHODIMP GetSelection(REFIID aRefID, void **aInt)
  {
    *aInt = nullptr;
    return mShellItemArray ? mShellItemArray->QueryInterface(aRefID, aInt) : E_FAIL;
  }

  
  IFACEMETHODIMP Initialize(PCWSTR aVerb, IPropertyBag* aPropBag)
  {
    if (!aVerb)
      return E_FAIL;
    
    Log(L"Initialize(%s)", aVerb);
    mVerb = aVerb;
    return S_OK;
  }

  
  IFACEMETHODIMP SetSite(IUnknown *aUnkSite)
  {
    SetInterface(&mUnkSite, aUnkSite);
    return S_OK;
  }

  IFACEMETHODIMP GetSite(REFIID aRefID, void **aInt)
  {
    *aInt = nullptr;
    return mUnkSite ? mUnkSite->QueryInterface(aRefID, aInt) : E_FAIL;
  }

  
  IFACEMETHODIMP GetValue(AHE_TYPE *aLaunchType)
  {
    Log(L"IExecuteCommandApplicationHostEnvironment::GetValue()");
    *aLaunchType = GetLaunchType();
    mIsDesktopRequest = (*aLaunchType == AHE_DESKTOP);
    SetLastAHE(*aLaunchType);
    return S_OK;
  }

  



  AHE_TYPE GetLaunchType() {
    AHE_TYPE ahe = GetLastAHE();
    Log(L"Previous AHE: %d", ahe);

    if (!mIsRestartMetroRequest && IsProcessRunning(kFirefoxExe, false)) {
      Log(L"Returning AHE_DESKTOP because desktop is already running");
      return AHE_DESKTOP;
    } else if (!mIsRestartDesktopRequest && IsProcessRunning(kMetroFirefoxExe, true)) {
      Log(L"Returning AHE_IMMERSIVE because Metro is already running");
      return AHE_IMMERSIVE;
    }

    if (mIsRestartDesktopRequest) {
      Log(L"Restarting in desktop host environment.");
      return AHE_DESKTOP;
    }

    if (mIsRestartMetroRequest) {
      Log(L"Restarting in metro host environment.");
      ahe = AHE_IMMERSIVE;
    }

    if (ahe == AHE_IMMERSIVE) {
      if (!IsDefaultBrowser()) {
        Log(L"returning AHE_DESKTOP because we are not the default browser");
        return AHE_DESKTOP;
      }

      if (!IsDX10Available()) {
        Log(L"returning AHE_DESKTOP because DX10 is not available");
        return AHE_DESKTOP;
      }
    }
    return ahe;
  }

  










  bool GetDesktopBrowserPath(CStringW& aPathBuffer)
  {
    
    

    if (mTargetIsDefaultBrowser || mTargetIsBrowser) {
      aPathBuffer = mTarget;
      return true;
    }

    if (!GetModulePath(aPathBuffer))
      return false;

    
    
    aPathBuffer.Append(L"\\");
    aPathBuffer.Append(kFirefoxExe);
    return true;
  }

  bool IsDefaultBrowser()
  {
    IApplicationAssociationRegistration* pAAR;
    HRESULT hr = CoCreateInstance(CLSID_ApplicationAssociationRegistration,
                                  nullptr,
                                  CLSCTX_INPROC,
                                  IID_IApplicationAssociationRegistration,
                                  (void**)&pAAR);
    if (FAILED(hr))
      return false;

    BOOL res = FALSE;
    hr = pAAR->QueryAppIsDefaultAll(AL_EFFECTIVE,
                                    APP_REG_NAME,
                                    &res);
    Log(L"QueryAppIsDefaultAll: %d", res);
    if (!res) {
      pAAR->Release();
      return false;
    }
    
    LPWSTR registeredApp;
    hr = pAAR->QueryCurrentDefault(L"http", AT_URLPROTOCOL, AL_EFFECTIVE,
                                    &registeredApp);
    pAAR->Release();
    Log(L"QueryCurrentDefault: %X", hr);
    if (FAILED(hr))
      return false;

    Log(L"registeredApp=%s", registeredApp);
    bool result = !wcsicmp(registeredApp, kDefaultMetroBrowserIDPathKey);
    CoTaskMemFree(registeredApp);
    if (!result)
      return false;

    
    
    CStringW selfPath;
    GetDesktopBrowserPath(selfPath);
    CStringW browserPath;
    GetDefaultBrowserPath(browserPath);

    return !selfPath.CompareNoCase(browserPath);
  }
private:
  ~CExecuteCommandVerb()
  {
    SafeRelease(&mShellItemArray);
    SafeRelease(&mUnkSite);
  }

  void LaunchDesktopBrowser();
  bool SetTargetPath(IShellItem* aItem);

  long mRef;
  IShellItemArray *mShellItemArray;
  IUnknown *mUnkSite;
  CStringW mVerb;
  CStringW mTarget;
  CStringW mParameters;
  bool mTargetIsFileSystemLink;
  bool mTargetIsDefaultBrowser;
  bool mTargetIsBrowser;
  DWORD mKeyState;
  bool mIsDesktopRequest;
  bool mIsRestartMetroRequest;
  bool mIsRestartDesktopRequest;
  bool mRequestMet;
};






static bool GetDefaultBrowserPath(CStringW& aPathBuffer)
{
  WCHAR buffer[MAX_PATH];
  DWORD length = MAX_PATH;

  if (FAILED(AssocQueryStringW(ASSOCF_NOTRUNCATE | ASSOCF_INIT_IGNOREUNKNOWN,
                               ASSOCSTR_EXECUTABLE,
                               kDefaultMetroBrowserIDPathKey, nullptr,
                               buffer, &length))) {
    Log(L"AssocQueryString failed.");
    return false;
  }

  
  if (lstrcmpiW(PathFindFileNameW(buffer), kFirefoxExe))
    return false;

  aPathBuffer = buffer;
  return true;
}







template <size_t N>
static bool GetDefaultBrowserAppModelID(WCHAR (&aIDBuffer)[N])
{
  HKEY key;
  if (RegOpenKeyExW(HKEY_CLASSES_ROOT, kDefaultMetroBrowserIDPathKey,
                    0, KEY_READ, &key) != ERROR_SUCCESS) {
    return false;
  }
  DWORD len = sizeof(aIDBuffer);
  memset(aIDBuffer, 0, len);
  if (RegQueryValueExW(key, L"AppUserModelID", nullptr, nullptr,
                       (LPBYTE)aIDBuffer, &len) != ERROR_SUCCESS || !len) {
    RegCloseKey(key);
    return false;
  }
  RegCloseKey(key);
  return true;
}

namespace {
  const FORMATETC kPlainTextFormat =
    {CF_TEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  const FORMATETC kPlainTextWFormat =
    {CF_UNICODETEXT, 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
}

bool HasPlainText(IDataObject* aDataObj) {
  return SUCCEEDED(aDataObj->QueryGetData((FORMATETC*)&kPlainTextWFormat)) ||
      SUCCEEDED(aDataObj->QueryGetData((FORMATETC*)&kPlainTextFormat));
}

bool GetPlainText(IDataObject* aDataObj, CStringW& cstrText)
{
  if (!HasPlainText(aDataObj))
    return false;

  STGMEDIUM store;

  
  if (SUCCEEDED(aDataObj->GetData((FORMATETC*)&kPlainTextWFormat, &store))) {
    
    cstrText = static_cast<LPCWSTR>(GlobalLock(store.hGlobal));
    GlobalUnlock(store.hGlobal);
    ReleaseStgMedium(&store);
    return true;
  }

  
  if (SUCCEEDED(aDataObj->GetData((FORMATETC*)&kPlainTextFormat, &store))) {
    
    cstrText = static_cast<char*>(GlobalLock(store.hGlobal));
    GlobalUnlock(store.hGlobal);
    ReleaseStgMedium(&store);
    return true;
  }

  return false;
}





bool CExecuteCommandVerb::SetTargetPath(IShellItem* aItem)
{
  if (!aItem)
    return false;

  CString cstrText;
  CComPtr<IDataObject> object;
  
  
  if (SUCCEEDED(aItem->BindToHandler(nullptr, BHID_DataObject,
                                     IID_IDataObject,
                                     reinterpret_cast<void**>(&object))) &&
      GetPlainText(object, cstrText)) {
    wchar_t scheme[16];
    URL_COMPONENTS components = {0};
    components.lpszScheme = scheme;
    components.dwSchemeLength = sizeof(scheme)/sizeof(scheme[0]);
    components.dwStructSize = sizeof(components);
    
    if (!InternetCrackUrlW(cstrText, 0, 0, &components)) {
      Log(L"Failed to identify object text '%s'", cstrText);
      return false;
    }

    mTargetIsFileSystemLink = (components.nScheme == INTERNET_SCHEME_FILE);
    mTarget = cstrText;

    return true;
  }

  Log(L"No data object or data object has no text.");

  
  LPWSTR str = nullptr;
  mTargetIsFileSystemLink = true;
  if (FAILED(aItem->GetDisplayName(SIGDN_FILESYSPATH, &str))) {
    mTargetIsFileSystemLink = false;
    if (FAILED(aItem->GetDisplayName(SIGDN_URL, &str))) {
      Log(L"Failed to get parameter string.");
      return false;
    }
  }
  mTarget = str;
  CoTaskMemFree(str);

  CStringW defaultPath;
  GetDefaultBrowserPath(defaultPath);
  mTargetIsDefaultBrowser = !mTarget.CompareNoCase(defaultPath);

  size_t browserEXELen = wcslen(kFirefoxExe);
  mTargetIsBrowser = mTarget.GetLength() >= browserEXELen &&
                     !mTarget.Right(browserEXELen).CompareNoCase(kFirefoxExe);

  return true;
}





void LaunchDesktopBrowserWithParams(CStringW& aBrowserPath, CStringW& aVerb, CStringW& aTarget, CStringW& aParameters,
                                    bool aTargetIsDefaultBrowser, bool aTargetIsBrowser)
{
  
  
  
  
  CStringW params;
  if (!aTargetIsDefaultBrowser && !aTargetIsBrowser && !aTarget.IsEmpty()) {
    
    GetDefaultBrowserPath(aBrowserPath);
    params += "-url ";
    params += "\"";
    params += aTarget;
    params += "\"";
  }

  
  if (!aParameters.IsEmpty()) {
    params += " ";
    params += aParameters;
  }

  Log(L"Desktop Launch: verb:%s exe:%s params:%s", aVerb, aBrowserPath, params);

  SHELLEXECUTEINFOW seinfo;
  memset(&seinfo, 0, sizeof(seinfo));
  seinfo.cbSize = sizeof(SHELLEXECUTEINFOW);
  seinfo.fMask  = SEE_MASK_FLAG_LOG_USAGE;
  seinfo.lpVerb = aVerb;
  seinfo.lpFile = aBrowserPath;
  seinfo.nShow  = SW_SHOWNORMAL;

  
  
  if (_wcsicmp(aTarget, L"http://-desktop/") != 0) {
    seinfo.lpParameters = params;
  }

  ShellExecuteEx(&seinfo);
}

void CExecuteCommandVerb::LaunchDesktopBrowser()
{
  CStringW browserPath;
  if (!GetDesktopBrowserPath(browserPath)) {
    return;
  }

  LaunchDesktopBrowserWithParams(browserPath, mVerb, mTarget, mParameters, mTargetIsDefaultBrowser, mTargetIsBrowser);
}

class AutoSetRequestMet
{
public:
  explicit AutoSetRequestMet(bool* aFlag) :
    mFlag(aFlag) {}
  ~AutoSetRequestMet() { if (mFlag) *mFlag = true; }
private:
  bool* mFlag;
};

static HRESULT
PrepareActivationManager(CComPtr<IApplicationActivationManager> &activateMgr)
{
  HRESULT hr = activateMgr.CoCreateInstance(CLSID_ApplicationActivationManager,
                                            nullptr, CLSCTX_LOCAL_SERVER);
  if (FAILED(hr)) {
    Log(L"CoCreateInstance failed, launching on desktop.");
    return E_FAIL;
  }

  
  
  hr = CoAllowSetForegroundWindow(activateMgr, nullptr);
  if (FAILED(hr)) {
    Log(L"CoAllowSetForegroundWindow result %X", hr);
    return E_FAIL;
  }

  return S_OK;
}

DWORD WINAPI
DelayedExecuteThread(LPVOID param)
{
  Log(L"Starting delayed execute thread...");
  bool &bRequestMet(*(bool*)param);
  AutoSetRequestMet asrm(&bRequestMet);

  CoInitialize(nullptr);

  CComPtr<IApplicationActivationManager> activateMgr;
  if (FAILED(PrepareActivationManager(activateMgr))) {
      Log(L"Warning: Could not prepare activation manager");
  }

  size_t currentWaitTime = 0;
  while(currentWaitTime < RESTART_WAIT_TIMEOUT) {
    if (!IsProcessRunning(kMetroFirefoxExe, true))
      break;
    currentWaitTime += RESTART_WAIT_PER_RETRY;
    Sleep(RESTART_WAIT_PER_RETRY);
  }

  Log(L"Done waiting, getting app ID");
  
  WCHAR appModelID[256];
  if (GetDefaultBrowserAppModelID(appModelID)) {
    Log(L"Activating application");
    DWORD processID;
    HRESULT hr = activateMgr->ActivateApplication(appModelID, L"", AO_NOSPLASHSCREEN, &processID);
    if (SUCCEEDED(hr)) {
      Log(L"Activate application succeeded");
    } else {
      Log(L"Activate application failed! (%x)", hr);
    }
  }

  CoUninitialize();
  return 0;
}

IFACEMETHODIMP CExecuteCommandVerb::Execute()
{
  Log(L"Execute()");

  if (!mTarget.GetLength()) {
    
    mRequestMet = true;
    return E_FAIL;
  }

  if (mIsRestartMetroRequest) {
    HANDLE thread = CreateThread(nullptr, 0, DelayedExecuteThread,
                                 &mRequestMet, 0, nullptr);
    CloseHandle(thread);
    return S_OK;
  }

  if (mIsRestartDesktopRequest) {
    CStringW browserPath;
    if (!GetDesktopBrowserPath(browserPath)) {
      return E_FAIL;
    }

    LaunchDesktopBrowserWithParams(browserPath,
                                   mVerb,
                                   mTarget,
                                   mParameters,
                                   mTargetIsDefaultBrowser,
                                   mTargetIsBrowser);
    return S_OK;
  }

  
  AutoSetRequestMet asrm(&mRequestMet);

  
  if (mIsDesktopRequest) {
    LaunchDesktopBrowser();
    return S_OK;
  }

  CComPtr<IApplicationActivationManager> activateMgr;
  if (FAILED(PrepareActivationManager(activateMgr))) {
      LaunchDesktopBrowser();
      return S_OK;
  }

  HRESULT hr;
  WCHAR appModelID[256];
  if (!GetDefaultBrowserAppModelID(appModelID)) {
    Log(L"GetDefaultBrowserAppModelID failed, launching on desktop.");
    LaunchDesktopBrowser();
    return S_OK;
  }

  Log(L"Metro Launch: verb:%s appid:%s params:%s", mVerb, appModelID, mTarget);

  
  DWORD processID;
  if (mTargetIsDefaultBrowser) {
    hr = activateMgr->ActivateApplication(appModelID, L"", AO_NONE, &processID);
    Log(L"ActivateApplication result %X", hr);
  
  } else if (mTargetIsFileSystemLink) {
    hr = activateMgr->ActivateForFile(appModelID, mShellItemArray, mVerb, &processID);
    Log(L"ActivateForFile result %X", hr);
  
  } else {
    hr = activateMgr->ActivateForProtocol(appModelID, mShellItemArray, &processID);
    Log(L"ActivateForProtocol result %X", hr);
  }
  return S_OK;
}

class ClassFactory : public IClassFactory 
{
public:
  ClassFactory(IUnknown *punkObject);
  ~ClassFactory();
  STDMETHODIMP Register(CLSCTX classContent, REGCLS classUse);
  STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
  STDMETHODIMP_(ULONG) AddRef() { return 2; }
  STDMETHODIMP_(ULONG) Release() { return 1; }
  STDMETHODIMP CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv);
  STDMETHODIMP LockServer(BOOL);
private:
  IUnknown* mUnkObject;
  DWORD mRegID;
};

ClassFactory::ClassFactory(IUnknown* aUnkObj) :
  mUnkObject(aUnkObj),
  mRegID(0)
{
  if (mUnkObject) {
    mUnkObject->AddRef();
  }
}

ClassFactory::~ClassFactory()
{
  if (mRegID) {
    CoRevokeClassObject(mRegID);
  }
  mUnkObject->Release();
}

STDMETHODIMP
ClassFactory::Register(CLSCTX aClass, REGCLS aUse)
{
  return CoRegisterClassObject(__uuidof(CExecuteCommandVerb),
                               static_cast<IClassFactory *>(this),
                               aClass, aUse, &mRegID);
}

STDMETHODIMP
ClassFactory::QueryInterface(REFIID riid, void **ppv)
{
  IUnknown *punk = nullptr;
  if (riid == IID_IUnknown || riid == IID_IClassFactory) {
    punk = static_cast<IClassFactory*>(this);
  }
  *ppv = punk;
  if (punk) {
    punk->AddRef();
    return S_OK;
  } else {
    return E_NOINTERFACE;
  }
}

STDMETHODIMP
ClassFactory::CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
  *ppv = nullptr;
  if (punkOuter)
    return CLASS_E_NOAGGREGATION;
  return mUnkObject->QueryInterface(riid, ppv);
}

LONG gObjRefCnt;

STDMETHODIMP
ClassFactory::LockServer(BOOL fLock)
{
  if (fLock)
    InterlockedIncrement(&gObjRefCnt);
  else
    InterlockedDecrement(&gObjRefCnt);
  Log(L"ClassFactory::LockServer() %d", gObjRefCnt);
  return S_OK;
}

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, PWSTR pszCmdLine, int)
{
#if defined(SHOW_CONSOLE)
  SetupConsole();
#endif
  

  if (!wcslen(pszCmdLine) || StrStrI(pszCmdLine, L"-Embedding"))
  {
      CoInitialize(nullptr);

      CExecuteCommandVerb *pHandler = new CExecuteCommandVerb();
      if (!pHandler)
        return E_OUTOFMEMORY;

      IUnknown* ppi;
      pHandler->QueryInterface(IID_IUnknown, (void**)&ppi);
      if (!ppi)
        return E_FAIL;

      ClassFactory classFactory(ppi);
      ppi->Release();
      ppi = nullptr;

      
      if (FAILED(classFactory.Register(CLSCTX_LOCAL_SERVER, REGCLS_SINGLEUSE)))
        return -1;

      if (!SetTimer(nullptr, 1, HEARTBEAT_MSEC, nullptr)) {
        Log(L"Failed to set timer, can't process request.");
        return -1;
      }

      MSG msg;
      long beatCount = 0;
      while (GetMessage(&msg, 0, 0, 0) > 0) {
        if (msg.message == WM_TIMER) {
          if (++beatCount > REQUEST_WAIT_TIMEOUT ||
              (pHandler->RequestMet() && pHandler->RefCount() < 2)) {
            break;
          }
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }

#ifdef DEBUG_DELAY_SHUTDOWN
      Sleep(10000);
#endif
      CoUninitialize();
      return 0;
  }
  return 0;
}
