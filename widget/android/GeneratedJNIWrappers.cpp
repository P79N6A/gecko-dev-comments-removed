




#include "nsXPCOMStrings.h"
#include "AndroidBridge.h"
#include "AndroidBridgeUtilities.h"

#ifdef DEBUG
#define ALOG_BRIDGE(args...) ALOG(args)
#else
#define ALOG_BRIDGE(args...) ((void)0)
#endif

using namespace mozilla;
void AndroidBridge::InitStubs(JNIEnv *jEnv) {
    initInit();

    mGeckoAppShellClass = getClassGlobalRef("org/mozilla/gecko/GeckoAppShell");
    jAcknowledgeEvent = getStaticMethod("acknowledgeEvent", "()V");
    jAddPluginViewWrapper = getStaticMethod("addPluginView", "(Landroid/view/View;FFFFZ)V");
    jAlertsProgressListener_OnProgress = getStaticMethod("alertsProgressListener_OnProgress", "(Ljava/lang/String;JJLjava/lang/String;)V");
    jCancelVibrate = getStaticMethod("cancelVibrate", "()V");
    jCheckURIVisited = getStaticMethod("checkUriVisited", "(Ljava/lang/String;)V");
    jClearMessageList = getStaticMethod("clearMessageList", "(I)V");
    jCloseCamera = getStaticMethod("closeCamera", "()V");
    jCloseNotification = getStaticMethod("closeNotification", "(Ljava/lang/String;)V");
    jCreateMessageListWrapper = getStaticMethod("createMessageList", "(JJ[Ljava/lang/String;IIZI)V");
    jCreateShortcut = getStaticMethod("createShortcut", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    jDeleteMessageWrapper = getStaticMethod("deleteMessage", "(II)V");
    jDisableBatteryNotifications = getStaticMethod("disableBatteryNotifications", "()V");
    jDisableNetworkNotifications = getStaticMethod("disableNetworkNotifications", "()V");
    jDisableScreenOrientationNotifications = getStaticMethod("disableScreenOrientationNotifications", "()V");
    jDisableSensor = getStaticMethod("disableSensor", "(I)V");
    jEnableBatteryNotifications = getStaticMethod("enableBatteryNotifications", "()V");
    jEnableLocation = getStaticMethod("enableLocation", "(Z)V");
    jEnableLocationHighAccuracy = getStaticMethod("enableLocationHighAccuracy", "(Z)V");
    jEnableNetworkNotifications = getStaticMethod("enableNetworkNotifications", "()V");
    jEnableScreenOrientationNotifications = getStaticMethod("enableScreenOrientationNotifications", "()V");
    jEnableSensor = getStaticMethod("enableSensor", "(I)V");
    jGetContext = getStaticMethod("getContext", "()Landroid/content/Context;");
    jGetCurrentBatteryInformationWrapper = getStaticMethod("getCurrentBatteryInformation", "()[D");
    jGetCurrentNetworkInformationWrapper = getStaticMethod("getCurrentNetworkInformation", "()[D");
    jGetDensity = getStaticMethod("getDensity", "()F");
    jGetDpiWrapper = getStaticMethod("getDpi", "()I");
    jGetExtensionFromMimeTypeWrapper = getStaticMethod("getExtensionFromMimeType", "(Ljava/lang/String;)Ljava/lang/String;");
    jGetHandlersForMimeTypeWrapper = getStaticMethod("getHandlersForMimeType", "(Ljava/lang/String;Ljava/lang/String;)[Ljava/lang/String;");
    jGetHandlersForURLWrapper = getStaticMethod("getHandlersForURL", "(Ljava/lang/String;Ljava/lang/String;)[Ljava/lang/String;");
    jGetIconForExtensionWrapper = getStaticMethod("getIconForExtension", "(Ljava/lang/String;I)[B");
    jGetMessageWrapper = getStaticMethod("getMessage", "(II)V");
    jGetMimeTypeFromExtensionsWrapper = getStaticMethod("getMimeTypeFromExtensions", "(Ljava/lang/String;)Ljava/lang/String;");
    jGetNextMessageInListWrapper = getStaticMethod("getNextMessageInList", "(II)V");
    jGetProxyForURIWrapper = getStaticMethod("getProxyForURI", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)Ljava/lang/String;");
    jGetScreenDepthWrapper = getStaticMethod("getScreenDepth", "()I");
    jGetScreenOrientationWrapper = getStaticMethod("getScreenOrientation", "()S");
    jGetShowPasswordSetting = getStaticMethod("getShowPasswordSetting", "()Z");
    jGetSystemColoursWrapper = getStaticMethod("getSystemColors", "()[I");
    jHandleGeckoMessageWrapper = getStaticMethod("handleGeckoMessage", "(Ljava/lang/String;)Ljava/lang/String;");
    jHideProgressDialog = getStaticMethod("hideProgressDialog", "()V");
    jInitCameraWrapper = getStaticMethod("initCamera", "(Ljava/lang/String;III)[I");
    jIsNetworkLinkKnown = getStaticMethod("isNetworkLinkKnown", "()Z");
    jIsNetworkLinkUp = getStaticMethod("isNetworkLinkUp", "()Z");
    jIsTablet = getStaticMethod("isTablet", "()Z");
    jKillAnyZombies = getStaticMethod("killAnyZombies", "()V");
    jLockScreenOrientation = getStaticMethod("lockScreenOrientation", "(I)V");
    jMarkURIVisited = getStaticMethod("markUriVisited", "(Ljava/lang/String;)V");
    jMoveTaskToBack = getStaticMethod("moveTaskToBack", "()V");
    jNetworkLinkType = getStaticMethod("networkLinkType", "()I");
    jNotifyDefaultPrevented = getStaticMethod("notifyDefaultPrevented", "(Z)V");
    jNotifyIME = getStaticMethod("notifyIME", "(I)V");
    jNotifyIMEChange = getStaticMethod("notifyIMEChange", "(Ljava/lang/String;III)V");
    jNotifyIMEContext = getStaticMethod("notifyIMEContext", "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    jNotifyWakeLockChanged = getStaticMethod("notifyWakeLockChanged", "(Ljava/lang/String;Ljava/lang/String;)V");
    jNotifyXreExit = getStaticMethod("onXreExit", "()V");
    jOpenUriExternal = getStaticMethod("openUriExternal", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z");
    jPerformHapticFeedback = getStaticMethod("performHapticFeedback", "(Z)V");
    jPumpMessageLoop = getStaticMethod("pumpMessageLoop", "()Z");
    jRegisterSurfaceTextureFrameListener = getStaticMethod("registerSurfaceTextureFrameListener", "(Ljava/lang/Object;I)V");
    jRemovePluginView = getStaticMethod("removePluginView", "(Landroid/view/View;Z)V");
    jScanMedia = getStaticMethod("scanMedia", "(Ljava/lang/String;Ljava/lang/String;)V");
    jScheduleRestart = getStaticMethod("scheduleRestart", "()V");
    jSendMessageWrapper = getStaticMethod("sendMessage", "(Ljava/lang/String;Ljava/lang/String;I)V");
    jSetFullScreen = getStaticMethod("setFullScreen", "(Z)V");
    jSetKeepScreenOn = getStaticMethod("setKeepScreenOn", "(Z)V");
    jSetSelectedLocale = getStaticMethod("setSelectedLocale", "(Ljava/lang/String;)V");
    jSetURITitle = getStaticMethod("setUriTitle", "(Ljava/lang/String;Ljava/lang/String;)V");
    jShowAlertNotificationWrapper = getStaticMethod("showAlertNotification", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
    jShowFilePickerAsyncWrapper = getStaticMethod("showFilePickerAsync", "(Ljava/lang/String;J)V");
    jShowFilePickerForExtensionsWrapper = getStaticMethod("showFilePickerForExtensions", "(Ljava/lang/String;)Ljava/lang/String;");
    jShowFilePickerForMimeTypeWrapper = getStaticMethod("showFilePickerForMimeType", "(Ljava/lang/String;)Ljava/lang/String;");
    jShowInputMethodPicker = getStaticMethod("showInputMethodPicker", "()V");
    jUnlockProfile = getStaticMethod("unlockProfile", "()Z");
    jUnlockScreenOrientation = getStaticMethod("unlockScreenOrientation", "()V");
    jUnregisterSurfaceTextureFrameListener = getStaticMethod("unregisterSurfaceTextureFrameListener", "(Ljava/lang/Object;)V");
    jVibrate1 = getStaticMethod("vibrate", "(J)V");
    jVibrateA = getStaticMethod("vibrate", "([JI)V");

    mGeckoJavaSamplerClass = getClassGlobalRef("org/mozilla/gecko/GeckoJavaSampler");
    jGetFrameNameJavaProfilingWrapper = getStaticMethod("getFrameName", "(III)Ljava/lang/String;");
    jGetSampleTimeJavaProfiling = getStaticMethod("getSampleTime", "(II)D");
    jGetThreadNameJavaProfilingWrapper = getStaticMethod("getThreadName", "(I)Ljava/lang/String;");
    jPauseJavaProfiling = getStaticMethod("pause", "()V");
    jStartJavaProfiling = getStaticMethod("start", "(II)V");
    jStopJavaProfiling = getStaticMethod("stop", "()V");
    jUnpauseJavaProfiling = getStaticMethod("unpause", "()V");

    mThumbnailHelperClass = getClassGlobalRef("org/mozilla/gecko/ThumbnailHelper");
    jSendThumbnail = getStaticMethod("notifyThumbnail", "(Ljava/nio/ByteBuffer;IZ)V");

    mGLControllerClass = getClassGlobalRef("org/mozilla/gecko/gfx/GLController");
    jCreateEGLSurfaceForCompositorWrapper = getMethod("createEGLSurfaceForCompositor", "()Ljavax/microedition/khronos/egl/EGLSurface;");

    mLayerViewClass = getClassGlobalRef("org/mozilla/gecko/gfx/LayerView");
    jRegisterCompositorWrapper = getStaticMethod("registerCxxCompositor", "()Lorg/mozilla/gecko/gfx/GLController;");

    mNativePanZoomControllerClass = getClassGlobalRef("org/mozilla/gecko/gfx/NativePanZoomController");
    jPostDelayedCallbackWrapper = getMethod("postDelayedCallback", "(J)V");
    jRequestContentRepaintWrapper = getMethod("requestContentRepaint", "(FFFFF)V");

    mClipboardClass = getClassGlobalRef("org/mozilla/gecko/util/Clipboard");
    jGetClipboardTextWrapper = getStaticMethod("getText", "()Ljava/lang/String;");
    jSetClipboardText = getStaticMethod("setText", "(Ljava/lang/CharSequence;)V");
}

void AndroidBridge::AcknowledgeEvent() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jAcknowledgeEvent);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::AddPluginViewWrapper(jobject a0, jfloat a1, jfloat a2, jfloat a3, jfloat a4, bool a5) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jvalue args[6];
    args[0].l = a0;
    args[1].f = a1;
    args[2].f = a2;
    args[3].f = a3;
    args[4].f = a4;
    args[5].z = a5;

    env->CallStaticVoidMethodA(mGeckoAppShellClass, jAddPluginViewWrapper, args);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::AlertsProgressListener_OnProgress(const nsAString& a0, int64_t a1, int64_t a2, const nsAString& a3) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(2) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jvalue args[4];
    args[0].l = NewJavaString(env, a0);
    args[1].j = a1;
    args[2].j = a2;
    args[3].l = NewJavaString(env, a3);

    env->CallStaticVoidMethodA(mGeckoAppShellClass, jAlertsProgressListener_OnProgress, args);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::CancelVibrate() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jCancelVibrate);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::CheckURIVisited(const nsAString& a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jstring j0 = NewJavaString(env, a0);

    env->CallStaticVoidMethod(mGeckoAppShellClass, jCheckURIVisited, j0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::ClearMessageList(int32_t a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jClearMessageList, a0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::CloseCamera() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jCloseCamera);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::CloseNotification(const nsAString& a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jstring j0 = NewJavaString(env, a0);

    env->CallStaticVoidMethod(mGeckoAppShellClass, jCloseNotification, j0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::CreateMessageListWrapper(int64_t a0, int64_t a1, jobjectArray a2, int32_t a3, int32_t a4, bool a5, int32_t a6) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jvalue args[7];
    args[0].j = a0;
    args[1].j = a1;
    args[2].l = a2;
    args[3].i = a3;
    args[4].i = a4;
    args[5].z = a5;
    args[6].i = a6;

    env->CallStaticVoidMethodA(mGeckoAppShellClass, jCreateMessageListWrapper, args);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::CreateShortcut(const nsAString& a0, const nsAString& a1, const nsAString& a2, const nsAString& a3) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(4) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jvalue args[4];
    args[0].l = NewJavaString(env, a0);
    args[1].l = NewJavaString(env, a1);
    args[2].l = NewJavaString(env, a2);
    args[3].l = NewJavaString(env, a3);

    env->CallStaticVoidMethodA(mGeckoAppShellClass, jCreateShortcut, args);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::DeleteMessageWrapper(int32_t a0, int32_t a1) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jDeleteMessageWrapper, a0, a1);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::DisableBatteryNotifications() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jDisableBatteryNotifications);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::DisableNetworkNotifications() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jDisableNetworkNotifications);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::DisableScreenOrientationNotifications() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jDisableScreenOrientationNotifications);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::DisableSensor(int32_t a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jDisableSensor, a0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::EnableBatteryNotifications() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jEnableBatteryNotifications);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::EnableLocation(bool a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jEnableLocation, a0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::EnableLocationHighAccuracy(bool a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jEnableLocationHighAccuracy, a0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::EnableNetworkNotifications() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jEnableNetworkNotifications);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::EnableScreenOrientationNotifications() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jEnableScreenOrientationNotifications);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::EnableSensor(int32_t a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jEnableSensor, a0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

jobject AndroidBridge::GetContext() {
    JNIEnv *env = GetJNIForThread();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return nullptr;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jGetContext);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return nullptr;
    }
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jdoubleArray AndroidBridge::GetCurrentBatteryInformationWrapper() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return nullptr;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jGetCurrentBatteryInformationWrapper);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return nullptr;
    }
    jdoubleArray ret = static_cast<jdoubleArray>(env->PopLocalFrame(temp));
    return ret;
}

