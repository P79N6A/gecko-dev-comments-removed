







































#include "nsLookAndFeel.h"
#include "nsXPLookAndFeel.h"
#include <windows.h>
#include <shellapi.h>
#include "nsWindow.h"

#ifndef WINCE
typedef HANDLE (WINAPI*OpenThemeDataPtr)(HWND hwnd, LPCWSTR pszClassList);
typedef HRESULT (WINAPI*CloseThemeDataPtr)(HANDLE hTheme);
typedef HRESULT (WINAPI*GetThemeColorPtr)(HANDLE hTheme, int iPartId,
                                          int iStateId, int iPropId, OUT COLORREF* pFont);
typedef BOOL (WINAPI*IsAppThemedPtr)(VOID);

static OpenThemeDataPtr openTheme = NULL;
static CloseThemeDataPtr closeTheme = NULL;
static GetThemeColorPtr getThemeColor = NULL;
static IsAppThemedPtr isAppThemed = NULL;

static const char kThemeLibraryName[] = "uxtheme.dll";
static HINSTANCE gThemeDLLInst = NULL;
static HANDLE gMenuTheme = NULL;

#define MENU_POPUPITEM 14

#define MPI_NORMAL 1
#define MPI_HOT 2
#define MPI_DISABLED 3
#define MPI_DISABLEDHOT 4


#define TMT_TEXTCOLOR 3803

#endif


#ifndef COLOR_MENUHILIGHT
#define COLOR_MENUHILIGHT    29
#endif
#ifndef SPI_GETFLATMENU
#define SPI_GETFLATMENU      0x1022
#endif
#ifndef SPI_GETMENUSHOWDELAY
#define SPI_GETMENUSHOWDELAY      106
#endif 
#ifndef WS_EX_LAYOUTRTL 
#define WS_EX_LAYOUTRTL         0x00400000L // Right to left mirroring
#endif

#ifndef WINCE
typedef UINT (CALLBACK *SHAppBarMessagePtr)(DWORD, PAPPBARDATA);
SHAppBarMessagePtr gSHAppBarMessage = NULL;
static HINSTANCE gShell32DLLInst = NULL;
#endif

static PRInt32 GetSystemParam(long flag, PRInt32 def)
{
#ifdef WINCE
    return def;
#else
    DWORD value; 
    return ::SystemParametersInfo(flag, 0, &value, 0) ? value : def;
#endif
}

nsLookAndFeel::nsLookAndFeel() : nsXPLookAndFeel()
{
#ifndef WINCE
  gShell32DLLInst = LoadLibrary("Shell32.dll");
  if (gShell32DLLInst)
  {
      gSHAppBarMessage = (SHAppBarMessagePtr) GetProcAddress(gShell32DLLInst,
                                                             "SHAppBarMessage");
  }
  gThemeDLLInst = LoadLibrary(kThemeLibraryName);
  if(gThemeDLLInst)
  {
    openTheme = (OpenThemeDataPtr)GetProcAddress(gThemeDLLInst, "OpenThemeData");
    closeTheme = (CloseThemeDataPtr)GetProcAddress(gThemeDLLInst, "CloseThemeData");
    getThemeColor = (GetThemeColorPtr)GetProcAddress(gThemeDLLInst, "GetThemeColor");
    isAppThemed = (IsAppThemedPtr)GetProcAddress(gThemeDLLInst, "IsAppThemed");
    gMenuTheme = openTheme(NULL, L"Menu");
  }
#endif
}

nsLookAndFeel::~nsLookAndFeel()
{
#ifndef WINCE
   if (gShell32DLLInst)
   {
       FreeLibrary(gShell32DLLInst);
       gShell32DLLInst = NULL;
       gSHAppBarMessage = NULL;
   }
#endif
}

