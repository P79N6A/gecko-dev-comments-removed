






























#ifndef __HelperAppService_h
#define __HelperAppService_h

#include "nsError.h"


#ifndef XP_WIN
#define XP_WIN
#endif

class nsIFactory;


extern "C" NS_EXPORT nsresult NS_NewHelperAppDlgFactory(nsIFactory** aFactory);
typedef nsresult (__cdecl *MakeFactoryType)(nsIFactory **);
#define kHelperAppDlgFactoryFuncName "NS_NewHelperAppDlgFactory"


extern "C" NS_EXPORT void InitHelperAppDlg(HINSTANCE instance);
typedef nsresult (__cdecl *InitHelperAppDlgType)(HINSTANCE instance);
#define kHelperAppDlgInitFuncName "InitHelperAppDlg"

#endif
