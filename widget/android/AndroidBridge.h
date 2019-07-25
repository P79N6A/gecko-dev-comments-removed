




































#ifndef AndroidBridge_h__
#define AndroidBridge_h__

#include <jni.h>
#include <android/log.h>
#include <cstdlib>
#include <pthread.h>

#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIRunnable.h"
#include "nsIObserver.h"
#include "nsThreadUtils.h"

#include "AndroidFlexViewWrapper.h"
#include "AndroidJavaWrappers.h"

#include "nsIMutableArray.h"
#include "nsIMIMEInfo.h"
#include "nsColor.h"
#include "BasicLayers.h"
#include "gfxRect.h"

#include "nsIAndroidBridge.h"





class nsWindow;
class nsIDOMMozSmsMessage;


extern "C" JNIEnv * GetJNIForThread();

extern bool mozilla_AndroidBridge_SetMainThread(void *);
extern jclass GetGeckoAppShellClass();

namespace base {
class Thread;
} 

namespace mozilla {

namespace hal {
class BatteryInformation;
class NetworkInformation;
} 

namespace dom {
namespace sms {
struct SmsFilterData;
} 
} 

namespace layers {
class CompositorParent;
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

    enum {
        LAYER_CLIENT_TYPE_NONE = 0,
        LAYER_CLIENT_TYPE_GL = 2            
    };

    static AndroidBridge *ConstructBridge(JNIEnv *jEnv,
                                          jclass jGeckoAppShellClass);

    static AndroidBridge *Bridge() {
        return sBridge;
    }

    static JavaVM *GetVM() {
        if (NS_LIKELY(sBridge))
            return sBridge->mJavaVM;
        return nsnull;
    }

    static JNIEnv *GetJNIEnv() {
        if (NS_LIKELY(sBridge)) {
            if ((void*)pthread_self() != sBridge->mThread) {
                __android_log_print(ANDROID_LOG_INFO, "AndroidBridge",
                                    "###!!!!!!! Something's grabbing the JNIEnv from the wrong thread! (thr %p should be %p)",
                                    (void*)pthread_self(), (void*)sBridge->mThread);
                return nsnull;
            }
            return sBridge->mJNIEnv;

        }
        return nsnull;
    }
    
    static jclass GetGeckoAppShellClass() {
        return sBridge->mGeckoAppShellClass;
    }

    
    
    
    
    
    bool SetMainThread(void *thr);

    
    static void NotifyIME(int aType, int aState);

    static void NotifyIMEEnabled(int aState, const nsAString& aTypeHint,
                                 const nsAString& aActionHint);

    static void NotifyIMEChange(const PRUnichar *aText, PRUint32 aTextLen, int aStart, int aEnd, int aNewEnd);

    nsresult TakeScreenshot(nsIDOMWindow *window, PRInt32 srcX, PRInt32 srcY, PRInt32 srcW, PRInt32 srcH, PRInt32 dstW, PRInt32 dstH, PRInt32 tabId, float scale);

    void AcknowledgeEventSync();

    void EnableDeviceMotion(bool aEnable);

    void EnableLocation(bool aEnable);

    void EnableSensor(int aSensorType);

    void DisableSensor(int aSensorType);

    void ReturnIMEQueryResult(const PRUnichar *aResult, PRUint32 aLen, int aSelStart, int aSelLen);

    void NotifyXreExit();

    void ScheduleRestart();

    void SetLayerClient(jobject jobj);
    AndroidGeckoLayerClient &GetLayerClient() { return *mLayerClient; }

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

    void Vibrate(const nsTArray<PRUint32>& aPattern);
    void CancelVibrate();

    void SetFullScreen(bool aFullScreen);

    void ShowInputMethodPicker();

    void SetPreventPanning(bool aPreventPanning);

    void HideProgressDialogOnce();

    bool IsNetworkLinkUp();

    bool IsNetworkLinkKnown();

    void SetSelectedLocale(const nsAString&);

    void GetSystemColors(AndroidSystemColors *aColors);

    void GetIconForExtension(const nsACString& aFileExt, PRUint32 aIconSize, PRUint8 * const aBuf);

    bool GetShowPasswordSetting();

    void FireAndWaitForTracerEvent();

    bool GetAccessibilityEnabled();