jdoubleArray AndroidBridge::GetCurrentNetworkInformationWrapper() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return nullptr;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jGetCurrentNetworkInformationWrapper);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return nullptr;
    }
    jdoubleArray ret = static_cast<jdoubleArray>(env->PopLocalFrame(temp));
    return ret;
}

jfloat AndroidBridge::GetDensity() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return 0.0;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return 0.0;
    }

    jfloat temp = env->CallStaticFloatMethod(mGeckoAppShellClass, jGetDensity);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return 0.0;
    }
    env->PopLocalFrame(NULL);
    return temp;
}

int32_t AndroidBridge::GetDpiWrapper() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return 0;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return 0;
    }

    int32_t temp = env->CallStaticIntMethod(mGeckoAppShellClass, jGetDpiWrapper);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return 0;
    }
    env->PopLocalFrame(NULL);
    return temp;
}

jstring AndroidBridge::GetExtensionFromMimeTypeWrapper(const nsAString& a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return nullptr;
    }

    if (env->PushLocalFrame(2) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    jstring j0 = NewJavaString(env, a0);

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jGetExtensionFromMimeTypeWrapper, j0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return nullptr;
    }
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

jobjectArray AndroidBridge::GetHandlersForMimeTypeWrapper(const nsAString& a0, const nsAString& a1) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return nullptr;
    }

    if (env->PushLocalFrame(3) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    jstring j0 = NewJavaString(env, a0);
    jstring j1 = NewJavaString(env, a1);

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jGetHandlersForMimeTypeWrapper, j0, j1);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return nullptr;
    }
    jobjectArray ret = static_cast<jobjectArray>(env->PopLocalFrame(temp));
    return ret;
}

