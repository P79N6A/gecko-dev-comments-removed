







































#include "nsDebug.h"

#include "nsWindow.h"
#include "nsUXThemeConstants.h"
#include "nsUXThemeData.h"

const char
nsUXThemeData::kThemeLibraryName[] = "uxtheme.dll";

HANDLE
nsUXThemeData::sThemes[eUXNumClasses];

HMODULE
nsUXThemeData::sThemeDLL = NULL;

BOOL
nsUXThemeData::sFlatMenus = FALSE;
PRPackedBool
nsUXThemeData::sIsXPOrLater = PR_FALSE;
PRPackedBool
nsUXThemeData::sIsVistaOrLater = PR_FALSE;

nsUXThemeData::OpenThemeDataPtr nsUXThemeData::openTheme = NULL;
nsUXThemeData::CloseThemeDataPtr nsUXThemeData::closeTheme = NULL;
nsUXThemeData::DrawThemeBackgroundPtr nsUXThemeData::drawThemeBG = NULL;
nsUXThemeData::DrawThemeEdgePtr nsUXThemeData::drawThemeEdge = NULL;
nsUXThemeData::GetThemeContentRectPtr nsUXThemeData::getThemeContentRect = NULL;
nsUXThemeData::GetThemePartSizePtr nsUXThemeData::getThemePartSize = NULL;
nsUXThemeData::GetThemeSysFontPtr nsUXThemeData::getThemeSysFont = NULL;
nsUXThemeData::GetThemeColorPtr nsUXThemeData::getThemeColor = NULL;
nsUXThemeData::GetThemeMarginsPtr nsUXThemeData::getThemeMargins = NULL;
nsUXThemeData::IsAppThemedPtr nsUXThemeData::isAppThemed = NULL;
nsUXThemeData::GetCurrentThemeNamePtr nsUXThemeData::getCurrentThemeName = NULL;

void
nsUXThemeData::Teardown() {
  Invalidate();
  if(sThemeDLL)
    FreeLibrary(sThemeDLL);
}

void
nsUXThemeData::Initialize()
{
  ::ZeroMemory(sThemes, sizeof(sThemes));
  NS_ASSERTION(!sThemeDLL, "nsUXThemeData being initialized twice!");
  sThemeDLL = ::LoadLibrary(kThemeLibraryName);
  if (sThemeDLL) {
    openTheme = (OpenThemeDataPtr)GetProcAddress(sThemeDLL, "OpenThemeData");
    closeTheme = (CloseThemeDataPtr)GetProcAddress(sThemeDLL, "CloseThemeData");
    drawThemeBG = (DrawThemeBackgroundPtr)GetProcAddress(sThemeDLL, "DrawThemeBackground");
    drawThemeEdge = (DrawThemeEdgePtr)GetProcAddress(sThemeDLL, "DrawThemeEdge");
    getThemeContentRect = (GetThemeContentRectPtr)GetProcAddress(sThemeDLL, "GetThemeBackgroundContentRect");
    getThemePartSize = (GetThemePartSizePtr)GetProcAddress(sThemeDLL, "GetThemePartSize");
    getThemeSysFont = (GetThemeSysFontPtr)GetProcAddress(sThemeDLL, "GetThemeSysFont");
    getThemeColor = (GetThemeColorPtr)GetProcAddress(sThemeDLL, "GetThemeColor");
    getThemeMargins = (GetThemeMarginsPtr)GetProcAddress(sThemeDLL, "GetThemeMargins");
    isAppThemed = (IsAppThemedPtr)GetProcAddress(sThemeDLL, "IsAppThemed");
    getCurrentThemeName = (GetCurrentThemeNamePtr)GetProcAddress(sThemeDLL, "GetCurrentThemeName");
  }

  PRInt32 version = ::GetWindowsVersion();
  sIsXPOrLater = version >= WINXP_VERSION;
  sIsVistaOrLater = version >= VISTA_VERSION;
  Invalidate();
}

void
nsUXThemeData::Invalidate() {
  for(int i = 0; i < eUXNumClasses; i++) {
    if(sThemes[i]) {
      closeTheme(sThemes[i]);
      sThemes[i] = NULL;
    }
  }
  if (sIsXPOrLater) {
    BOOL useFlat = PR_FALSE;
    sFlatMenus = ::SystemParametersInfo(SPI_GETFLATMENU, 0, &useFlat, 0) ?
                     useFlat : PR_FALSE;
  } else {
    
    
    
    
    sFlatMenus = PR_FALSE;
  }
}

HANDLE
nsUXThemeData::GetTheme(nsUXThemeClass cls) {
  NS_ASSERTION(cls < eUXNumClasses, "Invalid theme class!");
  if(!sThemeDLL)
    return NULL;
  if(!sThemes[cls])
  {
    sThemes[cls] = openTheme(NULL, GetClassName(cls));
  }
  return sThemes[cls];
}

const wchar_t *nsUXThemeData::GetClassName(nsUXThemeClass cls) {
  switch(cls) {
    case eUXButton:
      return L"Button";
    case eUXEdit:
      return L"Edit";
    case eUXTooltip:
      return L"Tooltip";
    case eUXRebar:
      return L"Rebar";
    case eUXMediaRebar:
      return L"Media::Rebar";
    case eUXCommunicationsRebar:
      return L"Communications::Rebar";
    case eUXBrowserTabBarRebar:
      return L"BrowserTabBar::Rebar";
    case eUXToolbar:
      return L"Toolbar";
    case eUXMediaToolbar:
      return L"Media::Toolbar";
    case eUXCommunicationsToolbar:
      return L"Communications::Toolbar";
    case eUXProgress:
      return L"Progress";
    case eUXTab:
      return L"Tab";
    case eUXScrollbar:
      return L"Scrollbar";
    case eUXTrackbar:
      return L"Trackbar";
    case eUXSpin:
      return L"Spin";
    case eUXStatus:
      return L"Status";
    case eUXCombobox:
      return L"Combobox";
    case eUXHeader:
      return L"Header";
    case eUXListview:
      return L"Listview";
    case eUXMenu:
      return L"Menu";
    default:
      NS_NOTREACHED("unknown uxtheme class");
      return L"";
  }
}
