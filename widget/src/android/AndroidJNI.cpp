




































#include "nsILocalFile.h"
#include "nsString.h"

#include "AndroidBridge.h"

#include <jni.h>
#include <pthread.h>
#include <dlfcn.h>

#include "nsAppShell.h"
#include "nsWindow.h"
#include <android/log.h>
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "nsINetworkLinkService.h"

#ifdef MOZ_CRASHREPORTER
#include "nsICrashReporter.h"
#include "nsExceptionHandler.h"
#endif


using namespace mozilla;



extern "C" {
    NS_EXPORT void JNICALL Java_org_mozilla_gecko_GeckoAppShell_nativeInit(JNIEnv *, jclass);
    NS_EXPORT void JNICALL Java_org_mozilla_gecko_GeckoAppShell_notifyGeckoOfEvent(JNIEnv *, jclass, jobject event);
    NS_EXPORT void JNICALL Java_org_mozilla_gecko_GeckoAppShell_setSurfaceView(JNIEnv *jenv, jclass, jobject sv);
    NS_EXPORT void JNICALL Java_org_mozilla_gecko_GeckoAppShell_onResume(JNIEnv *, jclass);
    NS_EXPORT void JNICALL Java_org_mozilla_gecko_GeckoAppShell_onLowMemory(JNIEnv *, jclass);
    NS_EXPORT void JNICALL Java_org_mozilla_gecko_GeckoAppShell_callObserver(JNIEnv *, jclass, jstring observerKey, jstring topic, jstring data);
    NS_EXPORT void JNICALL Java_org_mozilla_gecko_GeckoAppShell_removeObserver(JNIEnv *jenv, jclass, jstring jObserverKey);
    NS_EXPORT void JNICALL Java_org_mozilla_gecko_GeckoAppShell_onChangeNetworkLinkStatus(JNIEnv *, jclass, jstring status);
    NS_EXPORT void JNICALL Java_org_mozilla_gecko_GeckoAppShell_reportJavaCrash(JNIEnv *, jclass, jstring stack);
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
}

NS_EXPORT void JNICALL
Java_org_mozilla_gecko_GeckoAppShell_setSurfaceView(JNIEnv *jenv, jclass, jobject obj)
{
    AndroidBridge::Bridge()->SetSurfaceView(jenv->NewGlobalRef(obj));
}

NS_EXPORT void JNICALL
Java_org_mozilla_gecko_GeckoAppShell_onLowMemory(JNIEnv *jenv, jclass jc)
{
    if (nsAppShell::gAppShell) {
        nsAppShell::gAppShell->NotifyObservers(nsnull,
                                               "memory-pressure",
                                               NS_LITERAL_STRING("low-memory").get());
    }
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

NS_EXPORT void JNICALL
Java_org_mozilla_gecko_GeckoAppShell_onChangeNetworkLinkStatus(JNIEnv *jenv, jclass, jstring jStatus)
{
    if (!nsAppShell::gAppShell)
        return;

    nsJNIString sStatus(jStatus, jenv);

    nsAppShell::gAppShell->NotifyObservers(nsnull,
                                           NS_NETWORK_LINK_TOPIC,
                                           sStatus.get());
}

NS_EXPORT void JNICALL
Java_org_mozilla_gecko_GeckoAppShell_reportJavaCrash(JNIEnv *, jclass, jstring stack)
{
     nsJNIString javaStack(stack);
     CrashReporter::AppendAppNotesToCrashReport(
         NS_ConvertUTF16toUTF8(javaStack));
    abort();
}