jobjectArray AndroidBridge::GetHandlersForURLWrapper(const nsAString& a0, const nsAString& a1) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return nullptr;
    }

    if (env->PushLocalFrame(3) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    jstring j0 = NewJavaString(env, a0);
    jstring j1 = NewJavaString(env, a1);

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jGetHandlersForURLWrapper, j0, j1);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return nullptr;
    }
    jobjectArray ret = static_cast<jobjectArray>(env->PopLocalFrame(temp));
    return ret;
}

jbyteArray AndroidBridge::GetIconForExtensionWrapper(const nsAString& a0, int32_t a1) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return nullptr;
    }

    if (env->PushLocalFrame(2) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    jstring j0 = NewJavaString(env, a0);

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jGetIconForExtensionWrapper, j0, a1);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return nullptr;
    }
    jbyteArray ret = static_cast<jbyteArray>(env->PopLocalFrame(temp));
    return ret;
}

void AndroidBridge::GetMessageWrapper(int32_t a0, int32_t a1) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jGetMessageWrapper, a0, a1);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

jstring AndroidBridge::GetMimeTypeFromExtensionsWrapper(const nsAString& a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return nullptr;
    }

    if (env->PushLocalFrame(2) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    jstring j0 = NewJavaString(env, a0);

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jGetMimeTypeFromExtensionsWrapper, j0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return nullptr;
    }
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

