




#ifndef nsNPAPIPluginInstance_h_
#define nsNPAPIPluginInstance_h_

#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsPIDOMWindow.h"
#include "nsITimer.h"
#include "nsIPluginTagInfo.h"
#include "nsIURI.h"
#include "nsIChannel.h"
#include "nsInterfaceHashtable.h"
#include "nsHashKeys.h"
#ifdef MOZ_WIDGET_ANDROID
#include "nsAutoPtr.h"
#include "nsIRunnable.h"
#include "GLContext.h"
#include "nsSurfaceTexture.h"
#include <map>
class PluginEventRunnable;
class SharedPluginTexture;
#endif

#include "mozilla/TimeStamp.h"
#include "mozilla/PluginLibrary.h"

class JSObject;

class nsPluginStreamListenerPeer; 
class nsNPAPIPluginStreamListener; 
class nsIPluginInstanceOwner;
class nsIOutputStream;
class nsPluginInstanceOwner;

#if defined(OS_WIN)
const NPDrawingModel kDefaultDrawingModel = NPDrawingModelSyncWin;
#elif defined(MOZ_X11)
const NPDrawingModel kDefaultDrawingModel = NPDrawingModelSyncX;
#elif defined(XP_MACOSX)
#ifndef NP_NO_QUICKDRAW
const NPDrawingModel kDefaultDrawingModel = NPDrawingModelQuickDraw; 
#else
const NPDrawingModel kDefaultDrawingModel = NPDrawingModelCoreGraphics;
#endif
#else
const NPDrawingModel kDefaultDrawingModel = static_cast<NPDrawingModel>(0);
#endif








enum NSPluginCallReentry {
  NS_PLUGIN_CALL_SAFE_TO_REENTER_GECKO,
  NS_PLUGIN_CALL_UNSAFE_TO_REENTER_GECKO
};

class nsNPAPITimer
{
public:
  NPP npp;
  uint32_t id;
  nsCOMPtr<nsITimer> timer;
  void (*callback)(NPP npp, uint32_t timerID);
  bool inCallback;
  bool needUnschedule;
};

class nsNPAPIPluginInstance : public nsISupports
{
private:
  typedef mozilla::PluginLibrary PluginLibrary;

public:
  NS_DECL_ISUPPORTS

  nsresult Initialize(nsNPAPIPlugin *aPlugin, nsPluginInstanceOwner* aOwner, const char* aMIMEType);
  nsresult Start();
  nsresult Stop();
  nsresult SetWindow(NPWindow* window);
  nsresult NewStreamFromPlugin(const char* type, const char* target, nsIOutputStream* *result);
  nsresult Print(NPPrint* platformPrint);
  nsresult HandleEvent(void* event, int16_t* result,
                       NSPluginCallReentry aSafeToReenterGecko = NS_PLUGIN_CALL_UNSAFE_TO_REENTER_GECKO);
  nsresult GetValueFromPlugin(NPPVariable variable, void* value);
  nsresult GetDrawingModel(int32_t* aModel);
  nsresult IsRemoteDrawingCoreAnimation(bool* aDrawing);
  nsresult ContentsScaleFactorChanged(double aContentsScaleFactor);
  nsresult GetJSObject(JSContext *cx, JSObject** outObject);
  bool ShouldCache();
  nsresult IsWindowless(bool* isWindowless);
  nsresult AsyncSetWindow(NPWindow* window);
  nsresult GetImageContainer(mozilla::layers::ImageContainer **aContainer);
  nsresult GetImageSize(nsIntSize* aSize);
  nsresult NotifyPainted(void);
  nsresult GetIsOOP(bool* aIsOOP);
  nsresult SetBackgroundUnknown();
  nsresult BeginUpdateBackground(nsIntRect* aRect, gfxContext** aContext);
  nsresult EndUpdateBackground(gfxContext* aContext, nsIntRect* aRect);
  nsresult IsTransparent(bool* isTransparent);
  nsresult GetFormValue(nsAString& aValue);
  nsresult PushPopupsEnabledState(bool aEnabled);
  nsresult PopPopupsEnabledState();
  nsresult GetPluginAPIVersion(uint16_t* version);
  nsresult InvalidateRect(NPRect *invalidRect);
  nsresult InvalidateRegion(NPRegion invalidRegion);
  nsresult GetMIMEType(const char* *result);
  nsresult GetJSContext(JSContext* *outContext);
  nsPluginInstanceOwner* GetOwner();
  void SetOwner(nsPluginInstanceOwner *aOwner);
  nsresult ShowStatus(const char* message);
#if defined(MOZ_WIDGET_QT) && (MOZ_PLATFORM_MAEMO == 6)
  nsresult HandleGUIEvent(const nsGUIEvent& anEvent, bool* handled);
#endif

