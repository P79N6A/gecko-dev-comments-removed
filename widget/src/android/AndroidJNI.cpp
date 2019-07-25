




































#include "nsILocalFile.h"
#include "nsString.h"

#include "AndroidBridge.h"

#include <jni.h>
#include <pthread.h>
#include <dlfcn.h>

#include "nsAppShell.h"
#include "nsWindow.h"
#include <android/log.h>

using namespace mozilla;



extern "C" {
    NS_EXPORT void JNICALL Java_org_mozilla_gecko_GeckoAppShell_nativeInit(JNIEnv *, jclass);
    NS_EXPORT void JNICALL Java_org_mozilla_gecko_GeckoAppShell_notifyGeckoOfEvent(JNIEnv *, jclass, jobject event);
    NS_EXPORT void JNICALL Java_org_mozilla_gecko_GeckoAppShell_setSurfaceView(JNIEnv *jenv, jclass, jobject sv);
    NS_EXPORT void JNICALL Java_org_mozilla_gecko_GeckoAppShell_setInitialSize(JNIEnv *jenv, jclass, int width, int height);
    NS_EXPORT void JNICALL Java_org_mozilla_gecko_GeckoAppShell_onResume(JNIEnv *, jclass);
    NS_EXPORT void JNICALL Java_org_mozilla_gecko_GeckoAppShell_callObserver(JNIEnv *, jclass, jstring observerKey, jstring topic, jstring data);
    NS_EXPORT void JNICALL Java_org_mozilla_gecko_GeckoAppShell_removeObserver(JNIEnv *jenv, jclass, jstring jObserverKey);
}






NS_EXPORT void JNICALL
Java_org_mozilla_gecko_GeckoAppShell_nativeInit(JNIEnv *jenv, jclass jc)
{
    AndroidBridge::ConstructBridge(jenv, jc);
}

NS_EXPORT void JNICALL
Java_org_mozilla_gecko_GeckoAppShell_notifyGeckoOfEvent(JNIEnv *jenv, jclass jc, jobject event)
{
    
    if (nsAppShell::gAppShell)
        nsAppShell::gAppShell->PostEvent(new AndroidGeckoEvent(jenv, event));
    else if (!nsAppShell::gEarlyEvent)
        nsAppShell::gEarlyEvent = new AndroidGeckoEvent(jenv, event);
}

NS_EXPORT void JNICALL
Java_org_mozilla_gecko_GeckoAppShell_setSurfaceView(JNIEnv *jenv, jclass, jobject obj)
{
    AndroidBridge::Bridge()->SetSurfaceView(jenv->NewGlobalRef(obj));
}

NS_EXPORT void JNICALL
Java_org_mozilla_gecko_GeckoAppShell_setInitialSize(JNIEnv *jenv, jclass, int width, int height)
{
    nsWindow::SetInitialAndroidBounds(gfxIntSize(width, height));
}

NS_EXPORT void JNICALL
Java_org_mozilla_gecko_GeckoAppShell_onResume(JNIEnv *jenv, jclass jc)
{
    if (nsAppShell::gAppShell)
        nsAppShell::gAppShell->OnResume();
}

NS_EXPORT void JNICALL
Java_org_mozilla_gecko_GeckoAppShell_callObserver(JNIEnv *jenv, jclass, jstring jObserverKey, jstring jTopic, jstring jData)
{
    if (!nsAppShell::gAppShell)
        return;

    nsJNIString sObserverKey(jObserverKey, jenv);
    nsJNIString sTopic(jTopic, jenv);
    nsJNIString sData(jData, jenv);

    nsAppShell::gAppShell->CallObserver(sObserverKey, sTopic, sData);
}

NS_EXPORT void JNICALL
Java_org_mozilla_gecko_GeckoAppShell_removeObserver(JNIEnv *jenv, jclass, jstring jObserverKey)
{
    if (!nsAppShell::gAppShell)
        return;

    const jchar *observerKey = jenv->GetStringChars(jObserverKey, NULL);
    nsString sObserverKey(observerKey);
    sObserverKey.SetLength(jenv->GetStringLength(jObserverKey));
    jenv->ReleaseStringChars(jObserverKey, observerKey);

    nsAppShell::gAppShell->RemoveObserver(sObserverKey);
}
