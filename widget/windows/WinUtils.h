















































#ifndef mozilla_widget_WinUtils_h__
#define mozilla_widget_WinUtils_h__

#include "nscore.h"
#include <windows.h>
#include <shobjidl.h>
#include "nsAutoPtr.h"
#include "nsString.h"

class nsWindow;

namespace mozilla {
namespace widget {

class WinUtils {
public:
  enum WinVersion {
    WINXP_VERSION     = 0x501,
    WIN2K3_VERSION    = 0x502,
    VISTA_VERSION     = 0x600,
    
    WIN7_VERSION      = 0x601
    
    
  };
  static WinVersion GetWindowsVersion();

  













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

  


  static PRInt32 GetMonitorCount();

  



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

  




  static PRUint16 GetMouseInputSource();

  




  static HRESULT SHCreateItemFromParsingName(PCWSTR pszPath, IBindCtx *pbc,
                                             REFIID riid, void **ppv);

  







  static bool GetShellItemPath(IShellItem* aItem,
                               nsString& aResultString);

private:
  typedef HRESULT (WINAPI * SHCreateItemFromParsingNamePtr)(PCWSTR pszPath,
                                                            IBindCtx *pbc,
                                                            REFIID riid,
                                                            void **ppv);
  static SHCreateItemFromParsingNamePtr sCreateItemFromParsingName;

  




  static bool VistaCreateItemFromParsingNameInit();
};

} 
} 

#endif
