




#ifndef AndroidBridge_h__
#define AndroidBridge_h__

#include <jni.h>
#include <android/log.h>
#include <cstdlib>
#include <pthread.h>

#include "nsCOMPtr.h"
#include "nsCOMArray.h"

#include "GeneratedJNIWrappers.h"
#include "AndroidJavaWrappers.h"

#include "nsIMutableArray.h"
#include "nsIMIMEInfo.h"
#include "nsColor.h"
#include "gfxRect.h"

#include "nsIAndroidBridge.h"
#include "nsIMobileMessageCallback.h"

#include "mozilla/Likely.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Types.h"
#include "mozilla/jni/Utils.h"





class nsWindow;
class nsIDOMMozSmsMessage;
class nsIObserver;
class Task;

extern bool mozilla_AndroidBridge_SetMainThread(pthread_t);

namespace base {
class Thread;
} 

typedef void* EGLSurface;

namespace mozilla {

namespace hal {
class BatteryInformation;
class NetworkInformation;
} 

namespace dom {
namespace mobilemessage {
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
        mRunTime = mozilla::TimeStamp::Now() + mozilla::TimeDuration::FromMilliseconds(aDelayMs);
    }

    bool IsEarlierThan(DelayedTask *aOther) {
        return mRunTime < aOther->mRunTime;
    }

    int64_t MillisecondsToRunTime() {
        mozilla::TimeDuration timeLeft = mRunTime - mozilla::TimeStamp::Now();
        return (int64_t)timeLeft.ToMilliseconds();
    }

    Task* GetTask() {
        return mTask;
    }

private:
    Task* mTask;
    mozilla::TimeStamp mRunTime;
};

class AndroidBridge final
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

    static void RegisterJavaUiThread() {
        sJavaUiThread = pthread_self();
    }

    static bool IsJavaUiThread() {
        return pthread_equal(pthread_self(), sJavaUiThread);
    }

    static void ConstructBridge(JNIEnv *jEnv, jni::Object::Param clsLoader);

    static AndroidBridge *Bridge() {
        return sBridge;
    }

    static JavaVM *GetVM() {
        MOZ_ASSERT(sBridge);
        return sBridge->mJavaVM;
    }


    static JNIEnv *GetJNIEnv() {
        MOZ_ASSERT(sBridge);
        if (MOZ_UNLIKELY(!pthread_equal(pthread_self(), sBridge->mThread))) {
            MOZ_CRASH();
        }
        MOZ_ASSERT(sBridge->mJNIEnv);
        return sBridge->mJNIEnv;
    }

    static bool HasEnv() {
        return sBridge && sBridge->mJNIEnv;
    }

    static bool ThrowException(JNIEnv *aEnv, const char *aClass,
                               const char *aMessage) {

        return jni::ThrowException(aEnv, aClass, aMessage);
    }

    static bool ThrowException(JNIEnv *aEnv, const char *aMessage) {
        return jni::ThrowException(aEnv, aMessage);
    }

    static void HandleUncaughtException(JNIEnv *aEnv) {
        jni::HandleUncaughtException(aEnv);
    }

    
    
    
    
    
    bool SetMainThread(pthread_t thr);

    
    bool GetThreadNameJavaProfiling(uint32_t aThreadId, nsCString & aResult);
    bool GetFrameNameJavaProfiling(uint32_t aThreadId, uint32_t aSampleId, uint32_t aFrameId, nsCString & aResult);

    nsresult CaptureZoomedView(nsIDOMWindow *window, nsIntRect zoomedViewRect, jni::Object::Param buffer, float zoomFactor);
    nsresult CaptureThumbnail(nsIDOMWindow *window, int32_t bufW, int32_t bufH, int32_t tabId, jni::Object::Param buffer, bool &shouldStore);
    void GetDisplayPort(bool aPageSizeUpdate, bool aIsBrowserContentDisplayed, int32_t tabId, nsIAndroidViewport* metrics, nsIAndroidDisplayport** displayPort);
    void ContentDocumentChanged();
    bool IsContentDocumentDisplayed();

    bool ProgressiveUpdateCallback(bool aHasPendingNewThebesContent, const LayerRect& aDisplayPort, float aDisplayResolution, bool aDrawingCritical,
                                   mozilla::ParentLayerPoint& aScrollOffset, mozilla::CSSToParentLayerScale& aZoom);