void AndroidBridge::GetNextMessageInListWrapper(int32_t a0, int32_t a1) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jGetNextMessageInListWrapper, a0, a1);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

jstring AndroidBridge::GetProxyForURIWrapper(const nsAString& a0, const nsAString& a1, const nsAString& a2, int32_t a3) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return nullptr;
    }

    if (env->PushLocalFrame(4) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    jvalue args[4];
    args[0].l = NewJavaString(env, a0);
    args[1].l = NewJavaString(env, a1);
    args[2].l = NewJavaString(env, a2);
    args[3].i = a3;

    jobject temp = env->CallStaticObjectMethodA(mGeckoAppShellClass, jGetProxyForURIWrapper, args);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return nullptr;
    }
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

int32_t AndroidBridge::GetScreenDepthWrapper() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return 0;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return 0;
    }

    int32_t temp = env->CallStaticIntMethod(mGeckoAppShellClass, jGetScreenDepthWrapper);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return 0;
    }
    env->PopLocalFrame(NULL);
    return temp;
}

int16_t AndroidBridge::GetScreenOrientationWrapper() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return 0;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return 0;
    }

    int16_t temp = env->CallStaticShortMethod(mGeckoAppShellClass, jGetScreenOrientationWrapper);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return 0;
    }
    env->PopLocalFrame(NULL);
    return temp;
}

