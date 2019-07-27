




#include "nsParentalControlsService.h"
#include "nsString.h"
#include "nsIArray.h"
#include "nsIWidget.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIFile.h"
#include "nsILocalFileWin.h"
#include "nsArrayUtils.h"
#include "nsIXULAppInfo.h"
#include "mozilla/WindowsVersion.h"

using namespace mozilla;

static const CLSID CLSID_WinParentalControls = {0xE77CC89B,0x7401,0x4C04,{0x8C,0xED,0x14,0x9D,0xB3,0x5A,0xDD,0x04}};
static const IID IID_IWinParentalControls  = {0x28B4D88B,0xE072,0x49E6,{0x80,0x4D,0x26,0xED,0xBE,0x21,0xA7,0xB9}};

NS_IMPL_ISUPPORTS(nsParentalControlsService, nsIParentalControlsService)

static HINSTANCE gAdvAPIDLLInst = nullptr;

decltype(EventWrite)* gEventWrite = nullptr;
decltype(EventRegister)* gEventRegister = nullptr;
decltype(EventUnregister)* gEventUnregister = nullptr;

nsParentalControlsService::nsParentalControlsService() :
  mEnabled(false)
, mProvider(0)
, mPC(nullptr)
{
  HRESULT hr;
  CoInitialize(nullptr);
  hr = CoCreateInstance(CLSID_WinParentalControls, nullptr, CLSCTX_INPROC,
                        IID_IWinParentalControls, (void**)&mPC);
  if (FAILED(hr))
    return;

  nsRefPtr<IWPCSettings> wpcs;
  if (FAILED(mPC->GetUserSettings(nullptr, getter_AddRefs(wpcs)))) {
    
    mPC->Release();
    mPC = nullptr;
    return;
  }

  DWORD settings = 0;
  wpcs->GetRestrictions(&settings);

  
  
  bool enable = IsWin8OrLater() ? settings & WPCFLAG_WEB_FILTERED
                                : settings != WPCFLAG_NO_RESTRICTION;

  if (enable) {
    gAdvAPIDLLInst = ::LoadLibrary("Advapi32.dll");
    if(gAdvAPIDLLInst)
    {
      gEventWrite = (decltype(EventWrite)*) GetProcAddress(gAdvAPIDLLInst, "EventWrite");
      gEventRegister = (decltype(EventRegister)*) GetProcAddress(gAdvAPIDLLInst, "EventRegister");
      gEventUnregister = (decltype(EventUnregister)*) GetProcAddress(gAdvAPIDLLInst, "EventUnregister");
    }
    mEnabled = true;
  }
}

nsParentalControlsService::~nsParentalControlsService()
{
  if (mPC)
    mPC->Release();

  if (gEventUnregister && mProvider)
    gEventUnregister(mProvider);

  if (gAdvAPIDLLInst)
    ::FreeLibrary(gAdvAPIDLLInst);
}



NS_IMETHODIMP
nsParentalControlsService::GetParentalControlsEnabled(bool *aResult)
{
  *aResult = false;

  if (mEnabled)
    *aResult = true;

  return NS_OK;
}

