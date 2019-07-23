










































typedef void *BreakpadRef;

#ifdef __cplusplus
extern "C" {
#endif

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

  
#define kReporterMinidumpDirectoryKey "MinidumpDir"
#define kReporterMinidumpIDKey        "MinidumpID"




#define kDefaultLibrarySubdirectory   "Breakpad"



#define BREAKPAD_PRODUCT               "BreakpadProduct"
#define BREAKPAD_PRODUCT_DISPLAY       "BreakpadProductDisplay"
#define BREAKPAD_VERSION               "BreakpadVersion"
#define BREAKPAD_VENDOR                "BreakpadVendor"
#define BREAKPAD_URL                   "BreakpadURL"
#define BREAKPAD_REPORT_INTERVAL       "BreakpadReportInterval"
#define BREAKPAD_SKIP_CONFIRM          "BreakpadSkipConfirm"
#define BREAKPAD_CONFIRM_TIMEOUT       "BreakpadConfirmTimeout"
#define BREAKPAD_SEND_AND_EXIT         "BreakpadSendAndExit"
#define BREAKPAD_DUMP_DIRECTORY        "BreakpadMinidumpLocation"
#define BREAKPAD_INSPECTOR_LOCATION    "BreakpadInspectorLocation"
#define BREAKPAD_REPORTER_EXE_LOCATION \
  "BreakpadReporterExeLocation"
#define BREAKPAD_LOGFILES              "BreakpadLogFiles"
#define BREAKPAD_LOGFILE_UPLOAD_SIZE   "BreakpadLogFileTailSize"
#define BREAKPAD_REQUEST_COMMENTS      "BreakpadRequestComments"
#define BREAKPAD_COMMENTS              "BreakpadComments"
#define BREAKPAD_REQUEST_EMAIL         "BreakpadRequestEmail"
#define BREAKPAD_EMAIL                 "BreakpadEmail"
#define BREAKPAD_SERVER_TYPE           "BreakpadServerType"
#define BREAKPAD_SERVER_PARAMETER_DICT "BreakpadServerParameters"


#define BREAKPAD_PROCESS_START_TIME       "BreakpadProcStartTime"
#define BREAKPAD_PROCESS_UP_TIME          "BreakpadProcessUpTime"
#define BREAKPAD_PROCESS_CRASH_TIME       "BreakpadProcessCrashTime"
#define BREAKPAD_LOGFILE_KEY_PREFIX       "BreakpadAppLogFile"
#define BREAKPAD_SERVER_PARAMETER_PREFIX  "BreakpadServerParameterPrefix_"







typedef bool (*BreakpadFilterCallback)(int exception_type,
                                       int exception_code,
                                       mach_port_t crashing_thread,
                                       void *context);
















































































































































BreakpadRef BreakpadCreate(NSDictionary *parameters);


void BreakpadRelease(BreakpadRef ref);






void BreakpadSetFilterCallback(BreakpadRef ref,
                               BreakpadFilterCallback callback,
                               void *context);




















void BreakpadSetKeyValue(BreakpadRef ref, NSString *key, NSString *value);
NSString *BreakpadKeyValue(BreakpadRef ref, NSString *key);
void BreakpadRemoveKeyValue(BreakpadRef ref, NSString *key);





void BreakpadAddUploadParameter(BreakpadRef ref, NSString *key,
				NSString *value);



void BreakpadRemoveUploadParameter(BreakpadRef ref, NSString *key);


void BreakpadAddLogFile(BreakpadRef ref, NSString *logPathname);


void BreakpadGenerateAndSendReport(BreakpadRef ref);

#ifdef __cplusplus
}
#endif