    void SetLayerClient(widget::GeckoLayerClient::Param jobj);
    const widget::GeckoLayerClient::Ref& GetLayerClient() { return mLayerClient; }

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

    void Vibrate(const nsTArray<uint32_t>& aPattern);

    void GetSystemColors(AndroidSystemColors *aColors);

    void GetIconForExtension(const nsACString& aFileExt, uint32_t aIconSize, uint8_t * const aBuf);

    
    void RegisterCompositor(JNIEnv* env = nullptr);
    EGLSurface CreateEGLSurfaceForCompositor();

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

    void HandleGeckoMessage(JSContext* cx, JS::HandleObject message);

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
                          bool aLayersUpdated, ParentLayerPoint& aScrollOffset, CSSToParentLayerScale& aScale,
                          LayerMargin& aFixedLayerMargins, ScreenPoint& aOffset);
    void SyncFrameMetrics(const ParentLayerPoint& aScrollOffset, float aZoom, const CSSRect& aCssPageRect,
                          bool aLayersUpdated, const CSSRect& aDisplayPort, const CSSToLayerScale& aDisplayResolution,
                          bool aIsFirstPaint, LayerMargin& aFixedLayerMargins, ScreenPoint& aOffset);

    void AddPluginView(jobject view, const LayoutDeviceRect& rect, bool isFullScreen);

    
    
    
    
    uint32_t GetScreenOrientation();

    int GetAPIVersion() { return mAPIVersion; }
    bool IsHoneycomb() { return mAPIVersion >= 11 && mAPIVersion <= 13; }

    void ScheduleComposite();

    nsresult GetProxyForURI(const nsACString & aSpec,
                            const nsACString & aScheme,
                            const nsACString & aHost,
                            const int32_t      aPort,
                            nsACString & aResult);

    
    static jstring NewJavaString(JNIEnv* env, const char16_t* string, uint32_t len);
    static jstring NewJavaString(JNIEnv* env, const nsAString& string);
    static jstring NewJavaString(JNIEnv* env, const char* string);
    static jstring NewJavaString(JNIEnv* env, const nsACString& string);

    static jstring NewJavaString(AutoLocalJNIFrame* frame, const char16_t* string, uint32_t len);
    static jstring NewJavaString(AutoLocalJNIFrame* frame, const nsAString& string);
    static jstring NewJavaString(AutoLocalJNIFrame* frame, const char* string);
    static jstring NewJavaString(AutoLocalJNIFrame* frame, const nsACString& string);

    static jclass GetClassGlobalRef(JNIEnv* env, const char* className);
    static jfieldID GetFieldID(JNIEnv* env, jclass jClass, const char* fieldName, const char* fieldType);
    static jfieldID GetStaticFieldID(JNIEnv* env, jclass jClass, const char* fieldName, const char* fieldType);
    static jmethodID GetMethodID(JNIEnv* env, jclass jClass, const char* methodName, const char* methodType);
    static jmethodID GetStaticMethodID(JNIEnv* env, jclass jClass, const char* methodName, const char* methodType);

    static jni::Object::LocalRef ChannelCreate(jni::Object::Param);

    static void InputStreamClose(jni::Object::Param obj);
    static uint32_t InputStreamAvailable(jni::Object::Param obj);
    static nsresult InputStreamRead(jni::Object::Param obj, char *aBuf, uint32_t aCount, uint32_t *aRead);

    static nsresult GetExternalPublicDirectory(const nsAString& aType, nsAString& aPath);

protected:
    static pthread_t sJavaUiThread;
    static AndroidBridge* sBridge;
    nsTArray<nsCOMPtr<nsIMobileMessageCallback> > mSmsRequests;

    
    JavaVM *mJavaVM;

    
    JNIEnv *mJNIEnv;
    pthread_t mThread;

    widget::GeckoLayerClient::GlobalRef mLayerClient;

    
    jclass mAndroidSmsMessageClass;

    AndroidBridge();
    ~AndroidBridge();

    void InitStubs(JNIEnv *jEnv);
    void Init(JNIEnv *jEnv, jni::Object::Param clsLoader);

    bool mOpenedGraphicsLibraries;
    void OpenGraphicsLibraries();
    void* GetNativeSurface(JNIEnv* env, jobject surface);

    bool mHasNativeBitmapAccess;
    bool mHasNativeWindowAccess;
    bool mHasNativeWindowFallback;

    int mAPIVersion;

    bool QueueSmsRequest(nsIMobileMessageCallback* aRequest, uint32_t* aRequestIdOut);

    
    jclass jReadableByteChannel;
    jclass jChannels;
    jmethodID jChannelCreate;
    jmethodID jByteBufferRead;

    jclass jInputStream;
    jmethodID jClose;
    jmethodID jAvailable;

    
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
    widget::GLController::GlobalRef mGLControllerObj;

    
    jclass jStringClass;

    jni::Object::GlobalRef mClassLoader;
    jmethodID mClassLoaderLoadClass;

    
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
    
    
    nsTArray<DelayedTask*> mDelayedTaskQueue;
