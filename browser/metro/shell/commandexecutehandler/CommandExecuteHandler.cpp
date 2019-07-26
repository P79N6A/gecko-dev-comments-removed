





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


#define HEARTBEAT_MSEC 1000

#define REQUEST_WAIT_TIMEOUT 30

#define APP_REG_NAME L"Firefox"

static const WCHAR* kFirefoxExe = L"firefox.exe";
static const WCHAR* kMetroFirefoxExe = L"firefox.exe";
static const WCHAR* kDefaultMetroBrowserIDPathKey = L"FirefoxURL";
static const WCHAR* kDemoMetroBrowserIDPathKey = L"Mozilla.Firefox.URL";

template <class T>void SafeRelease(T **ppT)
{
  if (*ppT) {
    (*ppT)->Release();
    *ppT = NULL;
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
    mShellItemArray(NULL),
    mUnkSite(NULL),
    mTargetIsFileSystemLink(false),
    mIsDesktopRequest(true),
    mRequestMet(false)
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
    mParameters = aParameters;
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
      IShellItem* item = NULL;
      if (SUCCEEDED(aArray->GetItemAt(idx, &item))) {
        LPWSTR str = NULL;
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

    IShellItem* item = NULL;
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
    *aInt = NULL;
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
    *aInt = NULL;
    return mUnkSite ? mUnkSite->QueryInterface(aRefID, aInt) : E_FAIL;
  }

  
  IFACEMETHODIMP GetValue(AHE_TYPE *aLaunchType)
  {
    Log(L"IExecuteCommandApplicationHostEnvironment::GetValue()");
    *aLaunchType = AHE_DESKTOP;
    mIsDesktopRequest = true;

    if (!mUnkSite) {
      Log(L"No mUnkSite.");
      return S_OK;
    }

    HRESULT hr;
    IServiceProvider* pSvcProvider = NULL;
    hr = mUnkSite->QueryInterface(IID_IServiceProvider, (void**)&pSvcProvider);
    if (!pSvcProvider) {
      Log(L"Couldn't get IServiceProvider service from explorer. (%X)", hr);
      return S_OK;
    }

    IExecuteCommandHost* pHost = NULL;
    
    hr = pSvcProvider->QueryService(SID_ExecuteCommandHost,
                                    IID_IExecuteCommandHost, (void**)&pHost);
    if (!pHost) {
      Log(L"Couldn't get IExecuteCommandHost service from explorer. (%X)", hr);
      SafeRelease(&pSvcProvider);
      return S_OK;
    }
    SafeRelease(&pSvcProvider);

    EC_HOST_UI_MODE mode;
    if (FAILED(pHost->GetUIMode(&mode))) {
      Log(L"GetUIMode failed.");
      SafeRelease(&pHost);
      return S_OK;
    }

    
    
    
    Log(L"GetUIMode: %d", mode);

    if (!IsDefaultBrowser()) {
      mode = ECHUIM_DESKTOP;
    }

    if (mode == ECHUIM_DESKTOP) {
      Log(L"returning AHE_DESKTOP");
      SafeRelease(&pHost);
      return S_OK;
    }
    SafeRelease(&pHost);

    if (!IsDX10Available()) {
      Log(L"returning AHE_DESKTOP because DX10 is not available");
      *aLaunchType = AHE_DESKTOP;
      mIsDesktopRequest = true;
    } else {
      Log(L"returning AHE_IMMERSIVE");
      *aLaunchType = AHE_IMMERSIVE;
      mIsDesktopRequest = false;
    }
    return S_OK;
  }

  bool IsDefaultBrowser()
  {
    bool result = false;
    IApplicationAssociationRegistration* pAAR;
    HRESULT hr = CoCreateInstance(CLSID_ApplicationAssociationRegistration,
                                  NULL,
                                  CLSCTX_INPROC,
                                  IID_IApplicationAssociationRegistration,
                                  (void**)&pAAR);
    if (SUCCEEDED(hr)) {
      BOOL res;
      hr = pAAR->QueryAppIsDefaultAll(AL_EFFECTIVE,
                                      APP_REG_NAME,
                                      &res);
      Log(L"QueryAppIsDefaultAll: %d", res);
      if (!res) 
        return false;
      
      LPWSTR registeredApp;
      hr = pAAR->QueryCurrentDefault(L"http", AT_URLPROTOCOL, AL_EFFECTIVE,
                                      &registeredApp);
      Log(L"QueryCurrentDefault: %X", hr);
      if (SUCCEEDED(hr)) {
        Log(L"registeredApp=%s", registeredApp);
        result = !wcsicmp(registeredApp, kDefaultMetroBrowserIDPathKey);
        if (!result) {
          result = !wcsicmp(registeredApp, kDemoMetroBrowserIDPathKey);
        }
        CoTaskMemFree(registeredApp);
      } else {
        result = false;
      }

      pAAR->Release();
      return result;
    }
    return result;
  }
private:
  ~CExecuteCommandVerb()
  {
    SafeRelease(&mShellItemArray);
    SafeRelease(&mUnkSite);
  }

  void LaunchDesktopBrowser();
  bool SetTargetPath(IShellItem* aItem);
  bool IsTargetBrowser();

  long mRef;
  IShellItemArray *mShellItemArray;
  IUnknown *mUnkSite;
  CStringW mVerb;
  CStringW mTarget;
  CStringW mParameters;
  bool mTargetIsFileSystemLink;
  DWORD mKeyState;
  bool mIsDesktopRequest;
  bool mRequestMet;
};






static bool GetModulePath(CStringW& aPathBuffer)
{
  WCHAR buffer[MAX_PATH];
  memset(buffer, 0, sizeof(buffer));

  if (!GetModuleFileName(NULL, buffer, MAX_PATH)) {
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






static bool GetDesktopBrowserPath(CStringW& aPathBuffer)
{
  if (!GetModulePath(aPathBuffer))
    return false;

  
  
  aPathBuffer.Append(L"\\");
  aPathBuffer.Append(kFirefoxExe);
  return true;
}







static bool GetDefaultBrowserAppModelID(WCHAR* aIDBuffer,
                                        long aCharLength)
{
  if (!aIDBuffer || aCharLength <= 0)
    return false;

  memset(aIDBuffer, 0, (sizeof(WCHAR)*aCharLength));

  HKEY key;
  if (RegOpenKeyExW(HKEY_CLASSES_ROOT, kDefaultMetroBrowserIDPathKey,
                    0, KEY_READ, &key) != ERROR_SUCCESS) {
    return false;
  }
  DWORD len = aCharLength * sizeof(WCHAR);
  memset(aIDBuffer, 0, len);
  if (RegQueryValueExW(key, L"AppUserModelID", NULL, NULL,
                       (LPBYTE)aIDBuffer, &len) != ERROR_SUCCESS || !len) {
    RegCloseKey(key);
    return false;
  }
  RegCloseKey(key);
  return true;
}





bool CExecuteCommandVerb::IsTargetBrowser()
{
  if (!mTarget.GetLength() || !mTargetIsFileSystemLink)
    return false;

  CStringW modulePath;
  if (!GetModulePath(modulePath))
    return false;

  modulePath.MakeLower();

  CStringW tmpTarget = mTarget;
  tmpTarget.Replace(L"\"", L"");
  tmpTarget.MakeLower();
  
  CStringW checkPath;
  
  checkPath = modulePath;
  checkPath.Append(L"\\");
  checkPath.Append(kFirefoxExe);
  if (tmpTarget == checkPath) {
    return true;
  }
  return false;
}





bool CExecuteCommandVerb::SetTargetPath(IShellItem* aItem)
{
  if (!aItem)
    return false;

  LPWSTR str = NULL;
  mTargetIsFileSystemLink = true;
  if (FAILED(aItem->GetDisplayName(SIGDN_FILESYSPATH, &str))) {
    if (FAILED(aItem->GetDisplayName(SIGDN_URL, &str))) {
      return false;
    }
    mTargetIsFileSystemLink = false;
  }

  mTarget = str;
  CoTaskMemFree(str);
  return true;
}





void CExecuteCommandVerb::LaunchDesktopBrowser()
{
  CStringW browserPath;
  if (!GetDesktopBrowserPath(browserPath)) {
    return;
  }

  
  
  CStringW params;
  if (!IsTargetBrowser()) {
    params += "-url ";
    params += mTarget;
  }

  Log(L"Desktop Launch: verb:%s exe:%s params:%s", mVerb, browserPath, params); 

  SHELLEXECUTEINFOW seinfo;
  memset(&seinfo, 0, sizeof(seinfo));
  seinfo.cbSize = sizeof(SHELLEXECUTEINFOW);
  seinfo.fMask  = NULL;
  seinfo.hwnd   = NULL;
  seinfo.lpVerb = NULL;
  seinfo.lpFile = browserPath;
  seinfo.lpParameters =  params;
  seinfo.lpDirectory  = NULL;
  seinfo.nShow  = SW_SHOWNORMAL;
        
  ShellExecuteExW(&seinfo);
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

IFACEMETHODIMP CExecuteCommandVerb::Execute()
{
  Log(L"Execute()");

  
  AutoSetRequestMet asrm(&mRequestMet);

  if (!mTarget.GetLength()) {
    return E_FAIL;
  }

  
  if (mIsDesktopRequest) {
    LaunchDesktopBrowser();
    return S_OK;
  }

  
  IApplicationActivationManager* activateMgr = NULL;
  DWORD processID;
  if (FAILED(CoCreateInstance(CLSID_ApplicationActivationManager, NULL,
                              CLSCTX_LOCAL_SERVER,
                              IID_IApplicationActivationManager,
                              (void**)&activateMgr))) {
    Log(L"CoCreateInstance failed, launching on desktop.");
    LaunchDesktopBrowser();
    return S_OK;
  }
  
  HRESULT hr;
  WCHAR appModelID[256];
  if (!GetDefaultBrowserAppModelID(appModelID, (sizeof(appModelID)/sizeof(WCHAR)))) {
    Log(L"GetDefaultBrowserAppModelID failed, launching on desktop.");
    activateMgr->Release();
    LaunchDesktopBrowser();
    return S_OK;
  }

  
  
  hr = CoAllowSetForegroundWindow(activateMgr, NULL);
  if (FAILED(hr)) {
    Log(L"CoAllowSetForegroundWindow result %X", hr);
    activateMgr->Release();
    return false;
  }

  Log(L"Metro Launch: verb:%s appid:%s params:%s", mVerb, appModelID, mTarget); 

  
  if (IsTargetBrowser()) {
    hr = activateMgr->ActivateApplication(appModelID, L"", AO_NONE, &processID);
    Log(L"ActivateApplication result %X", hr);
  
  } else if (mTargetIsFileSystemLink) {
    hr = activateMgr->ActivateForFile(appModelID, mShellItemArray, mVerb, &processID);
    Log(L"ActivateForFile result %X", hr);
  
  } else {
    hr = activateMgr->ActivateForProtocol(appModelID, mShellItemArray, &processID);
    Log(L"ActivateForProtocol result %X", hr);
  }
  activateMgr->Release();
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
  IUnknown *punk = NULL;
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
  *ppv = NULL;
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
      CoInitialize(NULL);

      CExecuteCommandVerb *pHandler = new CExecuteCommandVerb();
      if (!pHandler)
        return E_OUTOFMEMORY;

      IUnknown* ppi;
      pHandler->QueryInterface(IID_IUnknown, (void**)&ppi);
      if (!ppi)
        return E_FAIL;

      ClassFactory classFactory(ppi);
      ppi->Release();
      ppi = NULL;

      
      if (FAILED(classFactory.Register(CLSCTX_LOCAL_SERVER, REGCLS_SINGLEUSE)))
        return -1;

      if (!SetTimer(NULL, 1, HEARTBEAT_MSEC, NULL)) {
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
