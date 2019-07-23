






































#include <windows.h>
#include "nscore.h"

#ifndef WM_DWMCOMPOSITIONCHANGED
#define WM_DWMCOMPOSITIONCHANGED        0x031E
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
  eUXNumClasses
};

struct MARGINS
{
  int cxLeftWidth;
  int cxRightWidth;
  int cyTopHeight;
  int cyBottomHeight;
};

class nsUXThemeData {
  static HMODULE sThemeDLL;
  static HMODULE sDwmDLL;
  static HANDLE sThemes[eUXNumClasses];
  
  static const wchar_t *GetClassName(nsUXThemeClass);

public:
  static const PRUnichar kThemeLibraryName[];
  static const PRUnichar kDwmLibraryName[];
  static BOOL sFlatMenus;
  static PRPackedBool sIsXPOrLater;
  static PRPackedBool sIsVistaOrLater;
  static PRPackedBool sHaveCompositor;
  static void Initialize();
  static void Teardown();
  static void Invalidate();
  static HANDLE GetTheme(nsUXThemeClass cls);

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

  
  typedef HRESULT (WINAPI*DwmExtendFrameIntoClientAreaProc)(HWND hWnd, const MARGINS *pMarInset);
  typedef HRESULT (WINAPI*DwmIsCompositionEnabledProc)(BOOL *pfEnabled);

  static DwmExtendFrameIntoClientAreaProc dwmExtendFrameIntoClientAreaPtr;
  static DwmIsCompositionEnabledProc dwmIsCompositionEnabledPtr;

  static PRBool CheckForCompositor() {
    BOOL compositionIsEnabled = FALSE;
    if(dwmIsCompositionEnabledPtr)
      dwmIsCompositionEnabledPtr(&compositionIsEnabled);
    return sHaveCompositor = (compositionIsEnabled != 0);
  }
};