nsresult nsLookAndFeel::NativeGetColor(const nsColorID aID, nscolor &aColor)
{
  nsresult res = NS_OK;

  int idx;
  switch (aID) {
    case eColor_WindowBackground:
        idx = COLOR_WINDOW;
        break;
    case eColor_WindowForeground:
        idx = COLOR_WINDOWTEXT;
        break;
    case eColor_WidgetBackground:
        idx = COLOR_BTNFACE;
        break;
    case eColor_WidgetForeground:
        idx = COLOR_BTNTEXT;
        break;
    case eColor_WidgetSelectBackground:
        idx = COLOR_HIGHLIGHT;
        break;
    case eColor_WidgetSelectForeground:
        idx = COLOR_HIGHLIGHTTEXT;
        break;
    case eColor_Widget3DHighlight:
        idx = COLOR_BTNHIGHLIGHT;
        break;
    case eColor_Widget3DShadow:
        idx = COLOR_BTNSHADOW;
        break;
    case eColor_TextBackground:
        idx = COLOR_WINDOW;
        break;
    case eColor_TextForeground:
        idx = COLOR_WINDOWTEXT;
        break;
    case eColor_TextSelectBackground:
    case eColor_IMESelectedRawTextBackground:
    case eColor_IMESelectedConvertedTextBackground:
        idx = COLOR_HIGHLIGHT;
        break;
    case eColor_TextSelectForeground:
    case eColor_IMESelectedRawTextForeground:
    case eColor_IMESelectedConvertedTextForeground:
        idx = COLOR_HIGHLIGHTTEXT;
        break;
    case eColor_IMERawInputBackground:
    case eColor_IMEConvertedTextBackground:
        aColor = NS_TRANSPARENT;
        return NS_OK;
    case eColor_IMERawInputForeground:
    case eColor_IMEConvertedTextForeground:
        aColor = NS_SAME_AS_FOREGROUND_COLOR;
        return NS_OK;
    case eColor_IMERawInputUnderline:
    case eColor_IMEConvertedTextUnderline:
        aColor = NS_SAME_AS_FOREGROUND_COLOR;
        return NS_OK;
    case eColor_IMESelectedRawTextUnderline:
    case eColor_IMESelectedConvertedTextUnderline:
        aColor = NS_TRANSPARENT;
        return NS_OK;

    
    case eColor_activeborder:
      idx = COLOR_ACTIVEBORDER;
      break;
    case eColor_activecaption:
      idx = COLOR_ACTIVECAPTION;
      break;
    case eColor_appworkspace:
      idx = COLOR_APPWORKSPACE;
      break;
    case eColor_background:
      idx = COLOR_BACKGROUND;
      break;
    case eColor_buttonface:
    case eColor__moz_buttonhoverface:
      idx = COLOR_BTNFACE;
      break;
    case eColor_buttonhighlight:
      idx = COLOR_BTNHIGHLIGHT;
      break;
    case eColor_buttonshadow:
      idx = COLOR_BTNSHADOW;
      break;
    case eColor_buttontext:
    case eColor__moz_buttonhovertext:
      idx = COLOR_BTNTEXT;
      break;
    case eColor_captiontext:
      idx = COLOR_CAPTIONTEXT;
      break;
    case eColor_graytext:
      idx = COLOR_GRAYTEXT;
      break;
    case eColor_highlight:
    case eColor__moz_menuhover:
      idx = COLOR_HIGHLIGHT;
      break;
    case eColor__moz_menubarhovertext:OSVERSIONINFOEX:
#ifndef WINCE
      if (GetWindowsVersion() < VISTA_VERSION || !isAppThemed())
#endif
      {
        
        
        idx = (GetSystemParam(SPI_GETFLATMENU, 0)) ?
                COLOR_HIGHLIGHTTEXT :
                COLOR_MENUTEXT;
        break;
      }
      
    case eColor__moz_menuhovertext:
#ifndef WINCE
      if (isAppThemed && isAppThemed() && GetWindowsVersion() >= VISTA_VERSION)
      {
        COLORREF color;
        HRESULT hr;
        hr = getThemeColor(gMenuTheme, MENU_POPUPITEM, MPI_HOT, TMT_TEXTCOLOR, &color);
        if (hr == S_OK)
        {
          aColor = COLOREF_2_NSRGB(color);
          return NS_OK;
        }
        
        else if (hr == E_HANDLE)
        {
          closeTheme(gMenuTheme);
          gMenuTheme = openTheme(NULL, L"Menu");
          
          
          getThemeColor(gMenuTheme, MENU_POPUPITEM, MPI_HOT, TMT_TEXTCOLOR, &color);
          aColor = COLOREF_2_NSRGB(color);
          return NS_OK;
        }
        
      }
#endif
    case eColor_highlighttext:
      idx = COLOR_HIGHLIGHTTEXT;
      break;
    case eColor_inactiveborder:
      idx = COLOR_INACTIVEBORDER;
      break;
    case eColor_inactivecaption:
      idx = COLOR_INACTIVECAPTION;
      break;
    case eColor_inactivecaptiontext:
      idx = COLOR_INACTIVECAPTIONTEXT;
      break;
    case eColor_infobackground:
      idx = COLOR_INFOBK;
      break;
    case eColor_infotext:
      idx = COLOR_INFOTEXT;
      break;
    case eColor_menu:
      idx = COLOR_MENU;
      break;
    case eColor_menutext:
      idx = COLOR_MENUTEXT;
      break;
    case eColor_scrollbar:
      idx = COLOR_SCROLLBAR;
      break;
    case eColor_threeddarkshadow:
      idx = COLOR_3DDKSHADOW;
      break;
    case eColor_threedface:
      idx = COLOR_3DFACE;
      break;
    case eColor_threedhighlight:
      idx = COLOR_3DHIGHLIGHT;
      break;
    case eColor_threedlightshadow:
      idx = COLOR_3DLIGHT;
      break;
    case eColor_threedshadow:
      idx = COLOR_3DSHADOW;
      break;
    case eColor_window:
      idx = COLOR_WINDOW;
      break;
    case eColor_windowframe:
      idx = COLOR_WINDOWFRAME;
      break;
    case eColor_windowtext:
      idx = COLOR_WINDOWTEXT;
      break;
    case eColor__moz_field:
      idx = COLOR_WINDOW;
      break;
    case eColor__moz_fieldtext:
      idx = COLOR_WINDOWTEXT;
      break;
    case eColor__moz_dialog:
    case eColor__moz_cellhighlight:
      idx = COLOR_3DFACE;
      break;
    case eColor__moz_dialogtext:
    case eColor__moz_cellhighlighttext:
      idx = COLOR_WINDOWTEXT;
      break;
    case eColor__moz_dragtargetzone:
      idx = COLOR_HIGHLIGHTTEXT;
      break;
    case eColor__moz_buttondefault:
      idx = COLOR_3DDKSHADOW;
      break;
    default:
      idx = COLOR_WINDOW;
      break;
    }

  DWORD color = ::GetSysColor(idx);
  aColor = COLOREF_2_NSRGB(color);

  return res;
}

NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricID aID, PRInt32 & aMetric)
{
  nsresult res = nsXPLookAndFeel::GetMetric(aID, aMetric);
  if (NS_SUCCEEDED(res))
    return res;
  res = NS_OK;

  switch (aID) {
    case eMetric_WindowTitleHeight:
        aMetric = ::GetSystemMetrics(SM_CYCAPTION);
        break;
#ifndef WINCE
    case eMetric_WindowBorderWidth:
        aMetric = ::GetSystemMetrics(SM_CXFRAME);
        break;
    case eMetric_WindowBorderHeight:
        aMetric = ::GetSystemMetrics(SM_CYFRAME);
        break;
#endif
    case eMetric_Widget3DBorder:
        aMetric = ::GetSystemMetrics(SM_CXEDGE);
        break;
    case eMetric_TextFieldBorder:
        aMetric = 3;
        break;
    case eMetric_TextFieldHeight:
        aMetric = 24;
        break;
    case eMetric_ButtonHorizontalInsidePaddingNavQuirks:
        aMetric = 10;
        break;
    case eMetric_ButtonHorizontalInsidePaddingOffsetNavQuirks:
        aMetric = 8;
        break;
    case eMetric_CheckboxSize:
        aMetric = 12;
        break;
    case eMetric_RadioboxSize:
        aMetric = 12;
        break;
    case eMetric_TextHorizontalInsideMinimumPadding:
        aMetric = 3;
        break;
    case eMetric_TextVerticalInsidePadding:
        aMetric = 0;
        break;
    case eMetric_TextShouldUseVerticalInsidePadding:
        aMetric = 0;
        break;
    case eMetric_TextShouldUseHorizontalInsideMinimumPadding:
        aMetric = 1;
        break;
    case eMetric_ListShouldUseHorizontalInsideMinimumPadding:
        aMetric = 0;
        break;
    case eMetric_ListHorizontalInsideMinimumPadding:
        aMetric = 3;
        break;
    case eMetric_ListShouldUseVerticalInsidePadding:
        aMetric = 0;
        break;
    case eMetric_ListVerticalInsidePadding:
        aMetric = 0;
        break;
    case eMetric_CaretBlinkTime:
        aMetric = (PRInt32)::GetCaretBlinkTime();
        break;
    case eMetric_CaretWidth:
        aMetric = 1;
        break;
    case eMetric_ShowCaretDuringSelection:
        aMetric = 0;
        break;
    case eMetric_SelectTextfieldsOnKeyFocus:
        
        
        aMetric = 1;
        break;
    case eMetric_SubmenuDelay:
        
        
        aMetric = GetSystemParam(SPI_GETMENUSHOWDELAY, 400);
        break;
    case eMetric_MenusCanOverlapOSBar:
        
        aMetric = 1;
        break;
    case eMetric_DragFullWindow:
        
        
        aMetric = GetSystemParam(SPI_GETDRAGFULLWINDOWS, 1);
        break;
#ifndef WINCE
    case eMetric_DragThresholdX:
        
        
        

        aMetric = ::GetSystemMetrics(SM_CXDRAG) - 1;
        break;
    case eMetric_DragThresholdY:
        aMetric = ::GetSystemMetrics(SM_CYDRAG) - 1;
        break;
    case eMetric_UseAccessibilityTheme:
        
        
        
        HIGHCONTRAST contrastThemeInfo;
        contrastThemeInfo.cbSize = sizeof(contrastThemeInfo);
        ::SystemParametersInfo(SPI_GETHIGHCONTRAST, 0, &contrastThemeInfo, 0);

        aMetric = ((contrastThemeInfo.dwFlags & HCF_HIGHCONTRASTON) != 0);
        break;
    case eMetric_IsScreenReaderActive:
        
        
        aMetric = GetSystemParam(SPI_GETSCREENREADER, 0);
      break;
#endif
    case eMetric_ScrollArrowStyle:
        aMetric = eMetric_ScrollArrowStyleSingle;
        break;
    case eMetric_ScrollSliderStyle:
        aMetric = eMetric_ScrollThumbStyleProportional;
        break;
    case eMetric_TreeOpenDelay:
        aMetric = 1000;
        break;
    case eMetric_TreeCloseDelay:
        aMetric = 0;
        break;
    case eMetric_TreeLazyScrollDelay:
        aMetric = 150;
        break;
    case eMetric_TreeScrollDelay:
        aMetric = 100;
        break;
    case eMetric_TreeScrollLinesMax:
        aMetric = 3;
        break;
#ifndef WINCE
    case eMetric_AlertNotificationOrigin:
        aMetric = 0;
        if (gSHAppBarMessage)
        {
          
          HWND shellWindow = FindWindow("Shell_TrayWnd", NULL);

          if (shellWindow != NULL)
          {
            
            APPBARDATA appBarData;
            appBarData.hWnd = shellWindow;
            appBarData.cbSize = sizeof(appBarData);
            if (gSHAppBarMessage(ABM_GETTASKBARPOS, &appBarData))
            {
              
              
              switch(appBarData.uEdge)
              {
                case ABE_LEFT:
                  aMetric = NS_ALERT_HORIZONTAL | NS_ALERT_LEFT;
                  break;
                case ABE_RIGHT:
                  aMetric = NS_ALERT_HORIZONTAL;
                  break;
                case ABE_TOP:
                  aMetric = NS_ALERT_TOP;
                  
                case ABE_BOTTOM:
                  
                  
                  if (::GetWindowLong(shellWindow, GWL_EXSTYLE) &
                        WS_EX_LAYOUTRTL)
                    aMetric |= NS_ALERT_LEFT;
                  break;
              }
            }
          }
        }
        break;
#endif

    default:
        aMetric = 0;
        res = NS_ERROR_FAILURE;
    }
  return res;
}

