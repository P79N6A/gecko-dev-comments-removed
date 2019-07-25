




































#ifndef AndroidBridge_h__
#define AndroidBridge_h__

#include <jni.h>
#include <android/log.h>

#include "nsCOMPtr.h"
#include "nsIRunnable.h"
#include "nsIObserver.h"

#include "AndroidJavaWrappers.h"

#include "nsIMutableArray.h"
#include "nsIMIMEInfo.h"





class nsWindow;

namespace mozilla {

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

    
    
    
    
    
    PRBool SetMainThread(void *thr);

    JNIEnv* AttachThread(PRBool asDaemon = PR_TRUE);

    
    static void NotifyIME(int aType, int aState);

    static void NotifyIMEEnabled(int aState, const nsAString& aTypeHint,
                                 const nsAString& aActionHint);

    static void NotifyIMEChange(const PRUnichar *aText, PRUint32 aTextLen, int aStart, int aEnd, int aNewEnd);

    void AcknowledgeEventSync();

    void EnableAccelerometer(bool aEnable);

    void EnableLocation(bool aEnable);

    void ReturnIMEQueryResult(const PRUnichar *aResult, PRUint32 aLen, int aSelStart, int aSelLen);

    void NotifyAppShellReady();

    void NotifyXreExit();

    void ScheduleRestart();

    void SetSurfaceView(jobject jobj);
    AndroidGeckoSurfaceView& SurfaceView() { return mSurfaceView; }

    PRBool GetHandlersForURL(const char *aURL, 
                             nsIMutableArray* handlersArray = nsnull,
                             nsIHandlerApp **aDefaultApp = nsnull,
                             const nsAString& aAction = EmptyString());

    PRBool GetHandlersForMimeType(const char *aMimeType,
                                  nsIMutableArray* handlersArray = nsnull,
                                  nsIHandlerApp **aDefaultApp = nsnull,
                                  const nsAString& aAction = EmptyString());

    PRBool OpenUriExternal(const nsACString& aUriSpec, const nsACString& aMimeType,
                           const nsAString& aPackageName = EmptyString(),
                           const nsAString& aClassName = EmptyString(),
                           const nsAString& aAction = EmptyString(),
                           const nsAString& aTitle = EmptyString());

    void GetMimeTypeFromExtensions(const nsACString& aFileExt, nsCString& aMimeType);
    void GetExtensionFromMimeType(const nsCString& aMimeType, nsACString& aFileExt);

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

    void PerformHapticFeedback(PRBool aIsLongPress);

    void SetFullScreen(PRBool aFullScreen);

    void ShowInputMethodPicker();

    void HideProgressDialogOnce();

    bool IsNetworkLinkUp();

    bool IsNetworkLinkKnown();

    void SetSelectedLocale(const nsAString&);

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

protected:
    static AndroidBridge *sBridge;

    
    JavaVM *mJavaVM;

    
    JNIEnv *mJNIEnv;
    void *mThread;

    
    AndroidGeckoSurfaceView mSurfaceView;

    
    jclass mGeckoAppShellClass;

    AndroidBridge() { }
    PRBool Init(JNIEnv *jEnv, jclass jGeckoApp);

    void EnsureJNIThread();

    
    jmethodID jNotifyIME;
    jmethodID jNotifyIMEEnabled;
    jmethodID jNotifyIMEChange;
    jmethodID jAcknowledgeEventSync;
    jmethodID jEnableAccelerometer;
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
    jmethodID jHideProgressDialog;
    jmethodID jPerformHapticFeedback;
    jmethodID jSetKeepScreenOn;
    jmethodID jIsNetworkLinkUp;
    jmethodID jIsNetworkLinkKnown;
    jmethodID jSetSelectedLocale;
    jmethodID jScanMedia;

    
    jclass jEGLSurfaceImplClass;
    jclass jEGLContextImplClass;
    jclass jEGLConfigImplClass;
    jclass jEGLDisplayImplClass;
    jclass jEGLContextClass;
    jclass jEGL10Class;
};

}

extern "C" JNIEnv * GetJNIForThread();
extern PRBool mozilla_AndroidBridge_SetMainThread(void *);
extern jclass GetGeckoAppShellClass();

#endif 
