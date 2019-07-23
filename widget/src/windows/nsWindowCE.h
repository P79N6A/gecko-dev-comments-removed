




































#include "nsWindow.h"

#ifndef WindowCE_h__
#define WindowCE_h__





#ifdef WINCE

#include "aygshell.h"
#include "imm.h"
#ifdef WINCE_WINDOWS_MOBILE
#define WINCE_HAVE_SOFTKB
#include "tpcshell.h"
#else
#undef WINCE_HAVE_SOFTKB
#include "winuserm.h"
#endif

#define IDI_APPLICATION         MAKEINTRESOURCE(32512)

#define SetWindowLongPtrA       SetWindowLongW
#define SetWindowLongPtrW       SetWindowLongW
#define GetWindowLongPtrW       GetWindowLongW
#define GWLP_WNDPROC            GWL_WNDPROC
#define GetPropW                GetProp
#define SetPropW                SetProp
#define RemovePropW             RemoveProp

#define MapVirtualKeyEx(a,b,c)  MapVirtualKey(a,b)

#ifndef WINCE_WINDOWS_MOBILE

#define BROWSEINFOW             BROWSEINFO
#define BFFM_SETSELECTIONW      BFFM_SETSELECTION
#define SHBrowseForFolderW(a)   SHBrowseForFolder(a)
#endif


inline void FlashWindow(HWND window, BOOL ignore){}
inline BOOL IsIconic(HWND inWnd){return false;}

class nsWindowCE {
public:
  static BOOL EnumChildWindows(HWND inParent, WNDENUMPROC inFunc, LPARAM inParam);

#if defined(WINCE_HAVE_SOFTKB)
  static void ToggleSoftKB(PRBool show);
  static void CreateSoftKeyMenuBar(HWND wnd);
  static void NotifySoftKbObservers(LPRECT = NULL);
  static PRBool sSIPInTransition;
  static TriStateBool sShowSIPButton;
#endif
};

#endif 

#endif 
