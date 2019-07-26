




#ifndef AndroidJNIWrapper_h__
#define AndroidJNIWrapper_h__

#include <jni.h>
#include <android/log.h>

extern "C" jclass jsjni_FindClass(const char *className);








extern "C" jclass jsjni_GetGlobalClassRef(const char *className);

extern "C" jmethodID jsjni_GetStaticMethodID(jclass methodClass,
                                       const char *methodName,
                                       const char *signature);
extern "C" bool jsjni_ExceptionCheck();
extern "C" void jsjni_CallStaticVoidMethodA(jclass cls, jmethodID method, jvalue *values);
extern "C" int jsjni_CallStaticIntMethodA(jclass cls, jmethodID method, jvalue *values);
extern "C" jobject jsjni_GetGlobalContextRef();
extern "C" JavaVM* jsjni_GetVM();

#endif 
