




#include "AndroidLayerViewWrapper.h"
#include "AndroidBridge.h"
#include "nsDebug.h"

using namespace mozilla;

#define ASSERT_THREAD() \
        NS_ASSERTION(pthread_self() == mThread, "Something is calling AndroidGLController from the wrong thread!")

static jfieldID jEGLSurfacePointerField = 0;

void AndroidEGLObject::Init(JNIEnv* aJEnv) {
    AutoLocalJNIFrame jniFrame(aJEnv);

    jclass jClass;
    jClass = reinterpret_cast<jclass>
        (aJEnv->NewGlobalRef(aJEnv->FindClass("com/google/android/gles_jni/EGLSurfaceImpl")));
    if (!jClass)
        return;

    jEGLSurfacePointerField = aJEnv->GetFieldID(jClass, "mEGLSurface", "I");
}

jmethodID AndroidGLController::jWaitForValidSurfaceMethod = 0;
jmethodID AndroidGLController::jProvideEGLSurfaceMethod = 0;

void
AndroidGLController::Init(JNIEnv* aJEnv)
{
    jclass jClass = reinterpret_cast<jclass>(aJEnv->NewGlobalRef(aJEnv->FindClass("org/mozilla/gecko/gfx/GLController")));

    jProvideEGLSurfaceMethod = aJEnv->GetMethodID(jClass, "provideEGLSurface",
                                                  "()Ljavax/microedition/khronos/egl/EGLSurface;");
    jWaitForValidSurfaceMethod = aJEnv->GetMethodID(jClass, "waitForValidSurface", "()V");
}

void
AndroidGLController::Acquire(JNIEnv* aJEnv, jobject aJObj)
{
    mJEnv = aJEnv;
    mThread = pthread_self();
    mJObj = aJEnv->NewGlobalRef(aJObj);
}

EGLSurface
AndroidGLController::ProvideEGLSurface()
{
    ASSERT_THREAD();

    if (!jEGLSurfacePointerField)
        return NULL;

    AutoLocalJNIFrame jniFrame(mJEnv);
    jobject jObj = mJEnv->CallObjectMethod(mJObj, jProvideEGLSurfaceMethod);
    if (jniFrame.CheckForException() || !jObj)
        return NULL;

    return reinterpret_cast<EGLSurface>(mJEnv->GetIntField(jObj, jEGLSurfacePointerField));
}

void
AndroidGLController::WaitForValidSurface()
{
    ASSERT_THREAD();
    AutoLocalJNIFrame jniFrame(mJEnv, 0);
    mJEnv->CallVoidMethod(mJObj, jWaitForValidSurfaceMethod);
}
