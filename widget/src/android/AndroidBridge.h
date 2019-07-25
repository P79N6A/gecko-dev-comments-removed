




































#ifndef AndroidBridge_h__
#define AndroidBridge_h__

#include <jni.h>
#include <android/log.h>

#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIRunnable.h"
#include "nsIObserver.h"

#include "AndroidJavaWrappers.h"

#include "nsIMutableArray.h"
#include "nsIMIMEInfo.h"
#include "nsColor.h"

#include "nsIAndroidBridge.h"





class nsWindow;

namespace mozilla {

namespace hal {
class BatteryInformation;
} 



typedef struct AndroidSystemColors {
    nscolor textColorPrimary;
    nscolor textColorPrimaryInverse;
    nscolor textColorSecondary;
    nscolor textColorSecondaryInverse;
    nscolor textColorTertiary;
    nscolor textColorTertiaryInverse;
    nscolor textColorHighlight;
    nscolor colorForeground;
    nscolor colorBackground;
    nscolor panelColorForeground;
    nscolor panelColorBackground;
} AndroidSystemColors;

class AndroidBridge
{
public:
    enum {
        NOTIFY_IME_RESETINPUTSTATE = 0,
        NOTIFY_IME_SETOPENSTATE = 1,
        NOTIFY_IME_CANCELCOMPOSITION = 2,
        NOTIFY_IME_FOCUSCHANGE = 3
    };

    static AndroidBridge *ConstructBridge(JNIEnv *jEnv,
                                          jclass jGeckoAppShellClass);

    static AndroidBridge *Bridge() {
        return sBridge;
    }

    static JavaVM *VM() {
        return sBridge->mJavaVM;
    }

    static JNIEnv *JNI() {
        sBridge->EnsureJNIThread();
        return sBridge->mJNIEnv;
    }

    static JNIEnv *JNIForThread() {
        if (NS_LIKELY(sBridge))
          return sBridge->AttachThread();
        return nsnull;
    }
    
    static jclass GetGeckoAppShellClass() {
        return sBridge->mGeckoAppShellClass;
    }

    
    
    
    
    
    bool SetMainThread(void *thr);

    JNIEnv* AttachThread(bool asDaemon = true);

    
    static void NotifyIME(int aType, int aState);

    static void NotifyIMEEnabled(int aState, const nsAString& aTypeHint,
                                 const nsAString& aActionHint);

    static void NotifyIMEChange(const PRUnichar *aText, PRUint32 aTextLen, int aStart, int aEnd, int aNewEnd);

    void AcknowledgeEventSync();

    void EnableDeviceMotion(bool aEnable);

    void EnableLocation(bool aEnable);

    void ReturnIMEQueryResult(const PRUnichar *aResult, PRUint32 aLen, int aSelStart, int aSelLen);

    void NotifyAppShellReady();

    void NotifyXreExit();

    void ScheduleRestart();

    void SetSurfaceView(jobject jobj);
    AndroidGeckoSurfaceView& SurfaceView() { return mSurfaceView; }

    bool GetHandlersForURL(const char *aURL, 
                             nsIMutableArray* handlersArray = nsnull,
                             nsIHandlerApp **aDefaultApp = nsnull,
                             const nsAString& aAction = EmptyString());

    bool GetHandlersForMimeType(const char *aMimeType,
                                  nsIMutableArray* handlersArray = nsnull,
                                  nsIHandlerApp **aDefaultApp = nsnull,
                                  const nsAString& aAction = EmptyString());

    bool OpenUriExternal(const nsACString& aUriSpec, const nsACString& aMimeType,
                           const nsAString& aPackageName = EmptyString(),
                           const nsAString& aClassName = EmptyString(),
                           const nsAString& aAction = EmptyString(),
                           const nsAString& aTitle = EmptyString());

    void GetMimeTypeFromExtensions(const nsACString& aFileExt, nsCString& aMimeType);
    void GetExtensionFromMimeType(const nsACString& aMimeType, nsACString& aFileExt);

    void MoveTaskToBack();

    bool GetClipboardText(nsAString& aText);

    void SetClipboardText(const nsAString& aText);
    
    void EmptyClipboard();

    bool ClipboardHasText();

