




































#ifndef jvmmgr_h___
#define jvmmgr_h___

#include "prtypes.h"
#include "jni.h"
#include "jsdbgapi.h"
#include "nsError.h"

#include "nsISecurityContext.h"

struct nsJVMManager;

typedef enum nsJVMStatus {
    nsJVMStatus_Enabled,  
    nsJVMStatus_Disabled, 
    nsJVMStatus_Running,  
    nsJVMStatus_Failed    
} nsJVMStatus;






PR_BEGIN_EXTERN_C

PR_EXTERN(void)
JVM_ReleaseJVMMgr(struct nsJVMManager* mgr);

PR_EXTERN(nsJVMStatus)
JVM_StartupJVM(void);

PR_EXTERN(nsJVMStatus)
JVM_ShutdownJVM(void);

PR_EXTERN(nsJVMStatus)
JVM_GetJVMStatus(void);

PR_EXTERN(PRBool)
JVM_AddToClassPath(const char* dirPath);

PR_EXTERN(void)
JVM_ShowConsole(void);

PR_EXTERN(void)
JVM_HideConsole(void);

PR_EXTERN(PRBool)
JVM_IsConsoleVisible(void);

PR_EXTERN(void)
JVM_PrintToConsole(const char* msg);

PR_EXTERN(void)
JVM_ShowPrefsWindow(void);

PR_EXTERN(void)
JVM_HidePrefsWindow(void);

PR_EXTERN(PRBool)
JVM_IsPrefsWindowVisible(void);

PR_EXTERN(void)
JVM_StartDebugger(void);

PR_EXTERN(JNIEnv*)
JVM_GetJNIEnv(void);

PR_IMPLEMENT(void)
JVM_ReleaseJNIEnv(JNIEnv *pJNIEnv);

PR_EXTERN(nsresult)
JVM_SpendTime(PRUint32 timeMillis);

PR_EXTERN(PRBool)
JVM_MaybeStartupLiveConnect(void);

PR_EXTERN(PRBool)
JVM_MaybeShutdownLiveConnect(void);

PR_EXTERN(PRBool)
JVM_IsLiveConnectEnabled(void);

PR_EXTERN(nsJVMStatus)
JVM_ShutdownJVM(void);

PR_EXTERN(PRBool)
JVM_NSISecurityContextImplies(JSStackFrame  *pCurrentFrame, const char* target, const char* action);

PR_EXTERN(JSPrincipals*)
JVM_GetJavaPrincipalsFromStack(JSStackFrame  *pCurrentFrame);

PR_EXTERN(void *)
JVM_GetJavaPrincipalsFromStackAsNSVector(JSStackFrame  *pCurrentFrame);

PR_EXTERN(JSStackFrame**)
JVM_GetStartJSFrameFromParallelStack(void);

PR_EXTERN(JSStackFrame*)
JVM_GetEndJSFrameFromParallelStack(JSStackFrame  *pCurrentFrame);

PR_EXTERN(nsISecurityContext*) 
JVM_GetJSSecurityContext();

PR_END_EXTERN_C

#endif 