bool AndroidBridge::GetShowPasswordSetting() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return false;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return false;
    }

    bool temp = env->CallStaticBooleanMethod(mGeckoAppShellClass, jGetShowPasswordSetting);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return false;
    }
    env->PopLocalFrame(NULL);
    return temp;
}

jintArray AndroidBridge::GetSystemColoursWrapper() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return nullptr;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jGetSystemColoursWrapper);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return nullptr;
    }
    jintArray ret = static_cast<jintArray>(env->PopLocalFrame(temp));
    return ret;
}

jstring AndroidBridge::HandleGeckoMessageWrapper(const nsAString& a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return nullptr;
    }

    if (env->PushLocalFrame(2) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    jstring j0 = NewJavaString(env, a0);

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jHandleGeckoMessageWrapper, j0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return nullptr;
    }
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

void AndroidBridge::HideProgressDialog() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jHideProgressDialog);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

jintArray AndroidBridge::InitCameraWrapper(const nsAString& a0, int32_t a1, int32_t a2, int32_t a3) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return nullptr;
    }

    if (env->PushLocalFrame(2) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    jvalue args[4];
    args[0].l = NewJavaString(env, a0);
    args[1].i = a1;
    args[2].i = a2;
    args[3].i = a3;

    jobject temp = env->CallStaticObjectMethodA(mGeckoAppShellClass, jInitCameraWrapper, args);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return nullptr;
    }
    jintArray ret = static_cast<jintArray>(env->PopLocalFrame(temp));
    return ret;
}

bool AndroidBridge::IsNetworkLinkKnown() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return false;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return false;
    }

    bool temp = env->CallStaticBooleanMethod(mGeckoAppShellClass, jIsNetworkLinkKnown);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return false;
    }
    env->PopLocalFrame(NULL);
    return temp;
}

bool AndroidBridge::IsNetworkLinkUp() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return false;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return false;
    }

    bool temp = env->CallStaticBooleanMethod(mGeckoAppShellClass, jIsNetworkLinkUp);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return false;
    }
    env->PopLocalFrame(NULL);
    return temp;
}

