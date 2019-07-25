




































#ifndef AndroidFlexViewWrapper_h__
#define AndroidFlexViewWrapper_h__

#include <jni.h>


#include <cassert>
#include <cstdlib>
#include <pthread.h>
#include <android/log.h>

typedef void *NativeType;

class AndroidEGLObject {
public:
    AndroidEGLObject(JNIEnv* aJEnv, jobject aJObj)
    : mPtr(reinterpret_cast<NativeType>(aJEnv->GetIntField(aJObj, jPointerField))) {}

    static void Init(JNIEnv* aJEnv);

    NativeType const& operator*() const {
        return mPtr;
    }

private:
    static jfieldID jPointerField;
    static const char* sClassName;
    static const char* sPointerFieldName;

    const NativeType mPtr;
};

typedef void *EGLConfig;
typedef void *EGLContext;
typedef void *EGLDisplay;
typedef void *EGLSurface;

class AndroidGLController {
public:
    static void Init(JNIEnv* aJEnv);

    void Acquire(JNIEnv *aJEnv, jobject aJObj);
    void Acquire(JNIEnv *aJEnv);
    void Release();

    void SetGLVersion(int aVersion);
    void InitGLContext();
    void DisposeGLContext();
    EGLDisplay GetEGLDisplay();
    EGLConfig GetEGLConfig();
    EGLContext GetEGLContext();
    EGLSurface GetEGLSurface();
    bool HasSurface();
    bool SwapBuffers();
    bool CheckForLostContext();
    void WaitForValidSurface();
    int GetWidth();
    int GetHeight();

private:
    static jmethodID jSetGLVersionMethod;
    static jmethodID jInitGLContextMethod;
    static jmethodID jDisposeGLContextMethod;
    static jmethodID jGetEGLDisplayMethod;
    static jmethodID jGetEGLConfigMethod;
    static jmethodID jGetEGLContextMethod;
    static jmethodID jGetEGLSurfaceMethod;
    static jmethodID jHasSurfaceMethod;
    static jmethodID jSwapBuffersMethod;
    static jmethodID jCheckForLostContextMethod;
    static jmethodID jWaitForValidSurfaceMethod;
    static jmethodID jGetWidthMethod;
    static jmethodID jGetHeightMethod;

    JNIEnv *mJEnv;
    jobject mJObj;
};

#endif

