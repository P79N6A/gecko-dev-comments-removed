






































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
  nsresult HandleEvent(void* event, PRInt16* result);
  nsresult GetValueFromPlugin(NPPVariable variable, void* value);
  nsresult GetDrawingModel(PRInt32* aModel);
  nsresult IsRemoteDrawingCoreAnimation(PRBool* aDrawing);
  nsresult GetJSObject(JSContext *cx, JSObject** outObject);
  nsresult DefineJavaProperties();
  nsresult ShouldCache(PRBool* shouldCache);
  nsresult IsWindowless(PRBool* isWindowless);
  nsresult AsyncSetWindow(NPWindow* window);
  nsresult GetImage(ImageContainer* aContainer, Image** aImage);
  nsresult GetImageSize(nsIntSize* aSize);
  nsresult NotifyPainted(void);
  nsresult UseAsyncPainting(PRBool* aIsAsync);
  nsresult SetBackgroundUnknown();
  nsresult BeginUpdateBackground(nsIntRect* aRect, gfxContext** aContext);
  nsresult EndUpdateBackground(gfxContext* aContext, nsIntRect* aRect);
  nsresult IsTransparent(PRBool* isTransparent);
  nsresult GetFormValue(nsAString& aValue);
  nsresult PushPopupsEnabledState(PRBool aEnabled);
  nsresult PopPopupsEnabledState();
  nsresult GetPluginAPIVersion(PRUint16* version);
  nsresult InvalidateRect(NPRect *invalidRect);
  nsresult InvalidateRegion(NPRegion invalidRegion);
  nsresult ForceRedraw();
  nsresult GetMIMEType(const char* *result);
  nsresult GetJSContext(JSContext* *outContext);
  nsresult GetOwner(nsIPluginInstanceOwner **aOwner);
  nsresult SetOwner(nsIPluginInstanceOwner *aOwner);
  nsresult ShowStatus(const char* message);
  nsresult InvalidateOwner();

  nsNPAPIPlugin* GetPlugin();

  nsresult GetNPP(NPP * aNPP);

  void SetURI(nsIURI* uri);
  nsIURI* GetURI();

  NPError SetWindowless(PRBool aWindowless);

  NPError SetWindowlessLocal(PRBool aWindowlessLocal);

  NPError SetTransparent(PRBool aTransparent);

  NPError SetWantsAllNetworkStreams(PRBool aWantsAllNetworkStreams);

  NPError SetUsesDOMForCursor(PRBool aUsesDOMForCursor);
  PRBool UsesDOMForCursor();

#ifdef XP_MACOSX
  void SetDrawingModel(NPDrawingModel aModel);
  void SetEventModel(NPEventModel aModel);
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

  
  mozilla::TimeStamp LastStopTime();

  
  nsresult SetCached(PRBool aCache);

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

  enum {
    NOT_STARTED,
    RUNNING,
    DESTROYING,
    DESTROYED
  } mRunning;

  
  
  PRPackedBool mWindowless;
  PRPackedBool mWindowlessLocal;
  PRPackedBool mTransparent;
  PRPackedBool mCached;
  PRPackedBool mUsesDOMForCursor;

public:
  
  PRPackedBool mInPluginInitCall;

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

  PRPackedBool mUsePluginLayersPref;
};

#endif 