bool AndroidBridge::IsTablet() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return false;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return false;
    }

    bool temp = env->CallStaticBooleanMethod(mGeckoAppShellClass, jIsTablet);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return false;
    }
    env->PopLocalFrame(NULL);
    return temp;
}

void AndroidBridge::KillAnyZombies() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jKillAnyZombies);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::LockScreenOrientation(int32_t a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jLockScreenOrientation, a0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::MarkURIVisited(const nsAString& a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jstring j0 = NewJavaString(env, a0);

    env->CallStaticVoidMethod(mGeckoAppShellClass, jMarkURIVisited, j0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::MoveTaskToBack() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jMoveTaskToBack);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

int32_t AndroidBridge::NetworkLinkType() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return 0;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return 0;
    }

    int32_t temp = env->CallStaticIntMethod(mGeckoAppShellClass, jNetworkLinkType);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return 0;
    }
    env->PopLocalFrame(NULL);
    return temp;
}

void AndroidBridge::NotifyDefaultPrevented(bool a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jNotifyDefaultPrevented, a0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::NotifyIME(int32_t a0) {
    if (!sBridge) {
        ALOG_BRIDGE("Aborted: No sBridge - %s", __PRETTY_FUNCTION__);
        return;
    }

    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(sBridge->mGeckoAppShellClass, sBridge->jNotifyIME, a0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::NotifyIMEChange(const nsAString& a0, int32_t a1, int32_t a2, int32_t a3) {
    if (!sBridge) {
        ALOG_BRIDGE("Aborted: No sBridge - %s", __PRETTY_FUNCTION__);
        return;
    }

    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jvalue args[4];
    args[0].l = NewJavaString(env, a0);
    args[1].i = a1;
    args[2].i = a2;
    args[3].i = a3;

    env->CallStaticVoidMethodA(sBridge->mGeckoAppShellClass, sBridge->jNotifyIMEChange, args);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::NotifyIMEContext(int32_t a0, const nsAString& a1, const nsAString& a2, const nsAString& a3) {
    if (!sBridge) {
        ALOG_BRIDGE("Aborted: No sBridge - %s", __PRETTY_FUNCTION__);
        return;
    }

    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(3) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jvalue args[4];
    args[0].i = a0;
    args[1].l = NewJavaString(env, a1);
    args[2].l = NewJavaString(env, a2);
    args[3].l = NewJavaString(env, a3);

    env->CallStaticVoidMethodA(sBridge->mGeckoAppShellClass, sBridge->jNotifyIMEContext, args);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::NotifyWakeLockChanged(const nsAString& a0, const nsAString& a1) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(2) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jstring j0 = NewJavaString(env, a0);
    jstring j1 = NewJavaString(env, a1);

    env->CallStaticVoidMethod(mGeckoAppShellClass, jNotifyWakeLockChanged, j0, j1);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::NotifyXreExit() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jNotifyXreExit);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

bool AndroidBridge::OpenUriExternal(const nsAString& a0, const nsAString& a1, const nsAString& a2, const nsAString& a3, const nsAString& a4, const nsAString& a5) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return false;
    }

    if (env->PushLocalFrame(6) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return false;
    }

    jvalue args[6];
    args[0].l = NewJavaString(env, a0);
    args[1].l = NewJavaString(env, a1);
    args[2].l = NewJavaString(env, a2);
    args[3].l = NewJavaString(env, a3);
    args[4].l = NewJavaString(env, a4);
    args[5].l = NewJavaString(env, a5);

    bool temp = env->CallStaticBooleanMethodA(mGeckoAppShellClass, jOpenUriExternal, args);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return false;
    }
    env->PopLocalFrame(NULL);
    return temp;
}

void AndroidBridge::PerformHapticFeedback(bool a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jPerformHapticFeedback, a0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

bool AndroidBridge::PumpMessageLoop() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return false;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return false;
    }

    bool temp = env->CallStaticBooleanMethod(mGeckoAppShellClass, jPumpMessageLoop);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return false;
    }
    env->PopLocalFrame(NULL);
    return temp;
}

