




#include "GeneratedJNIWrappers.h"
#include "AndroidBridgeUtilities.h"
#include "nsXPCOMStrings.h"
#include "AndroidBridge.h"

namespace mozilla {
namespace widget {
namespace android {
jclass DownloadsIntegration::mDownloadsIntegrationClass = 0;
jmethodID DownloadsIntegration::jScanMedia = 0;
void DownloadsIntegration::InitStubs(JNIEnv *env) {
    mDownloadsIntegrationClass = AndroidBridge::GetClassGlobalRef(env, "org/mozilla/gecko/DownloadsIntegration");
    jScanMedia = AndroidBridge::GetStaticMethodID(env, mDownloadsIntegrationClass, "scanMedia", "(Ljava/lang/String;Ljava/lang/String;)V");
}

DownloadsIntegration* DownloadsIntegration::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    DownloadsIntegration* ret = new DownloadsIntegration(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

void DownloadsIntegration::ScanMedia(const nsAString& a0, const nsAString& a1) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);
    jstring j1 = AndroidBridge::NewJavaString(env, a1);

    env->CallStaticVoidMethod(mDownloadsIntegrationClass, jScanMedia, j0, j1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}
jclass GeckoAppShell::mGeckoAppShellClass = 0;
jmethodID GeckoAppShell::jAcknowledgeEvent = 0;
jmethodID GeckoAppShell::jAddPluginViewWrapper = 0;
jmethodID GeckoAppShell::jAlertsProgressListener_OnProgress = 0;
jmethodID GeckoAppShell::jCancelVibrate = 0;
jmethodID GeckoAppShell::jCheckURIVisited = 0;
jmethodID GeckoAppShell::jClearMessageList = 0;
jmethodID GeckoAppShell::jCloseCamera = 0;
jmethodID GeckoAppShell::jCloseNotification = 0;
jmethodID GeckoAppShell::jConnectionGetMimeType = 0;
jmethodID GeckoAppShell::jCreateInputStream = 0;
jmethodID GeckoAppShell::jCreateMessageListWrapper = 0;
jmethodID GeckoAppShell::jCreateShortcut = 0;
jmethodID GeckoAppShell::jDeleteMessageWrapper = 0;
jmethodID GeckoAppShell::jDisableBatteryNotifications = 0;
jmethodID GeckoAppShell::jDisableNetworkNotifications = 0;
jmethodID GeckoAppShell::jDisableScreenOrientationNotifications = 0;
jmethodID GeckoAppShell::jDisableSensor = 0;
jmethodID GeckoAppShell::jEnableBatteryNotifications = 0;
jmethodID GeckoAppShell::jEnableLocation = 0;
jmethodID GeckoAppShell::jEnableLocationHighAccuracy = 0;
jmethodID GeckoAppShell::jEnableNetworkNotifications = 0;
jmethodID GeckoAppShell::jEnableScreenOrientationNotifications = 0;
jmethodID GeckoAppShell::jEnableSensor = 0;
jmethodID GeckoAppShell::jGamepadAdded = 0;
jmethodID GeckoAppShell::jGetConnection = 0;
jmethodID GeckoAppShell::jGetContext = 0;
jmethodID GeckoAppShell::jGetCurrentBatteryInformationWrapper = 0;
jmethodID GeckoAppShell::jGetCurrentNetworkInformationWrapper = 0;
jmethodID GeckoAppShell::jGetDensity = 0;
jmethodID GeckoAppShell::jGetDpiWrapper = 0;
jmethodID GeckoAppShell::jGetExtensionFromMimeTypeWrapper = 0;
jmethodID GeckoAppShell::jGetExternalPublicDirectory = 0;
jmethodID GeckoAppShell::jGetHandlersForMimeTypeWrapper = 0;
jmethodID GeckoAppShell::jGetHandlersForURLWrapper = 0;
jmethodID GeckoAppShell::jGetIconForExtensionWrapper = 0;
jmethodID GeckoAppShell::jGetMessageWrapper = 0;
jmethodID GeckoAppShell::jGetMimeTypeFromExtensionsWrapper = 0;
jmethodID GeckoAppShell::jGetNextMessageInListWrapper = 0;
jmethodID GeckoAppShell::jGetProxyForURIWrapper = 0;
jmethodID GeckoAppShell::jGetScreenDepthWrapper = 0;
jmethodID GeckoAppShell::jGetScreenOrientationWrapper = 0;
jmethodID GeckoAppShell::jGetShowPasswordSetting = 0;
jmethodID GeckoAppShell::jGetSystemColoursWrapper = 0;
jmethodID GeckoAppShell::jHandleGeckoMessageWrapper = 0;
jmethodID GeckoAppShell::jHandleUncaughtException = 0;
jmethodID GeckoAppShell::jHideProgressDialog = 0;
jmethodID GeckoAppShell::jInitCameraWrapper = 0;
jmethodID GeckoAppShell::jIsNetworkLinkKnown = 0;
jmethodID GeckoAppShell::jIsNetworkLinkUp = 0;
jmethodID GeckoAppShell::jIsTablet = 0;
jmethodID GeckoAppShell::jKillAnyZombies = 0;
jmethodID GeckoAppShell::jLoadPluginClass = 0;
jmethodID GeckoAppShell::jLockScreenOrientation = 0;
jmethodID GeckoAppShell::jMarkURIVisited = 0;
jmethodID GeckoAppShell::jMoveTaskToBack = 0;
jmethodID GeckoAppShell::jNetworkLinkType = 0;
jmethodID GeckoAppShell::jNotifyDefaultPrevented = 0;
jmethodID GeckoAppShell::jNotifyIME = 0;
jmethodID GeckoAppShell::jNotifyIMEChange = 0;
jmethodID GeckoAppShell::jNotifyIMEContext = 0;
jmethodID GeckoAppShell::jNotifyWakeLockChanged = 0;
jmethodID GeckoAppShell::jNotifyXreExit = 0;
jmethodID GeckoAppShell::jOpenUriExternal = 0;
jmethodID GeckoAppShell::jPerformHapticFeedback = 0;
jmethodID GeckoAppShell::jPumpMessageLoop = 0;
jmethodID GeckoAppShell::jRegisterSurfaceTextureFrameListener = 0;
jmethodID GeckoAppShell::jRemovePluginView = 0;
jmethodID GeckoAppShell::jRequestUiThreadCallback = 0;
jmethodID GeckoAppShell::jScheduleRestart = 0;
jmethodID GeckoAppShell::jSendMessageWrapper = 0;
jmethodID GeckoAppShell::jSetFullScreen = 0;
jmethodID GeckoAppShell::jSetKeepScreenOn = 0;
jmethodID GeckoAppShell::jSetURITitle = 0;
jmethodID GeckoAppShell::jShowAlertNotificationWrapper = 0;
jmethodID GeckoAppShell::jShowInputMethodPicker = 0;
jmethodID GeckoAppShell::jStartMonitoringGamepad = 0;
jmethodID GeckoAppShell::jStopMonitoringGamepad = 0;
jmethodID GeckoAppShell::jUnlockProfile = 0;
jmethodID GeckoAppShell::jUnlockScreenOrientation = 0;
jmethodID GeckoAppShell::jUnregisterSurfaceTextureFrameListener = 0;
jmethodID GeckoAppShell::jVibrate1 = 0;
jmethodID GeckoAppShell::jVibrateA = 0;
void GeckoAppShell::InitStubs(JNIEnv *env) {
    mGeckoAppShellClass = AndroidBridge::GetClassGlobalRef(env, "org/mozilla/gecko/GeckoAppShell");
    jAcknowledgeEvent = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "acknowledgeEvent", "()V");
    jAddPluginViewWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "addPluginView", "(Landroid/view/View;FFFFZ)V");
    jAlertsProgressListener_OnProgress = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "alertsProgressListener_OnProgress", "(Ljava/lang/String;JJLjava/lang/String;)V");
    jCancelVibrate = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "cancelVibrate", "()V");
    jCheckURIVisited = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "checkUriVisited", "(Ljava/lang/String;)V");
    jClearMessageList = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "clearMessageList", "(I)V");
    jCloseCamera = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "closeCamera", "()V");
    jCloseNotification = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "closeNotification", "(Ljava/lang/String;)V");
    jConnectionGetMimeType = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "connectionGetMimeType", "(Ljava/net/URLConnection;)Ljava/lang/String;");
    jCreateInputStream = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "createInputStream", "(Ljava/net/URLConnection;)Ljava/io/InputStream;");
    jCreateMessageListWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "createMessageList", "(JJ[Ljava/lang/String;ILjava/lang/String;ZZJZI)V");
    jCreateShortcut = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "createShortcut", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    jDeleteMessageWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "deleteMessage", "(II)V");
    jDisableBatteryNotifications = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "disableBatteryNotifications", "()V");
    jDisableNetworkNotifications = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "disableNetworkNotifications", "()V");
    jDisableScreenOrientationNotifications = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "disableScreenOrientationNotifications", "()V");
    jDisableSensor = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "disableSensor", "(I)V");
    jEnableBatteryNotifications = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "enableBatteryNotifications", "()V");
    jEnableLocation = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "enableLocation", "(Z)V");
    jEnableLocationHighAccuracy = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "enableLocationHighAccuracy", "(Z)V");
    jEnableNetworkNotifications = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "enableNetworkNotifications", "()V");
    jEnableScreenOrientationNotifications = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "enableScreenOrientationNotifications", "()V");
    jEnableSensor = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "enableSensor", "(I)V");
    jGamepadAdded = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "gamepadAdded", "(II)V");
    jGetConnection = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "getConnection", "(Ljava/lang/String;)Ljava/net/URLConnection;");
    jGetContext = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "getContext", "()Landroid/content/Context;");
    jGetCurrentBatteryInformationWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "getCurrentBatteryInformation", "()[D");
    jGetCurrentNetworkInformationWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "getCurrentNetworkInformation", "()[D");
    jGetDensity = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "getDensity", "()F");
    jGetDpiWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "getDpi", "()I");
    jGetExtensionFromMimeTypeWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "getExtensionFromMimeType", "(Ljava/lang/String;)Ljava/lang/String;");
    jGetExternalPublicDirectory = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "getExternalPublicDirectory", "(Ljava/lang/String;)Ljava/lang/String;");
    jGetHandlersForMimeTypeWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "getHandlersForMimeType", "(Ljava/lang/String;Ljava/lang/String;)[Ljava/lang/String;");
    jGetHandlersForURLWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "getHandlersForURL", "(Ljava/lang/String;Ljava/lang/String;)[Ljava/lang/String;");
    jGetIconForExtensionWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "getIconForExtension", "(Ljava/lang/String;I)[B");
    jGetMessageWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "getMessage", "(II)V");
    jGetMimeTypeFromExtensionsWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "getMimeTypeFromExtensions", "(Ljava/lang/String;)Ljava/lang/String;");
    jGetNextMessageInListWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "getNextMessageInList", "(II)V");
    jGetProxyForURIWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "getProxyForURI", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)Ljava/lang/String;");
    jGetScreenDepthWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "getScreenDepth", "()I");
    jGetScreenOrientationWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "getScreenOrientation", "()S");
    jGetShowPasswordSetting = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "getShowPasswordSetting", "()Z");
    jGetSystemColoursWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "getSystemColors", "()[I");
    jHandleGeckoMessageWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "handleGeckoMessage", "(Lorg/mozilla/gecko/util/NativeJSContainer;)V");
    jHandleUncaughtException = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "handleUncaughtException", "(Ljava/lang/Thread;Ljava/lang/Throwable;)V");
    jHideProgressDialog = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "hideProgressDialog", "()V");
    jInitCameraWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "initCamera", "(Ljava/lang/String;III)[I");
    jIsNetworkLinkKnown = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "isNetworkLinkKnown", "()Z");
    jIsNetworkLinkUp = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "isNetworkLinkUp", "()Z");
    jIsTablet = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "isTablet", "()Z");
    jKillAnyZombies = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "killAnyZombies", "()V");
    jLoadPluginClass = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "loadPluginClass", "(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Class;");
    jLockScreenOrientation = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "lockScreenOrientation", "(I)V");
    jMarkURIVisited = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "markUriVisited", "(Ljava/lang/String;)V");
    jMoveTaskToBack = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "moveTaskToBack", "()V");
    jNetworkLinkType = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "networkLinkType", "()I");
    jNotifyDefaultPrevented = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "notifyDefaultPrevented", "(Z)V");
    jNotifyIME = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "notifyIME", "(I)V");
    jNotifyIMEChange = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "notifyIMEChange", "(Ljava/lang/String;III)V");
    jNotifyIMEContext = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "notifyIMEContext", "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    jNotifyWakeLockChanged = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "notifyWakeLockChanged", "(Ljava/lang/String;Ljava/lang/String;)V");
    jNotifyXreExit = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "onXreExit", "()V");
    jOpenUriExternal = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "openUriExternal", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z");
    jPerformHapticFeedback = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "performHapticFeedback", "(Z)V");
    jPumpMessageLoop = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "pumpMessageLoop", "()Z");
    jRegisterSurfaceTextureFrameListener = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "registerSurfaceTextureFrameListener", "(Ljava/lang/Object;I)V");
    jRemovePluginView = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "removePluginView", "(Landroid/view/View;Z)V");
    jRequestUiThreadCallback = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "requestUiThreadCallback", "(J)V");
    jScheduleRestart = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "scheduleRestart", "()V");
    jSendMessageWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "sendMessage", "(Ljava/lang/String;Ljava/lang/String;I)V");
    jSetFullScreen = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "setFullScreen", "(Z)V");
    jSetKeepScreenOn = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "setKeepScreenOn", "(Z)V");
    jSetURITitle = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "setUriTitle", "(Ljava/lang/String;Ljava/lang/String;)V");
    jShowAlertNotificationWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "showAlertNotification", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    jShowInputMethodPicker = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "showInputMethodPicker", "()V");
    jStartMonitoringGamepad = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "startMonitoringGamepad", "()V");
    jStopMonitoringGamepad = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "stopMonitoringGamepad", "()V");
    jUnlockProfile = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "unlockProfile", "()Z");
    jUnlockScreenOrientation = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "unlockScreenOrientation", "()V");
    jUnregisterSurfaceTextureFrameListener = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "unregisterSurfaceTextureFrameListener", "(Ljava/lang/Object;)V");
    jVibrate1 = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "vibrate", "(J)V");
    jVibrateA = AndroidBridge::GetStaticMethodID(env, mGeckoAppShellClass, "vibrate", "([JI)V");
}

