




































#include "AndroidFlexViewWrapper.h"


static AndroidGLController sController;

template<>
const char *AndroidEGLDisplay::sClassName = "com/google/android/gles_jni/EGLDisplayImpl";
template<>
const char *AndroidEGLDisplay::sPointerFieldName = "mEGLDisplay";
template<>
jfieldID AndroidEGLDisplay::jPointerField = 0;
template<>
const char *AndroidEGLConfig::sClassName = "com/google/android/gles_jni/EGLConfigImpl";
template<>
const char *AndroidEGLConfig::sPointerFieldName = "mEGLConfig";
template<>
jfieldID AndroidEGLConfig::jPointerField = 0;
template<>
const char *AndroidEGLContext::sClassName = "com/google/android/gles_jni/EGLContextImpl";
template<>
const char *AndroidEGLContext::sPointerFieldName = "mEGLContext";
template<>
jfieldID AndroidEGLContext::jPointerField = 0;
template<>
const char *AndroidEGLSurface::sClassName = "com/google/android/gles_jni/EGLSurfaceImpl";
template<>
const char *AndroidEGLSurface::sPointerFieldName = "mEGLSurface";
template<>
jfieldID AndroidEGLSurface::jPointerField = 0;

jmethodID AndroidGLController::jSetGLVersionMethod = 0;
jmethodID AndroidGLController::jInitGLContextMethod = 0;
jmethodID AndroidGLController::jDisposeGLContextMethod = 0;
jmethodID AndroidGLController::jGetEGLDisplayMethod = 0;
jmethodID AndroidGLController::jGetEGLConfigMethod = 0;
jmethodID AndroidGLController::jGetEGLContextMethod = 0;
jmethodID AndroidGLController::jGetEGLSurfaceMethod = 0;
jmethodID AndroidGLController::jHasSurfaceMethod = 0;
jmethodID AndroidGLController::jSwapBuffersMethod = 0;
jmethodID AndroidGLController::jCheckForLostContextMethod = 0;
jmethodID AndroidGLController::jWaitForValidSurfaceMethod = 0;
jmethodID AndroidGLController::jGetWidthMethod = 0;
jmethodID AndroidGLController::jGetHeightMethod = 0;

void
AndroidGLController::Init(JNIEnv *aJEnv)
{
    const char *className = "org/mozilla/gecko/gfx/GLController";
    jclass jClass = reinterpret_cast<jclass>(aJEnv->NewGlobalRef(aJEnv->FindClass(className)));

    jSetGLVersionMethod = aJEnv->GetMethodID(jClass, "setGLVersion", "(I)V");
    jInitGLContextMethod = aJEnv->GetMethodID(jClass, "initGLContext", "()V");
    jDisposeGLContextMethod = aJEnv->GetMethodID(jClass, "disposeGLContext", "()V");
    jGetEGLDisplayMethod = aJEnv->GetMethodID(jClass, "getEGLDisplay",
                                              "()Ljavax/microedition/khronos/egl/EGLDisplay;");
    jGetEGLConfigMethod = aJEnv->GetMethodID(jClass, "getEGLConfig",
                                             "()Ljavax/microedition/khronos/egl/EGLConfig;");
    jGetEGLContextMethod = aJEnv->GetMethodID(jClass, "getEGLContext",
                                              "()Ljavax/microedition/khronos/egl/EGLContext;");
    jGetEGLSurfaceMethod = aJEnv->GetMethodID(jClass, "getEGLSurface",
                                              "()Ljavax/microedition/khronos/egl/EGLSurface;");
    jHasSurfaceMethod = aJEnv->GetMethodID(jClass, "hasSurface", "()Z");
    jSwapBuffersMethod = aJEnv->GetMethodID(jClass, "swapBuffers", "()Z");
    jCheckForLostContextMethod = aJEnv->GetMethodID(jClass, "checkForLostContext", "()Z");
    jWaitForValidSurfaceMethod = aJEnv->GetMethodID(jClass, "waitForValidSurface", "()V");
    jGetWidthMethod = aJEnv->GetMethodID(jClass, "getWidth", "()I");
    jGetHeightMethod = aJEnv->GetMethodID(jClass, "getHeight", "()I");
}

void
AndroidGLController::Acquire(JNIEnv* aJEnv, jobject aJObj)
{
    mJEnv = aJEnv;
    mJObj = aJEnv->NewGlobalRef(aJObj);
}

void
AndroidGLController::Acquire(JNIEnv* aJEnv)
{
    mJEnv = aJEnv;
}

void
AndroidGLController::Release()
{
    if (mJObj) {
        mJEnv->DeleteGlobalRef(mJObj);
        mJObj = NULL;
    }

    mJEnv = NULL;
}

void
AndroidGLController::SetGLVersion(int aVersion)
{
    mJEnv->CallVoidMethod(mJObj, jSetGLVersionMethod, aVersion);
}

void
AndroidGLController::InitGLContext()
{
    mJEnv->CallVoidMethod(mJObj, jInitGLContextMethod);
}

void
AndroidGLController::DisposeGLContext()
{
    mJEnv->CallVoidMethod(mJObj, jDisposeGLContextMethod);
}

EGLDisplay
AndroidGLController::GetEGLDisplay()
{
    AndroidEGLDisplay jEGLDisplay(mJEnv, mJEnv->CallObjectMethod(mJObj, jGetEGLDisplayMethod));
    return *jEGLDisplay;
}

EGLConfig
AndroidGLController::GetEGLConfig()
{
    AndroidEGLConfig jEGLConfig(mJEnv, mJEnv->CallObjectMethod(mJObj, jGetEGLConfigMethod));
    return *jEGLConfig;
}

EGLContext
AndroidGLController::GetEGLContext()
{
    AndroidEGLContext jEGLContext(mJEnv, mJEnv->CallObjectMethod(mJObj, jGetEGLContextMethod));
    return *jEGLContext;
}

EGLSurface
AndroidGLController::GetEGLSurface()
{
    AndroidEGLSurface jEGLSurface(mJEnv, mJEnv->CallObjectMethod(mJObj, jGetEGLSurfaceMethod));
    return *jEGLSurface;
}

bool
AndroidGLController::HasSurface()
{
    return mJEnv->CallBooleanMethod(mJObj, jHasSurfaceMethod);
}

bool
AndroidGLController::SwapBuffers()
{
    return mJEnv->CallBooleanMethod(mJObj, jSwapBuffersMethod);
}

bool
AndroidGLController::CheckForLostContext()
{
    return mJEnv->CallBooleanMethod(mJObj, jCheckForLostContextMethod);
}

void
AndroidGLController::WaitForValidSurface()
{
    mJEnv->CallVoidMethod(mJObj, jWaitForValidSurfaceMethod);
}

int
AndroidGLController::GetWidth()
{
    return mJEnv->CallIntMethod(mJObj, jGetWidthMethod);
}

int
AndroidGLController::GetHeight()
{
    return mJEnv->CallIntMethod(mJObj, jGetHeightMethod);
}


