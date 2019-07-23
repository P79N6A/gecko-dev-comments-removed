





























#ifndef __PrintingPromptService_h
#define __PrintingPromptService_h

#include "nsError.h"


#ifndef XP_WIN
#define XP_WIN
#endif

class nsIFactory;


extern "C" NS_EXPORT nsresult NS_NewPrintingPromptServiceFactory(nsIFactory** aFactory);
typedef nsresult (__cdecl *MakeFactoryType)(nsIFactory **);
#define kPrintingPromptServiceFactoryFuncName "NS_NewPrintingPromptServiceFactory"


extern "C" NS_EXPORT void InitPrintingPromptService(HINSTANCE instance);
typedef nsresult (__cdecl *InitPrintingPromptServiceType)(HINSTANCE instance);
#define kPrintingPromptServiceInitFuncName "InitPrintingPromptService"

#endif