NS_IMETHODIMP
nsParentalControlsService::GetBlockFileDownloadsEnabled(bool *aResult)
{
  *aResult = false;

  if (!mEnabled)
    return NS_ERROR_NOT_AVAILABLE;

  nsRefPtr<IWPCWebSettings> wpcws;
  if (SUCCEEDED(mPC->GetWebSettings(nullptr, getter_AddRefs(wpcws)))) {
    DWORD settings = 0;
    wpcws->GetSettings(&settings);
    if (settings == WPCFLAG_WEB_SETTING_DOWNLOADSBLOCKED)
      *aResult = true;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsParentalControlsService::GetLoggingEnabled(bool *aResult)
{
  *aResult = false;

  if (!mEnabled)
    return NS_ERROR_NOT_AVAILABLE;

  
  nsRefPtr<IWPCSettings> wpcs;
  if (SUCCEEDED(mPC->GetUserSettings(nullptr, getter_AddRefs(wpcs)))) {
    BOOL enabled = FALSE;
    wpcs->IsLoggingRequired(&enabled);
    if (enabled)
      *aResult = true;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsParentalControlsService::Log(int16_t aEntryType, bool blocked, nsIURI *aSource, nsIFile *aTarget)
{
  if (!mEnabled)
    return NS_ERROR_NOT_AVAILABLE;

  NS_ENSURE_ARG_POINTER(aSource);

  
  bool enabled;
  GetLoggingEnabled(&enabled);
  if (!enabled)
    return NS_ERROR_NOT_AVAILABLE;

  
  if (!mProvider) {
    if (!gEventRegister)
      return NS_ERROR_NOT_AVAILABLE;
    if (gEventRegister(&WPCPROV, nullptr, nullptr, &mProvider) != ERROR_SUCCESS)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  switch(aEntryType) {
    case ePCLog_URIVisit:
      
      break;
    case ePCLog_FileDownload:
      LogFileDownload(blocked, aSource, aTarget);
      break;
    default:
      break;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsParentalControlsService::RequestURIOverride(nsIURI *aTarget, nsIInterfaceRequestor *aWindowContext, bool *_retval)
{
  *_retval = false;

  if (!mEnabled)
    return NS_ERROR_NOT_AVAILABLE;

  NS_ENSURE_ARG_POINTER(aTarget);

  nsAutoCString spec;
  aTarget->GetSpec(spec);
  if (spec.IsEmpty())
    return NS_ERROR_INVALID_ARG;

  HWND hWnd = nullptr;
  
  nsCOMPtr<nsIWidget> widget(do_GetInterface(aWindowContext));
  if (widget)
    hWnd = (HWND)widget->GetNativeData(NS_NATIVE_WINDOW);
  if (hWnd == nullptr)
    hWnd = GetDesktopWindow();

  BOOL ret;
  nsRefPtr<IWPCWebSettings> wpcws;
  if (SUCCEEDED(mPC->GetWebSettings(nullptr, getter_AddRefs(wpcws)))) {
    wpcws->RequestURLOverride(hWnd, NS_ConvertUTF8toUTF16(spec).get(),
                              0, nullptr, &ret);
    *_retval = ret;
  }


  return NS_OK;
}


NS_IMETHODIMP
nsParentalControlsService::RequestURIOverrides(nsIArray *aTargets, nsIInterfaceRequestor *aWindowContext, bool *_retval)
{
  *_retval = false;

  if (!mEnabled)
    return NS_ERROR_NOT_AVAILABLE;

  NS_ENSURE_ARG_POINTER(aTargets);

  uint32_t arrayLength = 0;
  aTargets->GetLength(&arrayLength);
  if (!arrayLength)
    return NS_ERROR_INVALID_ARG;

  if (arrayLength == 1) {
    nsCOMPtr<nsIURI> uri = do_QueryElementAt(aTargets, 0);
    if (!uri)
      return NS_ERROR_INVALID_ARG;
    return RequestURIOverride(uri, aWindowContext, _retval);
  }

  HWND hWnd = nullptr;
  
  nsCOMPtr<nsIWidget> widget(do_GetInterface(aWindowContext));
  if (widget)
    hWnd = (HWND)widget->GetNativeData(NS_NATIVE_WINDOW);
  if (hWnd == nullptr)
    hWnd = GetDesktopWindow();

  
  nsAutoCString rootSpec;
  nsCOMPtr<nsIURI> rootURI = do_QueryElementAt(aTargets, 0);
  if (!rootURI)
    return NS_ERROR_INVALID_ARG;
  
  rootURI->GetSpec(rootSpec);
  if (rootSpec.IsEmpty())
    return NS_ERROR_INVALID_ARG;

  
  int32_t count = arrayLength - 1;
  nsAutoArrayPtr<LPCWSTR> arrUrls(new LPCWSTR[count]);
  if (!arrUrls)
    return NS_ERROR_OUT_OF_MEMORY;

  uint32_t uriIdx = 0, idx;
  for (idx = 1; idx < arrayLength; idx++)
  {
    nsCOMPtr<nsIURI> uri = do_QueryElementAt(aTargets, idx);
    if (!uri)
      continue;

    nsAutoCString subURI;
    if (NS_FAILED(uri->GetSpec(subURI)))
      continue;

    arrUrls[uriIdx] = (LPCWSTR)UTF8ToNewUnicode(subURI); 
    if (!arrUrls[uriIdx])
      continue;

    uriIdx++;
  }

  if (!uriIdx)
    return NS_ERROR_INVALID_ARG;

  BOOL ret; 
  nsRefPtr<IWPCWebSettings> wpcws;
  if (SUCCEEDED(mPC->GetWebSettings(nullptr, getter_AddRefs(wpcws)))) {
    wpcws->RequestURLOverride(hWnd, NS_ConvertUTF8toUTF16(rootSpec).get(),
                             uriIdx, (LPCWSTR*)arrUrls.get(), &ret);
   *_retval = ret;
  }

  
  for (idx = 0; idx < uriIdx; idx++)
    free((void*)arrUrls[idx]);

  return NS_OK;
}




void
nsParentalControlsService::LogFileDownload(bool blocked, nsIURI *aSource, nsIFile *aTarget)
{
  nsAutoCString curi;

  if (!gEventWrite)
    return;

  

  aSource->GetSpec(curi);
  nsAutoString uri = NS_ConvertUTF8toUTF16(curi);

  
  nsCOMPtr<nsIXULAppInfo> appInfo = do_GetService("@mozilla.org/xre/app-info;1");
  nsAutoCString asciiAppName;
  if (appInfo)
    appInfo->GetName(asciiAppName);
  nsAutoString appName = NS_ConvertUTF8toUTF16(asciiAppName);

  static const WCHAR fill[] = L"";
  
  
  EVENT_DATA_DESCRIPTOR eventData[WPC_ARGS_FILEDOWNLOADEVENT_CARGS];
  DWORD dwBlocked = blocked;

  EventDataDescCreate(&eventData[WPC_ARGS_FILEDOWNLOADEVENT_URL], (const void*)uri.get(),
                      ((ULONG)uri.Length()+1)*sizeof(WCHAR));
  EventDataDescCreate(&eventData[WPC_ARGS_FILEDOWNLOADEVENT_APPNAME], (const void*)appName.get(),
                      ((ULONG)appName.Length()+1)*sizeof(WCHAR));
  EventDataDescCreate(&eventData[WPC_ARGS_FILEDOWNLOADEVENT_VERSION], (const void*)fill, sizeof(fill));
  EventDataDescCreate(&eventData[WPC_ARGS_FILEDOWNLOADEVENT_BLOCKED], (const void*)&dwBlocked,
                      sizeof(dwBlocked));

  nsCOMPtr<nsILocalFileWin> local(do_QueryInterface(aTarget)); 
  if (local) {
    nsAutoString path;
    local->GetCanonicalPath(path);
    EventDataDescCreate(&eventData[WPC_ARGS_FILEDOWNLOADEVENT_PATH], (const void*)path.get(),
                        ((ULONG)path.Length()+1)*sizeof(WCHAR));
  }
  else {
    EventDataDescCreate(&eventData[WPC_ARGS_FILEDOWNLOADEVENT_PATH], (const void*)fill, sizeof(fill));
  }

  gEventWrite(mProvider, &WPCEVENT_WEB_FILEDOWNLOAD, ARRAYSIZE(eventData), eventData);
}

NS_IMETHODIMP
nsParentalControlsService::IsAllowed(int16_t aAction,
                                     nsIURI *aUri,
                                     bool *_retval)
{
  return NS_ERROR_NOT_AVAILABLE;
}
