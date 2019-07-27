







#ifndef BASE_DEBUG_DEBUGGER_H
#define BASE_DEBUG_DEBUGGER_H

#include "base/base_export.h"

namespace base {
namespace debug {



BASE_EXPORT bool SpawnDebuggerOnProcess(unsigned process_id);



BASE_EXPORT bool WaitForDebugger(int wait_seconds, bool silent);








BASE_EXPORT bool BeingDebugged();


BASE_EXPORT void BreakDebugger();






BASE_EXPORT void SetSuppressDebugUI(bool suppress);
BASE_EXPORT bool IsDebugUISuppressed();

}  
}  

#endif  
