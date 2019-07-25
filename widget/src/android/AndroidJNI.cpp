




































#include "nsILocalFile.h"

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
