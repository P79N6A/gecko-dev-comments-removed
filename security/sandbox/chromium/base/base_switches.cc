



#include "base/base_switches.h"

namespace switches {


const char kDisableBreakpad[]               = "disable-breakpad";




const char kEnableCrashReporter[]           = "enable-crash-reporter";


const char kFullMemoryCrashReport[]         = "full-memory-crash-report";




const char kLowEndDeviceMode[]              = "low-end-device-mode";


const char kNoErrorDialogs[]                = "noerrdialogs";



const char kTestChildProcess[]              = "test-child-process";



const char kV[]                             = "v";










const char kVModule[]                       = "vmodule";


const char kWaitForDebugger[]               = "wait-for-debugger";


const char kTraceToConsole[]                = "trace-to-console";



const char kTraceToFile[]                   = "trace-to-file";



const char kTraceToFileName[]               = "trace-to-file-name";




const char kProfilerTiming[]                = "profiler-timing";


const char kProfilerTimingDisabledValue[]   = "0";

#if defined(OS_POSIX)


const char kEnableCrashReporterForTesting[] =
    "enable-crash-reporter-for-testing";
#endif

}  