void AndroidBridge::RegisterSurfaceTextureFrameListener(jobject a0, int32_t a1) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jRegisterSurfaceTextureFrameListener, a0, a1);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::RemovePluginView(jobject a0, bool a1) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jRemovePluginView, a0, a1);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::ScanMedia(const nsAString& a0, const nsAString& a1) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(2) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jstring j0 = NewJavaString(env, a0);
    jstring j1 = NewJavaString(env, a1);

    env->CallStaticVoidMethod(mGeckoAppShellClass, jScanMedia, j0, j1);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::ScheduleRestart() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jScheduleRestart);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::SendMessageWrapper(const nsAString& a0, const nsAString& a1, int32_t a2) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(2) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jvalue args[3];
    args[0].l = NewJavaString(env, a0);
    args[1].l = NewJavaString(env, a1);
    args[2].i = a2;

    env->CallStaticVoidMethodA(mGeckoAppShellClass, jSendMessageWrapper, args);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::SetFullScreen(bool a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jSetFullScreen, a0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::SetKeepScreenOn(bool a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jSetKeepScreenOn, a0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::SetSelectedLocale(const nsAString& a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jstring j0 = NewJavaString(env, a0);

    env->CallStaticVoidMethod(mGeckoAppShellClass, jSetSelectedLocale, j0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::SetURITitle(const nsAString& a0, const nsAString& a1) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(2) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jstring j0 = NewJavaString(env, a0);
    jstring j1 = NewJavaString(env, a1);

    env->CallStaticVoidMethod(mGeckoAppShellClass, jSetURITitle, j0, j1);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::ShowAlertNotificationWrapper(const nsAString& a0, const nsAString& a1, const nsAString& a2, const nsAString& a3, const nsAString& a4) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(5) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jvalue args[5];
    args[0].l = NewJavaString(env, a0);
    args[1].l = NewJavaString(env, a1);
    args[2].l = NewJavaString(env, a2);
    args[3].l = NewJavaString(env, a3);
    args[4].l = NewJavaString(env, a4);

    env->CallStaticVoidMethodA(mGeckoAppShellClass, jShowAlertNotificationWrapper, args);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::ShowFilePickerAsyncWrapper(const nsAString& a0, int64_t a1) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jstring j0 = NewJavaString(env, a0);

    env->CallStaticVoidMethod(mGeckoAppShellClass, jShowFilePickerAsyncWrapper, j0, a1);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

jstring AndroidBridge::ShowFilePickerForExtensionsWrapper(const nsAString& a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return nullptr;
    }

    if (env->PushLocalFrame(2) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    jstring j0 = NewJavaString(env, a0);

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jShowFilePickerForExtensionsWrapper, j0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return nullptr;
    }
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

jstring AndroidBridge::ShowFilePickerForMimeTypeWrapper(const nsAString& a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return nullptr;
    }

    if (env->PushLocalFrame(2) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    jstring j0 = NewJavaString(env, a0);

    jobject temp = env->CallStaticObjectMethod(mGeckoAppShellClass, jShowFilePickerForMimeTypeWrapper, j0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return nullptr;
    }
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

void AndroidBridge::ShowInputMethodPicker() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jShowInputMethodPicker);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

bool AndroidBridge::UnlockProfile() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return false;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return false;
    }

    bool temp = env->CallStaticBooleanMethod(mGeckoAppShellClass, jUnlockProfile);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return false;
    }
    env->PopLocalFrame(NULL);
    return temp;
}

void AndroidBridge::UnlockScreenOrientation() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jUnlockScreenOrientation);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::UnregisterSurfaceTextureFrameListener(jobject a0) {
    JNIEnv *env = GetJNIForThread();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jUnregisterSurfaceTextureFrameListener, a0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::Vibrate1(int64_t a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jVibrate1, a0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::VibrateA(jlongArray a0, int32_t a1) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoAppShellClass, jVibrateA, a0, a1);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

jstring AndroidBridge::GetFrameNameJavaProfilingWrapper(int32_t a0, int32_t a1, int32_t a2) {
    JNIEnv *env = GetJNIForThread();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return nullptr;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    jvalue args[3];
    args[0].i = a0;
    args[1].i = a1;
    args[2].i = a2;

    jobject temp = env->CallStaticObjectMethodA(mGeckoJavaSamplerClass, jGetFrameNameJavaProfilingWrapper, args);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return nullptr;
    }
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

jdouble AndroidBridge::GetSampleTimeJavaProfiling(int32_t a0, int32_t a1) {
    JNIEnv *env = GetJNIForThread();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return 0.0;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return 0.0;
    }

    jdouble temp = env->CallStaticDoubleMethod(mGeckoJavaSamplerClass, jGetSampleTimeJavaProfiling, a0, a1);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return 0.0;
    }
    env->PopLocalFrame(NULL);
    return temp;
}

