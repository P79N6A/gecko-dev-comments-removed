





#include "WinUtils.h"

#include "gfxPlatform.h"
#include "nsWindow.h"
#include "nsWindowDefs.h"
#include "KeyboardLayout.h"
#include "nsIDOMMouseEvent.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/gfx/DataSurfaceHelpers.h"
#include "mozilla/Preferences.h"
#include "mozilla/RefPtr.h"
#include "mozilla/WindowsVersion.h"

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif 
#include "prlog.h"

#include "nsString.h"
#include "nsDirectoryServiceUtils.h"
#include "imgIContainer.h"
#include "imgITools.h"
#include "nsStringStream.h"
#include "nsNetUtil.h"
#ifdef MOZ_PLACES
#include "mozIAsyncFavicons.h"
#endif
#include "nsIIconURI.h"
#include "nsIDownloader.h"
#include "nsINetUtil.h"
#include "nsIChannel.h"
#include "nsIObserver.h"
#include "imgIEncoder.h"
#include "nsIThread.h"
#include "MainThreadUtils.h"
#include "gfxColor.h"
#ifdef MOZ_METRO
#include "winrt/MetroInput.h"
#include "winrt/MetroUtils.h"
#endif 

#ifdef NS_ENABLE_TSF
#include <textstor.h>
#include "nsTextStore.h"
#endif 

#ifdef PR_LOGGING
PRLogModuleInfo* gWindowsLog = nullptr;
#endif

using namespace mozilla::gfx;

