




#ifndef AndroidJNIWrapper_h__
#define AndroidJNIWrapper_h__

#include "mozilla/Types.h"
#include <jni.h>
#include <android/log.h>

extern "C" MOZ_EXPORT jclass jsjni_FindClass(const char *className);








extern "C" MOZ_EXPORT jclass jsjni_GetGlobalClassRef(const char *className);

extern "C" MOZ_EXPORT jmethodID jsjni_GetStaticMethodID(jclass methodClass,
                                       const char *methodName,
                                       const char *signature);
extern "C" MOZ_EXPORT bool jsjni_ExceptionCheck();
extern "C" MOZ_EXPORT void jsjni_CallStaticVoidMethodA(jclass cls, jmethodID method, jvalue *values);
extern "C" MOZ_EXPORT int jsjni_CallStaticIntMethodA(jclass cls, jmethodID method, jvalue *values);
extern "C" MOZ_EXPORT jobject jsjni_GetGlobalContextRef();
extern "C" MOZ_EXPORT JavaVM* jsjni_GetVM();
extern "C" MOZ_EXPORT JNIEnv* jsjni_GetJNIForThread();

#endif 