GeckoAppShell* GeckoAppShell::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    GeckoAppShell* ret = new GeckoAppShell(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

void GeckoAppShell::AcknowledgeEvent() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jAcknowledgeEvent);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::AddPluginViewWrapper(jobject a0, jfloat a1, jfloat a2, jfloat a3, jfloat a4, bool a5) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[6];
    args[0].l = a0;
    args[1].f = a1;
    args[2].f = a2;
    args[3].f = a3;
    args[4].f = a4;
    args[5].z = a5;

    env->CallStaticVoidMethodA(mGeckoAppShellClass, jAddPluginViewWrapper, args);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::AlertsProgressListener_OnProgress(const nsAString& a0, int64_t a1, int64_t a2, const nsAString& a3) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[4];
    args[0].l = AndroidBridge::NewJavaString(env, a0);
    args[1].j = a1;
    args[2].j = a2;
    args[3].l = AndroidBridge::NewJavaString(env, a3);

    env->CallStaticVoidMethodA(mGeckoAppShellClass, jAlertsProgressListener_OnProgress, args);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::CancelVibrate() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jCancelVibrate);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::CheckURIVisited(const nsAString& a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    env->CallStaticVoidMethod(mGeckoAppShellClass, jCheckURIVisited, j0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::ClearMessageList(int32_t a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jClearMessageList, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::CloseCamera() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jCloseCamera);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::CloseNotification(const nsAString& a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    env->CallStaticVoidMethod(mGeckoAppShellClass, jCloseNotification, j0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

jstring GeckoAppShell::ConnectionGetMimeType(jobject a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jConnectionGetMimeType, a0);
    AndroidBridge::HandleUncaughtException(env);
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

jobject GeckoAppShell::CreateInputStream(jobject a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jCreateInputStream, a0);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

void GeckoAppShell::CreateMessageListWrapper(int64_t a0, int64_t a1, jobjectArray a2, int32_t a3, const nsAString& a4, bool a5, bool a6, int64_t a7, bool a8, int32_t a9) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[10];
    args[0].j = a0;
    args[1].j = a1;
    args[2].l = a2;
    args[3].i = a3;
    args[4].l = AndroidBridge::NewJavaString(env, a4);
    args[5].z = a5;
    args[6].z = a6;
    args[7].j = a7;
    args[8].z = a8;
    args[9].i = a9;

    env->CallStaticVoidMethodA(mGeckoAppShellClass, jCreateMessageListWrapper, args);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::CreateShortcut(const nsAString& a0, const nsAString& a1, const nsAString& a2) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(3) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[3];
    args[0].l = AndroidBridge::NewJavaString(env, a0);
    args[1].l = AndroidBridge::NewJavaString(env, a1);
    args[2].l = AndroidBridge::NewJavaString(env, a2);

    env->CallStaticVoidMethodA(mGeckoAppShellClass, jCreateShortcut, args);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::DeleteMessageWrapper(int32_t a0, int32_t a1) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jDeleteMessageWrapper, a0, a1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::DisableBatteryNotifications() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jDisableBatteryNotifications);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::DisableNetworkNotifications() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jDisableNetworkNotifications);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::DisableScreenOrientationNotifications() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jDisableScreenOrientationNotifications);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::DisableSensor(int32_t a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jDisableSensor, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::EnableBatteryNotifications() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jEnableBatteryNotifications);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::EnableLocation(bool a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jEnableLocation, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::EnableLocationHighAccuracy(bool a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jEnableLocationHighAccuracy, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::EnableNetworkNotifications() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jEnableNetworkNotifications);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::EnableScreenOrientationNotifications() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jEnableScreenOrientationNotifications);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::EnableSensor(int32_t a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jEnableSensor, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::GamepadAdded(int32_t a0, int32_t a1) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jGamepadAdded, a0, a1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

jobject GeckoAppShell::GetConnection(const nsACString& a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jGetConnection, j0);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject GeckoAppShell::GetContext() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jGetContext);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jdoubleArray GeckoAppShell::GetCurrentBatteryInformationWrapper() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jGetCurrentBatteryInformationWrapper);
    AndroidBridge::HandleUncaughtException(env);
    jdoubleArray ret = static_cast<jdoubleArray>(env->PopLocalFrame(temp));
    return ret;
}