  nsNPAPIPlugin* GetPlugin();

  nsresult GetNPP(NPP * aNPP);

  NPError SetWindowless(bool aWindowless);

  NPError SetTransparent(bool aTransparent);

  NPError SetWantsAllNetworkStreams(bool aWantsAllNetworkStreams);

  NPError SetUsesDOMForCursor(bool aUsesDOMForCursor);
  bool UsesDOMForCursor();

  void SetDrawingModel(NPDrawingModel aModel);
  void RedrawPlugin();
#ifdef XP_MACOSX
  void SetEventModel(NPEventModel aModel);
#endif

#ifdef MOZ_WIDGET_ANDROID
  void NotifyForeground(bool aForeground);
  void NotifyOnScreen(bool aOnScreen);
  void MemoryPressure();
  void NotifyFullScreen(bool aFullScreen);
  void NotifySize(nsIntSize size);

  nsIntSize CurrentSize() { return mCurrentSize; }

  bool IsOnScreen() {
    return mOnScreen;
  }

  uint32_t GetANPDrawingModel() { return mANPDrawingModel; }
  void SetANPDrawingModel(uint32_t aModel);

  void* GetJavaSurface();

  void PostEvent(void* event);

  
  
  uint32_t FullScreenOrientation() { return mFullScreenOrientation; }
  void SetFullScreenOrientation(uint32_t orientation);

  void SetWakeLock(bool aLock);

  mozilla::gl::GLContext* GLContext();
  
  
  class TextureInfo {
  public:
    TextureInfo() :
      mTexture(0), mWidth(0), mHeight(0), mInternalFormat(0)
    {
    }

    TextureInfo(GLuint aTexture, int32_t aWidth, int32_t aHeight, GLuint aInternalFormat) :
      mTexture(aTexture), mWidth(aWidth), mHeight(aHeight), mInternalFormat(aInternalFormat)
    {
    }

    GLuint mTexture;
    int32_t mWidth;
    int32_t mHeight;
    GLuint mInternalFormat;
  };

  TextureInfo LockContentTexture();
  void ReleaseContentTexture(TextureInfo& aTextureInfo);

  
  void* AcquireContentWindow();

  mozilla::gl::SharedTextureHandle CreateSharedHandle();

  
  class VideoInfo {
  public:
    VideoInfo(nsSurfaceTexture* aSurfaceTexture) :
      mSurfaceTexture(aSurfaceTexture)
    {
    }

    ~VideoInfo()
    {
      mSurfaceTexture = nullptr;
    }

    nsRefPtr<nsSurfaceTexture> mSurfaceTexture;
    gfxRect mDimensions;
  };

  void* AcquireVideoWindow();
  void ReleaseVideoWindow(void* aWindow);
  void SetVideoDimensions(void* aWindow, gfxRect aDimensions);

  void GetVideos(nsTArray<VideoInfo*>& aVideos);

  void SetInverted(bool aInverted);
  bool Inverted() { return mInverted; }

  static nsNPAPIPluginInstance* GetFromNPP(NPP npp);
#endif

  nsresult NewStreamListener(const char* aURL, void* notifyData,
                             nsNPAPIPluginStreamListener** listener);

  nsNPAPIPluginInstance();
  virtual ~nsNPAPIPluginInstance();

  
  
  void Destroy();

  
  bool IsRunning() {
    return RUNNING == mRunning;
  }
  bool HasStartedDestroying() {
    return mRunning >= DESTROYING;
  }

  
  bool CanFireNotifications() {
    return mRunning == RUNNING || mRunning == DESTROYING;
  }

  
  mozilla::TimeStamp StopTime();

  
  void SetCached(bool aCache);

  already_AddRefed<nsPIDOMWindow> GetDOMWindow();

  nsresult PrivateModeStateChanged(bool aEnabled);

  nsresult IsPrivateBrowsing(bool *aEnabled);

  nsresult GetDOMElement(nsIDOMElement* *result);

