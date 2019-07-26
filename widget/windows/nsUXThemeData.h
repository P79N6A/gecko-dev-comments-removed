





#ifndef __UXThemeData_h__
#define __UXThemeData_h__
#include <windows.h>
#include <uxtheme.h>

#include "nscore.h"
#include "mozilla/LookAndFeel.h"
#include "WinUtils.h"

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
  WINTHEME_ZUNE         = 5,
  WINTHEME_AERO_LITE    = 6
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
  static HANDLE sThemes[eUXNumClasses];
  
  static const wchar_t *GetClassName(nsUXThemeClass);

public:
  static const wchar_t kThemeLibraryName[];
  static bool sFlatMenus;
  static bool sTitlebarInfoPopulatedAero;
  static bool sTitlebarInfoPopulatedThemed;
  static SIZE sCommandButtons[4];
  static mozilla::LookAndFeel::WindowsTheme sThemeId;
  static bool sIsDefaultWindowsTheme;
  static bool sIsHighContrastOn;

  static void Initialize();
  static void Teardown();
  static void Invalidate();
  static HANDLE GetTheme(nsUXThemeClass cls);
  static HMODULE GetThemeDLL();

  
  static void InitTitlebarInfo();
  static void UpdateTitlebarInfo(HWND aWnd);

  static void UpdateNativeThemeInfo();
  static mozilla::LookAndFeel::WindowsTheme GetNativeThemeId();
  static bool IsDefaultWindowTheme();
  static bool IsHighContrastOn();

  
  
  
  
  
  
  static bool CheckForCompositor(bool aUpdateCache = false);
};
#endif 
