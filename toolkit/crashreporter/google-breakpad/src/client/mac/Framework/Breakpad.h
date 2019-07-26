












































#define BREAKPAD_BOOTSTRAP_PARENT_PORT    "com.Breakpad.BootstrapParent"

typedef void *BreakpadRef;

#ifdef __cplusplus
extern "C" {
#endif

#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>

#include "BreakpadDefines.h"







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
