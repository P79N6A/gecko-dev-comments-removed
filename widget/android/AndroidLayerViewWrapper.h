




#ifndef AndroidLayerViewWrapper_h__
#define AndroidLayerViewWrapper_h__

#include <jni.h>
#include <pthread.h>

class AndroidEGLObject {
public:
    static void Init(JNIEnv* aJEnv);
};

typedef void* EGLSurface;

class AndroidGLController {
public:
    static void Init(JNIEnv* aJEnv);

    void Acquire(JNIEnv* aJEnv, jobject aJObj);
    EGLSurface ProvideEGLSurface();
    void WaitForValidSurface();

private:
    static jmethodID jWaitForValidSurfaceMethod;
    static jmethodID jProvideEGLSurfaceMethod;

    
    JNIEnv* mJEnv;
    pthread_t mThread;
    jobject mJObj;
};

#endif