namespace mozilla {
namespace widget {

NS_IMPL_ISUPPORTS(myDownloadObserver, nsIDownloadObserver)
#ifdef MOZ_PLACES
NS_IMPL_ISUPPORTS(AsyncFaviconDataReady, nsIFaviconDataCallback)
#endif
NS_IMPL_ISUPPORTS(AsyncEncodeAndWriteIcon, nsIRunnable)
NS_IMPL_ISUPPORTS(AsyncDeleteIconFromDisk, nsIRunnable)
NS_IMPL_ISUPPORTS(AsyncDeleteAllFaviconsFromDisk, nsIRunnable)


const char FaviconHelper::kJumpListCacheDir[] = "jumpListCache";
const char FaviconHelper::kShortcutCacheDir[] = "shortcutCache";


WinUtils::SHCreateItemFromParsingNamePtr WinUtils::sCreateItemFromParsingName = nullptr;
WinUtils::SHGetKnownFolderPathPtr WinUtils::sGetKnownFolderPath = nullptr;



static const wchar_t kShellLibraryName[] =  L"shell32.dll";
static HMODULE sShellDll = nullptr;
static const wchar_t kDwmLibraryName[] = L"dwmapi.dll";
static HMODULE sDwmDll = nullptr;

WinUtils::DwmExtendFrameIntoClientAreaProc WinUtils::dwmExtendFrameIntoClientAreaPtr = nullptr;
WinUtils::DwmIsCompositionEnabledProc WinUtils::dwmIsCompositionEnabledPtr = nullptr;
WinUtils::DwmSetIconicThumbnailProc WinUtils::dwmSetIconicThumbnailPtr = nullptr;
WinUtils::DwmSetIconicLivePreviewBitmapProc WinUtils::dwmSetIconicLivePreviewBitmapPtr = nullptr;
WinUtils::DwmGetWindowAttributeProc WinUtils::dwmGetWindowAttributePtr = nullptr;
WinUtils::DwmSetWindowAttributeProc WinUtils::dwmSetWindowAttributePtr = nullptr;
WinUtils::DwmInvalidateIconicBitmapsProc WinUtils::dwmInvalidateIconicBitmapsPtr = nullptr;
WinUtils::DwmDefWindowProcProc WinUtils::dwmDwmDefWindowProcPtr = nullptr;
WinUtils::DwmGetCompositionTimingInfoProc WinUtils::dwmGetCompositionTimingInfoPtr = nullptr;


void
WinUtils::Initialize()
{
#ifdef PR_LOGGING
  if (!gWindowsLog) {
    gWindowsLog = PR_NewLogModule("Widget");
  }
#endif
  if (!sDwmDll && IsVistaOrLater()) {
    sDwmDll = ::LoadLibraryW(kDwmLibraryName);

    if (sDwmDll) {
      dwmExtendFrameIntoClientAreaPtr = (DwmExtendFrameIntoClientAreaProc)::GetProcAddress(sDwmDll, "DwmExtendFrameIntoClientArea");
      dwmIsCompositionEnabledPtr = (DwmIsCompositionEnabledProc)::GetProcAddress(sDwmDll, "DwmIsCompositionEnabled");
      dwmSetIconicThumbnailPtr = (DwmSetIconicThumbnailProc)::GetProcAddress(sDwmDll, "DwmSetIconicThumbnail");
      dwmSetIconicLivePreviewBitmapPtr = (DwmSetIconicLivePreviewBitmapProc)::GetProcAddress(sDwmDll, "DwmSetIconicLivePreviewBitmap");
      dwmGetWindowAttributePtr = (DwmGetWindowAttributeProc)::GetProcAddress(sDwmDll, "DwmGetWindowAttribute");
      dwmSetWindowAttributePtr = (DwmSetWindowAttributeProc)::GetProcAddress(sDwmDll, "DwmSetWindowAttribute");
      dwmInvalidateIconicBitmapsPtr = (DwmInvalidateIconicBitmapsProc)::GetProcAddress(sDwmDll, "DwmInvalidateIconicBitmaps");
      dwmDwmDefWindowProcPtr = (DwmDefWindowProcProc)::GetProcAddress(sDwmDll, "DwmDefWindowProc");
      dwmGetCompositionTimingInfoPtr = (DwmGetCompositionTimingInfoProc)::GetProcAddress(sDwmDll, "DwmGetCompositionTimingInfo");
    }
  }
}


void
WinUtils::LogW(const wchar_t *fmt, ...)
{
  va_list args = nullptr;
  if(!lstrlenW(fmt)) {
    return;
  }
  va_start(args, fmt);
  int buflen = _vscwprintf(fmt, args);
  wchar_t* buffer = new wchar_t[buflen+1];
  if (!buffer) {
    va_end(args);
    return;
  }
  vswprintf(buffer, buflen, fmt, args);
  va_end(args);

  
  OutputDebugStringW(buffer);
  OutputDebugStringW(L"\n");

  int len = wcslen(buffer);
  if (len) {
    char* utf8 = new char[len+1];
    memset(utf8, 0, sizeof(utf8));
    if (WideCharToMultiByte(CP_ACP, 0, buffer,
                            -1, utf8, len+1, nullptr,
                            nullptr) > 0) {
      
      printf("%s\n", utf8);
#ifdef PR_LOGGING
      NS_ASSERTION(gWindowsLog, "Called WinUtils Log() but Widget "
                                   "log module doesn't exist!");
      PR_LOG(gWindowsLog, PR_LOG_ALWAYS, (utf8));
#endif
    }
    delete[] utf8;
  }
  delete[] buffer;
}


void
WinUtils::Log(const char *fmt, ...)
{
  va_list args = nullptr;
  if(!strlen(fmt)) {
    return;
  }
  va_start(args, fmt);
  int buflen = _vscprintf(fmt, args);
  char* buffer = new char[buflen+1];
  if (!buffer) {
    va_end(args);
    return;
  }
  vsprintf(buffer, fmt, args);
  va_end(args);

  
  OutputDebugStringA(buffer);
  OutputDebugStringW(L"\n");

  
  printf("%s\n", buffer);

#ifdef PR_LOGGING
  NS_ASSERTION(gWindowsLog, "Called WinUtils Log() but Widget "
                               "log module doesn't exist!");
  PR_LOG(gWindowsLog, PR_LOG_ALWAYS, (buffer));
#endif
  delete[] buffer;
}


double
WinUtils::LogToPhysFactor()
{
  
  if (XRE_GetWindowsEnvironment() == WindowsEnvironmentType_Metro) {
#ifdef MOZ_METRO
    return MetroUtils::LogToPhysFactor();
#else
    return 1.0;
#endif
  } else {
    HDC hdc = ::GetDC(nullptr);
    double result = ::GetDeviceCaps(hdc, LOGPIXELSY) / 96.0;
    ::ReleaseDC(nullptr, hdc);

    if (result == 0) {
      
      
      
      
      result = 1.0;
    }
    return result;
  }
}


double
WinUtils::PhysToLogFactor()
{
  
  return 1.0 / LogToPhysFactor();
}


double
WinUtils::PhysToLog(int32_t aValue)
{
  return double(aValue) * PhysToLogFactor();
}


int32_t
WinUtils::LogToPhys(double aValue)
{
  return int32_t(NS_round(aValue * LogToPhysFactor()));
}


bool
WinUtils::PeekMessage(LPMSG aMsg, HWND aWnd, UINT aFirstMessage,
                      UINT aLastMessage, UINT aOption)
{
#ifdef NS_ENABLE_TSF
  ITfMessagePump* msgPump = nsTextStore::GetMessagePump();
  if (msgPump) {
    BOOL ret = FALSE;
    HRESULT hr = msgPump->PeekMessageW(aMsg, aWnd, aFirstMessage, aLastMessage,
                                       aOption, &ret);
    NS_ENSURE_TRUE(SUCCEEDED(hr), false);
    return ret;
  }
#endif 
  return ::PeekMessageW(aMsg, aWnd, aFirstMessage, aLastMessage, aOption);
}


bool
WinUtils::GetMessage(LPMSG aMsg, HWND aWnd, UINT aFirstMessage,
                     UINT aLastMessage)
{
#ifdef NS_ENABLE_TSF
  ITfMessagePump* msgPump = nsTextStore::GetMessagePump();
  if (msgPump) {
    BOOL ret = FALSE;
    HRESULT hr = msgPump->GetMessageW(aMsg, aWnd, aFirstMessage, aLastMessage,
                                      &ret);
    NS_ENSURE_TRUE(SUCCEEDED(hr), false);
    return ret;
  }
#endif 
  return ::GetMessageW(aMsg, aWnd, aFirstMessage, aLastMessage);
}


void
WinUtils::WaitForMessage()
{
  DWORD result = ::MsgWaitForMultipleObjectsEx(0, NULL, INFINITE, QS_ALLINPUT,
                                               MWMO_INPUTAVAILABLE);
  NS_WARN_IF_FALSE(result != WAIT_FAILED, "Wait failed");

  
  
  
  
  if (result == WAIT_OBJECT_0) {
    MSG msg = {0};
    DWORD queue_status = ::GetQueueStatus(QS_MOUSE);
    if (HIWORD(queue_status) & QS_MOUSE &&
        !PeekMessage(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_NOREMOVE)) {
      ::WaitMessage();
    }
  }
}


bool
WinUtils::GetRegistryKey(HKEY aRoot,
                         char16ptr_t aKeyName,
                         char16ptr_t aValueName,
                         wchar_t* aBuffer,
                         DWORD aBufferLength)
{
  NS_PRECONDITION(aKeyName, "The key name is NULL");

  HKEY key;
  LONG result =
    ::RegOpenKeyExW(aRoot, aKeyName, 0, KEY_READ | KEY_WOW64_32KEY, &key);
  if (result != ERROR_SUCCESS) {
    result =
      ::RegOpenKeyExW(aRoot, aKeyName, 0, KEY_READ | KEY_WOW64_64KEY, &key);
    if (result != ERROR_SUCCESS) {
      return false;
    }
  }

  DWORD type;
  result =
    ::RegQueryValueExW(key, aValueName, nullptr, &type, (BYTE*) aBuffer,
                       &aBufferLength);
  ::RegCloseKey(key);
  if (result != ERROR_SUCCESS || type != REG_SZ) {
    return false;
  }
  if (aBuffer) {
    aBuffer[aBufferLength / sizeof(*aBuffer) - 1] = 0;
  }
  return true;
}


bool
WinUtils::HasRegistryKey(HKEY aRoot, char16ptr_t aKeyName)
{
  MOZ_ASSERT(aRoot, "aRoot must not be NULL");
  MOZ_ASSERT(aKeyName, "aKeyName must not be NULL");
  HKEY key;
  LONG result =
    ::RegOpenKeyExW(aRoot, aKeyName, 0, KEY_READ | KEY_WOW64_32KEY, &key);
  if (result != ERROR_SUCCESS) {
    result =
      ::RegOpenKeyExW(aRoot, aKeyName, 0, KEY_READ | KEY_WOW64_64KEY, &key);
    if (result != ERROR_SUCCESS) {
      return false;
    }
  }
  ::RegCloseKey(key);
  return true;
}


HWND
WinUtils::GetTopLevelHWND(HWND aWnd,
                          bool aStopIfNotChild,
                          bool aStopIfNotPopup)
{
  HWND curWnd = aWnd;
  HWND topWnd = nullptr;

  while (curWnd) {
    topWnd = curWnd;

    if (aStopIfNotChild) {
      DWORD_PTR style = ::GetWindowLongPtrW(curWnd, GWL_STYLE);

      VERIFY_WINDOW_STYLE(style);

      if (!(style & WS_CHILD)) 
        break;
    }

    HWND upWnd = ::GetParent(curWnd); 

    
    
    if (!upWnd && !aStopIfNotPopup) {
      upWnd = ::GetWindow(curWnd, GW_OWNER);
    }
    curWnd = upWnd;
  }

  return topWnd;
}

static const wchar_t*
GetNSWindowPropName()
{
  static wchar_t sPropName[40] = L"";
  if (!*sPropName) {
    _snwprintf(sPropName, 39, L"MozillansIWidgetPtr%p",
               ::GetCurrentProcessId());
    sPropName[39] = '\0';
  }
  return sPropName;
}


bool
WinUtils::SetNSWindowBasePtr(HWND aWnd, nsWindowBase* aWidget)
{
  if (!aWidget) {
    ::RemovePropW(aWnd, GetNSWindowPropName());
    return true;
  }
  return ::SetPropW(aWnd, GetNSWindowPropName(), (HANDLE)aWidget);
}


nsWindowBase*
WinUtils::GetNSWindowBasePtr(HWND aWnd)
{
  return static_cast<nsWindowBase*>(::GetPropW(aWnd, GetNSWindowPropName()));
}


nsWindow*
WinUtils::GetNSWindowPtr(HWND aWnd)
{
  return static_cast<nsWindow*>(::GetPropW(aWnd, GetNSWindowPropName()));
}

static BOOL CALLBACK
AddMonitor(HMONITOR, HDC, LPRECT, LPARAM aParam)
{
  (*(int32_t*)aParam)++;
  return TRUE;
}


int32_t
WinUtils::GetMonitorCount()
{
  int32_t monitorCount = 0;
  EnumDisplayMonitors(nullptr, nullptr, AddMonitor, (LPARAM)&monitorCount);
  return monitorCount;
}


bool
WinUtils::IsOurProcessWindow(HWND aWnd)
{
  if (!aWnd) {
    return false;
  }
  DWORD processId = 0;
  ::GetWindowThreadProcessId(aWnd, &processId);
  return (processId == ::GetCurrentProcessId());
}


HWND
WinUtils::FindOurProcessWindow(HWND aWnd)
{
  for (HWND wnd = ::GetParent(aWnd); wnd; wnd = ::GetParent(wnd)) {
    if (IsOurProcessWindow(wnd)) {
      return wnd;
    }
  }
  return nullptr;
}

static bool
IsPointInWindow(HWND aWnd, const POINT& aPointInScreen)
{
  RECT bounds;
  if (!::GetWindowRect(aWnd, &bounds)) {
    return false;
  }

  return (aPointInScreen.x >= bounds.left && aPointInScreen.x < bounds.right &&
          aPointInScreen.y >= bounds.top && aPointInScreen.y < bounds.bottom);
}






static HWND
FindTopmostWindowAtPoint(HWND aWnd, const POINT& aPointInScreen)
{
  if (!::IsWindowVisible(aWnd) || !IsPointInWindow(aWnd, aPointInScreen)) {
    return nullptr;
  }

  HWND childWnd = ::GetTopWindow(aWnd);
  while (childWnd) {
    HWND topmostWnd = FindTopmostWindowAtPoint(childWnd, aPointInScreen);
    if (topmostWnd) {
      return topmostWnd;
    }
    childWnd = ::GetNextWindow(childWnd, GW_HWNDNEXT);
  }

  return aWnd;
}

struct FindOurWindowAtPointInfo
{
  POINT mInPointInScreen;
  HWND mOutWnd;
};

static BOOL CALLBACK
FindOurWindowAtPointCallback(HWND aWnd, LPARAM aLPARAM)
{
  if (!WinUtils::IsOurProcessWindow(aWnd)) {
    
    return TRUE;
  }

  
  
  
  
  FindOurWindowAtPointInfo* info =
    reinterpret_cast<FindOurWindowAtPointInfo*>(aLPARAM);
  HWND childWnd = FindTopmostWindowAtPoint(aWnd, info->mInPointInScreen);
  if (!childWnd) {
    
    return TRUE;
  }

  
  info->mOutWnd = childWnd;
  return FALSE;
}


HWND
WinUtils::FindOurWindowAtPoint(const POINT& aPointInScreen)
{
  FindOurWindowAtPointInfo info;
  info.mInPointInScreen = aPointInScreen;
  info.mOutWnd = nullptr;

  
  EnumWindows(FindOurWindowAtPointCallback, reinterpret_cast<LPARAM>(&info));
  return info.mOutWnd;
}


UINT
WinUtils::GetInternalMessage(UINT aNativeMessage)
{
  switch (aNativeMessage) {
    case WM_MOUSEWHEEL:
      return MOZ_WM_MOUSEVWHEEL;
    case WM_MOUSEHWHEEL:
      return MOZ_WM_MOUSEHWHEEL;
    case WM_VSCROLL:
      return MOZ_WM_VSCROLL;
    case WM_HSCROLL:
      return MOZ_WM_HSCROLL;
    default:
      return aNativeMessage;
  }
}


UINT
WinUtils::GetNativeMessage(UINT aInternalMessage)
{
  switch (aInternalMessage) {
    case MOZ_WM_MOUSEVWHEEL:
      return WM_MOUSEWHEEL;
    case MOZ_WM_MOUSEHWHEEL:
      return WM_MOUSEHWHEEL;
    case MOZ_WM_VSCROLL:
      return WM_VSCROLL;
    case MOZ_WM_HSCROLL:
      return WM_HSCROLL;
    default:
      return aInternalMessage;
  }
}


uint16_t
WinUtils::GetMouseInputSource()
{
  int32_t inputSource = nsIDOMMouseEvent::MOZ_SOURCE_MOUSE;
  LPARAM lParamExtraInfo = ::GetMessageExtraInfo();
  if ((lParamExtraInfo & TABLET_INK_SIGNATURE) == TABLET_INK_CHECK) {
    inputSource = (lParamExtraInfo & TABLET_INK_TOUCH) ?
      nsIDOMMouseEvent::MOZ_SOURCE_TOUCH : nsIDOMMouseEvent::MOZ_SOURCE_PEN;
  }
  return static_cast<uint16_t>(inputSource);
}

bool
WinUtils::GetIsMouseFromTouch(uint32_t aEventType)
{
#define MOUSEEVENTF_FROMTOUCH 0xFF515700
  return (aEventType == NS_MOUSE_BUTTON_DOWN ||
          aEventType == NS_MOUSE_BUTTON_UP ||
          aEventType == NS_MOUSE_MOVE) &&
          (GetMessageExtraInfo() & MOUSEEVENTF_FROMTOUCH);
}


MSG
WinUtils::InitMSG(UINT aMessage, WPARAM wParam, LPARAM lParam, HWND aWnd)
{
  MSG msg;
  msg.message = aMessage;
  msg.wParam  = wParam;
  msg.lParam  = lParam;
  msg.hwnd    = aWnd;
  return msg;
}


HRESULT
WinUtils::SHCreateItemFromParsingName(PCWSTR pszPath, IBindCtx *pbc,
                                      REFIID riid, void **ppv)
{
  if (sCreateItemFromParsingName) {
    return sCreateItemFromParsingName(pszPath, pbc, riid, ppv);
  }

  if (!sShellDll) {
    sShellDll = ::LoadLibraryW(kShellLibraryName);
    if (!sShellDll) {
      return false;
    }
  }

  sCreateItemFromParsingName = (SHCreateItemFromParsingNamePtr)
    GetProcAddress(sShellDll, "SHCreateItemFromParsingName");
  if (!sCreateItemFromParsingName)
    return E_FAIL;

  return sCreateItemFromParsingName(pszPath, pbc, riid, ppv);
}


HRESULT 
WinUtils::SHGetKnownFolderPath(REFKNOWNFOLDERID rfid,
                               DWORD dwFlags,
                               HANDLE hToken,
                               PWSTR *ppszPath)
{
  if (sGetKnownFolderPath) {
    return sGetKnownFolderPath(rfid, dwFlags, hToken, ppszPath);
  }

  if (!sShellDll) {
    sShellDll = ::LoadLibraryW(kShellLibraryName);
    if (!sShellDll) {
      return false;
    }
  }

  sGetKnownFolderPath = (SHGetKnownFolderPathPtr)
    GetProcAddress(sShellDll, "SHGetKnownFolderPath");
  if (!sGetKnownFolderPath)
    return E_FAIL;

  return sGetKnownFolderPath(rfid, dwFlags, hToken, ppszPath);
}

#ifdef MOZ_PLACES






AsyncFaviconDataReady::AsyncFaviconDataReady(nsIURI *aNewURI, 
                                             nsCOMPtr<nsIThread> &aIOThread, 
                                             const bool aURLShortcut):
  mNewURI(aNewURI),
  mIOThread(aIOThread),
  mURLShortcut(aURLShortcut)
{
}

NS_IMETHODIMP
myDownloadObserver::OnDownloadComplete(nsIDownloader *downloader, 
                                     nsIRequest *request, 
                                     nsISupports *ctxt, 
                                     nsresult status, 
                                     nsIFile *result)
{
  return NS_OK;
}

nsresult AsyncFaviconDataReady::OnFaviconDataNotAvailable(void)
{
  if (!mURLShortcut) {
    return NS_OK;
  }

  nsCOMPtr<nsIFile> icoFile;
  nsresult rv = FaviconHelper::GetOutputIconPath(mNewURI, icoFile, mURLShortcut);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> mozIconURI;
  rv = NS_NewURI(getter_AddRefs(mozIconURI), "moz-icon://.html?size=32");
  if (NS_FAILED(rv)) {
    return rv;
  }
 
  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel), mozIconURI);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDownloadObserver> downloadObserver = new myDownloadObserver;
  nsCOMPtr<nsIStreamListener> listener;
  rv = NS_NewDownloader(getter_AddRefs(listener), downloadObserver, icoFile);
  NS_ENSURE_SUCCESS(rv, rv);

