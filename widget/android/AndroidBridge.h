




#ifndef AndroidBridge_h__
#define AndroidBridge_h__

#include <jni.h>
#include <android/log.h>
#include <cstdlib>
#include <pthread.h>

#include "nsCOMPtr.h"
#include "nsCOMArray.h"

#include "GeneratedJNIWrappers.h"

#include "nsIMutableArray.h"
#include "nsIMIMEInfo.h"
#include "nsColor.h"
#include "gfxRect.h"

#include "nsIAndroidBridge.h"
#include "nsIMobileMessageCallback.h"

#include "mozilla/Likely.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/layers/GeckoContentController.h"
#include "mozilla/TimeStamp.h"





class nsWindow;
class nsIDOMMozSmsMessage;
class nsIObserver;


extern "C" JNIEnv * GetJNIForThread();

extern bool mozilla_AndroidBridge_SetMainThread(pthread_t);

namespace base {
class Thread;
} 

typedef void* EGLSurface;

using namespace mozilla::widget::android;

namespace mozilla {

namespace hal {
class BatteryInformation;
class NetworkInformation;
} 

namespace dom {
namespace mobilemessage {
struct SmsFilterData;
struct SmsSegmentInfoData;
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

class nsFilePickerCallback : nsISupports {
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    virtual void handleResult(nsAString& filePath) = 0;
    nsFilePickerCallback() {}
protected:
    virtual ~nsFilePickerCallback() {}
};

class DelayedTask {
public:
    DelayedTask(Task* aTask, int aDelayMs) {
        mTask = aTask;
        mRunTime = TimeStamp::Now() + TimeDuration::FromMilliseconds(aDelayMs);
    }

    bool IsEarlierThan(DelayedTask *aOther) {
        return mRunTime < aOther->mRunTime;
    }

    int64_t MillisecondsToRunTime() {
        TimeDuration timeLeft = mRunTime - TimeStamp::Now();
        return (int64_t)timeLeft.ToMilliseconds();
    }

    Task* GetTask() {
        return mTask;
    }

private:
    Task* mTask;
    TimeStamp mRunTime;
};


class AndroidBridge MOZ_FINAL : public mozilla::layers::GeckoContentController
{
public:
    enum {
        
        
        NOTIFY_IME_OPEN_VKB = -2,
        NOTIFY_IME_REPLY_EVENT = -1,
    };

    enum {
        LAYER_CLIENT_TYPE_NONE = 0,
        LAYER_CLIENT_TYPE_GL = 2            
    };

    static void ConstructBridge(JNIEnv *jEnv);

    static AndroidBridge *Bridge() {
        return sBridge.get();
    }

    static JavaVM *GetVM() {
        if (MOZ_LIKELY(sBridge))
            return sBridge->mJavaVM;
        return nullptr;
    }

    static JNIEnv *GetJNIEnv() {
        if (MOZ_LIKELY(sBridge)) {
            if (!pthread_equal(pthread_self(), sBridge->mThread)) {
                __android_log_print(ANDROID_LOG_INFO, "AndroidBridge",
                                    "###!!!!!!! Something's grabbing the JNIEnv from the wrong thread! (thr %p should be %p)",
                                    (void*)pthread_self(), (void*)sBridge->mThread);
                MOZ_ASSERT(false, "###!!!!!!! Something's grabbing the JNIEnv from the wrong thread!");
                return nullptr;
            }
            return sBridge->mJNIEnv;

        }
        return nullptr;
    }

    
    
    
    
    
    bool SetMainThread(pthread_t thr);

    
    bool GetThreadNameJavaProfiling(uint32_t aThreadId, nsCString & aResult);
    bool GetFrameNameJavaProfiling(uint32_t aThreadId, uint32_t aSampleId, uint32_t aFrameId, nsCString & aResult);

    nsresult CaptureThumbnail(nsIDOMWindow *window, int32_t bufW, int32_t bufH, int32_t tabId, jobject buffer);
    void GetDisplayPort(bool aPageSizeUpdate, bool aIsBrowserContentDisplayed, int32_t tabId, nsIAndroidViewport* metrics, nsIAndroidDisplayport** displayPort);
    void ContentDocumentChanged();
    bool IsContentDocumentDisplayed();

    bool ProgressiveUpdateCallback(bool aHasPendingNewThebesContent, const LayerRect& aDisplayPort, float aDisplayResolution, bool aDrawingCritical, gfx::Rect& aViewport, float& aScaleX, float& aScaleY);

    void SetLayerClient(JNIEnv* env, jobject jobj);
    GeckoLayerClient* GetLayerClient() { return mLayerClient; }

