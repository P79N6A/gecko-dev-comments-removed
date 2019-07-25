






































#ifndef __UXThemeData_h__
#define __UXThemeData_h__
#include <windows.h>
#include <uxtheme.h>

#include "nscore.h"
#include "mozilla/LookAndFeel.h"

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
#include <dwmapi.h>
#endif

#include "nsWindowDefs.h"


#ifndef WM_DWMCOMPOSITIONCHANGED
#define WM_DWMCOMPOSITIONCHANGED        0x031E
#endif


#ifndef WM_DWMSENDICONICTHUMBNAIL
#define WM_DWMSENDICONICTHUMBNAIL 0x0323
#define WM_DWMSENDICONICLIVEPREVIEWBITMAP 0x0326
#endif

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
#define DWMWA_FORCE_ICONIC_REPRESENTATION 7
#define DWMWA_HAS_ICONIC_BITMAP           10
#endif

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
#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
   static HMODULE sDwmDLL;
#endif
  static HANDLE sThemes[eUXNumClasses];
  
  static const wchar_t *GetClassName(nsUXThemeClass);

public:
  static const PRUnichar kThemeLibraryName[];
#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
   static const PRUnichar kDwmLibraryName[];
#endif
  static BOOL sFlatMenus;
  static bool sIsXPOrLater;
  static bool sIsVistaOrLater;
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
#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
  static HMODULE GetDwmDLL();
#endif

  
  static void InitTitlebarInfo();
  static void UpdateTitlebarInfo(HWND aWnd);

  static void UpdateNativeThemeInfo();
  static mozilla::LookAndFeel::WindowsTheme GetNativeThemeId();
  static bool IsDefaultWindowTheme();

  static inline BOOL IsAppThemed() {
    return isAppThemed && isAppThemed();
  }

  static inline HRESULT GetThemeColor(nsUXThemeClass cls, int iPartId, int iStateId,
                                                   int iPropId, OUT COLORREF* pFont) {
    if(!getThemeColor)
      return E_FAIL;
    return getThemeColor(GetTheme(cls), iPartId, iStateId, iPropId, pFont);
  }

  
  typedef HANDLE (WINAPI*OpenThemeDataPtr)(HWND hwnd, LPCWSTR pszClassList);
  typedef HRESULT (WINAPI*CloseThemeDataPtr)(HANDLE hTheme);
  typedef HRESULT (WINAPI*DrawThemeBackgroundPtr)(HANDLE hTheme, HDC hdc, int iPartId, 
                                            int iStateId, const RECT *pRect,
                                            const RECT* pClipRect);
  typedef HRESULT (WINAPI*DrawThemeEdgePtr)(HANDLE hTheme, HDC hdc, int iPartId, 
                                            int iStateId, const RECT *pDestRect,
                                            uint uEdge, uint uFlags,
                                            const RECT* pContentRect);
  typedef HRESULT (WINAPI*GetThemeContentRectPtr)(HANDLE hTheme, HDC hdc, int iPartId,
                                            int iStateId, const RECT* pRect,
                                            RECT* pContentRect);
  typedef HRESULT (WINAPI*GetThemeBackgroundRegionPtr)(HANDLE hTheme, HDC hdc, int iPartId,
                                            int iStateId, const RECT* pRect,
                                            HRGN *pRegion);
  typedef HRESULT (WINAPI*GetThemePartSizePtr)(HANDLE hTheme, HDC hdc, int iPartId,
                                         int iStateId, RECT* prc, int ts,
                                         SIZE* psz);
  typedef HRESULT (WINAPI*GetThemeSysFontPtr)(HANDLE hTheme, int iFontId, OUT LOGFONT* pFont);
  typedef HRESULT (WINAPI*GetThemeColorPtr)(HANDLE hTheme, int iPartId,
                                     int iStateId, int iPropId, OUT COLORREF* pFont);
  typedef HRESULT (WINAPI*GetThemeMarginsPtr)(HANDLE hTheme, HDC hdc, int iPartId,
                                           int iStateid, int iPropId,
                                           LPRECT prc, MARGINS *pMargins);
  typedef BOOL (WINAPI*IsAppThemedPtr)(VOID);
  typedef HRESULT (WINAPI*GetCurrentThemeNamePtr)(LPWSTR pszThemeFileName, int dwMaxNameChars,
                                                  LPWSTR pszColorBuff, int cchMaxColorChars,
                                                  LPWSTR pszSizeBuff, int cchMaxSizeChars);
  typedef COLORREF (WINAPI*GetThemeSysColorPtr)(HANDLE hTheme, int iColorID);
  typedef BOOL (WINAPI*IsThemeBackgroundPartiallyTransparentPtr)(HANDLE hTheme, int iPartId, int iStateId);

  static OpenThemeDataPtr openTheme;
  static CloseThemeDataPtr closeTheme;
  static DrawThemeBackgroundPtr drawThemeBG;
  static DrawThemeEdgePtr drawThemeEdge;
  static GetThemeContentRectPtr getThemeContentRect;
  static GetThemeBackgroundRegionPtr getThemeBackgroundRegion;
  static GetThemePartSizePtr getThemePartSize;
  static GetThemeSysFontPtr getThemeSysFont;
  static GetThemeColorPtr getThemeColor;
  static GetThemeMarginsPtr getThemeMargins;
  static IsAppThemedPtr isAppThemed;
  static GetCurrentThemeNamePtr getCurrentThemeName;
  static GetThemeSysColorPtr getThemeSysColor;
  static IsThemeBackgroundPartiallyTransparentPtr isThemeBackgroundPartiallyTransparent;

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
  
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
#endif 

  
  
  
  
  
  
  static bool CheckForCompositor(bool aUpdateCache = false) {
    static BOOL sCachedValue = FALSE;
#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
    if(aUpdateCache && dwmIsCompositionEnabledPtr) {
      dwmIsCompositionEnabledPtr(&sCachedValue);
    }
#endif 
    return (sCachedValue != FALSE);
  }
};
#endif 