  channel->AsyncOpen(listener, nullptr);
  return NS_OK;
}

NS_IMETHODIMP
AsyncFaviconDataReady::OnComplete(nsIURI *aFaviconURI,
                                  uint32_t aDataLen,
                                  const uint8_t *aData, 
                                  const nsACString &aMimeType)
{
  if (!aDataLen || !aData) {
    if (mURLShortcut) {
      OnFaviconDataNotAvailable();
    }
    
    return NS_OK;
  }

  nsCOMPtr<nsIFile> icoFile;
  nsresult rv = FaviconHelper::GetOutputIconPath(mNewURI, icoFile, mURLShortcut);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsAutoString path;
  rv = icoFile->GetPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIInputStream> stream;
  rv = NS_NewByteInputStream(getter_AddRefs(stream),
                             reinterpret_cast<const char*>(aData),
                             aDataLen,
                             NS_ASSIGNMENT_DEPEND);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAutoCString mimeTypeOfInputData;
  mimeTypeOfInputData.AssignLiteral("image/vnd.microsoft.icon");
  nsCOMPtr<imgIContainer> container;
  nsCOMPtr<imgITools> imgtool = do_CreateInstance("@mozilla.org/image/tools;1");
  rv = imgtool->DecodeImageData(stream, aMimeType,
                                getter_AddRefs(container));
  NS_ENSURE_SUCCESS(rv, rv);

  RefPtr<SourceSurface> surface =
    container->GetFrame(imgIContainer::FRAME_FIRST, 0);
  NS_ENSURE_TRUE(surface, NS_ERROR_FAILURE);

  RefPtr<DataSourceSurface> dataSurface;
  IntSize size;

  if (mURLShortcut) {
    
    size.width = 48;
    size.height = 48;
    dataSurface =
      Factory::CreateDataSourceSurface(size, SurfaceFormat::B8G8R8A8);
    NS_ENSURE_TRUE(dataSurface, NS_ERROR_FAILURE);

    DataSourceSurface::MappedSurface map;
    if (!dataSurface->Map(DataSourceSurface::MapType::WRITE, &map)) {
      return NS_ERROR_FAILURE;
    }

    RefPtr<DrawTarget> dt =
      Factory::CreateDrawTargetForData(BackendType::CAIRO,
                                       map.mData,
                                       dataSurface->GetSize(),
                                       map.mStride,
                                       dataSurface->GetFormat());
    dt->FillRect(Rect(0, 0, size.width, size.height),
                 ColorPattern(Color(1.0f, 1.0f, 1.0f, 1.0f)));
    dt->DrawSurface(surface,
                    Rect(16, 16, 16, 16),
                    Rect(Point(0, 0),
                         Size(surface->GetSize().width, surface->GetSize().height)));

    dataSurface->Unmap();
  } else {
    
    
    
    
    
    
    
    size.width = surface->GetSize().width;
    size.height = surface->GetSize().height;
    dataSurface = surface->GetDataSurface();
    NS_ENSURE_TRUE(dataSurface, NS_ERROR_FAILURE);
  }

  
  
  uint8_t *data = SurfaceToPackedBGRA(dataSurface);
  if (!data) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  int32_t stride = 4 * size.width;
  int32_t dataLength = stride * size.height;

  
  nsCOMPtr<nsIRunnable> event = new AsyncEncodeAndWriteIcon(path, data,
                                                            dataLength,
                                                            stride,
                                                            size.width,
                                                            size.height,
                                                            mURLShortcut);
  mIOThread->Dispatch(event, NS_DISPATCH_NORMAL);

  return NS_OK;
}
#endif