    bool GetHandlersForURL(const nsAString& aURL,
                           nsIMutableArray* handlersArray = nullptr,
                           nsIHandlerApp **aDefaultApp = nullptr,
                           const nsAString& aAction = EmptyString());

    bool GetHandlersForMimeType(const nsAString& aMimeType,
                                nsIMutableArray* handlersArray = nullptr,
                                nsIHandlerApp **aDefaultApp = nullptr,
                                const nsAString& aAction = EmptyString());

    void GetMimeTypeFromExtensions(const nsACString& aFileExt, nsCString& aMimeType);
    void GetExtensionFromMimeType(const nsACString& aMimeType, nsACString& aFileExt);

    bool GetClipboardText(nsAString& aText);

    void ShowAlertNotification(const nsAString& aImageUrl,
                               const nsAString& aAlertTitle,
                               const nsAString& aAlertText,
                               const nsAString& aAlertData,
                               nsIObserver *aAlertListener,
                               const nsAString& aAlertName);

    int GetDPI();
    int GetScreenDepth();

    void ShowFilePickerForExtensions(nsAString& aFilePath, const nsAString& aExtensions);
    void ShowFilePickerForMimeType(nsAString& aFilePath, const nsAString& aMimeType);
    void ShowFilePickerAsync(const nsAString& aMimeType, nsFilePickerCallback* callback);

    void Vibrate(const nsTArray<uint32_t>& aPattern);

    void GetSystemColors(AndroidSystemColors *aColors);

    void GetIconForExtension(const nsACString& aFileExt, uint32_t aIconSize, uint8_t * const aBuf);

    
    void RegisterCompositor(JNIEnv* env = NULL);
    EGLSurface ProvideEGLSurface();

    bool GetStaticStringField(const char *classID, const char *field, nsAString &result, JNIEnv* env = nullptr);

    bool GetStaticIntField(const char *className, const char *fieldName, int32_t* aInt, JNIEnv* env = nullptr);

    
    bool HasNativeBitmapAccess();

    bool ValidateBitmap(jobject bitmap, int width, int height);

    void *LockBitmap(jobject bitmap);

    
    
    
    jobject GetGlobalContextRef(void);

    void UnlockBitmap(jobject bitmap);

    
    enum {
        WINDOW_FORMAT_RGBA_8888          = 1,
        WINDOW_FORMAT_RGBX_8888          = 2,
        WINDOW_FORMAT_RGB_565            = 4
    };

    bool HasNativeWindowAccess();

    void *AcquireNativeWindow(JNIEnv* aEnv, jobject aSurface);
    void ReleaseNativeWindow(void *window);

    void *AcquireNativeWindowFromSurfaceTexture(JNIEnv* aEnv, jobject aSurface);
    void ReleaseNativeWindowForSurfaceTexture(void *window);

    bool LockWindow(void *window, unsigned char **bits, int *width, int *height, int *format, int *stride);
    bool UnlockWindow(void *window);
    
    void HandleGeckoMessage(const nsAString& message, nsAString &aRet);

    bool InitCamera(const nsCString& contentType, uint32_t camera, uint32_t *width, uint32_t *height, uint32_t *fps);

    void GetCurrentBatteryInformation(hal::BatteryInformation* aBatteryInfo);

    nsresult GetSegmentInfoForText(const nsAString& aText,
                                   nsIMobileMessageCallback* aRequest);
    void SendMessage(const nsAString& aNumber, const nsAString& aText,
                     nsIMobileMessageCallback* aRequest);
    void GetMessage(int32_t aMessageId, nsIMobileMessageCallback* aRequest);
    void DeleteMessage(int32_t aMessageId, nsIMobileMessageCallback* aRequest);
    void CreateMessageList(const dom::mobilemessage::SmsFilterData& aFilter,
                           bool aReverse, nsIMobileMessageCallback* aRequest);
    void GetNextMessageInList(int32_t aListId, nsIMobileMessageCallback* aRequest);
    already_AddRefed<nsIMobileMessageCallback> DequeueSmsRequest(uint32_t aRequestId);

    void GetCurrentNetworkInformation(hal::NetworkInformation* aNetworkInfo);

