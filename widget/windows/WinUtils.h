




#ifndef mozilla_widget_WinUtils_h__
#define mozilla_widget_WinUtils_h__

#include "nscore.h"
#include <windows.h>
#include <shobjidl.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsRegion.h"

#include "nsThreadUtils.h"
#include "nsICryptoHash.h"
#ifdef MOZ_PLACES
#include "nsIFaviconService.h"
#endif
#include "nsIDownloader.h"
#include "nsIURI.h"
#include "nsIWidget.h"

#include "mozilla/Attributes.h"

class nsWindow;
class nsWindowBase;
struct KeyPair;
struct nsIntRect;

namespace mozilla {
namespace widget {




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

class myDownloadObserver MOZ_FINAL : public nsIDownloadObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOWNLOADOBSERVER
};

class WinUtils {
public:
  enum WinVersion {
    WINXP_VERSION     = 0x501,
    WIN2K3_VERSION    = 0x502,
    VISTA_VERSION     = 0x600,
    WIN7_VERSION      = 0x601,
    WIN8_VERSION      = 0x602,
    WIN8_1_VERSION    = 0x603
  };
  static WinVersion GetWindowsVersion();

  
  
  static bool GetWindowsServicePackVersion(UINT& aOutMajor, UINT& aOutMinor);

  





  static bool PeekMessage(LPMSG aMsg, HWND aWnd, UINT aFirstMessage,
                          UINT aLastMessage, UINT aOption);
  static bool GetMessage(LPMSG aMsg, HWND aWnd, UINT aFirstMessage,
                         UINT aLastMessage);

  









  static void WaitForMessage();

  













  static bool GetRegistryKey(HKEY aRoot,
                             const PRUnichar* aKeyName,
                             const PRUnichar* aValueName,
                             PRUnichar* aBuffer,
                             DWORD aBufferLength);

  







  static bool HasRegistryKey(HKEY aRoot,
                             const PRUnichar* aKeyName);

  




















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

  static DwmExtendFrameIntoClientAreaProc dwmExtendFrameIntoClientAreaPtr;
  static DwmIsCompositionEnabledProc dwmIsCompositionEnabledPtr;
  static DwmSetIconicThumbnailProc dwmSetIconicThumbnailPtr;
  static DwmSetIconicLivePreviewBitmapProc dwmSetIconicLivePreviewBitmapPtr;
  static DwmGetWindowAttributeProc dwmGetWindowAttributePtr;
  static DwmSetWindowAttributeProc dwmSetWindowAttributePtr;
  static DwmInvalidateIconicBitmapsProc dwmInvalidateIconicBitmapsPtr;
  static DwmDefWindowProcProc dwmDwmDefWindowProcPtr;
  static DwmGetCompositionTimingInfoProc dwmGetCompositionTimingInfoPtr;

  static void Initialize();

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
class AsyncFaviconDataReady MOZ_FINAL : public nsIFaviconDataCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFAVICONDATACALLBACK
  
  AsyncFaviconDataReady(nsIURI *aNewURI, 
                        nsCOMPtr<nsIThread> &aIOThread, 
                        const bool aURLShortcut);
  nsresult OnFaviconDataNotAvailable(void);
private:
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
  virtual ~AsyncEncodeAndWriteIcon();

private:
  nsAutoString mIconPath;
  nsAutoCString mMimeTypeOfInputData;
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
  virtual ~AsyncDeleteIconFromDisk();

private:
  nsAutoString mIconPath;
};

class AsyncDeleteAllFaviconsFromDisk : public nsIRunnable
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  AsyncDeleteAllFaviconsFromDisk();
  virtual ~AsyncDeleteAllFaviconsFromDisk();
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
