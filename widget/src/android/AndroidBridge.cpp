




































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
    jGetHandlersForMimeType = (jmethodID) jEnv->GetStaticMethodID(jGeckoAppShellClass, "getHandlersForMimeType", "(Ljava/lang/String;)[Ljava/lang/String;");
    jOpenUriExternal = (jmethodID) jEnv->GetStaticMethodID(jGeckoAppShellClass, "openUriExternal", "(Ljava/lang/String;Ljava/lang/String;)Z");
    jGetMimeTypeFromExtension = (jmethodID) jEnv->GetStaticMethodID(jGeckoAppShellClass, "getMimeTypeFromExtension", "(Ljava/lang/String;)Ljava/lang/String;");
    jMoveTaskToBack = (jmethodID) jEnv->GetStaticMethodID(jGeckoAppShellClass, "moveTaskToBack", "()V");
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
AndroidBridge::GetHandlersForMimeType(const char *aMimeType, nsStringArray* aStringArray)
{
    NS_PRECONDITION(aStringArray != nsnull, "null array pointer passed in");
    AutoLocalJNIFrame jniFrame;
    NS_ConvertUTF8toUTF16 wMimeType(aMimeType);
    jstring jstr = mJNIEnv->NewString(wMimeType.get(), wMimeType.Length());
    jobject obj = mJNIEnv->CallStaticObjectMethod(mGeckoAppShellClass, 
                                                  jGetHandlersForMimeType, 
                                                  jstr);
    jobjectArray arr = static_cast<jobjectArray>(obj);
    if (!arr)
        return;
    jsize len = mJNIEnv->GetArrayLength(arr);
    for (jsize i = 0; i < len; i+=2) {
        jstring jstr = static_cast<jstring>(mJNIEnv->GetObjectArrayElement(arr, i));
        nsJNIString jniStr(jstr);
        aStringArray->AppendString(jniStr);
    } 
}

PRBool
AndroidBridge::OpenUriExternal(nsCString& aUriSpec, nsCString& aMimeType) 
{
    AutoLocalJNIFrame jniFrame;
    NS_ConvertUTF8toUTF16 wUriSpec(aUriSpec);
    NS_ConvertUTF8toUTF16 wMimeType(aMimeType);
    jstring jstrUri = mJNIEnv->NewString(wUriSpec.get(), wUriSpec.Length());
    jstring jstrType = mJNIEnv->NewString(wMimeType.get(), wMimeType.Length());
    return mJNIEnv->CallStaticBooleanMethod(mGeckoAppShellClass,
                                            jOpenUriExternal,
                                            jstrUri, jstrType);
}

void
AndroidBridge::GetMimeTypeFromExtension(const nsCString& aFileExt, nsCString& aMimeType) {
    AutoLocalJNIFrame jniFrame;
    NS_ConvertUTF8toUTF16 wFileExt(aFileExt);
    jstring jstrExt = mJNIEnv->NewString(wFileExt.get(), wFileExt.Length());
    jstring jstrType =  static_cast<jstring>(mJNIEnv->CallStaticObjectMethod(mGeckoAppShellClass,
                                                                             jGetMimeTypeFromExtension,
                                                                             jstrExt));
    nsJNIString jniStr(jstrType);
    aMimeType.Assign(NS_ConvertUTF16toUTF8(jniStr.get()));
}

void
AndroidBridge::MoveTaskToBack()
{
    mJNIEnv->CallStaticVoidMethod(mGeckoAppShellClass, jMoveTaskToBack);
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

