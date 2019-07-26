





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

#ifdef SHOW_CONSOLE
#define DEBUG_DELAY_SHUTDOWN 1
#endif


#define HEARTBEAT_MSEC 250

#define REQUEST_WAIT_TIMEOUT 30

#define APP_REG_NAME L"Firefox"

const WCHAR* kFirefoxExe = L"firefox.exe";
static const WCHAR* kDefaultMetroBrowserIDPathKey = L"FirefoxURL";
static const WCHAR* kMetroRestartCmdLine = L"--metro-restart";
static const WCHAR* kMetroUpdateCmdLine = L"--metro-update";
static const WCHAR* kDesktopRestartCmdLine = L"--desktop-restart";
static const WCHAR* kNsisLaunchCmdLine = L"--launchmetro";
static const WCHAR* kExplorerLaunchCmdLine = L"-Embedding";

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
    mRef(0),
    mShellItemArray(nullptr),
    mUnkSite(nullptr),
    mTargetIsFileSystemLink(false),
    mTargetIsDefaultBrowser(false),
    mTargetIsBrowser(false),
    mRequestType(DEFAULT_LAUNCH),
    mRequestMet(false),
    mDelayedLaunchType(NONE),
    mVerb(L"open")
  {
  }

  ~CExecuteCommandVerb()
  {
  }

  bool RequestMet() { return mRequestMet; }
  void SetRequestMet();
  long RefCount() { return mRef; }
  void HeartBeat();

  
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

    if (!_wcsicmp(aParameters, kMetroRestartCmdLine)) {
      mRequestType = METRO_RESTART;
    } else if (_wcsicmp(aParameters, kMetroUpdateCmdLine) == 0) {
      mRequestType = METRO_UPDATE;
    } else if (_wcsicmp(aParameters, kDesktopRestartCmdLine) == 0) {
      mRequestType = DESKTOP_RESTART;
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
    return S_OK;
  }

  





  AHE_TYPE GetLaunchType()
  {
    AHE_TYPE ahe = GetLastAHE();
    Log(L"Previous AHE: %d", ahe);

    
    
    if (mRequestType == DESKTOP_RESTART) {
      Log(L"Restarting in desktop host environment.");
      return AHE_DESKTOP;
    } else if (mRequestType == METRO_RESTART) {
      Log(L"Restarting in metro host environment.");
      ahe = AHE_IMMERSIVE;
    } else if (mRequestType == METRO_UPDATE) {
      
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

  bool DefaultLaunchIsDesktop()
  {
    return GetLaunchType() == AHE_DESKTOP;
  }

  bool DefaultLaunchIsMetro()
  {
    return GetLaunchType() == AHE_IMMERSIVE;
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

  



  void CommandLineMetroLaunch()
  {
    mTargetIsDefaultBrowser = true;
    LaunchMetroBrowser();
  }

private:
  void LaunchDesktopBrowser();
  bool LaunchMetroBrowser();
  bool SetTargetPath(IShellItem* aItem);
  bool TestForUpdateLock();

  


  enum RequestType {
    DEFAULT_LAUNCH,
    DESKTOP_RESTART,
    METRO_RESTART,
    METRO_UPDATE,
  };

  RequestType mRequestType;

  


  enum DelayedLaunchType {
    NONE,
    DESKTOP,
    METRO,
  };

  DelayedLaunchType mDelayedLaunchType;

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





void LaunchDesktopBrowserWithParams(CStringW& aBrowserPath, CStringW& aVerb,
                                    CStringW& aTarget, CStringW& aParameters,
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

  Log(L"Desktop Launch: verb:'%s' exe:'%s' params:'%s'", aVerb, aBrowserPath, params);

  
  
  if (!_wcsicmp(aTarget, L"http://-desktop/")) {
    
    params.Empty();
  }

  PROCESS_INFORMATION procInfo;
  STARTUPINFO startInfo;
  memset(&procInfo, 0, sizeof(PROCESS_INFORMATION));
  memset(&startInfo, 0, sizeof(STARTUPINFO));

  startInfo.cb = sizeof(STARTUPINFO);
  startInfo.dwFlags = STARTF_USESHOWWINDOW;
  startInfo.wShowWindow = SW_SHOWNORMAL;

  BOOL result =
    CreateProcessW(aBrowserPath, static_cast<LPWSTR>(params.GetBuffer()),
                   NULL, NULL, FALSE, 0, NULL, NULL, &startInfo, &procInfo);
  if (!result) {
    Log(L"CreateProcess failed! (%d)", GetLastError());
    return;
  }
  
  
  
  if (!AllowSetForegroundWindow(procInfo.dwProcessId)) {
    Log(L"AllowSetForegroundWindow failed! (%d)", GetLastError());
  }
  CloseHandle(procInfo.hThread);
  CloseHandle(procInfo.hProcess);
  Log(L"Desktop browser process id: %d", procInfo.dwProcessId);
}

void
CExecuteCommandVerb::LaunchDesktopBrowser()
{
  CStringW browserPath;
  if (!GetDesktopBrowserPath(browserPath)) {
    return;
  }

  LaunchDesktopBrowserWithParams(browserPath, mVerb, mTarget, mParameters,
                                 mTargetIsDefaultBrowser, mTargetIsBrowser);
}

void
CExecuteCommandVerb::HeartBeat()
{
  if (mRequestType == METRO_UPDATE && mDelayedLaunchType == DESKTOP &&
      !IsMetroProcessRunning()) {
    mDelayedLaunchType = NONE;
    LaunchDesktopBrowser();
    SetRequestMet();
  }
  if (mDelayedLaunchType == METRO && !TestForUpdateLock()) {
    mDelayedLaunchType = NONE;
    LaunchMetroBrowser();
    SetRequestMet();
  }
}

bool
CExecuteCommandVerb::TestForUpdateLock()
{
  CStringW browserPath;
  if (!GetDefaultBrowserPath(browserPath)) {
    return false;
  }

  HANDLE hFile = CreateFileW(browserPath,
                             FILE_EXECUTE, FILE_SHARE_READ|FILE_SHARE_WRITE,
                             nullptr, OPEN_EXISTING, 0, nullptr);
  if (hFile != INVALID_HANDLE_VALUE) {
    CloseHandle(hFile);
    return false;
  }
  return true;
}

bool
CExecuteCommandVerb::LaunchMetroBrowser()
{
  HRESULT hr;

  CComPtr<IApplicationActivationManager> activateMgr;
  hr = activateMgr.CoCreateInstance(CLSID_ApplicationActivationManager,
                                    nullptr, CLSCTX_LOCAL_SERVER);
  if (FAILED(hr)) {
    Log(L"CoCreateInstance failed, launching on desktop.");
    return false;
  }

  
  
  hr = CoAllowSetForegroundWindow(activateMgr, nullptr);
  if (FAILED(hr)) {
    Log(L"CoAllowSetForegroundWindow result %X", hr);
  }

  WCHAR appModelID[256];
  if (!GetDefaultBrowserAppModelID(appModelID)) {
    Log(L"GetDefaultBrowserAppModelID failed.");
    return false;
  }

  Log(L"Metro Launch: verb:'%s' appid:'%s' params:'%s'", mVerb, appModelID, mTarget);

  
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
  return true;
}

void CExecuteCommandVerb::SetRequestMet()
{
  SafeRelease(&mShellItemArray);
  SafeRelease(&mUnkSite);
  mRequestMet = true;
  Log(L"Request met, exiting.");
}

IFACEMETHODIMP CExecuteCommandVerb::Execute()
{
  Log(L"Execute()");

  if (!mTarget.GetLength()) {
    
    SetRequestMet();
    return E_FAIL;
  }

  if (!IsDX10Available()) {
    Log(L"Can't launch in metro due to missing hardware acceleration features.");
    mRequestType = DESKTOP_RESTART;
  } 

  
  
  if (mRequestType == METRO_UPDATE) {
    
    
    
    
    mParameters = kMetroUpdateCmdLine;
    mDelayedLaunchType = DESKTOP;
    return S_OK;
  }

  
  if (mRequestType == DESKTOP_RESTART ||
      (mRequestType == DEFAULT_LAUNCH && DefaultLaunchIsDesktop())) {
    LaunchDesktopBrowser();
    SetRequestMet();
    return S_OK;
  }

  
  
  if (TestForUpdateLock()) {
    mDelayedLaunchType = METRO;
    return S_OK;
  }

  LaunchMetroBrowser();
  SetRequestMet();
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

  
  if (pszCmdLine && StrStrI(pszCmdLine, kNsisLaunchCmdLine))
  {
    CoInitialize(nullptr);
    CExecuteCommandVerb *pHandler = new CExecuteCommandVerb();
    if (!pHandler)
      return E_OUTOFMEMORY;
    pHandler->CommandLineMetroLaunch();
    delete pHandler;
    CoUninitialize();
    return 0;
  }

  if (!wcslen(pszCmdLine) || StrStrI(pszCmdLine, kExplorerLaunchCmdLine))
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
          pHandler->HeartBeat();
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