AsyncEncodeAndWriteIcon::AsyncEncodeAndWriteIcon(const nsAString &aIconPath,
                                                 uint8_t *aBuffer,
                                                 uint32_t aBufferLength,
                                                 uint32_t aStride,
                                                 uint32_t aWidth,
                                                 uint32_t aHeight,
                                                 const bool aURLShortcut) :
  mURLShortcut(aURLShortcut),
  mIconPath(aIconPath),
  mBuffer(aBuffer),
  mBufferLength(aBufferLength),
  mStride(aStride),
  mWidth(aWidth),
  mHeight(aHeight)
{
}

NS_IMETHODIMP AsyncEncodeAndWriteIcon::Run()
{
  NS_PRECONDITION(!NS_IsMainThread(), "Should not be called on the main thread.");

  nsCOMPtr<nsIInputStream> iconStream;
  nsRefPtr<imgIEncoder> encoder =
    do_CreateInstance("@mozilla.org/image/encoder;2?"
                      "type=image/vnd.microsoft.icon");
  NS_ENSURE_TRUE(encoder, NS_ERROR_FAILURE);
  nsresult rv = encoder->InitFromData(mBuffer, mBufferLength,
                                      mWidth, mHeight,
                                      mStride,
                                      imgIEncoder::INPUT_FORMAT_HOSTARGB,
                                      EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);
  CallQueryInterface(encoder.get(), getter_AddRefs(iconStream));
  if (!iconStream) {
    return NS_ERROR_FAILURE;
  }

  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIFile> icoFile
    = do_CreateInstance("@mozilla.org/file/local;1");
  NS_ENSURE_TRUE(icoFile, NS_ERROR_FAILURE);
  rv = icoFile->InitWithPath(mIconPath);

  
  nsCOMPtr<nsIFile> dirPath;
  icoFile->GetParent(getter_AddRefs(dirPath));
  rv = (dirPath->Create(nsIFile::DIRECTORY_TYPE, 0777));
  if (NS_FAILED(rv) && rv != NS_ERROR_FILE_ALREADY_EXISTS) {
    return rv;
  }

  
  nsCOMPtr<nsIOutputStream> outputStream;
  rv = NS_NewLocalFileOutputStream(getter_AddRefs(outputStream), icoFile);
  NS_ENSURE_SUCCESS(rv, rv);

  
  uint64_t bufSize64;
  rv = iconStream->Available(&bufSize64);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(bufSize64 <= UINT32_MAX, NS_ERROR_FILE_TOO_BIG);

  uint32_t bufSize = (uint32_t)bufSize64;

  
  
  nsCOMPtr<nsIOutputStream> bufferedOutputStream;
  rv = NS_NewBufferedOutputStream(getter_AddRefs(bufferedOutputStream),
                                  outputStream, bufSize);
  NS_ENSURE_SUCCESS(rv, rv);

  
  uint32_t wrote;
  rv = bufferedOutputStream->WriteFrom(iconStream, bufSize, &wrote);
  NS_ASSERTION(bufSize == wrote, 
              "Icon wrote size should be equal to requested write size");

  
  bufferedOutputStream->Close();
  outputStream->Close();
  if (mURLShortcut) {
    SendMessage(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETNONCLIENTMETRICS, 0);
  }
  return rv;
}

