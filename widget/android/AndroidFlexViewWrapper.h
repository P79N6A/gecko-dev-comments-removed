




































#ifndef AndroidFlexViewWrapper_h__
#define AndroidFlexViewWrapper_h__

#include <jni.h>
#include <cassert>
#include <cstdlib>
#include <pthread.h>
#include <android/log.h>

class AndroidEGLObject {
public:
    static void Init(JNIEnv* aJEnv);
};

typedef void *EGLSurface;

class AndroidGLController {
public:
    static void Init(JNIEnv* aJEnv);

    void Acquire(JNIEnv *aJEnv, jobject aJObj);
    void SetGLVersion(int aVersion);
    EGLSurface ProvideEGLSurface();
    void WaitForValidSurface();

private:
    static jmethodID jSetGLVersionMethod;
    static jmethodID jWaitForValidSurfaceMethod;
    static jmethodID jProvideEGLSurfaceMethod;

    
    JNIEnv *mJEnv;
    void *mThread;
    jobject mJObj;
};

#endif

