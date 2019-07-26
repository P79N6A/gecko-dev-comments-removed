



#include "base/base_switches.h"

namespace switches {





const char kDebugOnStart[]                  = "debug-on-start";


const char kDisableBreakpad[]               = "disable-breakpad";


const char kEnableDCHECK[]                  = "enable-dcheck";


const char kFullMemoryCrashReport[]         = "full-memory-crash-report";


const char kNoErrorDialogs[]                = "noerrdialogs";



const char kTestChildProcess[]              = "test-child-process";



const char kV[]                             = "v";










const char kVModule[]                       = "vmodule";


const char kWaitForDebugger[]               = "wait-for-debugger";


const char kTraceToConsole[]                = "trace-to-console";

#if defined(OS_POSIX)




const char kEnableCrashReporter[]           = "enable-crash-reporter";
#endif

}  
