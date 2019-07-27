




#ifndef mozilla_widget_WinUtils_h__
#define mozilla_widget_WinUtils_h__

#include "nscore.h"
#include <windows.h>
#include <shobjidl.h>
#include <uxtheme.h>
#include <dwmapi.h>


#undef GetMessage
#undef CreateEvent
#undef GetClassName
#undef GetBinaryType
#undef RemoveDirectory

#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsRegion.h"
#include "nsRect.h"

#include "nsIRunnable.h"
#include "nsICryptoHash.h"
#ifdef MOZ_PLACES
#include "nsIFaviconService.h"
#endif
#include "nsIDownloader.h"
#include "nsIURI.h"
#include "nsIWidget.h"
#include "nsIThread.h"

#include "mozilla/Attributes.h"







#define NS_INLINE_DECL_IUNKNOWN_REFCOUNTING(_class)                           \
public:                                                                       \
  STDMETHODIMP_(ULONG) AddRef()                                               \
  {                                                                           \
    MOZ_ASSERT_TYPE_OK_FOR_REFCOUNTING(_class)                                \
    MOZ_ASSERT(int32_t(mRefCnt) >= 0, "illegal refcnt");                      \
    NS_ASSERT_OWNINGTHREAD(_class);                                           \
    ++mRefCnt;                                                                \
    NS_LOG_ADDREF(this, mRefCnt, #_class, sizeof(*this));                     \
    return static_cast<ULONG>(mRefCnt.get());                                 \
  }                                                                           \
  STDMETHODIMP_(ULONG) Release()                                              \
  {                                                                           \
    MOZ_ASSERT(int32_t(mRefCnt) > 0,                                          \
      "Release called on object that has already been released!");            \
    NS_ASSERT_OWNINGTHREAD(_class);                                           \
    --mRefCnt;                                                                \
    NS_LOG_RELEASE(this, mRefCnt, #_class);                                   \
    if (mRefCnt == 0) {                                                       \
      NS_ASSERT_OWNINGTHREAD(_class);                                         \
      mRefCnt = 1; /* stabilize */                                            \
      delete this;                                                            \
      return 0;                                                               \
    }                                                                         \
    return static_cast<ULONG>(mRefCnt.get());                                 \
  }                                                                           \
protected:                                                                    \
  nsAutoRefCnt mRefCnt;                                                       \
  NS_DECL_OWNINGTHREAD                                                        \
public:

class nsWindow;
class nsWindowBase;
struct KeyPair;

namespace mozilla {
namespace widget {


typedef struct {
  const char * mStr;
  UINT         mId;
} EventMsgInfo;
extern EventMsgInfo gAllEvents[];




#ifndef QS_RAWINPUT
#define QS_RAWINPUT 0x0400
#endif

#ifndef QS_TOUCH
#define QS_TOUCH    0x0800
#define QS_POINTER  0x1000
#endif

#define MOZ_QS_ALLEVENT (QS_KEY | QS_MOUSEMOVE | QS_MOUSEBUTTON | \
                         QS_POSTMESSAGE | QS_TIMER | QS_PAINT |   \
                         QS_SENDMESSAGE | QS_HOTKEY |             \
                         QS_ALLPOSTMESSAGE | QS_RAWINPUT |        \
                         QS_TOUCH | QS_POINTER)


#define LogFunction() mozilla::widget::WinUtils::Log(__FUNCTION__)
#define LogThread() mozilla::widget::WinUtils::Log("%s: IsMainThread:%d ThreadId:%X", __FUNCTION__, NS_IsMainThread(), GetCurrentThreadId())
#define LogThis() mozilla::widget::WinUtils::Log("[%X] %s", this, __FUNCTION__)
#define LogException(e) mozilla::widget::WinUtils::Log("%s Exception:%s", __FUNCTION__, e->ToString()->Data())
#define LogHRESULT(hr) mozilla::widget::WinUtils::Log("%s hr=%X", __FUNCTION__, hr)

#ifdef MOZ_PLACES
class myDownloadObserver final : public nsIDownloadObserver
{
  ~myDownloadObserver() {}

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOWNLOADOBSERVER
};
#endif

class WinUtils {
public:
  



  static double LogToPhysFactor();
  static double PhysToLogFactor();
  static int32_t LogToPhys(double aValue);
  static double PhysToLog(int32_t aValue);

  



  static void Log(const char *fmt, ...);
  static void LogW(const wchar_t *fmt, ...);

  





  static bool PeekMessage(LPMSG aMsg, HWND aWnd, UINT aFirstMessage,
                          UINT aLastMessage, UINT aOption);
  static bool GetMessage(LPMSG aMsg, HWND aWnd, UINT aFirstMessage,
                         UINT aLastMessage);

  










  static void WaitForMessage(DWORD aTimeoutMs = INFINITE);

  













  static bool GetRegistryKey(HKEY aRoot,
                             char16ptr_t aKeyName,
                             char16ptr_t aValueName,
                             wchar_t* aBuffer,
                             DWORD aBufferLength);

  







  static bool HasRegistryKey(HKEY aRoot,
                             char16ptr_t aKeyName);

  




















  static HWND GetTopLevelHWND(HWND aWnd, 
                              bool aStopIfNotChild = false, 
                              bool aStopIfNotPopup = true);

  







  static bool SetNSWindowBasePtr(HWND aWnd, nsWindowBase* aWidget);
  static nsWindowBase* GetNSWindowBasePtr(HWND aWnd);
  static nsWindow* GetNSWindowPtr(HWND aWnd);

  


  static int32_t GetMonitorCount();

  



  static bool IsOurProcessWindow(HWND aWnd);

  






  static HWND FindOurProcessWindow(HWND aWnd);

  






  static HWND FindOurWindowAtPoint(const POINT& aPointInScreen);

  



  static MSG InitMSG(UINT aMessage, WPARAM wParam, LPARAM lParam, HWND aWnd);

  




  static WORD GetScanCode(LPARAM aLParam)
  {
    return (aLParam >> 16) & 0xFF;
  }

  



  static bool IsExtendedScanCode(LPARAM aLParam)
  {
    return (aLParam & 0x1000000) != 0;
  }

  




  static UINT GetInternalMessage(UINT aNativeMessage);

  



  static UINT GetNativeMessage(UINT aInternalMessage);

  




  static uint16_t GetMouseInputSource();

  static bool GetIsMouseFromTouch(uint32_t aEventType);

  



  static HRESULT SHCreateItemFromParsingName(PCWSTR pszPath, IBindCtx *pbc,
                                             REFIID riid, void **ppv);

  



  static HRESULT SHGetKnownFolderPath(REFKNOWNFOLDERID rfid,
                                      DWORD dwFlags,
                                      HANDLE hToken,
                                      PWSTR *ppszPath);
  







  static bool GetShellItemPath(IShellItem* aItem,
                               nsString& aResultString);

  





  static nsIntRegion ConvertHRGNToRegion(HRGN aRgn);

  





  static nsIntRect ToIntRect(const RECT& aRect);

  



  static void InvalidatePluginAsWorkaround(nsIWidget *aWidget, const nsIntRect &aRect);

  


  static bool IsIMEEnabled(const InputContext& aInputContext);
  static bool IsIMEEnabled(IMEState::Enabled aIMEState);

  



  static void SetupKeyModifiersSequence(nsTArray<KeyPair>* aArray,
                                        uint32_t aModifiers);

  
  typedef HRESULT (WINAPI*DwmExtendFrameIntoClientAreaProc)(HWND hWnd, const MARGINS *pMarInset);
  typedef HRESULT (WINAPI*DwmIsCompositionEnabledProc)(BOOL *pfEnabled);
  typedef HRESULT (WINAPI*DwmSetIconicThumbnailProc)(HWND hWnd, HBITMAP hBitmap, DWORD dwSITFlags);
  typedef HRESULT (WINAPI*DwmSetIconicLivePreviewBitmapProc)(HWND hWnd, HBITMAP hBitmap, POINT *pptClient, DWORD dwSITFlags);
  typedef HRESULT (WINAPI*DwmGetWindowAttributeProc)(HWND hWnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);
  typedef HRESULT (WINAPI*DwmSetWindowAttributeProc)(HWND hWnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);
  typedef HRESULT (WINAPI*DwmInvalidateIconicBitmapsProc)(HWND hWnd);
  typedef HRESULT (WINAPI*DwmDefWindowProcProc)(HWND hWnd, UINT msg, LPARAM lParam, WPARAM wParam, LRESULT *aRetValue);
  typedef HRESULT (WINAPI*DwmGetCompositionTimingInfoProc)(HWND hWnd, DWM_TIMING_INFO *info);
  typedef HRESULT (WINAPI*DwmFlushProc)(void);

  static DwmExtendFrameIntoClientAreaProc dwmExtendFrameIntoClientAreaPtr;
  static DwmIsCompositionEnabledProc dwmIsCompositionEnabledPtr;
  static DwmSetIconicThumbnailProc dwmSetIconicThumbnailPtr;
  static DwmSetIconicLivePreviewBitmapProc dwmSetIconicLivePreviewBitmapPtr;
  static DwmGetWindowAttributeProc dwmGetWindowAttributePtr;
  static DwmSetWindowAttributeProc dwmSetWindowAttributePtr;
  static DwmInvalidateIconicBitmapsProc dwmInvalidateIconicBitmapsPtr;
  static DwmDefWindowProcProc dwmDwmDefWindowProcPtr;
  static DwmGetCompositionTimingInfoProc dwmGetCompositionTimingInfoPtr;
  static DwmFlushProc dwmFlushProcPtr;

  static void Initialize();

  static bool ShouldHideScrollbars();

private:
  typedef HRESULT (WINAPI * SHCreateItemFromParsingNamePtr)(PCWSTR pszPath,
                                                            IBindCtx *pbc,
                                                            REFIID riid,
                                                            void **ppv);
  static SHCreateItemFromParsingNamePtr sCreateItemFromParsingName;
  typedef HRESULT (WINAPI * SHGetKnownFolderPathPtr)(REFKNOWNFOLDERID rfid,
                                                     DWORD dwFlags,
                                                     HANDLE hToken,
                                                     PWSTR *ppszPath);
  static SHGetKnownFolderPathPtr sGetKnownFolderPath;
};

#ifdef MOZ_PLACES
class AsyncFaviconDataReady final : public nsIFaviconDataCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFAVICONDATACALLBACK
  
  AsyncFaviconDataReady(nsIURI *aNewURI, 
                        nsCOMPtr<nsIThread> &aIOThread, 
                        const bool aURLShortcut);
  nsresult OnFaviconDataNotAvailable(void);
private:
  ~AsyncFaviconDataReady() {}

  nsCOMPtr<nsIURI> mNewURI;
  nsCOMPtr<nsIThread> mIOThread;
  const bool mURLShortcut;
};
#endif




class AsyncEncodeAndWriteIcon : public nsIRunnable
{
public:
  const bool mURLShortcut;
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  
  AsyncEncodeAndWriteIcon(const nsAString &aIconPath,
                          uint8_t *aData, uint32_t aDataLen, uint32_t aStride,
                          uint32_t aWidth, uint32_t aHeight,
                          const bool aURLShortcut);

private:
  virtual ~AsyncEncodeAndWriteIcon();

  nsAutoString mIconPath;
  nsAutoArrayPtr<uint8_t> mBuffer;
  HMODULE sDwmDLL;
  uint32_t mBufferLength;
  uint32_t mStride;
  uint32_t mWidth;
  uint32_t mHeight;
};


class AsyncDeleteIconFromDisk : public nsIRunnable
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  AsyncDeleteIconFromDisk(const nsAString &aIconPath);

private:
  virtual ~AsyncDeleteIconFromDisk();

  nsAutoString mIconPath;
};

class AsyncDeleteAllFaviconsFromDisk : public nsIRunnable
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  AsyncDeleteAllFaviconsFromDisk(bool aIgnoreRecent = false);

private:
  virtual ~AsyncDeleteAllFaviconsFromDisk();

  int32_t mIcoNoDeleteSeconds;
  bool mIgnoreRecent;
};

class FaviconHelper
{
public:
  static const char kJumpListCacheDir[];
  static const char kShortcutCacheDir[];
  static nsresult ObtainCachedIconFile(nsCOMPtr<nsIURI> aFaviconPageURI,
                                       nsString &aICOFilePath,
                                       nsCOMPtr<nsIThread> &aIOThread,
                                       bool aURLShortcut);

  static nsresult HashURI(nsCOMPtr<nsICryptoHash> &aCryptoHash, 
                          nsIURI *aUri,
                          nsACString& aUriHash);

  static nsresult GetOutputIconPath(nsCOMPtr<nsIURI> aFaviconPageURI,
                                    nsCOMPtr<nsIFile> &aICOFile,
                                    bool aURLShortcut);

  static nsresult 
  CacheIconFileFromFaviconURIAsync(nsCOMPtr<nsIURI> aFaviconPageURI,
                                   nsCOMPtr<nsIFile> aICOFile,
                                   nsCOMPtr<nsIThread> &aIOThread,
                                   bool aURLShortcut);

  static int32_t GetICOCacheSecondsTimeout();
};



} 
} 

#endif
