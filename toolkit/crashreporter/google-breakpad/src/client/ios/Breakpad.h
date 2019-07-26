






































typedef void *BreakpadRef;

#ifdef __cplusplus
extern "C" {
#endif

#include <Foundation/Foundation.h>

#include <client/apple/Framework/BreakpadDefines.h>


#define BREAKPAD_OUTPUT_DUMP_FILE   "BreakpadDumpFile"
#define BREAKPAD_OUTPUT_CONFIG_FILE "BreakpadConfigFile"







typedef bool (*BreakpadFilterCallback)(int exception_type,
                                       int exception_code,
                                       mach_port_t crashing_thread,
                                       void *context);






























































































BreakpadRef BreakpadCreate(NSDictionary *parameters);


void BreakpadRelease(BreakpadRef ref);




















void BreakpadSetKeyValue(BreakpadRef ref, NSString *key, NSString *value);
NSString *BreakpadKeyValue(BreakpadRef ref, NSString *key);
void BreakpadRemoveKeyValue(BreakpadRef ref, NSString *key);





void BreakpadAddUploadParameter(BreakpadRef ref, NSString *key,
                                NSString *value);



void BreakpadRemoveUploadParameter(BreakpadRef ref, NSString *key);




bool BreakpadHasCrashReportToUpload(BreakpadRef ref);


void BreakpadUploadNextReport(BreakpadRef ref);



void BreakpadUploadData(BreakpadRef ref, NSData *data, NSString *name,
                        NSDictionary *server_parameters);





NSDictionary *BreakpadGenerateReport(BreakpadRef ref,
                                     NSDictionary *server_parameters);

#ifdef __cplusplus
}
#endif
