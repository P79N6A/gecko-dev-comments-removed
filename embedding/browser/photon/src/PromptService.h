




































#ifndef __PromptService_h
#define __PromptService_h

#include "nsError.h"

class nsIFactory;

extern "C" NS_EXPORT nsresult NS_NewPromptServiceFactory(nsIFactory** aFactory);
#define kPromptServiceFactoryFuncName "NS_NewPromptServiceFactory"

extern "C" NS_EXPORT void InitPromptService( );
#define kPromptServiceInitFuncName "InitPromptService"

#endif
