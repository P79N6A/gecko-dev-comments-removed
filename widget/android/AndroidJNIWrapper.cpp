




#include "mozilla/Util.h"

#include <android/log.h>
#include <dlfcn.h>
#include <prthread.h>

#include "mozilla/Assertions.h"
#include "nsThreadUtils.h"
#include "AndroidBridge.h"

#ifdef DEBUG
#define ALOG_BRIDGE(args...) ALOG(args)
#else
#define ALOG_BRIDGE(args...)
#endif

extern "C" {
  __attribute__ ((visibility("default")))
  jclass
  jsjni_FindClass(const char *className) {
    
    
    MOZ_ASSERT(NS_IsMainThread());
    JNIEnv *env = mozilla::AndroidBridge::GetJNIEnv();
    if (!env) return NULL;
    return env->FindClass(className);
  }

  __attribute__ ((visibility("default")))
  jmethodID
  jsjni_GetStaticMethodID(jclass methodClass,
                          const char *methodName,
                          const char *signature) {
    JNIEnv *env = mozilla::AndroidBridge::GetJNIEnv();
    if (!env) return NULL;
    return env->GetStaticMethodID(methodClass, methodName, signature);
  }

  __attribute__ ((visibility("default")))
  bool
  jsjni_ExceptionCheck() {
    JNIEnv *env = mozilla::AndroidBridge::GetJNIEnv();
    if (!env) return NULL;
    return env->ExceptionCheck();
  }

  __attribute__ ((visibility("default")))
  void
  jsjni_CallStaticVoidMethodA(jclass cls,
                              jmethodID method,
                              jvalue *values) {
    JNIEnv *env = mozilla::AndroidBridge::GetJNIEnv();
    if (!env) return;

    mozilla::AutoLocalJNIFrame jniFrame(env);
    env->CallStaticVoidMethodA(cls, method, values);
  }

  __attribute__ ((visibility("default")))
  int
  jsjni_CallStaticIntMethodA(jclass cls,
                             jmethodID method,
                             jvalue *values) {
    JNIEnv *env = mozilla::AndroidBridge::GetJNIEnv();
    if (!env) return -1;

    mozilla::AutoLocalJNIFrame jniFrame(env);
    return env->CallStaticIntMethodA(cls, method, values);
  }
}