    class AutoLocalJNIFrame {
    public:
        AutoLocalJNIFrame(int nEntries = 128)
            : mEntries(nEntries)
        {
            mJNIEnv = AndroidBridge::GetJNIEnv();
            Push();
        }

        AutoLocalJNIFrame(JNIEnv* aJNIEnv, int nEntries = 128)
            : mEntries(nEntries)
        {
            mJNIEnv = aJNIEnv ? aJNIEnv : AndroidBridge::GetJNIEnv();

            Push();
        }

        
        
        
        void Purge() {
            if (mJNIEnv) {
                mJNIEnv->PopLocalFrame(NULL);
                Push();
            }
        }

        ~AutoLocalJNIFrame() {
            if (!mJNIEnv)
                return;

            jthrowable exception = mJNIEnv->ExceptionOccurred();
            if (exception) {
                mJNIEnv->ExceptionDescribe();
                mJNIEnv->ExceptionClear();
            }

            mJNIEnv->PopLocalFrame(NULL);
        }

    private:
        void Push() {
            if (!mJNIEnv)
                return;

            
            
            
            mJNIEnv->PushLocalFrame(mEntries + 1);
        }

        int mEntries;
        JNIEnv* mJNIEnv;
    };

    
    void *CallEglCreateWindowSurface(void *dpy, void *config, AndroidGeckoSurfaceView& surfaceView);

    
    void RegisterCompositor();
    EGLSurface ProvideEGLSurface();

    bool GetStaticStringField(const char *classID, const char *field, nsAString &result);

    bool GetStaticIntField(const char *className, const char *fieldName, PRInt32* aInt);

    void SetKeepScreenOn(bool on);

    void ScanMedia(const nsAString& aFile, const nsACString& aMimeType);

    void CreateShortcut(const nsAString& aTitle, const nsAString& aURI, const nsAString& aIconData, const nsAString& aIntent);

    
    bool HasNativeBitmapAccess();

    bool ValidateBitmap(jobject bitmap, int width, int height);

    void *LockBitmap(jobject bitmap);

    void UnlockBitmap(jobject bitmap);

    void PostToJavaThread(JNIEnv *aEnv, nsIRunnable* aRunnable, bool aMainThread = false);

    void ExecuteNextRunnable(JNIEnv *aEnv);

    
    enum {
        WINDOW_FORMAT_RGBA_8888          = 1,
        WINDOW_FORMAT_RGBX_8888          = 2,
        WINDOW_FORMAT_RGB_565            = 4,
    };

    bool HasNativeWindowAccess();

    void *AcquireNativeWindow(jobject surface);
    void ReleaseNativeWindow(void *window);
    bool SetNativeWindowFormat(void *window, int width, int height, int format);

    bool LockWindow(void *window, unsigned char **bits, int *width, int *height, int *format, int *stride);
    bool UnlockWindow(void *window);
    
    void HandleGeckoMessage(const nsAString& message, nsAString &aRet);

    nsCOMPtr<nsIAndroidDrawMetadataProvider> GetDrawMetadataProvider();

    void EmitGeckoAccessibilityEvent (PRInt32 eventType, const nsTArray<nsString>& text, const nsAString& description, bool enabled, bool checked, bool password);

    void CheckURIVisited(const nsAString& uri);
    void MarkURIVisited(const nsAString& uri);

    bool InitCamera(const nsCString& contentType, PRUint32 camera, PRUint32 *width, PRUint32 *height, PRUint32 *fps);

    void CloseCamera();

    void EnableBatteryNotifications();
    void DisableBatteryNotifications();
    void GetCurrentBatteryInformation(hal::BatteryInformation* aBatteryInfo);

    PRUint16 GetNumberOfMessagesForText(const nsAString& aText);
    void SendMessage(const nsAString& aNumber, const nsAString& aText, PRInt32 aRequestId, PRUint64 aProcessId);
    PRInt32 SaveSentMessage(const nsAString& aRecipient, const nsAString& aBody, PRUint64 aDate);
    void GetMessage(PRInt32 aMessageId, PRInt32 aRequestId, PRUint64 aProcessId);
    void DeleteMessage(PRInt32 aMessageId, PRInt32 aRequestId, PRUint64 aProcessId);
    void CreateMessageList(const dom::sms::SmsFilterData& aFilter, bool aReverse, PRInt32 aRequestId, PRUint64 aProcessId);
    void GetNextMessageInList(PRInt32 aListId, PRInt32 aRequestId, PRUint64 aProcessId);
    void ClearMessageList(PRInt32 aListId);