jdoubleArray GeckoAppShell::GetCurrentNetworkInformationWrapper() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jGetCurrentNetworkInformationWrapper);
    AndroidBridge::HandleUncaughtException(env);
    jdoubleArray ret = static_cast<jdoubleArray>(env->PopLocalFrame(temp));
    return ret;
}

jfloat GeckoAppShell::GetDensity() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jfloat temp = env->CallStaticFloatMethod(mGeckoAppShellClass, jGetDensity);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

int32_t GeckoAppShell::GetDpiWrapper() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    int32_t temp = env->CallStaticIntMethod(mGeckoAppShellClass, jGetDpiWrapper);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

jstring GeckoAppShell::GetExtensionFromMimeTypeWrapper(const nsAString& a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jGetExtensionFromMimeTypeWrapper, j0);
    AndroidBridge::HandleUncaughtException(env);
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

jstring GeckoAppShell::GetExternalPublicDirectory(const nsAString& a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jGetExternalPublicDirectory, j0);
    AndroidBridge::HandleUncaughtException(env);
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

jobjectArray GeckoAppShell::GetHandlersForMimeTypeWrapper(const nsAString& a0, const nsAString& a1) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(3) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);
    jstring j1 = AndroidBridge::NewJavaString(env, a1);

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jGetHandlersForMimeTypeWrapper, j0, j1);
    AndroidBridge::HandleUncaughtException(env);
    jobjectArray ret = static_cast<jobjectArray>(env->PopLocalFrame(temp));
    return ret;
}

jobjectArray GeckoAppShell::GetHandlersForURLWrapper(const nsAString& a0, const nsAString& a1) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(3) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);
    jstring j1 = AndroidBridge::NewJavaString(env, a1);

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jGetHandlersForURLWrapper, j0, j1);
    AndroidBridge::HandleUncaughtException(env);
    jobjectArray ret = static_cast<jobjectArray>(env->PopLocalFrame(temp));
    return ret;
}

jbyteArray GeckoAppShell::GetIconForExtensionWrapper(const nsAString& a0, int32_t a1) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jGetIconForExtensionWrapper, j0, a1);
    AndroidBridge::HandleUncaughtException(env);
    jbyteArray ret = static_cast<jbyteArray>(env->PopLocalFrame(temp));
    return ret;
}

void GeckoAppShell::GetMessageWrapper(int32_t a0, int32_t a1) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jGetMessageWrapper, a0, a1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

jstring GeckoAppShell::GetMimeTypeFromExtensionsWrapper(const nsAString& a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jGetMimeTypeFromExtensionsWrapper, j0);
    AndroidBridge::HandleUncaughtException(env);
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

void GeckoAppShell::GetNextMessageInListWrapper(int32_t a0, int32_t a1) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jGetNextMessageInListWrapper, a0, a1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

jstring GeckoAppShell::GetProxyForURIWrapper(const nsAString& a0, const nsAString& a1, const nsAString& a2, int32_t a3) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(4) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[4];
    args[0].l = AndroidBridge::NewJavaString(env, a0);
    args[1].l = AndroidBridge::NewJavaString(env, a1);
    args[2].l = AndroidBridge::NewJavaString(env, a2);
    args[3].i = a3;

    jobject temp = env->CallStaticObjectMethodA(mGeckoAppShellClass, jGetProxyForURIWrapper, args);
    AndroidBridge::HandleUncaughtException(env);
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

int32_t GeckoAppShell::GetScreenDepthWrapper() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    int32_t temp = env->CallStaticIntMethod(mGeckoAppShellClass, jGetScreenDepthWrapper);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

int16_t GeckoAppShell::GetScreenOrientationWrapper() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    int16_t temp = env->CallStaticShortMethod(mGeckoAppShellClass, jGetScreenOrientationWrapper);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

bool GeckoAppShell::GetShowPasswordSetting() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    bool temp = env->CallStaticBooleanMethod(mGeckoAppShellClass, jGetShowPasswordSetting);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

jintArray GeckoAppShell::GetSystemColoursWrapper() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jGetSystemColoursWrapper);
    AndroidBridge::HandleUncaughtException(env);
    jintArray ret = static_cast<jintArray>(env->PopLocalFrame(temp));
    return ret;
}

void GeckoAppShell::HandleGeckoMessageWrapper(jobject a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jHandleGeckoMessageWrapper, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::HandleUncaughtException(jobject a0, jthrowable a1) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(2) != 0) {
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jHandleUncaughtException, a0, a1);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::HideProgressDialog() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jHideProgressDialog);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

jintArray GeckoAppShell::InitCameraWrapper(const nsAString& a0, int32_t a1, int32_t a2, int32_t a3) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[4];
    args[0].l = AndroidBridge::NewJavaString(env, a0);
    args[1].i = a1;
    args[2].i = a2;
    args[3].i = a3;

    jobject temp = env->CallStaticObjectMethodA(mGeckoAppShellClass, jInitCameraWrapper, args);
    AndroidBridge::HandleUncaughtException(env);
    jintArray ret = static_cast<jintArray>(env->PopLocalFrame(temp));
    return ret;
}

bool GeckoAppShell::IsNetworkLinkKnown() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    bool temp = env->CallStaticBooleanMethod(mGeckoAppShellClass, jIsNetworkLinkKnown);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

bool GeckoAppShell::IsNetworkLinkUp() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    bool temp = env->CallStaticBooleanMethod(mGeckoAppShellClass, jIsNetworkLinkUp);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

bool GeckoAppShell::IsTablet() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    bool temp = env->CallStaticBooleanMethod(mGeckoAppShellClass, jIsTablet);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

void GeckoAppShell::KillAnyZombies() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jKillAnyZombies);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

jclass GeckoAppShell::LoadPluginClass(const nsAString& a0, const nsAString& a1) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(3) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);
    jstring j1 = AndroidBridge::NewJavaString(env, a1);

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jLoadPluginClass, j0, j1);
    AndroidBridge::HandleUncaughtException(env);
    jclass ret = static_cast<jclass>(env->PopLocalFrame(temp));
    return ret;
}

