




































#include "AndroidFlexViewWrapper.h"


static AndroidGLController sController;

static const char *sEGLDisplayClassName = "com/google/android/gles_jni/EGLDisplayImpl";
static const char *sEGLDisplayPointerFieldName = "mEGLDisplay";
static jfieldID jEGLDisplayPointerField = 0;

static const char *sEGLConfigClassName = "com/google/android/gles_jni/EGLConfigImpl";
static const char *sEGLConfigPointerFieldName = "mEGLConfig";
static jfieldID jEGLConfigPointerField = 0;

static const char *sEGLContextClassName = "com/google/android/gles_jni/EGLContextImpl";
static const char *sEGLContextPointerFieldName = "mEGLContext";
static jfieldID jEGLContextPointerField = 0;

static const char *sEGLSurfaceClassName = "com/google/android/gles_jni/EGLSurfaceImpl";
static const char *sEGLSurfacePointerFieldName = "mEGLSurface";
static jfieldID jEGLSurfacePointerField = 0;

void AndroidEGLObject::Init(JNIEnv* aJEnv) {
    jclass jClass;
    jClass = reinterpret_cast<jclass>
        (aJEnv->NewGlobalRef(aJEnv->FindClass(sEGLDisplayClassName)));
    jEGLDisplayPointerField = aJEnv->GetFieldID(jClass, sEGLDisplayPointerFieldName, "I");
    jClass = reinterpret_cast<jclass>
        (aJEnv->NewGlobalRef(aJEnv->FindClass(sEGLConfigClassName)));
    jEGLConfigPointerField = aJEnv->GetFieldID(jClass, sEGLConfigPointerFieldName, "I");
    jClass = reinterpret_cast<jclass>
        (aJEnv->NewGlobalRef(aJEnv->FindClass(sEGLContextClassName)));
    jEGLContextPointerField = aJEnv->GetFieldID(jClass, sEGLContextPointerFieldName, "I");
    jClass = reinterpret_cast<jclass>
        (aJEnv->NewGlobalRef(aJEnv->FindClass(sEGLSurfaceClassName)));
    jEGLSurfacePointerField = aJEnv->GetFieldID(jClass, sEGLSurfacePointerFieldName, "I");
}

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
    jobject jObj = mJEnv->CallObjectMethod(mJObj, jGetEGLDisplayMethod);
    return reinterpret_cast<EGLDisplay>(mJEnv->GetIntField(jObj, jEGLDisplayPointerField));
}

EGLConfig
AndroidGLController::GetEGLConfig()
{
    jobject jObj = mJEnv->CallObjectMethod(mJObj, jGetEGLConfigMethod);
    return reinterpret_cast<EGLConfig>(mJEnv->GetIntField(jObj, jEGLConfigPointerField));
}

EGLContext
AndroidGLController::GetEGLContext()
{
    jobject jObj = mJEnv->CallObjectMethod(mJObj, jGetEGLContextMethod);
    return reinterpret_cast<EGLContext>(mJEnv->GetIntField(jObj, jEGLContextPointerField));
}

EGLSurface
AndroidGLController::GetEGLSurface()
{
    jobject jObj = mJEnv->CallObjectMethod(mJObj, jGetEGLSurfaceMethod);
    return reinterpret_cast<EGLSurface>(mJEnv->GetIntField(jObj, jEGLSurfacePointerField));
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