    void SetFirstPaintViewport(const LayerIntPoint& aOffset, const CSSToLayerScale& aZoom, const CSSRect& aCssPageRect);
    void SetPageRect(const CSSRect& aCssPageRect);
    void SyncViewportInfo(const LayerIntRect& aDisplayPort, const CSSToLayerScale& aDisplayResolution,
                          bool aLayersUpdated, ScreenPoint& aScrollOffset, CSSToScreenScale& aScale,
                          LayerMargin& aFixedLayerMargins, ScreenPoint& aOffset);
    void SyncFrameMetrics(const ScreenPoint& aScrollOffset, float aZoom, const CSSRect& aCssPageRect,
                          bool aLayersUpdated, const CSSRect& aDisplayPort, const CSSToLayerScale& aDisplayResolution,
                          bool aIsFirstPaint, LayerMargin& aFixedLayerMargins, ScreenPoint& aOffset);

    void AddPluginView(jobject view, const LayoutDeviceRect& rect, bool isFullScreen);

    
    
    
    
    uint32_t GetScreenOrientation();

    int GetAPIVersion() { return mAPIVersion; }
    bool IsHoneycomb() { return mAPIVersion >= 11 && mAPIVersion <= 13; }

    void ScheduleComposite();

    void GetGfxInfoData(nsACString& aRet);
    nsresult GetProxyForURI(const nsACString & aSpec,
                            const nsACString & aScheme,
                            const nsACString & aHost,
                            const int32_t      aPort,
                            nsACString & aResult);

    
    static jstring NewJavaString(JNIEnv* env, const PRUnichar* string, uint32_t len);
    static jstring NewJavaString(JNIEnv* env, const nsAString& string);
    static jstring NewJavaString(JNIEnv* env, const char* string);
    static jstring NewJavaString(JNIEnv* env, const nsACString& string);

    static jstring NewJavaString(AutoLocalJNIFrame* frame, const PRUnichar* string, uint32_t len);
    static jstring NewJavaString(AutoLocalJNIFrame* frame, const nsAString& string);
    static jstring NewJavaString(AutoLocalJNIFrame* frame, const char* string);
    static jstring NewJavaString(AutoLocalJNIFrame* frame, const nsACString& string);

    static jclass GetClassGlobalRef(JNIEnv* env, const char* className);
    static jfieldID GetFieldID(JNIEnv* env, jclass jClass, const char* fieldName, const char* fieldType);
    static jfieldID GetStaticFieldID(JNIEnv* env, jclass jClass, const char* fieldName, const char* fieldType);
    static jmethodID GetMethodID(JNIEnv* env, jclass jClass, const char* methodName, const char* methodType);
    static jmethodID GetStaticMethodID(JNIEnv* env, jclass jClass, const char* methodName, const char* methodType);
protected:
    static StaticRefPtr<AndroidBridge> sBridge;
    nsTArray<nsCOMPtr<nsIMobileMessageCallback> > mSmsRequests;

    
    JavaVM *mJavaVM;

    
    JNIEnv *mJNIEnv;
    pthread_t mThread;

    GeckoLayerClient *mLayerClient = NULL;

    
    jclass mAndroidSmsMessageClass;

    AndroidBridge();
    ~AndroidBridge();

    void InitStubs(JNIEnv *jEnv);
    bool Init(JNIEnv *jEnv);

    bool mOpenedGraphicsLibraries;
    void OpenGraphicsLibraries();
    void* GetNativeSurface(JNIEnv* env, jobject surface);

    bool mHasNativeBitmapAccess;
    bool mHasNativeWindowAccess;
    bool mHasNativeWindowFallback;

    int mAPIVersion;

    bool QueueSmsRequest(nsIMobileMessageCallback* aRequest, uint32_t* aRequestIdOut);

    
    jmethodID jNotifyAppShellReady;
    jmethodID jGetOutstandingDrawEvents;
    jmethodID jPostToJavaThread;
    jmethodID jCreateSurface;
    jmethodID jShowSurface;
    jmethodID jHideSurface;
    jmethodID jDestroySurface;

    jmethodID jCalculateLength;

    
    jclass jSurfaceClass;
    jfieldID jSurfacePointerField;

    jclass jLayerView;

    jfieldID jEGLSurfacePointerField;
    GLController *mGLControllerObj;

    
    jclass jStringClass;

    
    int (* AndroidBitmap_getInfo)(JNIEnv *env, jobject bitmap, void *info);
    int (* AndroidBitmap_lockPixels)(JNIEnv *env, jobject bitmap, void **buffer);
    int (* AndroidBitmap_unlockPixels)(JNIEnv *env, jobject bitmap);

    void* (*ANativeWindow_fromSurface)(JNIEnv *env, jobject surface);
    void* (*ANativeWindow_fromSurfaceTexture)(JNIEnv *env, jobject surfaceTexture);
    void (*ANativeWindow_release)(void *window);
    int (*ANativeWindow_setBuffersGeometry)(void *window, int width, int height, int format);