void GeckoAppShell::LockScreenOrientation(int32_t a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jLockScreenOrientation, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::MarkURIVisited(const nsAString& a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    env->CallStaticVoidMethod(mGeckoAppShellClass, jMarkURIVisited, j0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::MoveTaskToBack() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jMoveTaskToBack);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

int32_t GeckoAppShell::NetworkLinkType() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    int32_t temp = env->CallStaticIntMethod(mGeckoAppShellClass, jNetworkLinkType);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

void GeckoAppShell::NotifyDefaultPrevented(bool a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jNotifyDefaultPrevented, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::NotifyIME(int32_t a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jNotifyIME, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::NotifyIMEChange(const nsAString& a0, int32_t a1, int32_t a2, int32_t a3) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[4];
    args[0].l = AndroidBridge::NewJavaString(env, a0);
    args[1].i = a1;
    args[2].i = a2;
    args[3].i = a3;

    env->CallStaticVoidMethodA(mGeckoAppShellClass, jNotifyIMEChange, args);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::NotifyIMEContext(int32_t a0, const nsAString& a1, const nsAString& a2, const nsAString& a3) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(3) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[4];
    args[0].i = a0;
    args[1].l = AndroidBridge::NewJavaString(env, a1);
    args[2].l = AndroidBridge::NewJavaString(env, a2);
    args[3].l = AndroidBridge::NewJavaString(env, a3);

    env->CallStaticVoidMethodA(mGeckoAppShellClass, jNotifyIMEContext, args);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::NotifyWakeLockChanged(const nsAString& a0, const nsAString& a1) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);
    jstring j1 = AndroidBridge::NewJavaString(env, a1);

    env->CallStaticVoidMethod(mGeckoAppShellClass, jNotifyWakeLockChanged, j0, j1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::NotifyXreExit() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jNotifyXreExit);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

bool GeckoAppShell::OpenUriExternal(const nsAString& a0, const nsAString& a1, const nsAString& a2, const nsAString& a3, const nsAString& a4, const nsAString& a5) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(6) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[6];
    args[0].l = AndroidBridge::NewJavaString(env, a0);
    args[1].l = AndroidBridge::NewJavaString(env, a1);
    args[2].l = AndroidBridge::NewJavaString(env, a2);
    args[3].l = AndroidBridge::NewJavaString(env, a3);
    args[4].l = AndroidBridge::NewJavaString(env, a4);
    args[5].l = AndroidBridge::NewJavaString(env, a5);

    bool temp = env->CallStaticBooleanMethodA(mGeckoAppShellClass, jOpenUriExternal, args);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

void GeckoAppShell::PerformHapticFeedback(bool a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jPerformHapticFeedback, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

bool GeckoAppShell::PumpMessageLoop() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    bool temp = env->CallStaticBooleanMethod(mGeckoAppShellClass, jPumpMessageLoop);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

void GeckoAppShell::RegisterSurfaceTextureFrameListener(jobject a0, int32_t a1) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jRegisterSurfaceTextureFrameListener, a0, a1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::RemovePluginView(jobject a0, bool a1) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jRemovePluginView, a0, a1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::RequestUiThreadCallback(int64_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jRequestUiThreadCallback, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::ScheduleRestart() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jScheduleRestart);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::SendMessageWrapper(const nsAString& a0, const nsAString& a1, int32_t a2) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[3];
    args[0].l = AndroidBridge::NewJavaString(env, a0);
    args[1].l = AndroidBridge::NewJavaString(env, a1);
    args[2].i = a2;

    env->CallStaticVoidMethodA(mGeckoAppShellClass, jSendMessageWrapper, args);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::SetFullScreen(bool a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jSetFullScreen, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::SetKeepScreenOn(bool a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jSetKeepScreenOn, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::SetURITitle(const nsAString& a0, const nsAString& a1) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);
    jstring j1 = AndroidBridge::NewJavaString(env, a1);

    env->CallStaticVoidMethod(mGeckoAppShellClass, jSetURITitle, j0, j1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::ShowAlertNotificationWrapper(const nsAString& a0, const nsAString& a1, const nsAString& a2, const nsAString& a3, const nsAString& a4) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(5) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[5];
    args[0].l = AndroidBridge::NewJavaString(env, a0);
    args[1].l = AndroidBridge::NewJavaString(env, a1);
    args[2].l = AndroidBridge::NewJavaString(env, a2);
    args[3].l = AndroidBridge::NewJavaString(env, a3);
    args[4].l = AndroidBridge::NewJavaString(env, a4);

    env->CallStaticVoidMethodA(mGeckoAppShellClass, jShowAlertNotificationWrapper, args);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::ShowInputMethodPicker() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jShowInputMethodPicker);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::StartMonitoringGamepad() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jStartMonitoringGamepad);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::StopMonitoringGamepad() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jStopMonitoringGamepad);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

bool GeckoAppShell::UnlockProfile() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    bool temp = env->CallStaticBooleanMethod(mGeckoAppShellClass, jUnlockProfile);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

void GeckoAppShell::UnlockScreenOrientation() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jUnlockScreenOrientation);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::UnregisterSurfaceTextureFrameListener(jobject a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jUnregisterSurfaceTextureFrameListener, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::Vibrate1(int64_t a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jVibrate1, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoAppShell::VibrateA(jlongArray a0, int32_t a1) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jVibrateA, a0, a1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}
jclass GeckoJavaSampler::mGeckoJavaSamplerClass = 0;
jmethodID GeckoJavaSampler::jGetFrameNameJavaProfilingWrapper = 0;
jmethodID GeckoJavaSampler::jGetSampleTimeJavaProfiling = 0;
jmethodID GeckoJavaSampler::jGetThreadNameJavaProfilingWrapper = 0;
jmethodID GeckoJavaSampler::jPauseJavaProfiling = 0;
jmethodID GeckoJavaSampler::jStartJavaProfiling = 0;
jmethodID GeckoJavaSampler::jStopJavaProfiling = 0;
jmethodID GeckoJavaSampler::jUnpauseJavaProfiling = 0;
void GeckoJavaSampler::InitStubs(JNIEnv *env) {
    mGeckoJavaSamplerClass = AndroidBridge::GetClassGlobalRef(env, "org/mozilla/gecko/GeckoJavaSampler");
    jGetFrameNameJavaProfilingWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoJavaSamplerClass, "getFrameName", "(III)Ljava/lang/String;");
    jGetSampleTimeJavaProfiling = AndroidBridge::GetStaticMethodID(env, mGeckoJavaSamplerClass, "getSampleTime", "(II)D");
    jGetThreadNameJavaProfilingWrapper = AndroidBridge::GetStaticMethodID(env, mGeckoJavaSamplerClass, "getThreadName", "(I)Ljava/lang/String;");
    jPauseJavaProfiling = AndroidBridge::GetStaticMethodID(env, mGeckoJavaSamplerClass, "pause", "()V");
    jStartJavaProfiling = AndroidBridge::GetStaticMethodID(env, mGeckoJavaSamplerClass, "start", "(II)V");
    jStopJavaProfiling = AndroidBridge::GetStaticMethodID(env, mGeckoJavaSamplerClass, "stop", "()V");
    jUnpauseJavaProfiling = AndroidBridge::GetStaticMethodID(env, mGeckoJavaSamplerClass, "unpause", "()V");
}

GeckoJavaSampler* GeckoJavaSampler::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    GeckoJavaSampler* ret = new GeckoJavaSampler(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

jstring GeckoJavaSampler::GetFrameNameJavaProfilingWrapper(int32_t a0, int32_t a1, int32_t a2) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[3];
    args[0].i = a0;
    args[1].i = a1;
    args[2].i = a2;

    jobject temp = env->CallStaticObjectMethodA(mGeckoJavaSamplerClass, jGetFrameNameJavaProfilingWrapper, args);
    AndroidBridge::HandleUncaughtException(env);
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

jdouble GeckoJavaSampler::GetSampleTimeJavaProfiling(int32_t a0, int32_t a1) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jdouble temp = env->CallStaticDoubleMethod(mGeckoJavaSamplerClass, jGetSampleTimeJavaProfiling, a0, a1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

jstring GeckoJavaSampler::GetThreadNameJavaProfilingWrapper(int32_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallStaticObjectMethod(mGeckoJavaSamplerClass, jGetThreadNameJavaProfilingWrapper, a0);
    AndroidBridge::HandleUncaughtException(env);
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

void GeckoJavaSampler::PauseJavaProfiling() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoJavaSamplerClass, jPauseJavaProfiling);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoJavaSampler::StartJavaProfiling(int32_t a0, int32_t a1) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoJavaSamplerClass, jStartJavaProfiling, a0, a1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoJavaSampler::StopJavaProfiling() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoJavaSamplerClass, jStopJavaProfiling);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoJavaSampler::UnpauseJavaProfiling() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mGeckoJavaSamplerClass, jUnpauseJavaProfiling);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}
jclass RestrictedProfiles::mRestrictedProfilesClass = 0;
jmethodID RestrictedProfiles::jGetUserRestrictions = 0;
jmethodID RestrictedProfiles::jIsAllowed = 0;
jmethodID RestrictedProfiles::jIsUserRestricted = 0;
void RestrictedProfiles::InitStubs(JNIEnv *env) {
    mRestrictedProfilesClass = AndroidBridge::GetClassGlobalRef(env, "org/mozilla/gecko/RestrictedProfiles");
    jGetUserRestrictions = AndroidBridge::GetStaticMethodID(env, mRestrictedProfilesClass, "getUserRestrictions", "()Ljava/lang/String;");
    jIsAllowed = AndroidBridge::GetStaticMethodID(env, mRestrictedProfilesClass, "isAllowed", "(ILjava/lang/String;)Z");
    jIsUserRestricted = AndroidBridge::GetStaticMethodID(env, mRestrictedProfilesClass, "isUserRestricted", "()Z");
}

RestrictedProfiles* RestrictedProfiles::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    RestrictedProfiles* ret = new RestrictedProfiles(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

jstring RestrictedProfiles::GetUserRestrictions() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallStaticObjectMethod(mRestrictedProfilesClass, jGetUserRestrictions);
    AndroidBridge::HandleUncaughtException(env);
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

bool RestrictedProfiles::IsAllowed(int32_t a0, const nsAString& a1) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j1 = AndroidBridge::NewJavaString(env, a1);

    bool temp = env->CallStaticBooleanMethod(mRestrictedProfilesClass, jIsAllowed, a0, j1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

bool RestrictedProfiles::IsUserRestricted() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    bool temp = env->CallStaticBooleanMethod(mRestrictedProfilesClass, jIsUserRestricted);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}
jclass SurfaceBits::mSurfaceBitsClass = 0;
jmethodID SurfaceBits::jSurfaceBits = 0;
jfieldID SurfaceBits::jbuffer = 0;
jfieldID SurfaceBits::jformat = 0;
jfieldID SurfaceBits::jheight = 0;
jfieldID SurfaceBits::jwidth = 0;
void SurfaceBits::InitStubs(JNIEnv *env) {
    mSurfaceBitsClass = AndroidBridge::GetClassGlobalRef(env, "org/mozilla/gecko/SurfaceBits");
    jSurfaceBits = AndroidBridge::GetMethodID(env, mSurfaceBitsClass, "<init>", "()V");
    jbuffer = AndroidBridge::GetFieldID(env, mSurfaceBitsClass, "buffer", "Ljava/nio/ByteBuffer;");
    jformat = AndroidBridge::GetFieldID(env, mSurfaceBitsClass, "format", "I");
    jheight = AndroidBridge::GetFieldID(env, mSurfaceBitsClass, "height", "I");
    jwidth = AndroidBridge::GetFieldID(env, mSurfaceBitsClass, "width", "I");
}

SurfaceBits* SurfaceBits::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    SurfaceBits* ret = new SurfaceBits(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

SurfaceBits::SurfaceBits() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    Init(env->NewObject(mSurfaceBitsClass, jSurfaceBits), env);
    env->PopLocalFrame(nullptr);
}

jobject SurfaceBits::getbuffer() {
    JNIEnv *env = GetJNIForThread();
    return static_cast<jobject>(env->GetObjectField(wrapped_obj, jbuffer));
}

void SurfaceBits::setbuffer(jobject a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetObjectField(wrapped_obj, jbuffer, a0);
}

int32_t SurfaceBits::getformat() {
    JNIEnv *env = GetJNIForThread();
    return env->GetIntField(wrapped_obj, jformat);
}

void SurfaceBits::setformat(int32_t a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetIntField(wrapped_obj, jformat, a0);
}

int32_t SurfaceBits::getheight() {
    JNIEnv *env = GetJNIForThread();
    return env->GetIntField(wrapped_obj, jheight);
}

void SurfaceBits::setheight(int32_t a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetIntField(wrapped_obj, jheight, a0);
}

int32_t SurfaceBits::getwidth() {
    JNIEnv *env = GetJNIForThread();
    return env->GetIntField(wrapped_obj, jwidth);
}

void SurfaceBits::setwidth(int32_t a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetIntField(wrapped_obj, jwidth, a0);
}
jclass ThumbnailHelper::mThumbnailHelperClass = 0;
jmethodID ThumbnailHelper::jSendThumbnail = 0;
void ThumbnailHelper::InitStubs(JNIEnv *env) {
    mThumbnailHelperClass = AndroidBridge::GetClassGlobalRef(env, "org/mozilla/gecko/ThumbnailHelper");
    jSendThumbnail = AndroidBridge::GetStaticMethodID(env, mThumbnailHelperClass, "notifyThumbnail", "(Ljava/nio/ByteBuffer;IZZ)V");
}

ThumbnailHelper* ThumbnailHelper::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    ThumbnailHelper* ret = new ThumbnailHelper(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

void ThumbnailHelper::SendThumbnail(jobject a0, int32_t a1, bool a2, bool a3) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[4];
    args[0].l = a0;
    args[1].i = a1;
    args[2].z = a2;
    args[3].z = a3;

    env->CallStaticVoidMethodA(mThumbnailHelperClass, jSendThumbnail, args);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}
jclass DisplayPortMetrics::mDisplayPortMetricsClass = 0;
jmethodID DisplayPortMetrics::jDisplayPortMetrics = 0;
jfieldID DisplayPortMetrics::jMPosition = 0;
jfieldID DisplayPortMetrics::jResolution = 0;
void DisplayPortMetrics::InitStubs(JNIEnv *env) {
    mDisplayPortMetricsClass = AndroidBridge::GetClassGlobalRef(env, "org/mozilla/gecko/gfx/DisplayPortMetrics");
    jDisplayPortMetrics = AndroidBridge::GetMethodID(env, mDisplayPortMetricsClass, "<init>", "(FFFFF)V");
    jMPosition = AndroidBridge::GetFieldID(env, mDisplayPortMetricsClass, "mPosition", "Landroid/graphics/RectF;");
    jResolution = AndroidBridge::GetFieldID(env, mDisplayPortMetricsClass, "resolution", "F");
}

DisplayPortMetrics* DisplayPortMetrics::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    DisplayPortMetrics* ret = new DisplayPortMetrics(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

DisplayPortMetrics::DisplayPortMetrics(jfloat a0, jfloat a1, jfloat a2, jfloat a3, jfloat a4) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[5];
    args[0].f = a0;
    args[1].f = a1;
    args[2].f = a2;
    args[3].f = a3;
    args[4].f = a4;

    Init(env->NewObjectA(mDisplayPortMetricsClass, jDisplayPortMetrics, args), env);
    env->PopLocalFrame(nullptr);
}

jobject DisplayPortMetrics::getMPosition() {
    JNIEnv *env = GetJNIForThread();
    return static_cast<jobject>(env->GetObjectField(wrapped_obj, jMPosition));
}

jfloat DisplayPortMetrics::getResolution() {
    JNIEnv *env = GetJNIForThread();
    return env->GetFloatField(wrapped_obj, jResolution);
}
jclass GLController::mGLControllerClass = 0;
jmethodID GLController::jCreateEGLSurfaceForCompositorWrapper = 0;
void GLController::InitStubs(JNIEnv *env) {
    mGLControllerClass = AndroidBridge::GetClassGlobalRef(env, "org/mozilla/gecko/gfx/GLController");
    jCreateEGLSurfaceForCompositorWrapper = AndroidBridge::GetMethodID(env, mGLControllerClass, "createEGLSurfaceForCompositor", "()Ljavax/microedition/khronos/egl/EGLSurface;");
}

GLController* GLController::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    GLController* ret = new GLController(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

jobject GLController::CreateEGLSurfaceForCompositorWrapper() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jCreateEGLSurfaceForCompositorWrapper);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}
jclass GeckoLayerClient::mGeckoLayerClientClass = 0;
jmethodID GeckoLayerClient::jActivateProgram = 0;
jmethodID GeckoLayerClient::jContentDocumentChanged = 0;
jmethodID GeckoLayerClient::jCreateFrame = 0;
jmethodID GeckoLayerClient::jDeactivateProgramAndRestoreState = 0;
jmethodID GeckoLayerClient::jGetDisplayPort = 0;
jmethodID GeckoLayerClient::jIsContentDocumentDisplayed = 0;
jmethodID GeckoLayerClient::jProgressiveUpdateCallback = 0;
jmethodID GeckoLayerClient::jSetFirstPaintViewport = 0;
jmethodID GeckoLayerClient::jSetPageRect = 0;
jmethodID GeckoLayerClient::jSyncFrameMetrics = 0;
jmethodID GeckoLayerClient::jSyncViewportInfo = 0;
void GeckoLayerClient::InitStubs(JNIEnv *env) {
    mGeckoLayerClientClass = AndroidBridge::GetClassGlobalRef(env, "org/mozilla/gecko/gfx/GeckoLayerClient");
    jActivateProgram = AndroidBridge::GetMethodID(env, mGeckoLayerClientClass, "activateProgram", "()V");
    jContentDocumentChanged = AndroidBridge::GetMethodID(env, mGeckoLayerClientClass, "contentDocumentChanged", "()V");
    jCreateFrame = AndroidBridge::GetMethodID(env, mGeckoLayerClientClass, "createFrame", "()Lorg/mozilla/gecko/gfx/LayerRenderer$Frame;");
    jDeactivateProgramAndRestoreState = AndroidBridge::GetMethodID(env, mGeckoLayerClientClass, "deactivateProgramAndRestoreState", "(ZIIII)V");
    jGetDisplayPort = AndroidBridge::GetMethodID(env, mGeckoLayerClientClass, "getDisplayPort", "(ZZILorg/mozilla/gecko/gfx/ImmutableViewportMetrics;)Lorg/mozilla/gecko/gfx/DisplayPortMetrics;");
    jIsContentDocumentDisplayed = AndroidBridge::GetMethodID(env, mGeckoLayerClientClass, "isContentDocumentDisplayed", "()Z");
    jProgressiveUpdateCallback = AndroidBridge::GetMethodID(env, mGeckoLayerClientClass, "progressiveUpdateCallback", "(ZFFFFFZ)Lorg/mozilla/gecko/gfx/ProgressiveUpdateData;");
    jSetFirstPaintViewport = AndroidBridge::GetMethodID(env, mGeckoLayerClientClass, "setFirstPaintViewport", "(FFFFFFF)V");
    jSetPageRect = AndroidBridge::GetMethodID(env, mGeckoLayerClientClass, "setPageRect", "(FFFF)V");
    jSyncFrameMetrics = AndroidBridge::GetMethodID(env, mGeckoLayerClientClass, "syncFrameMetrics", "(FFFFFFFZIIIIFZ)Lorg/mozilla/gecko/gfx/ViewTransform;");
    jSyncViewportInfo = AndroidBridge::GetMethodID(env, mGeckoLayerClientClass, "syncViewportInfo", "(IIIIFZ)Lorg/mozilla/gecko/gfx/ViewTransform;");
}

GeckoLayerClient* GeckoLayerClient::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    GeckoLayerClient* ret = new GeckoLayerClient(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

void GeckoLayerClient::ActivateProgram() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallVoidMethod(wrapped_obj, jActivateProgram);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoLayerClient::ContentDocumentChanged() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallVoidMethod(wrapped_obj, jContentDocumentChanged);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

jobject GeckoLayerClient::CreateFrame() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jCreateFrame);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

void GeckoLayerClient::DeactivateProgramAndRestoreState(bool a0, int32_t a1, int32_t a2, int32_t a3, int32_t a4) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[5];
    args[0].z = a0;
    args[1].i = a1;
    args[2].i = a2;
    args[3].i = a3;
    args[4].i = a4;

    env->CallVoidMethodA(wrapped_obj, jDeactivateProgramAndRestoreState, args);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

jobject GeckoLayerClient::GetDisplayPort(bool a0, bool a1, int32_t a2, jobject a3) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[4];
    args[0].z = a0;
    args[1].z = a1;
    args[2].i = a2;
    args[3].l = a3;

    jobject temp = env->CallObjectMethodA(wrapped_obj, jGetDisplayPort, args);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

bool GeckoLayerClient::IsContentDocumentDisplayed() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    bool temp = env->CallBooleanMethod(wrapped_obj, jIsContentDocumentDisplayed);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

jobject GeckoLayerClient::ProgressiveUpdateCallback(bool a0, jfloat a1, jfloat a2, jfloat a3, jfloat a4, jfloat a5, bool a6) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[7];
    args[0].z = a0;
    args[1].f = a1;
    args[2].f = a2;
    args[3].f = a3;
    args[4].f = a4;
    args[5].f = a5;
    args[6].z = a6;

    jobject temp = env->CallObjectMethodA(wrapped_obj, jProgressiveUpdateCallback, args);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

void GeckoLayerClient::SetFirstPaintViewport(jfloat a0, jfloat a1, jfloat a2, jfloat a3, jfloat a4, jfloat a5, jfloat a6) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[7];
    args[0].f = a0;
    args[1].f = a1;
    args[2].f = a2;
    args[3].f = a3;
    args[4].f = a4;
    args[5].f = a5;
    args[6].f = a6;

    env->CallVoidMethodA(wrapped_obj, jSetFirstPaintViewport, args);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void GeckoLayerClient::SetPageRect(jfloat a0, jfloat a1, jfloat a2, jfloat a3) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[4];
    args[0].f = a0;
    args[1].f = a1;
    args[2].f = a2;
    args[3].f = a3;

    env->CallVoidMethodA(wrapped_obj, jSetPageRect, args);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

jobject GeckoLayerClient::SyncFrameMetrics(jfloat a0, jfloat a1, jfloat a2, jfloat a3, jfloat a4, jfloat a5, jfloat a6, bool a7, int32_t a8, int32_t a9, int32_t a10, int32_t a11, jfloat a12, bool a13) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[14];
    args[0].f = a0;
    args[1].f = a1;
    args[2].f = a2;
    args[3].f = a3;
    args[4].f = a4;
    args[5].f = a5;
    args[6].f = a6;
    args[7].z = a7;
    args[8].i = a8;
    args[9].i = a9;
    args[10].i = a10;
    args[11].i = a11;
    args[12].f = a12;
    args[13].z = a13;

    jobject temp = env->CallObjectMethodA(wrapped_obj, jSyncFrameMetrics, args);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject GeckoLayerClient::SyncViewportInfo(int32_t a0, int32_t a1, int32_t a2, int32_t a3, jfloat a4, bool a5) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[6];
    args[0].i = a0;
    args[1].i = a1;
    args[2].i = a2;
    args[3].i = a3;
    args[4].f = a4;
    args[5].z = a5;

    jobject temp = env->CallObjectMethodA(wrapped_obj, jSyncViewportInfo, args);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}