  nsNPAPITimer* TimerWithID(uint32_t id, uint32_t* index);
  uint32_t      ScheduleTimer(uint32_t interval, NPBool repeat, void (*timerFunc)(NPP npp, uint32_t timerID));
  void          UnscheduleTimer(uint32_t timerID);
  NPError       PopUpContextMenu(NPMenu* menu);
  NPBool        ConvertPoint(double sourceX, double sourceY, NPCoordinateSpace sourceSpace, double *destX, double *destY, NPCoordinateSpace destSpace);


  nsTArray<nsNPAPIPluginStreamListener*> *StreamListeners();

  nsTArray<nsPluginStreamListenerPeer*> *FileCachedStreamListeners();

  nsresult AsyncSetWindow(NPWindow& window);

  void URLRedirectResponse(void* notifyData, NPBool allow);

  NPError InitAsyncSurface(NPSize *size, NPImageFormat format,
                           void *initData, NPAsyncSurface *surface);
  NPError FinalizeAsyncSurface(NPAsyncSurface *surface);
  void SetCurrentAsyncSurface(NPAsyncSurface *surface, NPRect *changed);

  
  
  void CarbonNPAPIFailure();

  
  double GetContentsScaleFactor();

  static bool InPluginCallUnsafeForReentry() { return gInUnsafePluginCalls > 0; }
  static void BeginPluginCall(NSPluginCallReentry aReentryState)
  {
    if (aReentryState == NS_PLUGIN_CALL_UNSAFE_TO_REENTER_GECKO) {
      ++gInUnsafePluginCalls;
    }
  }
  static void EndPluginCall(NSPluginCallReentry aReentryState)
  {
    if (aReentryState == NS_PLUGIN_CALL_UNSAFE_TO_REENTER_GECKO) {
      NS_ASSERTION(gInUnsafePluginCalls > 0, "Must be in plugin call");
      --gInUnsafePluginCalls;
    }
  }

protected:

  nsresult GetTagType(nsPluginTagType *result);
  nsresult GetAttributes(uint16_t& n, const char*const*& names,
                         const char*const*& values);
  nsresult GetParameters(uint16_t& n, const char*const*& names,
                         const char*const*& values);
  nsresult GetMode(int32_t *result);

  
  void CheckJavaC2PJSObjectQuirk(uint16_t paramCount,
                                 const char* const* names,
                                 const char* const* values);

  
  
  NPP_t mNPP;

  NPDrawingModel mDrawingModel;

#ifdef MOZ_WIDGET_ANDROID
  uint32_t mANPDrawingModel;

  friend class PluginEventRunnable;

  nsTArray<nsCOMPtr<PluginEventRunnable>> mPostedEvents;
  void PopPostedEvent(PluginEventRunnable* r);
  void OnSurfaceTextureFrameAvailable();

  uint32_t mFullScreenOrientation;
  bool mWakeLocked;
  bool mFullScreen;
  bool mInverted;

  nsRefPtr<SharedPluginTexture> mContentTexture;
  nsRefPtr<nsSurfaceTexture> mContentSurface;
#endif

  enum {
    NOT_STARTED,
    RUNNING,
    DESTROYING,
    DESTROYED
  } mRunning;

  
  
  bool mWindowless;
  bool mTransparent;
  bool mCached;
  bool mUsesDOMForCursor;

public:
  
  bool mInPluginInitCall;

  nsXPIDLCString mFakeURL;

private:
  nsNPAPIPlugin* mPlugin;

  nsTArray<nsNPAPIPluginStreamListener*> mStreamListeners;

  nsTArray<nsPluginStreamListenerPeer*> mFileCachedStreamListeners;

  nsTArray<PopupControlState> mPopupStates;

  char* mMIMEType;

  
  
  nsPluginInstanceOwner *mOwner;

  nsTArray<nsNPAPITimer*> mTimers;

  
  void* mCurrentPluginEvent;

  
  
  mozilla::TimeStamp mStopTime;

#ifdef MOZ_WIDGET_ANDROID
  void EnsureSharedTexture();
  nsSurfaceTexture* CreateSurfaceTexture();

  std::map<void*, VideoInfo*> mVideos;
  bool mOnScreen;

  nsIntSize mCurrentSize;
#endif

  
  bool mHaveJavaC2PJSObjectQuirk;

  static uint32_t gInUnsafePluginCalls;
};

#endif 
