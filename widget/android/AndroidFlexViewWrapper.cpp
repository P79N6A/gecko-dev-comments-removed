




































#include "AndroidFlexViewWrapper.h"
#include "nsDebug.h"

#define ASSERT_THREAD() \
        NS_ASSERTION((void*)pthread_self() == mThread, "Something is calling AndroidGLController from the wrong thread!")

static jfieldID jEGLSurfacePointerField = 0;

void AndroidEGLObject::Init(JNIEnv* aJEnv) {
    jclass jClass;
    jClass = reinterpret_cast<jclass>
        (aJEnv->NewGlobalRef(aJEnv->FindClass("com/google/android/gles_jni/EGLSurfaceImpl")));
    jEGLSurfacePointerField = aJEnv->GetFieldID(jClass, "mEGLSurface", "I");
}

jmethodID AndroidGLController::jSetGLVersionMethod = 0;
jmethodID AndroidGLController::jWaitForValidSurfaceMethod = 0;
jmethodID AndroidGLController::jProvideEGLSurfaceMethod = 0;

void
AndroidGLController::Init(JNIEnv *aJEnv)
{
    jclass jClass = reinterpret_cast<jclass>(aJEnv->NewGlobalRef(aJEnv->FindClass("org/mozilla/gecko/gfx/GLController")));

    jSetGLVersionMethod = aJEnv->GetMethodID(jClass, "setGLVersion", "(I)V");
    jProvideEGLSurfaceMethod = aJEnv->GetMethodID(jClass, "provideEGLSurface",
                                                  "()Ljavax/microedition/khronos/egl/EGLSurface;");
    jWaitForValidSurfaceMethod = aJEnv->GetMethodID(jClass, "waitForValidSurface", "()V");
}

void
AndroidGLController::Acquire(JNIEnv* aJEnv, jobject aJObj)
{
    mJEnv = aJEnv;
    mThread = (void*)pthread_self();
    mJObj = aJEnv->NewGlobalRef(aJObj);
}

void
AndroidGLController::SetGLVersion(int aVersion)
{
    ASSERT_THREAD();
    mJEnv->CallVoidMethod(mJObj, jSetGLVersionMethod, aVersion);
}

EGLSurface
AndroidGLController::ProvideEGLSurface()
{
    ASSERT_THREAD();
    jobject jObj = mJEnv->CallObjectMethod(mJObj, jProvideEGLSurfaceMethod);
    return reinterpret_cast<EGLSurface>(mJEnv->GetIntField(jObj, jEGLSurfacePointerField));
}

void
AndroidGLController::WaitForValidSurface()
{
    ASSERT_THREAD();
    mJEnv->CallVoidMethod(mJObj, jWaitForValidSurfaceMethod);
}