jclass ImmutableViewportMetrics::mImmutableViewportMetricsClass = 0;
jmethodID ImmutableViewportMetrics::jImmutableViewportMetrics = 0;
void ImmutableViewportMetrics::InitStubs(JNIEnv *env) {
    mImmutableViewportMetricsClass = AndroidBridge::GetClassGlobalRef(env, "org/mozilla/gecko/gfx/ImmutableViewportMetrics");
    jImmutableViewportMetrics = AndroidBridge::GetMethodID(env, mImmutableViewportMetricsClass, "<init>", "(FFFFFFFFFFFFF)V");
}

ImmutableViewportMetrics* ImmutableViewportMetrics::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    ImmutableViewportMetrics* ret = new ImmutableViewportMetrics(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

ImmutableViewportMetrics::ImmutableViewportMetrics(jfloat a0, jfloat a1, jfloat a2, jfloat a3, jfloat a4, jfloat a5, jfloat a6, jfloat a7, jfloat a8, jfloat a9, jfloat a10, jfloat a11, jfloat a12) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[13];
    args[0].f = a0;
    args[1].f = a1;
    args[2].f = a2;
    args[3].f = a3;
    args[4].f = a4;
    args[5].f = a5;
    args[6].f = a6;
    args[7].f = a7;
    args[8].f = a8;
    args[9].f = a9;
    args[10].f = a10;
    args[11].f = a11;
    args[12].f = a12;

    Init(env->NewObjectA(mImmutableViewportMetricsClass, jImmutableViewportMetrics, args), env);
    env->PopLocalFrame(nullptr);
}
jclass LayerView::mLayerViewClass = 0;
jmethodID LayerView::jRegisterCompositorWrapper = 0;
void LayerView::InitStubs(JNIEnv *env) {
    mLayerViewClass = AndroidBridge::GetClassGlobalRef(env, "org/mozilla/gecko/gfx/LayerView");
    jRegisterCompositorWrapper = AndroidBridge::GetStaticMethodID(env, mLayerViewClass, "registerCxxCompositor", "()Lorg/mozilla/gecko/gfx/GLController;");
}

