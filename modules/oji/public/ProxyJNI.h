








































#ifndef PROXY_JNI_H
#define PROXY_JNI_H

#ifndef JNI_H
#include <jni.h>
#endif

class nsIJVMPlugin;
class nsISecureEnv;
class nsISecurityContext;



JNIEnv* CreateProxyJNI(nsIJVMPlugin* jvmPlugin, nsISecureEnv* secureEnv = NULL);





void DeleteProxyJNI(JNIEnv* proxyEnv);





nsISecureEnv* GetSecureEnv(JNIEnv* proxyEnv);




PR_EXTERN(void) SetSecurityContext(JNIEnv* proxyEnv, nsISecurityContext *context);




PR_EXTERN(nsresult) GetSecurityContext(JNIEnv* proxyEnv, nsISecurityContext **context);
#endif 