NS_IMETHODIMP nsLookAndFeel::GetMetric(const nsMetricFloatID aID, float & aMetric)
{
  nsresult res = nsXPLookAndFeel::GetMetric(aID, aMetric);
  if (NS_SUCCEEDED(res))
    return res;
  res = NS_OK;

  switch (aID) {
    case eMetricFloat_TextFieldVerticalInsidePadding:
        aMetric = 0.25f;
        break;
    case eMetricFloat_TextFieldHorizontalInsidePadding:
        aMetric = 1.025f;
        break;
    case eMetricFloat_TextAreaVerticalInsidePadding:
        aMetric = 0.40f;
        break;
    case eMetricFloat_TextAreaHorizontalInsidePadding:
        aMetric = 0.40f;
        break;
    case eMetricFloat_ListVerticalInsidePadding:
        aMetric = 0.10f;
        break;
    case eMetricFloat_ListHorizontalInsidePadding:
        aMetric = 0.40f;
        break;
    case eMetricFloat_ButtonVerticalInsidePadding:
        aMetric = 0.25f;
        break;
    case eMetricFloat_ButtonHorizontalInsidePadding:
        aMetric = 0.25f;
        break;
    case eMetricFloat_IMEUnderlineRelativeSize:
        aMetric = 1.0f;
        break;
    default:
        aMetric = -1.0;
        res = NS_ERROR_FAILURE;
    }
  return res;
}