    void ShowAlertNotification(const nsAString& aImageUrl,
                               const nsAString& aAlertTitle,
                               const nsAString& aAlertText,
                               const nsAString& aAlertData,
                               nsIObserver *aAlertListener,
                               const nsAString& aAlertName);

    void AlertsProgressListener_OnProgress(const nsAString& aAlertName,
                                           PRInt64 aProgress,
                                           PRInt64 aProgressMax,
                                           const nsAString& aAlertText);

    void AlertsProgressListener_OnCancel(const nsAString& aAlertName);

    int GetDPI();

    void ShowFilePicker(nsAString& aFilePath, nsAString& aFilters);

    void PerformHapticFeedback(bool aIsLongPress);

    void SetFullScreen(bool aFullScreen);

    void ShowInputMethodPicker();

    bool IsNetworkLinkUp();

    bool IsNetworkLinkKnown();

    void SetSelectedLocale(const nsAString&);

    void GetSystemColors(AndroidSystemColors *aColors);

    void GetIconForExtension(const nsACString& aFileExt, PRUint32 aIconSize, PRUint8 * const aBuf);

    bool GetShowPasswordSetting();

    void FireAndWaitForTracerEvent();

    bool GetAccessibilityEnabled();

    struct AutoLocalJNIFrame {
        AutoLocalJNIFrame(int nEntries = 128) : mEntries(nEntries) {
            
            
            
            AndroidBridge::Bridge()->JNI()->PushLocalFrame(mEntries + 1);
        }
        
        
        
        void Purge() {
            AndroidBridge::Bridge()->JNI()->PopLocalFrame(NULL);
            AndroidBridge::Bridge()->JNI()->PushLocalFrame(mEntries);
        }
        ~AutoLocalJNIFrame() {
            jthrowable exception =
                AndroidBridge::Bridge()->JNI()->ExceptionOccurred();
            if (exception) {
                AndroidBridge::Bridge()->JNI()->ExceptionDescribe();
                AndroidBridge::Bridge()->JNI()->ExceptionClear();
            }
            AndroidBridge::Bridge()->JNI()->PopLocalFrame(NULL);
        }
        int mEntries;
    };

    
    void *CallEglCreateWindowSurface(void *dpy, void *config, AndroidGeckoSurfaceView& surfaceView);

    bool GetStaticStringField(const char *classID, const char *field, nsAString &result);

    bool GetStaticIntField(const char *className, const char *fieldName, PRInt32* aInt);

    void SetKeepScreenOn(bool on);

    void ScanMedia(const nsAString& aFile, const nsACString& aMimeType);

    void CreateShortcut(const nsAString& aTitle, const nsAString& aURI, const nsAString& aIconData, const nsAString& aIntent);

    
    bool HasNativeBitmapAccess();

    bool ValidateBitmap(jobject bitmap, int width, int height);

    void *LockBitmap(jobject bitmap);

    void UnlockBitmap(jobject bitmap);

    void PostToJavaThread(nsIRunnable* aRunnable, bool aMainThread = false);

    void ExecuteNextRunnable();

    
    enum {
        WINDOW_FORMAT_RGBA_8888          = 1,
        WINDOW_FORMAT_RGBX_8888          = 2,
        WINDOW_FORMAT_RGB_565            = 4,
    };

    bool HasNativeWindowAccess();

    void *AcquireNativeWindow(jobject surface);
    void ReleaseNativeWindow(void *window);
    bool SetNativeWindowFormat(void *window, int format);

    bool LockWindow(void *window, unsigned char **bits, int *width, int *height, int *format, int *stride);
    bool UnlockWindow(void *window);
    
    void HandleGeckoMessage(const nsAString& message, nsAString &aRet);

    void CheckURIVisited(const nsAString& uri);
    void MarkURIVisited(const nsAString& uri);

    bool InitCamera(const nsCString& contentType, PRUint32 camera, PRUint32 *width, PRUint32 *height, PRUint32 *fps);

    void CloseCamera();

    void EnableBatteryNotifications();
    void DisableBatteryNotifications();
    void GetCurrentBatteryInformation(hal::BatteryInformation* aBatteryInfo);

protected:
    static AndroidBridge *sBridge;

    
    JavaVM *mJavaVM;

    
    JNIEnv *mJNIEnv;
    void *mThread;

    
    AndroidGeckoSurfaceView mSurfaceView;

    
    jclass mGeckoAppShellClass;