AsyncEncodeAndWriteIcon::~AsyncEncodeAndWriteIcon()
{
}

AsyncDeleteIconFromDisk::AsyncDeleteIconFromDisk(const nsAString &aIconPath)
  : mIconPath(aIconPath)
{
}

NS_IMETHODIMP AsyncDeleteIconFromDisk::Run()
{
  
  nsCOMPtr<nsIFile> icoFile = do_CreateInstance("@mozilla.org/file/local;1");
  NS_ENSURE_TRUE(icoFile, NS_ERROR_FAILURE);
  nsresult rv = icoFile->InitWithPath(mIconPath);
  NS_ENSURE_SUCCESS(rv, rv);

  
  bool exists;
  rv = icoFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (StringTail(mIconPath, 4).LowerCaseEqualsASCII(".ico")) {
    
    bool exists;
    if (NS_FAILED(icoFile->Exists(&exists)) || !exists)
      return NS_ERROR_FAILURE;

    
    icoFile->Remove(false);
  }

  return NS_OK;
}

AsyncDeleteIconFromDisk::~AsyncDeleteIconFromDisk()
{
}

AsyncDeleteAllFaviconsFromDisk::
  AsyncDeleteAllFaviconsFromDisk(bool aIgnoreRecent)
  : mIgnoreRecent(aIgnoreRecent)
{
}

