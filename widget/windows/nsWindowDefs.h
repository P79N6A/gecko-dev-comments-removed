




#ifndef WindowDefs_h__
#define WindowDefs_h__





#include "nsBaseWidget.h"
#include "nsdefs.h"
#include "resource.h"









#define MOZ_WM_APP_QUIT                   (WM_APP+0x0300)

#define MOZ_WM_TRACE                      (WM_APP+0x0301)


#define MOZ_WM_MOUSEVWHEEL                (WM_APP+0x0310)
#define MOZ_WM_MOUSEHWHEEL                (WM_APP+0x0311)
#define MOZ_WM_VSCROLL                    (WM_APP+0x0312)
#define MOZ_WM_HSCROLL                    (WM_APP+0x0313)
#define MOZ_WM_MOUSEWHEEL_FIRST           MOZ_WM_MOUSEVWHEEL
#define MOZ_WM_MOUSEWHEEL_LAST            MOZ_WM_HSCROLL


#define MOZ_WM_REACTIVATE                 (WM_APP+0x0314)



#define MOZ_WM_ENSUREVISIBLE              (WM_APP+0x374F)

#ifndef SM_CXPADDEDBORDER
#define SM_CXPADDEDBORDER                 92
#endif


#ifndef SM_MAXIMUMTOUCHES
#define SM_MAXIMUMTOUCHES                 95
#endif

#ifndef WM_THEMECHANGED
#define WM_THEMECHANGED                   0x031A
#endif

#ifndef WM_GETOBJECT
#define WM_GETOBJECT                      0x03d
#endif

#ifndef PBT_APMRESUMEAUTOMATIC
#define PBT_APMRESUMEAUTOMATIC            0x0012
#endif

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL                    0x020E
#endif

#ifndef MOUSEEVENTF_HWHEEL
#define MOUSEEVENTF_HWHEEL                0x01000
#endif

#ifndef WM_MOUSELEAVE
#define WM_MOUSELEAVE                     0x02A3
#endif

#ifndef SPI_GETWHEELSCROLLCHARS
#define SPI_GETWHEELSCROLLCHARS           0x006C
#endif

#ifndef SPI_SETWHEELSCROLLCHARS
#define SPI_SETWHEELSCROLLCHARS           0x006D
#endif

#ifndef MAPVK_VSC_TO_VK
#define MAPVK_VK_TO_VSC                   0
#define MAPVK_VSC_TO_VK                   1
#define MAPVK_VK_TO_CHAR                  2
#define MAPVK_VSC_TO_VK_EX                3
#define MAPVK_VK_TO_VSC_EX                4
#endif

#ifndef WM_DWMCOMPOSITIONCHANGED
#define WM_DWMCOMPOSITIONCHANGED          0x031E
#endif
#ifndef WM_DWMNCRENDERINGCHANGED
#define WM_DWMNCRENDERINGCHANGED          0x031F
#endif
#ifndef WM_DWMCOLORIZATIONCOLORCHANGED
#define WM_DWMCOLORIZATIONCOLORCHANGED    0x0320
#endif
#ifndef WM_DWMWINDOWMAXIMIZEDCHANGE
#define WM_DWMWINDOWMAXIMIZEDCHANGE       0x0321
#endif


#define kWindowPositionSlop               20


#define MOZ_SYSCONTEXT_X_POS              20
#define MOZ_SYSCONTEXT_Y_POS              20


#define CS_XP_DROPSHADOW                  0x00020000



#define MAX_RECTS_IN_REGION               100



#ifndef WM_APPCOMMAND
#define WM_APPCOMMAND                     0x0319
#endif

#define FAPPCOMMAND_MASK                  0xF000

#ifndef WM_GETTITLEBARINFOEX
#define WM_GETTITLEBARINFOEX              0x033F
#endif

#ifndef CCHILDREN_TITLEBAR
#define CCHILDREN_TITLEBAR                5
#endif

#ifndef APPCOMMAND_BROWSER_BACKWARD
  #define APPCOMMAND_BROWSER_BACKWARD       1
  #define APPCOMMAND_BROWSER_FORWARD        2
  #define APPCOMMAND_BROWSER_REFRESH        3
  #define APPCOMMAND_BROWSER_STOP           4
  #define APPCOMMAND_BROWSER_SEARCH         5
  #define APPCOMMAND_BROWSER_FAVORITES      6
  #define APPCOMMAND_BROWSER_HOME           7

  























  #define GET_APPCOMMAND_LPARAM(lParam)     ((short)(HIWORD(lParam) & ~FAPPCOMMAND_MASK))

  





#endif 


#define TABLET_INK_SIGNATURE 0xFFFFFF00
#define TABLET_INK_CHECK     0xFF515700
#define TABLET_INK_TOUCH     0x00000080
#define MOUSE_INPUT_SOURCE() WinUtils::GetMouseInputSource()








typedef enum
{
    TRI_UNKNOWN = -1,
    TRI_FALSE = 0,
    TRI_TRUE = 1
} TriStateBool;
















const uint32_t kMaxClassNameLength   = 40;
const char kClassNameHidden[]        = "MozillaHiddenWindowClass";
const char kClassNameGeneral[]       = "MozillaWindowClass";
const char kClassNameDialog[]        = "MozillaDialogClass";
const char kClassNameDropShadow[]    = "MozillaDropShadowWindowClass";
const char kClassNameTemp[]          = "MozillaTempWindowClass";








struct KeyPair {
  uint8_t mGeneral;
  uint8_t mSpecific;
  KeyPair(uint32_t aGeneral, uint32_t aSpecific)
    : mGeneral(uint8_t(aGeneral)), mSpecific(uint8_t(aSpecific)) {}
};

#if (WINVER < 0x0600)
struct TITLEBARINFOEX
{
    DWORD cbSize;
    RECT rcTitleBar;
    DWORD rgstate[CCHILDREN_TITLEBAR + 1];
    RECT rgrect[CCHILDREN_TITLEBAR + 1];
};
#endif

namespace mozilla {
namespace widget {

struct MSGResult
{
  
  LRESULT& mResult;
  
  bool mConsumed;

  MSGResult(LRESULT* aResult = nullptr) :
    mResult(aResult ? *aResult : mDefaultResult), mConsumed(false)
  {
  }

private:
  LRESULT mDefaultResult;
};

} 
} 







#define NSRGB_2_COLOREF(color) \
      RGB(NS_GET_R(color),NS_GET_G(color),NS_GET_B(color))
#define COLOREF_2_NSRGB(color) \
      NS_RGB(GetRValue(color), GetGValue(color), GetBValue(color))

#define VERIFY_WINDOW_STYLE(s) \
      NS_ASSERTION(((s) & (WS_CHILD | WS_POPUP)) != (WS_CHILD | WS_POPUP), \
      "WS_POPUP and WS_CHILD are mutually exclusive")

#endif 