    AndroidBridge() { }
    bool Init(JNIEnv *jEnv, jclass jGeckoApp);

    void EnsureJNIThread();

    bool mOpenedGraphicsLibraries;
    void OpenGraphicsLibraries();

    bool mHasNativeBitmapAccess;
    bool mHasNativeWindowAccess;

    nsCOMArray<nsIRunnable> mRunnableQueue;

    
    jmethodID jNotifyIME;
    jmethodID jNotifyIMEEnabled;
    jmethodID jNotifyIMEChange;
    jmethodID jAcknowledgeEventSync;
    jmethodID jEnableDeviceMotion;
    jmethodID jEnableLocation;
    jmethodID jReturnIMEQueryResult;
    jmethodID jNotifyAppShellReady;
    jmethodID jNotifyXreExit;
    jmethodID jScheduleRestart;
    jmethodID jGetOutstandingDrawEvents;
    jmethodID jGetHandlersForMimeType;
    jmethodID jGetHandlersForURL;
    jmethodID jOpenUriExternal;
    jmethodID jGetMimeTypeFromExtensions;
    jmethodID jGetExtensionFromMimeType;
    jmethodID jMoveTaskToBack;
    jmethodID jGetClipboardText;
    jmethodID jSetClipboardText;
    jmethodID jShowAlertNotification;
    jmethodID jShowFilePicker;
    jmethodID jAlertsProgressListener_OnProgress;
    jmethodID jAlertsProgressListener_OnCancel;
    jmethodID jGetDpi;
    jmethodID jSetFullScreen;
    jmethodID jShowInputMethodPicker;
    jmethodID jPerformHapticFeedback;
    jmethodID jSetKeepScreenOn;
    jmethodID jIsNetworkLinkUp;
    jmethodID jIsNetworkLinkKnown;
    jmethodID jSetSelectedLocale;
    jmethodID jScanMedia;
    jmethodID jGetSystemColors;
    jmethodID jGetIconForExtension;
    jmethodID jFireAndWaitForTracerEvent;
    jmethodID jCreateShortcut;
    jmethodID jGetShowPasswordSetting;
    jmethodID jGetAccessibilityEnabled;
    jmethodID jPostToJavaThread;
    jmethodID jInitCamera;
    jmethodID jCloseCamera;
    jmethodID jHandleGeckoMessage;
    jmethodID jCheckUriVisited;
    jmethodID jMarkUriVisited;
    jmethodID jEnableBatteryNotifications;
    jmethodID jDisableBatteryNotifications;
    jmethodID jGetCurrentBatteryInformation;

    
    jclass jEGLSurfaceImplClass;
    jclass jEGLContextImplClass;
    jclass jEGLConfigImplClass;
    jclass jEGLDisplayImplClass;
    jclass jEGLContextClass;
    jclass jEGL10Class;

    
    int (* AndroidBitmap_getInfo)(JNIEnv *env, jobject bitmap, void *info);
    int (* AndroidBitmap_lockPixels)(JNIEnv *env, jobject bitmap, void **buffer);
    int (* AndroidBitmap_unlockPixels)(JNIEnv *env, jobject bitmap);

    void* (*ANativeWindow_fromSurface)(JNIEnv *env, jobject surface);
    void (*ANativeWindow_release)(void *window);
    int (*ANativeWindow_setBuffersGeometry)(void *window, int width, int height, int format);

    int (* ANativeWindow_lock)(void *window, void *outBuffer, void *inOutDirtyBounds);
    int (* ANativeWindow_unlockAndPost)(void *window);
};

}



#define NS_ANDROIDBRIDGE_CID \
{ 0x0FE2321D, 0xEBD9, 0x467D, \
    { 0xA7, 0x43, 0x03, 0xA6, 0x8D, 0x40, 0x59, 0x9E } }

class nsAndroidBridge : public nsIAndroidBridge
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIANDROIDBRIDGE

  nsAndroidBridge();

private:
  ~nsAndroidBridge();

protected:
};


extern "C" JNIEnv * GetJNIForThread();
extern bool mozilla_AndroidBridge_SetMainThread(void *);
extern jclass GetGeckoAppShellClass();

#endif