NS_IMETHODIMP AsyncDeleteAllFaviconsFromDisk::Run()
{
  
  nsCOMPtr<nsIFile> jumpListCacheDir;
  nsresult rv = NS_GetSpecialDirectory("ProfLDS", 
    getter_AddRefs(jumpListCacheDir));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = jumpListCacheDir->AppendNative(
      nsDependentCString(FaviconHelper::kJumpListCacheDir));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsISimpleEnumerator> entries;
  rv = jumpListCacheDir->GetDirectoryEntries(getter_AddRefs(entries));
  NS_ENSURE_SUCCESS(rv, rv);

  
  do {
    bool hasMore = false;
    if (NS_FAILED(entries->HasMoreElements(&hasMore)) || !hasMore)
      break;

    nsCOMPtr<nsISupports> supp;
    if (NS_FAILED(entries->GetNext(getter_AddRefs(supp))))
      break;

    nsCOMPtr<nsIFile> currFile(do_QueryInterface(supp));
    nsAutoString path;
    if (NS_FAILED(currFile->GetPath(path)))
      continue;

    if (StringTail(path, 4).LowerCaseEqualsASCII(".ico")) {
      
      bool exists;
      if (NS_FAILED(currFile->Exists(&exists)) || !exists)
        continue;

      if (mIgnoreRecent) {
        
        
        int64_t fileModTime = 0;
        rv = currFile->GetLastModifiedTime(&fileModTime);
        fileModTime /= PR_MSEC_PER_SEC;
        
        
        
        int32_t icoNoDeleteSeconds =
          FaviconHelper::GetICOCacheSecondsTimeout() + 600;
        int64_t nowTime = PR_Now() / int64_t(PR_USEC_PER_SEC);
        if (NS_FAILED(rv) ||
          (nowTime - fileModTime) < icoNoDeleteSeconds) {
          continue;
        }
      }

      
      currFile->Remove(false);
    }
  } while(true);

  return NS_OK;
}

