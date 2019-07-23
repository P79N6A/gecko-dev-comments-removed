




































#ifndef __PromptService_h
#define __PromptService_h

#include "nsError.h"


#ifndef XP_WIN
#define XP_WIN
#endif

class nsIFactory;


extern "C" NS_EXPORT nsresult NS_NewPromptServiceFactory(nsIFactory** aFactory);
typedef nsresult (__cdecl *MakeFactoryType)(nsIFactory **);
#define kPromptServiceFactoryFuncName "NS_NewPromptServiceFactory"


extern "C" NS_EXPORT void InitPromptService(HINSTANCE instance);
typedef nsresult (__cdecl *InitPromptServiceType)(HINSTANCE instance);
#define kPromptServiceInitFuncName "InitPromptService"

#endif
