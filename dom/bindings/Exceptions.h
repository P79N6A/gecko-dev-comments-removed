




#ifndef mozilla_dom_Exceptions_h__
#define mozilla_dom_Exceptions_h__



#include <stdint.h>
#include "jspubtd.h"
#include "nsIException.h"

class nsIStackFrame;
template <class T>
class already_AddRefed;

namespace mozilla {
namespace dom {

class Exception;

void
Throw(JSContext* cx, nsresult rv, const char* sz);

bool
ThrowExceptionObject(JSContext* aCx, Exception* aException);

bool
ThrowExceptionObject(JSContext* aCx, nsIException* aException);


namespace exceptions {

already_AddRefed<nsIStackFrame>
CreateStack(JSContext* cx);

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