AsyncDeleteAllFaviconsFromDisk::~AsyncDeleteAllFaviconsFromDisk()
{
}












nsresult FaviconHelper::ObtainCachedIconFile(nsCOMPtr<nsIURI> aFaviconPageURI,
                                             nsString &aICOFilePath,
                                             nsCOMPtr<nsIThread> &aIOThread,
                                             bool aURLShortcut)
{
  
  nsCOMPtr<nsIFile> icoFile;
  nsresult rv = GetOutputIconPath(aFaviconPageURI, icoFile, aURLShortcut);
  NS_ENSURE_SUCCESS(rv, rv);

  
  bool exists;
  rv = icoFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (exists) {

    
    int64_t fileModTime = 0;
    rv = icoFile->GetLastModifiedTime(&fileModTime);
    fileModTime /= PR_MSEC_PER_SEC;
    int32_t icoReCacheSecondsTimeout = GetICOCacheSecondsTimeout();
    int64_t nowTime = PR_Now() / int64_t(PR_USEC_PER_SEC);

    
    
    
    if (NS_FAILED(rv) ||
        (nowTime - fileModTime) > icoReCacheSecondsTimeout) {
        CacheIconFileFromFaviconURIAsync(aFaviconPageURI, icoFile, aIOThread, aURLShortcut);
        return NS_ERROR_NOT_AVAILABLE;
    }
  } else {

    
    
    CacheIconFileFromFaviconURIAsync(aFaviconPageURI, icoFile, aIOThread, aURLShortcut);
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  rv = icoFile->GetPath(aICOFilePath);
  return rv;
}

nsresult FaviconHelper::HashURI(nsCOMPtr<nsICryptoHash> &aCryptoHash, 
                                nsIURI *aUri, 
                                nsACString& aUriHash)
{
  if (!aUri)
    return NS_ERROR_INVALID_ARG;

  nsAutoCString spec;
  nsresult rv = aUri->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aCryptoHash) {
    aCryptoHash = do_CreateInstance(NS_CRYPTO_HASH_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = aCryptoHash->Init(nsICryptoHash::MD5);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aCryptoHash->Update(reinterpret_cast<const uint8_t*>(spec.BeginReading()), 
                           spec.Length());
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aCryptoHash->Finish(true, aUriHash);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}






nsresult FaviconHelper::GetOutputIconPath(nsCOMPtr<nsIURI> aFaviconPageURI,
  nsCOMPtr<nsIFile> &aICOFile,
  bool aURLShortcut)
{
  
  nsAutoCString inputURIHash;
  nsCOMPtr<nsICryptoHash> cryptoHash;
  nsresult rv = HashURI(cryptoHash, aFaviconPageURI,
                        inputURIHash);
  NS_ENSURE_SUCCESS(rv, rv);
  char* cur = inputURIHash.BeginWriting();
  char* end = inputURIHash.EndWriting();
  for (; cur < end; ++cur) {
    if ('/' == *cur) {
      *cur = '_';
    }
  }

  
  rv = NS_GetSpecialDirectory("ProfLDS", getter_AddRefs(aICOFile));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!aURLShortcut)
    rv = aICOFile->AppendNative(nsDependentCString(kJumpListCacheDir));
  else
    rv = aICOFile->AppendNative(nsDependentCString(kShortcutCacheDir));
  NS_ENSURE_SUCCESS(rv, rv);

  
  inputURIHash.AppendLiteral(".ico");
  rv = aICOFile->AppendNative(inputURIHash);

  return rv;
}



nsresult 
  FaviconHelper::CacheIconFileFromFaviconURIAsync(nsCOMPtr<nsIURI> aFaviconPageURI,
                                                  nsCOMPtr<nsIFile> aICOFile,
                                                  nsCOMPtr<nsIThread> &aIOThread,
                                                  bool aURLShortcut)
{
#ifdef MOZ_PLACES
  
  nsCOMPtr<mozIAsyncFavicons> favIconSvc(
    do_GetService("@mozilla.org/browser/favicon-service;1"));
  NS_ENSURE_TRUE(favIconSvc, NS_ERROR_FAILURE);

  nsCOMPtr<nsIFaviconDataCallback> callback = 
    new mozilla::widget::AsyncFaviconDataReady(aFaviconPageURI, 
                                               aIOThread, 
                                               aURLShortcut);

  favIconSvc->GetFaviconDataForPage(aFaviconPageURI, callback);
#endif
  return NS_OK;
}


int32_t FaviconHelper::GetICOCacheSecondsTimeout() {

  
  
  
  
  
  const int32_t kSecondsPerDay = 86400;
  static bool alreadyObtained = false;
  static int32_t icoReCacheSecondsTimeout = kSecondsPerDay;
  if (alreadyObtained) {
    return icoReCacheSecondsTimeout;
  }

  
  const char PREF_ICOTIMEOUT[]  = "browser.taskbar.lists.icoTimeoutInSeconds";
  icoReCacheSecondsTimeout = Preferences::GetInt(PREF_ICOTIMEOUT, 
                                                 kSecondsPerDay);
  alreadyObtained = true;
  return icoReCacheSecondsTimeout;
}





bool
WinUtils::GetShellItemPath(IShellItem* aItem,
                           nsString& aResultString)
{
  NS_ENSURE_TRUE(aItem, false);
  LPWSTR str = nullptr;
  if (FAILED(aItem->GetDisplayName(SIGDN_FILESYSPATH, &str)))
    return false;
  aResultString.Assign(str);
  CoTaskMemFree(str);
  return !aResultString.IsEmpty();
}


nsIntRegion
WinUtils::ConvertHRGNToRegion(HRGN aRgn)
{
  NS_ASSERTION(aRgn, "Don't pass NULL region here");

  nsIntRegion rgn;

  DWORD size = ::GetRegionData(aRgn, 0, nullptr);
  nsAutoTArray<uint8_t,100> buffer;
  buffer.SetLength(size);

  RGNDATA* data = reinterpret_cast<RGNDATA*>(buffer.Elements());
  if (!::GetRegionData(aRgn, size, data))
    return rgn;

  if (data->rdh.nCount > MAX_RECTS_IN_REGION) {
    rgn = ToIntRect(data->rdh.rcBound);
    return rgn;
  }

  RECT* rects = reinterpret_cast<RECT*>(data->Buffer);
  for (uint32_t i = 0; i < data->rdh.nCount; ++i) {
    RECT* r = rects + i;
    rgn.Or(rgn, ToIntRect(*r));
  }

  return rgn;
}

nsIntRect
WinUtils::ToIntRect(const RECT& aRect)
{
  return nsIntRect(aRect.left, aRect.top,
                   aRect.right - aRect.left,
                   aRect.bottom - aRect.top);
}


bool
WinUtils::IsIMEEnabled(const InputContext& aInputContext)
{
  return IsIMEEnabled(aInputContext.mIMEState.mEnabled);
}


bool
WinUtils::IsIMEEnabled(IMEState::Enabled aIMEState)
{
  return (aIMEState == IMEState::ENABLED ||
          aIMEState == IMEState::PLUGIN);
}


void
WinUtils::SetupKeyModifiersSequence(nsTArray<KeyPair>* aArray,
                                    uint32_t aModifiers)
{
  for (uint32_t i = 0; i < ArrayLength(sModifierKeyMap); ++i) {
    const uint32_t* map = sModifierKeyMap[i];
    if (aModifiers & map[0]) {
      aArray->AppendElement(KeyPair(map[1], map[2]));
    }
  }
}


bool
WinUtils::ShouldHideScrollbars()
{
#ifdef MOZ_METRO
  if (XRE_GetWindowsEnvironment() == WindowsEnvironmentType_Metro) {
    return widget::winrt::MetroInput::IsInputModeImprecise();
  }
#endif 
  return false;
}

} 
} 
