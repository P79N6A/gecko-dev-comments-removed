



































#ifndef _nsJavaInterfaces_h_
#define _nsJavaInterfaces_h_

#include "jni.h"
#include "nscore.h"

#define MOZILLA_NATIVE(func) Java_org_mozilla_xpcom_internal_MozillaImpl_##func
#define GRE_NATIVE(func) Java_org_mozilla_xpcom_internal_GREImpl_##func
#define XPCOM_NATIVE(func) Java_org_mozilla_xpcom_internal_XPCOMImpl_##func
#define JAVAPROXY_NATIVE(func) \
          Java_org_mozilla_xpcom_internal_XPCOMJavaProxy_##func
#define LOCKPROXY_NATIVE(func) Java_org_mozilla_xpcom_ProfileLock_##func
#define JXUTILS_NATIVE(func) \
          Java_org_mozilla_xpcom_internal_JavaXPCOMMethods_##func


extern "C" NS_EXPORT void JNICALL
MOZILLA_NATIVE(initialize) (JNIEnv* env, jobject);

extern "C" NS_EXPORT void JNICALL
GRE_NATIVE(initEmbedding) (JNIEnv* env, jobject, jobject aLibXULDirectory,
                           jobject aAppDirectory, jobject aAppDirProvider);

extern "C" NS_EXPORT void JNICALL
GRE_NATIVE(termEmbedding) (JNIEnv *env, jobject);

extern "C" NS_EXPORT jobject JNICALL
GRE_NATIVE(lockProfileDirectory) (JNIEnv *, jobject, jobject aDirectory);

extern "C" NS_EXPORT void JNICALL
GRE_NATIVE(notifyProfile) (JNIEnv *env, jobject);

extern "C" NS_EXPORT jobject JNICALL
XPCOM_NATIVE(initXPCOM) (JNIEnv* env, jobject, jobject aMozBinDirectory,
                         jobject aAppFileLocProvider);

extern "C" NS_EXPORT void JNICALL
XPCOM_NATIVE(shutdownXPCOM) (JNIEnv *env, jobject, jobject aServMgr);

extern "C" NS_EXPORT jobject JNICALL
XPCOM_NATIVE(newLocalFile) (JNIEnv *env, jobject, jstring aPath,
                            jboolean aFollowLinks);

extern "C" NS_EXPORT jobject JNICALL
XPCOM_NATIVE(getComponentManager) (JNIEnv *env, jobject);

extern "C" NS_EXPORT jobject JNICALL
XPCOM_NATIVE(getComponentRegistrar) (JNIEnv *env, jobject);

extern "C" NS_EXPORT jobject JNICALL
XPCOM_NATIVE(getServiceManager) (JNIEnv *env, jobject);

extern "C" NS_EXPORT jobject JNICALL
JAVAPROXY_NATIVE(callXPCOMMethod) (JNIEnv *env, jclass that, jobject aJavaProxy,
                                   jstring aMethodName, jobjectArray aParams);

extern "C" NS_EXPORT void JNICALL
JAVAPROXY_NATIVE(finalizeProxy) (JNIEnv *env, jclass that, jobject aJavaProxy);

extern "C" NS_EXPORT jboolean JNICALL
JAVAPROXY_NATIVE(isSameXPCOMObject) (JNIEnv *env, jclass that, jobject aProxy1,
                                     jobject aProxy2);

extern "C" NS_EXPORT void JNICALL
LOCKPROXY_NATIVE(release) (JNIEnv *env, jclass that, jlong aLockObject);

extern "C" NS_EXPORT jlong JNICALL
MOZILLA_NATIVE(getNativeHandleFromAWT) (JNIEnv* env, jobject, jobject widget);

extern "C" NS_EXPORT jlong JNICALL
JXUTILS_NATIVE(wrapJavaObject) (JNIEnv* env, jobject, jobject aJavaObject,
                                jstring aIID);

extern "C" NS_EXPORT jobject JNICALL
JXUTILS_NATIVE(wrapXPCOMObject) (JNIEnv* env, jobject, jlong aXPCOMObject,
                                 jstring aIID);

#endif 
