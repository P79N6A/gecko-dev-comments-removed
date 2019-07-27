




#include <android/log.h>
#include <dlfcn.h>
#include <prthread.h>

#include "mozilla/DebugOnly.h"
#include "mozilla/Assertions.h"
#include "nsThreadUtils.h"
#include "AndroidBridge.h"

extern "C" {
  jclass __jsjni_GetGlobalClassRef(const char *className);
}

class GetGlobalClassRefRunnable : public nsRunnable {
  public:
    GetGlobalClassRefRunnable(const char *className, jclass *foundClass) :
        mClassName(className), mResult(foundClass) {}
    NS_IMETHOD Run() {
        *mResult = __jsjni_GetGlobalClassRef(mClassName);
        return NS_OK;
    }
  private:
    const char *mClassName;
    jclass *mResult;
};

extern "C" {
  __attribute__ ((visibility("default")))
  jclass
  jsjni_FindClass(const char *className) {
    
    
    MOZ_ASSERT(NS_IsMainThread());
    JNIEnv *env = mozilla::AndroidBridge::GetJNIEnv();
    return env->FindClass(className);
  }

  jclass
  __jsjni_GetGlobalClassRef(const char *className) {
    
    JNIEnv *env = mozilla::AndroidBridge::GetJNIEnv();
    jclass globalRef = static_cast<jclass>(env->NewGlobalRef(env->FindClass(className)));
    if (!globalRef)
      return nullptr;

    
    return globalRef;
  }

  __attribute__ ((visibility("default")))
  jclass
  jsjni_GetGlobalClassRef(const char *className) {
    nsCOMPtr<nsIThread> mainThread;
    mozilla::DebugOnly<nsresult> rv = NS_GetMainThread(getter_AddRefs(mainThread));
    MOZ_ASSERT(NS_SUCCEEDED(rv));

    jclass foundClass;
    nsCOMPtr<nsIRunnable> runnable_ref(new GetGlobalClassRefRunnable(className,
                                                                     &foundClass));
    mainThread->Dispatch(runnable_ref, NS_DISPATCH_SYNC);
    if (!foundClass)
      return nullptr;

    return foundClass;
  }

  __attribute__ ((visibility("default")))
  jmethodID
  jsjni_GetStaticMethodID(jclass methodClass,
                          const char *methodName,
                          const char *signature) {
    JNIEnv *env = mozilla::AndroidBridge::GetJNIEnv();
    return env->GetStaticMethodID(methodClass, methodName, signature);
  }

  __attribute__ ((visibility("default")))
  bool
  jsjni_ExceptionCheck() {
    JNIEnv *env = mozilla::AndroidBridge::GetJNIEnv();
    return env->ExceptionCheck();
  }

  __attribute__ ((visibility("default")))
  void
  jsjni_CallStaticVoidMethodA(jclass cls,
                              jmethodID method,
                              jvalue *values) {
    JNIEnv *env = mozilla::AndroidBridge::GetJNIEnv();

    mozilla::AutoLocalJNIFrame jniFrame(env);
    env->CallStaticVoidMethodA(cls, method, values);
  }

  __attribute__ ((visibility("default")))
  int
  jsjni_CallStaticIntMethodA(jclass cls,
                             jmethodID method,
                             jvalue *values) {
    JNIEnv *env = mozilla::AndroidBridge::GetJNIEnv();

    mozilla::AutoLocalJNIFrame jniFrame(env);
    return env->CallStaticIntMethodA(cls, method, values);
  }

  __attribute__ ((visibility("default")))
  jobject jsjni_GetGlobalContextRef() {
    return mozilla::AndroidBridge::Bridge()->GetGlobalContextRef();
  }

  __attribute__ ((visibility("default")))
  JavaVM* jsjni_GetVM() {
    return mozilla::AndroidBridge::GetVM();
  }

  __attribute__ ((visibility("default")))
  JNIEnv* jsjni_GetJNIForThread() {
    return GetJNIForThread();
  }
}
