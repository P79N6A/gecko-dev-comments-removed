




































#ifndef lcglue_h___
#define lcglue_h___

#include "prtypes.h"
#include "jni.h"
#include "jsdbgapi.h"
#include "nsError.h"

#include "nsIJVMThreadManager.h"
#include "nsISecurityContext.h"





struct JVMContext {
	JNIEnv					*proxyEnv;					
	JSJavaThreadState		*jsj_env;					
};

JVMContext* GetJVMContext();
void JVM_InitLCGlue(void);      

#endif 
