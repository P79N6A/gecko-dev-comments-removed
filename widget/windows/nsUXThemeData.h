





#ifndef __UXThemeData_h__
#define __UXThemeData_h__
#include <windows.h>
#include <uxtheme.h>

#include "nscore.h"
#include "mozilla/LookAndFeel.h"

#include <dwmapi.h>

#include "nsWindowDefs.h"


#ifndef WM_DWMCOMPOSITIONCHANGED
#define WM_DWMCOMPOSITIONCHANGED        0x031E
#endif


#ifndef WM_DWMSENDICONICTHUMBNAIL
#define WM_DWMSENDICONICTHUMBNAIL 0x0323
#define WM_DWMSENDICONICLIVEPREVIEWBITMAP 0x0326
#endif

#define DWMWA_FORCE_ICONIC_REPRESENTATION 7
#define DWMWA_HAS_ICONIC_BITMAP           10

enum nsUXThemeClass {
  eUXButton = 0,
  eUXEdit,
  eUXTooltip,
  eUXRebar,
  eUXMediaRebar,
  eUXCommunicationsRebar,
  eUXBrowserTabBarRebar,
  eUXToolbar,
  eUXMediaToolbar,
  eUXCommunicationsToolbar,
  eUXProgress,
  eUXTab,
  eUXScrollbar,
  eUXTrackbar,
  eUXSpin,
  eUXStatus,
  eUXCombobox,
  eUXHeader,
  eUXListview,
  eUXMenu,
  eUXWindowFrame,
  eUXNumClasses
};


enum WindowsTheme {
  WINTHEME_UNRECOGNIZED = 0,
  WINTHEME_CLASSIC      = 1, 
  WINTHEME_AERO         = 2,
  WINTHEME_LUNA         = 3,
  WINTHEME_ROYALE       = 4,
  WINTHEME_ZUNE         = 5
};
enum WindowsThemeColor {
  WINTHEMECOLOR_UNRECOGNIZED = 0,
  WINTHEMECOLOR_NORMAL       = 1,
  WINTHEMECOLOR_HOMESTEAD    = 2,
  WINTHEMECOLOR_METALLIC     = 3
};

#define CMDBUTTONIDX_MINIMIZE    0
#define CMDBUTTONIDX_RESTORE     1
#define CMDBUTTONIDX_CLOSE       2
#define CMDBUTTONIDX_BUTTONBOX   3

class nsUXThemeData {
  static HMODULE sThemeDLL;
   static HMODULE sDwmDLL;
  static HANDLE sThemes[eUXNumClasses];
  
  static const wchar_t *GetClassName(nsUXThemeClass);

public:
  static const PRUnichar kThemeLibraryName[];
   static const PRUnichar kDwmLibraryName[];
  static bool sFlatMenus;
  static bool sTitlebarInfoPopulatedAero;
  static bool sTitlebarInfoPopulatedThemed;
  static SIZE sCommandButtons[4];
  static mozilla::LookAndFeel::WindowsTheme sThemeId;
  static bool sIsDefaultWindowsTheme;

  static void Initialize();
  static void Teardown();
  static void Invalidate();
  static HANDLE GetTheme(nsUXThemeClass cls);
  static HMODULE GetThemeDLL();
  static HMODULE GetDwmDLL();

  
  static void InitTitlebarInfo();
  static void UpdateTitlebarInfo(HWND aWnd);

  static void UpdateNativeThemeInfo();
  static mozilla::LookAndFeel::WindowsTheme GetNativeThemeId();
  static bool IsDefaultWindowTheme();

  
  typedef HRESULT (WINAPI*DwmExtendFrameIntoClientAreaProc)(HWND hWnd, const MARGINS *pMarInset);
  typedef HRESULT (WINAPI*DwmIsCompositionEnabledProc)(BOOL *pfEnabled);
  typedef HRESULT (WINAPI*DwmSetIconicThumbnailProc)(HWND hWnd, HBITMAP hBitmap, DWORD dwSITFlags);
  typedef HRESULT (WINAPI*DwmSetIconicLivePreviewBitmapProc)(HWND hWnd, HBITMAP hBitmap, POINT *pptClient, DWORD dwSITFlags);
  typedef HRESULT (WINAPI*DwmGetWindowAttributeProc)(HWND hWnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);
  typedef HRESULT (WINAPI*DwmSetWindowAttributeProc)(HWND hWnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);
  typedef HRESULT (WINAPI*DwmInvalidateIconicBitmapsProc)(HWND hWnd);
  typedef HRESULT (WINAPI*DwmDefWindowProcProc)(HWND hWnd, UINT msg, LPARAM lParam, WPARAM wParam, LRESULT *aRetValue);

  static DwmExtendFrameIntoClientAreaProc dwmExtendFrameIntoClientAreaPtr;
  static DwmIsCompositionEnabledProc dwmIsCompositionEnabledPtr;
  static DwmSetIconicThumbnailProc dwmSetIconicThumbnailPtr;
  static DwmSetIconicLivePreviewBitmapProc dwmSetIconicLivePreviewBitmapPtr;
  static DwmGetWindowAttributeProc dwmGetWindowAttributePtr;
  static DwmSetWindowAttributeProc dwmSetWindowAttributePtr;
  static DwmInvalidateIconicBitmapsProc dwmInvalidateIconicBitmapsPtr;
  static DwmDefWindowProcProc dwmDwmDefWindowProcPtr;

  
  
  
  
  
  
  static bool CheckForCompositor(bool aUpdateCache = false) {
    static BOOL sCachedValue = FALSE;
    if(aUpdateCache && dwmIsCompositionEnabledPtr) {
      dwmIsCompositionEnabledPtr(&sCachedValue);
    }
    return (sCachedValue != FALSE);
  }
};
#endif 
