






































#ifndef nsObjCExceptions_h_
#define nsObjCExceptions_h_

#import <Foundation/NSException.h>
#import <Foundation/NSObjCRuntime.h>
#include <unistd.h>
#include <signal.h>
#include "nsError.h"




static void nsObjCExceptionLog(NSException *e)
{
  NSLog(@"%@: %@", [e name], [e reason]);
}

static void nsObjCExceptionAbort()
{
  
  
  
  int* foo = NULL;
  *foo = 1;
}

static void nsObjCExceptionLogAbort(NSException *e)
{
  nsObjCExceptionLog(e);
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
  nsObjCExceptionLogAbort(_exn);                   \
}

#define NS_OBJC_TRY_EXPR_ABORT(_e)                 \
({                                                 \
   typeof(_e) _tmp;                                \
   @try { _tmp = (_e); }                           \
   @catch(NSException *_exn) {                     \
     nsObjCExceptionLogAbort(_exn);                \
   }                                               \
   _tmp;                                           \
})


#define NS_OBJC_BEGIN_TRY_ABORT_BLOCK @try {
#define NS_OBJC_END_TRY_ABORT_BLOCK   } @catch(NSException *_exn) {            \
                                        nsObjCExceptionLogAbort(_exn);         \
                                      }





#define NS_OBJC_BEGIN_TRY_ABORT_BLOCK_NIL @try {
#define NS_OBJC_END_TRY_ABORT_BLOCK_NIL   } @catch(NSException *_exn) {        \
                                            nsObjCExceptionLogAbort(_exn);     \
                                          }                                    \
                                          return nil;

#define NS_OBJC_BEGIN_TRY_ABORT_BLOCK_NSNULL @try {
#define NS_OBJC_END_TRY_ABORT_BLOCK_NSNULL   } @catch(NSException *_exn) {     \
                                               nsObjCExceptionLogAbort(_exn);  \
                                             }                                 \
                                             return nsnull;

#define NS_OBJC_BEGIN_TRY_ABORT_BLOCK_NSRESULT @try {
#define NS_OBJC_END_TRY_ABORT_BLOCK_NSRESULT   } @catch(NSException *_exn) {   \
                                                 nsObjCExceptionLogAbort(_exn);\
                                               }                               \
                                               return NS_ERROR_FAILURE;

#define NS_OBJC_BEGIN_TRY_ABORT_BLOCK_RETURN    @try {
#define NS_OBJC_END_TRY_ABORT_BLOCK_RETURN(_rv) } @catch(NSException *_exn) {   \
                                                  nsObjCExceptionLogAbort(_exn);\
                                                }                               \
                                                return _rv;

#endif 
