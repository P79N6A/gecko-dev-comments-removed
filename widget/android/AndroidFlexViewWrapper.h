




































#include <jni.h>
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <cassert>
#include <cstdlib>

#include <pthread.h>

#define NS_ASSERTION(cond, msg) assert((cond) && msg)

namespace {

JavaVM *sJVM = NULL;

template<typename T>
class AndroidEGLObject {
public:
    AndroidEGLObject(JNIEnv* aJEnv, jobject aJObj)
    : mPtr(reinterpret_cast<typename T::NativeType>(aJEnv->GetIntField(aJObj, jPointerField))) {}

    static void Init(JNIEnv* aJEnv) {
        jclass jClass = reinterpret_cast<jclass>
            (aJEnv->NewGlobalRef(aJEnv->FindClass(sClassName)));
        jPointerField = aJEnv->GetFieldID(jClass, sPointerFieldName, "I");
    }

    typename T::NativeType const& operator*() const {
        return mPtr;
    }

private:
    static jfieldID jPointerField;
    static const char* sClassName;
    static const char* sPointerFieldName;

    const typename T::NativeType mPtr;
};

typedef void *EGLConfig;
typedef void *EGLContext;
typedef void *EGLDisplay;
typedef void *EGLSurface;

class AndroidEGLDisplayInfo {
public:
    typedef EGLDisplay NativeType;
private:
    AndroidEGLDisplayInfo() {}
};

class AndroidEGLConfigInfo {
public:
    typedef EGLConfig NativeType;
private:
    AndroidEGLConfigInfo() {}
};

class AndroidEGLContextInfo {
public:
    typedef EGLContext NativeType;
private:
    AndroidEGLContextInfo() {}
};

class AndroidEGLSurfaceInfo {
public:
    typedef EGLSurface NativeType;
private:
    AndroidEGLSurfaceInfo() {}
};

typedef AndroidEGLObject<AndroidEGLDisplayInfo> AndroidEGLDisplay;
typedef AndroidEGLObject<AndroidEGLConfigInfo> AndroidEGLConfig;
typedef AndroidEGLObject<AndroidEGLContextInfo> AndroidEGLContext;
typedef AndroidEGLObject<AndroidEGLSurfaceInfo> AndroidEGLSurface;

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

}