    bool IsTablet();

    void GetCurrentNetworkInformation(hal::NetworkInformation* aNetworkInfo);
    void EnableNetworkNotifications();
    void DisableNetworkNotifications();

    void SetCompositorParent(mozilla::layers::CompositorParent* aCompositorParent,
                             base::Thread* aCompositorThread);
    void ScheduleComposite();
    void SchedulePauseComposition();
    void ScheduleResumeComposition();
    void SetFirstPaintViewport(float aOffsetX, float aOffsetY, float aZoom, float aPageWidth, float aPageHeight);
    void SetPageSize(float aZoom, float aPageWidth, float aPageHeight);
    void SetViewTransformGetter(AndroidViewTransformGetter& aViewTransformGetter);
    void GetViewTransform(nsIntPoint& aScrollOffset, float& aScaleX, float& aScaleY);

    jobject CreateSurface();
    void DestroySurface(jobject surface);
    void ShowSurface(jobject surface, const gfxRect& aRect, bool aInverted, bool aBlend);
    void HideSurface(jobject surface);

protected:
    static AndroidBridge *sBridge;

    
    JavaVM *mJavaVM;

    
    JNIEnv *mJNIEnv;
    void *mThread;

    
    AndroidGeckoSurfaceView mSurfaceView;

    AndroidGeckoLayerClient *mLayerClient;

    nsRefPtr<mozilla::layers::CompositorParent> mCompositorParent;
    base::Thread *mCompositorThread;
    AndroidViewTransformGetter *mViewTransformGetter;

    
    jclass mGeckoAppShellClass;

    AndroidBridge();
    ~AndroidBridge();

    bool Init(JNIEnv *jEnv, jclass jGeckoApp);

    bool mOpenedGraphicsLibraries;
    void OpenGraphicsLibraries();

    bool mHasNativeBitmapAccess;
    bool mHasNativeWindowAccess;

    nsCOMArray<nsIRunnable> mRunnableQueue;

    
    jmethodID jNotifyIME;
    jmethodID jNotifyIMEEnabled;
    jmethodID jNotifyIMEChange;
    jmethodID jNotifyScreenShot;
    jmethodID jAcknowledgeEventSync;
    jmethodID jEnableDeviceMotion;
    jmethodID jEnableLocation;
    jmethodID jEnableSensor;
    jmethodID jDisableSensor;
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
    jmethodID jSetPreventPanning;
    jmethodID jHideProgressDialog;
    jmethodID jPerformHapticFeedback;
    jmethodID jVibrate1;
    jmethodID jVibrateA;
    jmethodID jCancelVibrate;
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
    jmethodID jPostToJavaThread;
    jmethodID jInitCamera;
    jmethodID jCloseCamera;
    jmethodID jIsTablet;
    jmethodID jEnableBatteryNotifications;
    jmethodID jDisableBatteryNotifications;
    jmethodID jGetCurrentBatteryInformation;
    jmethodID jGetAccessibilityEnabled;
    jmethodID jHandleGeckoMessage;
    jmethodID jCheckUriVisited;
    jmethodID jMarkUriVisited;
    jmethodID jEmitGeckoAccessibilityEvent;

    jmethodID jNumberOfMessages;
    jmethodID jSendMessage;
    jmethodID jSaveSentMessage;
    jmethodID jGetMessage;
    jmethodID jDeleteMessage;
    jmethodID jCreateMessageList;
    jmethodID jGetNextMessageinList;
    jmethodID jClearMessageList;

    jmethodID jGetCurrentNetworkInformation;
    jmethodID jEnableNetworkNotifications;
    jmethodID jDisableNetworkNotifications;

    
    jclass jEGLSurfaceImplClass;
    jclass jEGLContextImplClass;
    jclass jEGLConfigImplClass;
    jclass jEGLDisplayImplClass;
    jclass jEGLContextClass;
    jclass jEGL10Class;

    jclass jFlexSurfaceView;
    jmethodID jRegisterCompositorMethod;

    
    jclass jStringClass;

    
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


#endif