jstring AndroidBridge::GetThreadNameJavaProfilingWrapper(int32_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return nullptr;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    jobject temp = env->CallStaticObjectMethod(mGeckoJavaSamplerClass, jGetThreadNameJavaProfilingWrapper, a0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return nullptr;
    }
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

void AndroidBridge::PauseJavaProfiling() {
    JNIEnv *env = GetJNIForThread();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoJavaSamplerClass, jPauseJavaProfiling);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::StartJavaProfiling(int32_t a0, int32_t a1) {
    JNIEnv *env = GetJNIForThread();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoJavaSamplerClass, jStartJavaProfiling, a0, a1);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::StopJavaProfiling() {
    JNIEnv *env = GetJNIForThread();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoJavaSamplerClass, jStopJavaProfiling);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::UnpauseJavaProfiling() {
    JNIEnv *env = GetJNIForThread();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallStaticVoidMethod(mGeckoJavaSamplerClass, jUnpauseJavaProfiling);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::SendThumbnail(jobject a0, int32_t a1, bool a2) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jvalue args[3];
    args[0].l = a0;
    args[1].i = a1;
    args[2].z = a2;

    env->CallStaticVoidMethodA(mThumbnailHelperClass, jSendThumbnail, args);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

jobject AndroidBridge::CreateEGLSurfaceForCompositorWrapper(jobject aTarget) {
    JNIEnv *env = GetJNIForThread();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return nullptr;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    jobject temp = env->CallObjectMethod(aTarget, jCreateEGLSurfaceForCompositorWrapper);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return nullptr;
    }
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

jobject AndroidBridge::RegisterCompositorWrapper() {
    JNIEnv *env = GetJNIForThread();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return nullptr;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    jobject temp = env->CallStaticObjectMethod(mLayerViewClass, jRegisterCompositorWrapper);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return nullptr;
    }
    jobject ret = static_cast<jobject>(env->PopLocalFrame(temp));
    return ret;
}

void AndroidBridge::PostDelayedCallbackWrapper(jobject aTarget, int64_t a0) {
    JNIEnv *env = GetJNIForThread();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    env->CallVoidMethod(aTarget, jPostDelayedCallbackWrapper, a0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

void AndroidBridge::RequestContentRepaintWrapper(jobject aTarget, jfloat a0, jfloat a1, jfloat a2, jfloat a3, jfloat a4) {
    JNIEnv *env = GetJNIForThread();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(0) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jvalue args[5];
    args[0].f = a0;
    args[1].f = a1;
    args[2].f = a2;
    args[3].f = a3;
    args[4].f = a4;

    env->CallVoidMethodA(aTarget, jRequestContentRepaintWrapper, args);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}

jstring AndroidBridge::GetClipboardTextWrapper() {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return nullptr;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    jobject temp = env->CallStaticObjectMethod(mClipboardClass, jGetClipboardTextWrapper);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return nullptr;
    }
    jstring ret = static_cast<jstring>(env->PopLocalFrame(temp));
    return ret;
}

void AndroidBridge::SetClipboardText(const nsAString& a0) {
    JNIEnv *env = GetJNIEnv();
    if (!env) {
        ALOG_BRIDGE("Aborted: No env - %s", __PRETTY_FUNCTION__);
        return;
    }

    if (env->PushLocalFrame(1) != 0) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }

    jstring j0 = NewJavaString(env, a0);

    env->CallStaticVoidMethod(mClipboardClass, jSetClipboardText, j0);

    if (env->ExceptionCheck()) {
        ALOG_BRIDGE("Exceptional exit of: %s", __PRETTY_FUNCTION__);
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->PopLocalFrame(NULL);
        return;
    }
    env->PopLocalFrame(NULL);
}