PRUnichar nsLookAndFeel::GetPasswordCharacter()
{
  static PRUnichar passwordCharacter = 0;
  if (!passwordCharacter) {
    passwordCharacter = '*';
#ifndef WINCE
    OSVERSIONINFO osversion;
    osversion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    ::GetVersionEx(&osversion);
    if (osversion.dwMajorVersion > 5 ||
        osversion.dwMajorVersion == 5 && osversion.dwMinorVersion > 0)
      passwordCharacter = 0x25cf;
#endif
  }
  return passwordCharacter;
}

#ifdef NS_DEBUG

NS_IMETHODIMP nsLookAndFeel::GetNavSize(const nsMetricNavWidgetID aWidgetID,
                                        const nsMetricNavFontID   aFontID, 
                                        const PRInt32             aFontSize, 
                                        nsSize &aSize)
{
  nsresult rv = nsXPLookAndFeel::GetNavSize(aWidgetID, aFontID, aFontSize,
                                            aSize);
  if (NS_SUCCEEDED(rv))
    return rv;

  aSize.width  = 0;
  aSize.height = 0;

  if (aFontSize < 1 || aFontSize > 7) {
    return NS_ERROR_FAILURE;
  }

  PRInt32 kTextFieldWidths[2][7] = {
    {106,147,169,211,253,338,506}, 
    {152,214,237,281,366,495,732}  
  };

  PRInt32 kTextFieldHeights[2][7] = {
    {18,21,24,27,33,45,63}, 
    {18,21,24,27,34,48,67}  
  };

  PRInt32 kTextAreaWidths[2][7] = {
    {121,163,184,226,268,352,520}, 
    {163,226,247,289,373,499,730}  
  };

  PRInt32 kTextAreaHeights[2][7] = {
    {40,44,48,52,60,76,100}, 
    {40,44,48,52,62,80,106}  
  };

  switch (aWidgetID) {
    case eMetricSize_TextField:
      aSize.width  = kTextFieldWidths[aFontID][aFontSize-1];
      aSize.height = kTextFieldHeights[aFontID][aFontSize-1];
      break;
    case eMetricSize_TextArea:
      aSize.width  = kTextAreaWidths[aFontID][aFontSize-1];
      aSize.height = kTextAreaHeights[aFontID][aFontSize-1];
      break;
  } 

  return NS_OK;

}
#endif
