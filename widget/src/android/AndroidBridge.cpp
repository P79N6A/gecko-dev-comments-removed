




































#include <android/log.h>

#include <pthread.h>
#include <prthread.h>

#include "AndroidBridge.h"

using namespace mozilla;

static PRUintn sJavaEnvThreadIndex = 0;

AndroidBridge *AndroidBridge::sBridge = 0;

static void
JavaThreadDetachFunc(void *arg)
{
    JNIEnv *env = (JNIEnv*) arg;
    JavaVM *vm = NULL;
    env->GetJavaVM(&vm);
    vm->DetachCurrentThread();
}

AndroidBridge *
AndroidBridge::ConstructBridge(JNIEnv *jEnv,
                               jclass jGeckoAppShellClass)
{
    





    putenv(strdup("NSS_DISABLE_UNLOAD=1"));

    sBridge = new AndroidBridge();
    if (!sBridge->Init(jEnv, jGeckoAppShellClass)) {
        delete sBridge;
        sBridge = 0;
    }

    PR_NewThreadPrivateIndex(&sJavaEnvThreadIndex, JavaThreadDetachFunc);

    return sBridge;
}

PRBool
AndroidBridge::Init(JNIEnv *jEnv,
                    jclass jGeckoAppShellClass)
{
    jEnv->GetJavaVM(&mJavaVM);

    mJNIEnv = nsnull;
    mThread = nsnull;

    mGeckoAppShellClass = (jclass) jEnv->NewGlobalRef(jGeckoAppShellClass);

    jShowIME = (jmethodID) jEnv->GetStaticMethodID(jGeckoAppShellClass, "showIME", "(I)V");
    jEnableAccelerometer = (jmethodID) jEnv->GetStaticMethodID(jGeckoAppShellClass, "enableAccelerometer", "(Z)V");
    jEnableLocation = (jmethodID) jEnv->GetStaticMethodID(jGeckoAppShellClass, "enableLocation", "(Z)V");
    jReturnIMEQueryResult = (jmethodID) jEnv->GetStaticMethodID(jGeckoAppShellClass, "returnIMEQueryResult", "(Ljava/lang/String;II)V");
    jScheduleRestart = (jmethodID) jEnv->GetStaticMethodID(jGeckoAppShellClass, "scheduleRestart", "()V");
    jNotifyXreExit = (jmethodID) jEnv->GetStaticMethodID(jGeckoAppShellClass, "onXreExit", "()V");

    InitAndroidJavaWrappers(jEnv);

    
    
    

    return PR_TRUE;
}

JNIEnv *
AndroidBridge::AttachThread(PRBool asDaemon)
{
    JNIEnv *jEnv = (JNIEnv*) PR_GetThreadPrivate(sJavaEnvThreadIndex);
    if (jEnv)
        return jEnv;

    JavaVMAttachArgs args = {
        JNI_VERSION_1_2,
        "GeckoThread",
        NULL
    };

    jint res = 0;

    if (asDaemon) {
        res = mJavaVM->AttachCurrentThreadAsDaemon(&jEnv, &args);
    } else {
        res = mJavaVM->AttachCurrentThread(&jEnv, &args);
    }

    if (res != 0) {
        ALOG("AttachCurrentThread failed!");
        return nsnull;
    }

    PR_SetThreadPrivate(sJavaEnvThreadIndex, jEnv);

    return jEnv;
}

PRBool
AndroidBridge::SetMainThread(void *thr)
{
    if (thr) {
        mJNIEnv = AttachThread(PR_FALSE);
        if (!mJNIEnv)
            return PR_FALSE;

        mThread = thr;
    } else {
        mJNIEnv = nsnull;
        mThread = nsnull;
    }

    return PR_TRUE;
}

void
AndroidBridge::EnsureJNIThread()
{
    JNIEnv *env;
    if (mJavaVM->AttachCurrentThread(&env, NULL) != 0) {
        ALOG("EnsureJNIThread: test Attach failed!");
        return;
    }

    if ((void*)pthread_self() != mThread) {
        ALOG("###!!!!!!! Something's grabbing the JNIEnv from the wrong thread! (thr %p should be %p)",
             (void*)pthread_self(), (void*)mThread);
    }
}

void
AndroidBridge::ShowIME(int aState)
{
    mJNIEnv->CallStaticVoidMethod(mGeckoAppShellClass, jShowIME, aState);
}

void
AndroidBridge::EnableAccelerometer(bool aEnable)
{
    mJNIEnv->CallStaticVoidMethod(mGeckoAppShellClass, jEnableAccelerometer, aEnable);
}

void
AndroidBridge::EnableLocation(bool aEnable)
{
    mJNIEnv->CallStaticVoidMethod(mGeckoAppShellClass, jEnableLocation, aEnable);
}

void
AndroidBridge::ReturnIMEQueryResult(const PRUnichar *result, PRUint32 len, int selectionStart, int selectionEnd)
{
    jvalue args[3];
    args[0].l = mJNIEnv->NewString(result, len);
    args[1].i = selectionStart;
    args[2].i = selectionEnd;
    mJNIEnv->CallStaticVoidMethodA(mGeckoAppShellClass, jReturnIMEQueryResult, args);
}

void
AndroidBridge::ScheduleRestart()
{
    ALOG("scheduling reboot");
    mJNIEnv->CallStaticVoidMethod(mGeckoAppShellClass, jScheduleRestart);
}

void
AndroidBridge::NotifyXreExit()
{
    ALOG("xre exiting");
    mJNIEnv->CallStaticVoidMethod(mGeckoAppShellClass, jNotifyXreExit);
}

void
AndroidBridge::SetSurfaceView(jobject obj)
{
    mSurfaceView.Init(obj);
}


PRBool
mozilla_AndroidBridge_SetMainThread(void *thr)
{
    return AndroidBridge::Bridge()->SetMainThread(thr);
}

JavaVM *
mozilla_AndroidBridge_GetJavaVM()
{
    return AndroidBridge::Bridge()->VM();
}

JNIEnv *
mozilla_AndroidBridge_AttachThread(PRBool asDaemon)
{
    return AndroidBridge::Bridge()->AttachThread(asDaemon);
}

extern "C" JNIEnv * GetJNIForThread()
{
  return mozilla::AndroidBridge::JNIForThread();
}