LayerView* LayerView::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    LayerView* ret = new LayerView(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

jobject LayerView::RegisterCompositorWrapper() {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallStaticObjectMethod(mLayerViewClass, jRegisterCompositorWrapper);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}
jclass NativePanZoomController::mNativePanZoomControllerClass = 0;
jmethodID NativePanZoomController::jRequestContentRepaintWrapper = 0;
void NativePanZoomController::InitStubs(JNIEnv *env) {
    mNativePanZoomControllerClass = AndroidBridge::GetClassGlobalRef(env, "org/mozilla/gecko/gfx/NativePanZoomController");
    jRequestContentRepaintWrapper = AndroidBridge::GetMethodID(env, mNativePanZoomControllerClass, "requestContentRepaint", "(FFFFF)V");
}

NativePanZoomController* NativePanZoomController::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    NativePanZoomController* ret = new NativePanZoomController(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

void NativePanZoomController::RequestContentRepaintWrapper(jfloat a0, jfloat a1, jfloat a2, jfloat a3, jfloat a4) {
    JNIEnv *env = GetJNIForThread();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[5];
    args[0].f = a0;
    args[1].f = a1;
    args[2].f = a2;
    args[3].f = a3;
    args[4].f = a4;

    env->CallVoidMethodA(wrapped_obj, jRequestContentRepaintWrapper, args);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}
jclass ProgressiveUpdateData::mProgressiveUpdateDataClass = 0;
jmethodID ProgressiveUpdateData::jProgressiveUpdateData = 0;
jmethodID ProgressiveUpdateData::jsetViewport = 0;
jfieldID ProgressiveUpdateData::jabort = 0;
jfieldID ProgressiveUpdateData::jscale = 0;
jfieldID ProgressiveUpdateData::jx = 0;
jfieldID ProgressiveUpdateData::jy = 0;
void ProgressiveUpdateData::InitStubs(JNIEnv *env) {
    mProgressiveUpdateDataClass = AndroidBridge::GetClassGlobalRef(env, "org/mozilla/gecko/gfx/ProgressiveUpdateData");
    jProgressiveUpdateData = AndroidBridge::GetMethodID(env, mProgressiveUpdateDataClass, "<init>", "()V");
    jsetViewport = AndroidBridge::GetMethodID(env, mProgressiveUpdateDataClass, "setViewport", "(Lorg/mozilla/gecko/gfx/ImmutableViewportMetrics;)V");
    jabort = AndroidBridge::GetFieldID(env, mProgressiveUpdateDataClass, "abort", "Z");
    jscale = AndroidBridge::GetFieldID(env, mProgressiveUpdateDataClass, "scale", "F");
    jx = AndroidBridge::GetFieldID(env, mProgressiveUpdateDataClass, "x", "F");
    jy = AndroidBridge::GetFieldID(env, mProgressiveUpdateDataClass, "y", "F");
}

ProgressiveUpdateData* ProgressiveUpdateData::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    ProgressiveUpdateData* ret = new ProgressiveUpdateData(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

ProgressiveUpdateData::ProgressiveUpdateData() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    Init(env->NewObject(mProgressiveUpdateDataClass, jProgressiveUpdateData), env);
    env->PopLocalFrame(nullptr);
}

void ProgressiveUpdateData::setViewport(jobject a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallVoidMethod(wrapped_obj, jsetViewport, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

bool ProgressiveUpdateData::getabort() {
    JNIEnv *env = GetJNIForThread();
    return env->GetBooleanField(wrapped_obj, jabort);
}

void ProgressiveUpdateData::setabort(bool a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetBooleanField(wrapped_obj, jabort, a0);
}

jfloat ProgressiveUpdateData::getscale() {
    JNIEnv *env = GetJNIForThread();
    return env->GetFloatField(wrapped_obj, jscale);
}

void ProgressiveUpdateData::setscale(jfloat a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetFloatField(wrapped_obj, jscale, a0);
}

jfloat ProgressiveUpdateData::getx() {
    JNIEnv *env = GetJNIForThread();
    return env->GetFloatField(wrapped_obj, jx);
}

void ProgressiveUpdateData::setx(jfloat a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetFloatField(wrapped_obj, jx, a0);
}

jfloat ProgressiveUpdateData::gety() {
    JNIEnv *env = GetJNIForThread();
    return env->GetFloatField(wrapped_obj, jy);
}

void ProgressiveUpdateData::sety(jfloat a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetFloatField(wrapped_obj, jy, a0);
}
jclass ViewTransform::mViewTransformClass = 0;
jmethodID ViewTransform::jViewTransform = 0;
jfieldID ViewTransform::jfixedLayerMarginBottom = 0;
jfieldID ViewTransform::jfixedLayerMarginLeft = 0;
jfieldID ViewTransform::jfixedLayerMarginRight = 0;
jfieldID ViewTransform::jfixedLayerMarginTop = 0;
jfieldID ViewTransform::joffsetX = 0;
jfieldID ViewTransform::joffsetY = 0;
jfieldID ViewTransform::jscale = 0;
jfieldID ViewTransform::jx = 0;
jfieldID ViewTransform::jy = 0;
void ViewTransform::InitStubs(JNIEnv *env) {
    mViewTransformClass = AndroidBridge::GetClassGlobalRef(env, "org/mozilla/gecko/gfx/ViewTransform");
    jViewTransform = AndroidBridge::GetMethodID(env, mViewTransformClass, "<init>", "(FFF)V");
    jfixedLayerMarginBottom = AndroidBridge::GetFieldID(env, mViewTransformClass, "fixedLayerMarginBottom", "F");
    jfixedLayerMarginLeft = AndroidBridge::GetFieldID(env, mViewTransformClass, "fixedLayerMarginLeft", "F");
    jfixedLayerMarginRight = AndroidBridge::GetFieldID(env, mViewTransformClass, "fixedLayerMarginRight", "F");
    jfixedLayerMarginTop = AndroidBridge::GetFieldID(env, mViewTransformClass, "fixedLayerMarginTop", "F");
    joffsetX = AndroidBridge::GetFieldID(env, mViewTransformClass, "offsetX", "F");
    joffsetY = AndroidBridge::GetFieldID(env, mViewTransformClass, "offsetY", "F");
    jscale = AndroidBridge::GetFieldID(env, mViewTransformClass, "scale", "F");
    jx = AndroidBridge::GetFieldID(env, mViewTransformClass, "x", "F");
    jy = AndroidBridge::GetFieldID(env, mViewTransformClass, "y", "F");
}

ViewTransform* ViewTransform::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    ViewTransform* ret = new ViewTransform(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

ViewTransform::ViewTransform(jfloat a0, jfloat a1, jfloat a2) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jvalue args[3];
    args[0].f = a0;
    args[1].f = a1;
    args[2].f = a2;

    Init(env->NewObjectA(mViewTransformClass, jViewTransform, args), env);
    env->PopLocalFrame(nullptr);
}

jfloat ViewTransform::getfixedLayerMarginBottom() {
    JNIEnv *env = GetJNIForThread();
    return env->GetFloatField(wrapped_obj, jfixedLayerMarginBottom);
}

void ViewTransform::setfixedLayerMarginBottom(jfloat a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetFloatField(wrapped_obj, jfixedLayerMarginBottom, a0);
}

jfloat ViewTransform::getfixedLayerMarginLeft() {
    JNIEnv *env = GetJNIForThread();
    return env->GetFloatField(wrapped_obj, jfixedLayerMarginLeft);
}

void ViewTransform::setfixedLayerMarginLeft(jfloat a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetFloatField(wrapped_obj, jfixedLayerMarginLeft, a0);
}

jfloat ViewTransform::getfixedLayerMarginRight() {
    JNIEnv *env = GetJNIForThread();
    return env->GetFloatField(wrapped_obj, jfixedLayerMarginRight);
}

void ViewTransform::setfixedLayerMarginRight(jfloat a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetFloatField(wrapped_obj, jfixedLayerMarginRight, a0);
}

jfloat ViewTransform::getfixedLayerMarginTop() {
    JNIEnv *env = GetJNIForThread();
    return env->GetFloatField(wrapped_obj, jfixedLayerMarginTop);
}

void ViewTransform::setfixedLayerMarginTop(jfloat a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetFloatField(wrapped_obj, jfixedLayerMarginTop, a0);
}

jfloat ViewTransform::getoffsetX() {
    JNIEnv *env = GetJNIForThread();
    return env->GetFloatField(wrapped_obj, joffsetX);
}

void ViewTransform::setoffsetX(jfloat a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetFloatField(wrapped_obj, joffsetX, a0);
}

jfloat ViewTransform::getoffsetY() {
    JNIEnv *env = GetJNIForThread();
    return env->GetFloatField(wrapped_obj, joffsetY);
}

void ViewTransform::setoffsetY(jfloat a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetFloatField(wrapped_obj, joffsetY, a0);
}

jfloat ViewTransform::getscale() {
    JNIEnv *env = GetJNIForThread();
    return env->GetFloatField(wrapped_obj, jscale);
}

void ViewTransform::setscale(jfloat a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetFloatField(wrapped_obj, jscale, a0);
}

jfloat ViewTransform::getx() {
    JNIEnv *env = GetJNIForThread();
    return env->GetFloatField(wrapped_obj, jx);
}

void ViewTransform::setx(jfloat a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetFloatField(wrapped_obj, jx, a0);
}

jfloat ViewTransform::gety() {
    JNIEnv *env = GetJNIForThread();
    return env->GetFloatField(wrapped_obj, jy);
}

void ViewTransform::sety(jfloat a0) {
    JNIEnv *env = GetJNIForThread();
    env->SetFloatField(wrapped_obj, jy, a0);
}
jclass NativeZip::mNativeZipClass = 0;
jmethodID NativeZip::jCreateInputStream = 0;
void NativeZip::InitStubs(JNIEnv *env) {
    mNativeZipClass = AndroidBridge::GetClassGlobalRef(env, "org/mozilla/gecko/mozglue/NativeZip");
    jCreateInputStream = AndroidBridge::GetMethodID(env, mNativeZipClass, "createInputStream", "(Ljava/nio/ByteBuffer;I)Ljava/io/InputStream;");
}

NativeZip* NativeZip::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    NativeZip* ret = new NativeZip(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

jobject NativeZip::CreateInputStream(jobject a0, int32_t a1) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(2) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallObjectMethod(wrapped_obj, jCreateInputStream, a0, a1);
    AndroidBridge::HandleUncaughtException(env);
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}
jclass MatrixBlobCursor::mMatrixBlobCursorClass = 0;
jmethodID MatrixBlobCursor::jMatrixBlobCursor = 0;
jmethodID MatrixBlobCursor::jMatrixBlobCursor0 = 0;
jmethodID MatrixBlobCursor::jAddRow = 0;
jmethodID MatrixBlobCursor::jAddRow1 = 0;
jmethodID MatrixBlobCursor::jAddRow2 = 0;
void MatrixBlobCursor::InitStubs(JNIEnv *env) {
    mMatrixBlobCursorClass = AndroidBridge::GetClassGlobalRef(env, "org/mozilla/gecko/sqlite/MatrixBlobCursor");
    jMatrixBlobCursor = AndroidBridge::GetMethodID(env, mMatrixBlobCursorClass, "<init>", "([Ljava/lang/String;)V");
    jMatrixBlobCursor0 = AndroidBridge::GetMethodID(env, mMatrixBlobCursorClass, "<init>", "([Ljava/lang/String;I)V");
    jAddRow = AndroidBridge::GetMethodID(env, mMatrixBlobCursorClass, "addRow", "(Ljava/lang/Iterable;)V");
    jAddRow1 = AndroidBridge::GetMethodID(env, mMatrixBlobCursorClass, "addRow", "(Ljava/util/ArrayList;I)V");
    jAddRow2 = AndroidBridge::GetMethodID(env, mMatrixBlobCursorClass, "addRow", "([Ljava/lang/Object;)V");
}

MatrixBlobCursor* MatrixBlobCursor::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    MatrixBlobCursor* ret = new MatrixBlobCursor(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

MatrixBlobCursor::MatrixBlobCursor(jobjectArray a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    Init(env->NewObject(mMatrixBlobCursorClass, jMatrixBlobCursor, a0), env);
    env->PopLocalFrame(nullptr);
}

MatrixBlobCursor::MatrixBlobCursor(jobjectArray a0, int32_t a1) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    Init(env->NewObject(mMatrixBlobCursorClass, jMatrixBlobCursor0, a0, a1), env);
    env->PopLocalFrame(nullptr);
}

void MatrixBlobCursor::AddRow(jobject a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallVoidMethod(wrapped_obj, jAddRow, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void MatrixBlobCursor::AddRow(jobject a0, int32_t a1) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallVoidMethod(wrapped_obj, jAddRow1, a0, a1);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void MatrixBlobCursor::AddRow(jobjectArray a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallVoidMethod(wrapped_obj, jAddRow2, a0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}
jclass SQLiteBridgeException::mSQLiteBridgeExceptionClass = 0;
jmethodID SQLiteBridgeException::jSQLiteBridgeException = 0;
jmethodID SQLiteBridgeException::jSQLiteBridgeException0 = 0;
jfieldID SQLiteBridgeException::jserialVersionUID = 0;
void SQLiteBridgeException::InitStubs(JNIEnv *env) {
    mSQLiteBridgeExceptionClass = AndroidBridge::GetClassGlobalRef(env, "org/mozilla/gecko/sqlite/SQLiteBridgeException");
    jSQLiteBridgeException = AndroidBridge::GetMethodID(env, mSQLiteBridgeExceptionClass, "<init>", "()V");
    jSQLiteBridgeException0 = AndroidBridge::GetMethodID(env, mSQLiteBridgeExceptionClass, "<init>", "(Ljava/lang/String;)V");
    jserialVersionUID = AndroidBridge::GetStaticFieldID(env, mSQLiteBridgeExceptionClass, "serialVersionUID", "J");
}

SQLiteBridgeException* SQLiteBridgeException::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    SQLiteBridgeException* ret = new SQLiteBridgeException(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

SQLiteBridgeException::SQLiteBridgeException() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    Init(env->NewObject(mSQLiteBridgeExceptionClass, jSQLiteBridgeException), env);
    env->PopLocalFrame(nullptr);
}

SQLiteBridgeException::SQLiteBridgeException(const nsAString& a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    Init(env->NewObject(mSQLiteBridgeExceptionClass, jSQLiteBridgeException0, j0), env);
    env->PopLocalFrame(nullptr);
}

int64_t SQLiteBridgeException::getserialVersionUID() {
    JNIEnv *env = GetJNIForThread();
    return env->GetStaticLongField(mSQLiteBridgeExceptionClass, jserialVersionUID);
}
jclass Clipboard::mClipboardClass = 0;
jmethodID Clipboard::jClearText = 0;
jmethodID Clipboard::jGetClipboardTextWrapper = 0;
jmethodID Clipboard::jHasText = 0;
jmethodID Clipboard::jSetClipboardText = 0;
void Clipboard::InitStubs(JNIEnv *env) {
    mClipboardClass = AndroidBridge::GetClassGlobalRef(env, "org/mozilla/gecko/util/Clipboard");
    jClearText = AndroidBridge::GetStaticMethodID(env, mClipboardClass, "clearText", "()V");
    jGetClipboardTextWrapper = AndroidBridge::GetStaticMethodID(env, mClipboardClass, "getText", "()Ljava/lang/String;");
    jHasText = AndroidBridge::GetStaticMethodID(env, mClipboardClass, "hasText", "()Z");
    jSetClipboardText = AndroidBridge::GetStaticMethodID(env, mClipboardClass, "setText", "(Ljava/lang/CharSequence;)V");
}

Clipboard* Clipboard::Wrap(jobject obj) {
    JNIEnv *env = GetJNIForThread();
    Clipboard* ret = new Clipboard(obj, env);
    env->DeleteLocalRef(obj);
    return ret;
}

void Clipboard::ClearText() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    env->CallStaticVoidMethod(mClipboardClass, jClearText);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

jstring Clipboard::GetClipboardTextWrapper() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jobject temp = env->CallStaticObjectMethod(mClipboardClass, jGetClipboardTextWrapper);
    AndroidBridge::HandleUncaughtException(env);
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

bool Clipboard::HasText() {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(0) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    bool temp = env->CallStaticBooleanMethod(mClipboardClass, jHasText);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
    return temp;
}

void Clipboard::SetClipboardText(const nsAString& a0) {
    JNIEnv *env = AndroidBridge::GetJNIEnv();
    if (env->PushLocalFrame(1) != 0) {
        AndroidBridge::HandleUncaughtException(env);
        MOZ_CRASH("Exception should have caused crash.");
    }

    jstring j0 = AndroidBridge::NewJavaString(env, a0);

    env->CallStaticVoidMethod(mClipboardClass, jSetClipboardText, j0);
    AndroidBridge::HandleUncaughtException(env);
    env->PopLocalFrame(nullptr);
}

void InitStubs(JNIEnv *env) {
    DownloadsIntegration::InitStubs(env);
    GeckoAppShell::InitStubs(env);
    GeckoJavaSampler::InitStubs(env);
    RestrictedProfiles::InitStubs(env);
    SurfaceBits::InitStubs(env);
    ThumbnailHelper::InitStubs(env);
    DisplayPortMetrics::InitStubs(env);
    GLController::InitStubs(env);
    GeckoLayerClient::InitStubs(env);
    ImmutableViewportMetrics::InitStubs(env);
    LayerView::InitStubs(env);
    NativePanZoomController::InitStubs(env);
    ProgressiveUpdateData::InitStubs(env);
    ViewTransform::InitStubs(env);
    NativeZip::InitStubs(env);
    MatrixBlobCursor::InitStubs(env);
    SQLiteBridgeException::InitStubs(env);
    Clipboard::InitStubs(env);
}
} 
} 
} 
