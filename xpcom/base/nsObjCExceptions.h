






#undef try
#undef catch

#ifndef nsObjCExceptions_h_
#define nsObjCExceptions_h_

#import <Foundation/Foundation.h>

#ifdef DEBUG
#import <ExceptionHandling/NSExceptionHandler.h>
#endif

#if defined(MOZ_CRASHREPORTER) && defined(__cplusplus)
#include "nsICrashReporter.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#endif

#include <unistd.h>
#include <signal.h>
#include "nsError.h"


#undef try
#undef catch










__attribute__((unused))
static void
nsObjCExceptionLog(NSException* aException)
{
  NSLog(@"Mozilla has caught an Obj-C exception [%@: %@]",
        [aException name], [aException reason]);

#if defined(MOZ_CRASHREPORTER) && defined(__cplusplus)
  
  nsCOMPtr<nsICrashReporter> crashReporter =
    do_GetService("@mozilla.org/toolkit/crash-reporter;1");
  if (crashReporter) {
    crashReporter->AppendObjCExceptionInfoToAppNotes(static_cast<void*>(aException));
  }
#endif

#ifdef DEBUG
  @try {
    
    
    NSArray* stackTrace = nil;
    if ([aException respondsToSelector:@selector(callStackReturnAddresses)]) {
      NSArray* addresses = (NSArray*)
        [aException performSelector:@selector(callStackReturnAddresses)];
      if ([addresses count]) {
        stackTrace = addresses;
      }
    }

    
    
    if (!stackTrace) {
      stackTrace = [[aException userInfo] objectForKey:NSStackTraceKey];
    }

    if (stackTrace) {
      
      
      NSMutableArray* args =
        [NSMutableArray arrayWithCapacity:[stackTrace count] + 3];

      [args addObject:@"-p"];
      int pid = [[NSProcessInfo processInfo] processIdentifier];
      [args addObject:[NSString stringWithFormat:@"%d", pid]];

      [args addObject:@"-printHeader"];

      unsigned int stackCount = [stackTrace count];
      unsigned int stackIndex = 0;
      for (; stackIndex < stackCount; stackIndex++) {
        unsigned long address =
          [[stackTrace objectAtIndex:stackIndex] unsignedLongValue];
        [args addObject:[NSString stringWithFormat:@"0x%lx", address]];
      }

      NSPipe* outPipe = [NSPipe pipe];

      NSTask* task = [[NSTask alloc] init];
      [task setLaunchPath:@"/usr/bin/atos"];
      [task setArguments:args];
      [task setStandardOutput:outPipe];
      [task setStandardError:outPipe];

      NSLog(@"Generating stack trace for Obj-C exception...");

      
      
      [task launch];

      [task waitUntilExit];
      [task release];

      NSData* outData =
        [[outPipe fileHandleForReading] readDataToEndOfFile];
      NSString* outString =
        [[NSString alloc] initWithData:outData encoding:NSUTF8StringEncoding];

      NSLog(@"Stack trace:\n%@", outString);

      [outString release];
    } else {
      NSLog(@"<No stack information available for Obj-C exception>");
    }
  }
  @catch (NSException* exn) {
    NSLog(@"Failed to generate stack trace for Obj-C exception [%@: %@]",
          [exn name], [exn reason]);
  }
#endif
}

__attribute__((unused))
static void
nsObjCExceptionAbort()
{
  
  
  
  int* foo = nullptr;
  *foo = 1;
}

__attribute__((unused))
static void
nsObjCExceptionLogAbort(NSException* aException)
{
  nsObjCExceptionLog(aException);
  nsObjCExceptionAbort();
}

#define NS_OBJC_TRY(_e, _fail)                     \
@try { _e; }                                       \
@catch(NSException *_exn) {                        \
  nsObjCExceptionLog(_exn);                        \
  _fail;                                           \
}

#define NS_OBJC_TRY_EXPR(_e, _fail)                \
({                                                 \
   typeof(_e) _tmp;                                \
   @try { _tmp = (_e); }                           \
   @catch(NSException *_exn) {                     \
     nsObjCExceptionLog(_exn);                     \
     _fail;                                        \
   }                                               \
   _tmp;                                           \
})

#define NS_OBJC_TRY_EXPR_NULL(_e)                  \
NS_OBJC_TRY_EXPR(_e, 0)

#define NS_OBJC_TRY_IGNORE(_e)                     \
NS_OBJC_TRY(_e, )




#define NS_OBJC_TRY_ABORT(_e)                      \
@try { _e; }                                       \
@catch(NSException *_exn) {                        \
  nsObjCExceptionLog(_exn);                        \
}

#define NS_OBJC_TRY_EXPR_ABORT(_e)                 \
({                                                 \
   typeof(_e) _tmp;                                \
   @try { _tmp = (_e); }                           \
   @catch(NSException *_exn) {                     \
     nsObjCExceptionLog(_exn);                     \
   }                                               \
   _tmp;                                           \
})


#define NS_OBJC_BEGIN_TRY_ABORT_BLOCK @try {
#define NS_OBJC_END_TRY_ABORT_BLOCK   } @catch(NSException *_exn) {             \
                                        nsObjCExceptionLog(_exn);               \
                                      }





#define NS_OBJC_BEGIN_TRY_ABORT_BLOCK_NIL @try {
#define NS_OBJC_END_TRY_ABORT_BLOCK_NIL   } @catch(NSException *_exn) {         \
                                            nsObjCExceptionLog(_exn);           \
                                          }                                     \
                                          return nil;

#define NS_OBJC_BEGIN_TRY_ABORT_BLOCK_NSNULL @try {
#define NS_OBJC_END_TRY_ABORT_BLOCK_NSNULL   } @catch(NSException *_exn) {      \
                                               nsObjCExceptionLog(_exn);        \
                                             }                                  \
                                             return nullptr;

#define NS_OBJC_BEGIN_TRY_ABORT_BLOCK_NSRESULT @try {
#define NS_OBJC_END_TRY_ABORT_BLOCK_NSRESULT   } @catch(NSException *_exn) {    \
                                                 nsObjCExceptionLog(_exn);      \
                                               }                                \
                                               return NS_ERROR_FAILURE;

#define NS_OBJC_BEGIN_TRY_ABORT_BLOCK_RETURN    @try {
#define NS_OBJC_END_TRY_ABORT_BLOCK_RETURN(_rv) } @catch(NSException *_exn) {   \
                                                  nsObjCExceptionLog(_exn);\
                                                }                               \
                                                return _rv;

#define NS_OBJC_BEGIN_TRY_LOGONLY_BLOCK @try {
#define NS_OBJC_END_TRY_LOGONLY_BLOCK   } @catch(NSException *_exn) {           \
                                          nsObjCExceptionLog(_exn);             \
                                        }

#define NS_OBJC_BEGIN_TRY_LOGONLY_BLOCK_RETURN    @try {
#define NS_OBJC_END_TRY_LOGONLY_BLOCK_RETURN(_rv) } @catch(NSException *_exn) { \
                                                    nsObjCExceptionLog(_exn);   \
                                                  }                             \
                                                  return _rv;

#endif 
