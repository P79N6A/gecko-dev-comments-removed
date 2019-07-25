





































#ifndef WindowDefs_h__
#define WindowDefs_h__





#include "nsBaseWidget.h"
#include "nsdefs.h"
#include "resource.h"









#define MOZ_WM_APP_QUIT                   (WM_APP+0x0300)

#define MOZ_WM_TRACE                      (WM_APP+0x0301)


#define WIN2K_VERSION                     0x500
#define WINXP_VERSION                     0x501
#define WIN2K3_VERSION                    0x502
#define VISTA_VERSION                     0x600
#define WIN7_VERSION                      0x601

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

#ifndef WM_MOUSELEAVE
#define WM_MOUSELEAVE                     0x02A3
#endif

#ifndef SPI_GETWHEELSCROLLCHARS
#define SPI_GETWHEELSCROLLCHARS           0x006C
#endif

#ifndef MAPVK_VSC_TO_VK
#define MAPVK_VK_TO_VSC                   0
#define MAPVK_VSC_TO_VK                   1
#define MAPVK_VK_TO_CHAR                  2
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

#if defined(WINCE)
#ifndef RDW_NOINTERNALPAINT
#define RDW_NOINTERNALPAINT     0
#endif
#ifndef ERROR
#define ERROR 0
#endif
#endif 


#if !defined(WINCE)
#define TABLET_INK_SIGNATURE 0xFFFFFF00
#define TABLET_INK_CHECK     0xFF515700
#define TABLET_INK_TOUCH     0x00000080
#define MOUSE_INPUT_SOURCE() GetMouseInputSource()
#else
#define MOUSE_INPUT_SOURCE() nsIDOMNSMouseEvent::MOZ_SOURCE_MOUSE
#endif








typedef enum
{
    TRI_UNKNOWN = -1,
    TRI_FALSE = 0,
    TRI_TRUE = 1
} TriStateBool;
















const PRUint32 kMaxClassNameLength   = 40;
const char kClassNameHidden[]        = "MozillaHiddenWindowClass";
const char kClassNameGeneral[]       = "MozillaWindowClass";
const char kClassNameDialog[]        = "MozillaDialogClass";
const char kClassNameDropShadow[]    = "MozillaDropShadowWindowClass";
const char kClassNameTemp[]          = "MozillaTempWindowClass";

static const PRUint32 sModifierKeyMap[][3] = {
  { nsIWidget::CAPS_LOCK, VK_CAPITAL, 0 },
  { nsIWidget::NUM_LOCK,  VK_NUMLOCK, 0 },
  { nsIWidget::SHIFT_L,   VK_SHIFT,   VK_LSHIFT },
  { nsIWidget::SHIFT_R,   VK_SHIFT,   VK_RSHIFT },
  { nsIWidget::CTRL_L,    VK_CONTROL, VK_LCONTROL },
  { nsIWidget::CTRL_R,    VK_CONTROL, VK_RCONTROL },
  { nsIWidget::ALT_L,     VK_MENU,    VK_LMENU },
  { nsIWidget::ALT_R,     VK_MENU,    VK_RMENU }
};








struct nsAlternativeCharCode; 
struct nsFakeCharMessage {
  UINT mCharCode;
  UINT mScanCode;
};


struct nsModifierKeyState {
  PRBool mIsShiftDown;
  PRBool mIsControlDown;
  PRBool mIsAltDown;

  nsModifierKeyState();
  nsModifierKeyState(PRBool aIsShiftDown, PRBool aIsControlDown,
                     PRBool aIsAltDown) :
    mIsShiftDown(aIsShiftDown), mIsControlDown(aIsControlDown),
    mIsAltDown(aIsAltDown)
  {
  }
};


struct KeyPair {
  PRUint8 mGeneral;
  PRUint8 mSpecific;
  KeyPair(PRUint32 aGeneral, PRUint32 aSpecific)
    : mGeneral(PRUint8(aGeneral)), mSpecific(PRUint8(aSpecific)) {}
};

#ifndef TITLEBARINFOEX
struct TITLEBARINFOEX
{
    DWORD cbSize;
    RECT rcTitleBar;
    DWORD rgstate[CCHILDREN_TITLEBAR + 1];
    RECT rgrect[CCHILDREN_TITLEBAR + 1];
};
#endif







#define NSRGB_2_COLOREF(color) \
      RGB(NS_GET_R(color),NS_GET_G(color),NS_GET_B(color))
#define COLOREF_2_NSRGB(color) \
      NS_RGB(GetRValue(color), GetGValue(color), GetBValue(color))

#define VERIFY_WINDOW_STYLE(s) \
      NS_ASSERTION(((s) & (WS_CHILD | WS_POPUP)) != (WS_CHILD | WS_POPUP), \
      "WS_POPUP and WS_CHILD are mutually exclusive")

#endif