    int (* ANativeWindow_lock)(void *window, void *outBuffer, void *inOutDirtyBounds);
    int (* ANativeWindow_unlockAndPost)(void *window);

    int (* Surface_lock)(void* surface, void* surfaceInfo, void* region, bool block);
    int (* Surface_unlockAndPost)(void* surface);
    void (* Region_constructor)(void* region);
    void (* Region_set)(void* region, void* rect);

private:
    NativePanZoomController* mNativePanZoomController;
    
    
    
    nsTArray<DelayedTask*> mDelayedTaskQueue;

public:
    NativePanZoomController* SetNativePanZoomController(jobject obj);
    
    void RequestContentRepaint(const mozilla::layers::FrameMetrics& aFrameMetrics) MOZ_OVERRIDE;
    void HandleDoubleTap(const CSSIntPoint& aPoint) MOZ_OVERRIDE;
    void HandleSingleTap(const CSSIntPoint& aPoint) MOZ_OVERRIDE;
    void HandleLongTap(const CSSIntPoint& aPoint) MOZ_OVERRIDE;
    void SendAsyncScrollDOMEvent(mozilla::layers::FrameMetrics::ViewID aScrollId,
                                 const CSSRect& aContentRect,
                                 const CSSSize& aScrollableSize) MOZ_OVERRIDE;
    void PostDelayedTask(Task* aTask, int aDelayMs) MOZ_OVERRIDE;
    int64_t RunDelayedTasks();
};

class AutoJObject {
public:
    AutoJObject(JNIEnv* aJNIEnv = NULL) : mObject(NULL)
    {
        mJNIEnv = aJNIEnv ? aJNIEnv : AndroidBridge::GetJNIEnv();
    }

    AutoJObject(JNIEnv* aJNIEnv, jobject aObject)
    {
        mJNIEnv = aJNIEnv ? aJNIEnv : AndroidBridge::GetJNIEnv();
        mObject = aObject;
    }

    ~AutoJObject() {
        if (mObject)
            mJNIEnv->DeleteLocalRef(mObject);
    }

    jobject operator=(jobject aObject)
    {
        if (mObject) {
            mJNIEnv->DeleteLocalRef(mObject);
        }
        return mObject = aObject;
    }

    operator jobject() {
        return mObject;
    }
private:
    JNIEnv* mJNIEnv;
    jobject mObject;
};

class AutoLocalJNIFrame {
public:
    AutoLocalJNIFrame(int nEntries = 128)
        : mEntries(nEntries), mHasFrameBeenPushed(false)
    {
        mJNIEnv = AndroidBridge::GetJNIEnv();
        Push();
    }

    AutoLocalJNIFrame(JNIEnv* aJNIEnv, int nEntries = 128)
        : mEntries(nEntries), mHasFrameBeenPushed(false)
    {
        mJNIEnv = aJNIEnv ? aJNIEnv : AndroidBridge::GetJNIEnv();

        Push();
    }

    
    
    
    void Purge() {
        if (mJNIEnv) {
            if (mHasFrameBeenPushed)
                mJNIEnv->PopLocalFrame(NULL);
            Push();
        }
    }

    JNIEnv* GetEnv() {
        return mJNIEnv;
    }

    bool CheckForException() {
        if (mJNIEnv->ExceptionCheck()) {
            mJNIEnv->ExceptionDescribe();
            mJNIEnv->ExceptionClear();
            return true;
        }

        return false;
    }

    ~AutoLocalJNIFrame() {
        if (!mJNIEnv)
            return;

        CheckForException();

        if (mHasFrameBeenPushed)
            mJNIEnv->PopLocalFrame(NULL);
    }

private:
    void Push() {
        if (!mJNIEnv)
            return;

        
        
        
        jint ret = mJNIEnv->PushLocalFrame(mEntries + 1);
        NS_ABORT_IF_FALSE(ret == 0, "Failed to push local JNI frame");
        if (ret < 0)
            CheckForException();
        else
            mHasFrameBeenPushed = true;
    }

    int mEntries;
    JNIEnv* mJNIEnv;
    bool mHasFrameBeenPushed;
};

}

#define NS_ANDROIDBRIDGE_CID \
{ 0x0FE2321D, 0xEBD9, 0x467D, \
    { 0xA7, 0x43, 0x03, 0xA6, 0x8D, 0x40, 0x59, 0x9E } }

class nsAndroidBridge MOZ_FINAL : public nsIAndroidBridge
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