public:
    void PostTaskToUiThread(Task* aTask, int aDelayMs);
    int64_t RunDelayedUiThreadTasks();
};

class AutoJNIClass {
private:
    JNIEnv* const mEnv;
    const jclass mClass;

public:
    AutoJNIClass(JNIEnv* jEnv, const char* name)
        : mEnv(jEnv)
        , mClass(AndroidBridge::GetClassGlobalRef(jEnv, name))
    {}

    ~AutoJNIClass() {
        mEnv->DeleteGlobalRef(mClass);
    }

    jclass getRawRef() const {
        return mClass;
    }

    jclass getGlobalRef() const {
        return static_cast<jclass>(mEnv->NewGlobalRef(mClass));
    }

    jfieldID getField(const char* name, const char* type) const {
        return AndroidBridge::GetFieldID(mEnv, mClass, name, type);
    }

    jfieldID getStaticField(const char* name, const char* type) const {
        return AndroidBridge::GetStaticFieldID(mEnv, mClass, name, type);
    }

    jmethodID getMethod(const char* name, const char* type) const {
        return AndroidBridge::GetMethodID(mEnv, mClass, name, type);
    }

    jmethodID getStaticMethod(const char* name, const char* type) const {
        return AndroidBridge::GetStaticMethodID(mEnv, mClass, name, type);
    }
};

class AutoJObject {
public:
    AutoJObject(JNIEnv* aJNIEnv = nullptr) : mObject(nullptr)
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
    AutoLocalJNIFrame(int nEntries = 15)
        : mEntries(nEntries)
        , mJNIEnv(AndroidBridge::GetJNIEnv())
        , mHasFrameBeenPushed(false)
    {
        MOZ_ASSERT(mJNIEnv);
        Push();
    }

    AutoLocalJNIFrame(JNIEnv* aJNIEnv, int nEntries = 15)
        : mEntries(nEntries)
        , mJNIEnv(aJNIEnv ? aJNIEnv : AndroidBridge::GetJNIEnv())
        , mHasFrameBeenPushed(false)
    {
        MOZ_ASSERT(mJNIEnv);
        Push();
    }

    ~AutoLocalJNIFrame() {
        if (mHasFrameBeenPushed) {
            Pop();
        }
    }

    JNIEnv* GetEnv() {
        return mJNIEnv;
    }

    bool CheckForException() {
        if (mJNIEnv->ExceptionCheck()) {
            jni::HandleUncaughtException(mJNIEnv);
            return true;
        }
        return false;
    }

    
    
    
    void Purge() {
        Pop();
        Push();
    }

    template <typename ReturnType = jobject>
    ReturnType Pop(ReturnType aResult = nullptr) {
        MOZ_ASSERT(mHasFrameBeenPushed);
        mHasFrameBeenPushed = false;
        return static_cast<ReturnType>(
            mJNIEnv->PopLocalFrame(static_cast<jobject>(aResult)));
    }

private:
    void Push() {
        MOZ_ASSERT(!mHasFrameBeenPushed);
        
        
        
        if (mJNIEnv->PushLocalFrame(mEntries + 1) != 0) {
            CheckForException();
            return;
        }
        mHasFrameBeenPushed = true;
    }

    const int mEntries;
    JNIEnv* const mJNIEnv;
    bool mHasFrameBeenPushed;
};

}

#define NS_ANDROIDBRIDGE_CID \
{ 0x0FE2321D, 0xEBD9, 0x467D, \
    { 0xA7, 0x43, 0x03, 0xA6, 0x8D, 0x40, 0x59, 0x9E } }

class nsAndroidBridge final : public nsIAndroidBridge
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
