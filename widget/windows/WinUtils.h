




#ifndef mozilla_widget_WinUtils_h__
#define mozilla_widget_WinUtils_h__

#include "nscore.h"
#include <windows.h>
#include <shobjidl.h>
#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsRegion.h"
#include "nsRect.h"

#include "nsThreadUtils.h"
#include "nsICryptoHash.h"
#ifdef MOZ_PLACES
#include "nsIFaviconService.h"
#endif
#include "nsIDownloader.h"
#include "nsIURI.h"

#include "mozilla/Attributes.h"

class nsWindow;

namespace mozilla {
namespace widget {

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
    
    WIN8_VERSION      = 0x602
  };
  static WinVersion GetWindowsVersion();

  





  static bool PeekMessage(LPMSG aMsg, HWND aWnd, UINT aFirstMessage,
                          UINT aLastMessage, UINT aOption);
  static bool GetMessage(LPMSG aMsg, HWND aWnd, UINT aFirstMessage,
                         UINT aLastMessage);
  













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

  





  static bool SetNSWindowPtr(HWND aWnd, nsWindow* aWindow);
  static nsWindow* GetNSWindowPtr(HWND aWnd);

  


  static int32_t GetMonitorCount();

  



  static bool IsOurProcessWindow(HWND aWnd);

  






  static HWND FindOurProcessWindow(HWND aWnd);

  






  static HWND FindOurWindowAtPoint(const POINT& aPointInScreen);

  



  static MSG InitMSG(UINT aMessage, WPARAM wParam, LPARAM lParam);

  




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




class AsyncWriteIconToDisk : public nsIRunnable
{
public:
  const bool mURLShortcut;
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  
  AsyncWriteIconToDisk(const nsAString &aIconPath,
                       const nsACString &aMimeTypeOfInputData,
                       uint8_t *aData, 
                       uint32_t aDataLen,
                       const bool aURLShortcut);
  virtual ~AsyncWriteIconToDisk();

private:
  nsAutoString mIconPath;
  nsAutoCString mMimeTypeOfInputData;
  nsAutoArrayPtr<uint8_t> mBuffer;
  uint32_t mBufferLength;
};

class AsyncDeleteIconFromDisk : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  AsyncDeleteIconFromDisk(const nsAString &aIconPath);
  virtual ~AsyncDeleteIconFromDisk();

private:
  nsAutoString mIconPath;
};

class AsyncDeleteAllFaviconsFromDisk : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
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
