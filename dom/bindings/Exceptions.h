




#ifndef mozilla_dom_Exceptions_h__
#define mozilla_dom_Exceptions_h__



#include <stdint.h>
#include "jspubtd.h"
#include "nsIException.h"

class nsIStackFrame;
class nsPIDOMWindow;
template <class T>
struct already_AddRefed;

namespace mozilla {
namespace dom {

class Exception;

bool
Throw(JSContext* cx, nsresult rv, const char* sz = nullptr);


void
ThrowAndReport(nsPIDOMWindow* aWindow, nsresult aRv,
               const char* aMessage = nullptr);

bool
ThrowExceptionObject(JSContext* aCx, Exception* aException);

bool
ThrowExceptionObject(JSContext* aCx, nsIException* aException);



already_AddRefed<Exception>
CreateException(JSContext* aCx, nsresult aRv, const char* aMessage = nullptr);

already_AddRefed<nsIStackFrame>
GetCurrentJSStack();


namespace exceptions {



already_AddRefed<nsIStackFrame>
CreateStack(JSContext* aCx, int32_t aMaxDepth = -1);

already_AddRefed<nsIStackFrame>
CreateStackFrameLocation(uint32_t aLanguage,
                         const char* aFilename,
                         const char* aFunctionName,
                         int32_t aLineNumber,
                         nsIStackFrame* aCaller);

} 
} 
} 

#endif
