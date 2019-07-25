






































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
#include "nsIRunnable.h"
#endif

#include "mozilla/TimeStamp.h"
#include "mozilla/PluginLibrary.h"

struct JSObject;

class nsPluginStreamListenerPeer; 
class nsNPAPIPluginStreamListener; 
class nsIPluginInstanceOwner;
class nsIPluginStreamListener;
class nsIOutputStream;

class nsNPAPITimer
{
public:
  NPP npp;
  uint32_t id;
  nsCOMPtr<nsITimer> timer;
  void (*callback)(NPP npp, uint32_t timerID);
  bool inCallback;
};

class nsNPAPIPluginInstance : public nsISupports
{
private:
  typedef mozilla::PluginLibrary PluginLibrary;

public:
  NS_DECL_ISUPPORTS

  nsresult Initialize(nsIPluginInstanceOwner* aOwner, const char* aMIMEType);
  nsresult Start();
  nsresult Stop();
  nsresult SetWindow(NPWindow* window);
  nsresult NewStreamToPlugin(nsIPluginStreamListener** listener);
  nsresult NewStreamFromPlugin(const char* type, const char* target, nsIOutputStream* *result);
  nsresult Print(NPPrint* platformPrint);
#ifdef MOZ_WIDGET_ANDROID
  nsresult PostEvent(void* event) { return 0; };
#endif
  nsresult HandleEvent(void* event, PRInt16* result);
  nsresult GetValueFromPlugin(NPPVariable variable, void* value);
  nsresult GetDrawingModel(PRInt32* aModel);
  nsresult IsRemoteDrawingCoreAnimation(bool* aDrawing);
  nsresult GetJSObject(JSContext *cx, JSObject** outObject);
  nsresult DefineJavaProperties();
  bool ShouldCache();
  nsresult IsWindowless(bool* isWindowless);
  nsresult AsyncSetWindow(NPWindow* window);
  nsresult GetImageContainer(ImageContainer **aContainer);
  nsresult GetImageSize(nsIntSize* aSize);
  nsresult NotifyPainted(void);
  nsresult UseAsyncPainting(bool* aIsAsync);
  nsresult SetBackgroundUnknown();
  nsresult BeginUpdateBackground(nsIntRect* aRect, gfxContext** aContext);
  nsresult EndUpdateBackground(gfxContext* aContext, nsIntRect* aRect);
  nsresult IsTransparent(bool* isTransparent);
  nsresult GetFormValue(nsAString& aValue);
  nsresult PushPopupsEnabledState(bool aEnabled);
  nsresult PopPopupsEnabledState();
  nsresult GetPluginAPIVersion(PRUint16* version);
  nsresult InvalidateRect(NPRect *invalidRect);
  nsresult InvalidateRegion(NPRegion invalidRegion);
  nsresult GetMIMEType(const char* *result);
  nsresult GetJSContext(JSContext* *outContext);
  nsresult GetOwner(nsIPluginInstanceOwner **aOwner);
  nsresult SetOwner(nsIPluginInstanceOwner *aOwner);
  nsresult ShowStatus(const char* message);
  nsresult InvalidateOwner();
#if defined(MOZ_WIDGET_QT) && (MOZ_PLATFORM_MAEMO == 6)
  nsresult HandleGUIEvent(const nsGUIEvent& anEvent, bool* handled);
#endif

  nsNPAPIPlugin* GetPlugin();

  nsresult GetNPP(NPP * aNPP);

  void SetURI(nsIURI* uri);
  nsIURI* GetURI();

  NPError SetWindowless(bool aWindowless);

  NPError SetTransparent(bool aTransparent);

  NPError SetWantsAllNetworkStreams(bool aWantsAllNetworkStreams);

  NPError SetUsesDOMForCursor(bool aUsesDOMForCursor);
  bool UsesDOMForCursor();

#ifdef XP_MACOSX
  void SetDrawingModel(NPDrawingModel aModel);
  void SetEventModel(NPEventModel aModel);
#endif

#ifdef MOZ_WIDGET_ANDROID
  void NotifyForeground(bool aForeground);
  void NotifyOnScreen(bool aOnScreen);
  void MemoryPressure();

  bool IsOnScreen() {
    return mOnScreen;
  }

  PRUint32 GetANPDrawingModel() { return mANPDrawingModel; }
  void SetANPDrawingModel(PRUint32 aModel);

  
  void* GetJavaSurface();
  void SetJavaSurface(void* aSurface);
  void RequestJavaSurface();
#endif

  nsresult NewStreamListener(const char* aURL, void* notifyData,
                             nsIPluginStreamListener** listener);

  nsNPAPIPluginInstance(nsNPAPIPlugin* plugin);
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

  
  nsresult SetCached(bool aCache);

  already_AddRefed<nsPIDOMWindow> GetDOMWindow();

  nsresult PrivateModeStateChanged();

  nsresult GetDOMElement(nsIDOMElement* *result);

  nsNPAPITimer* TimerWithID(uint32_t id, PRUint32* index);
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

protected:
  nsresult InitializePlugin();

  nsresult GetTagType(nsPluginTagType *result);
  nsresult GetAttributes(PRUint16& n, const char*const*& names,
                         const char*const*& values);
  nsresult GetParameters(PRUint16& n, const char*const*& names,
                         const char*const*& values);
  nsresult GetMode(PRInt32 *result);

  
  
  NPP_t mNPP;

#ifdef XP_MACOSX
  NPDrawingModel mDrawingModel;
#endif

#ifdef MOZ_WIDGET_ANDROID
  PRUint32 mANPDrawingModel;
  nsCOMPtr<nsIRunnable> mSurfaceGetter;
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

  
  
  nsIPluginInstanceOwner *mOwner;

  nsTArray<nsNPAPITimer*> mTimers;

  
  void* mCurrentPluginEvent;

  
  
  mozilla::TimeStamp mStopTime;

  nsCOMPtr<nsIURI> mURI;

  bool mUsePluginLayersPref;
#ifdef MOZ_WIDGET_ANDROID
  void* mSurface;
  bool mOnScreen;
#endif
};

#endif 
