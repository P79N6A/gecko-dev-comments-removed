






































#ifndef nsNPAPIPluginInstance_h_
#define nsNPAPIPluginInstance_h_

#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsIPluginInstance.h"
#include "nsPIDOMWindow.h"
#include "nsITimer.h"
#include "nsIPluginTagInfo.h"
#include "nsIURI.h"
#include "nsIChannel.h"
#include "nsInterfaceHashtable.h"
#include "nsHashKeys.h"

#include "mozilla/TimeStamp.h"
#include "mozilla/PluginLibrary.h"

class nsPluginStreamListenerPeer; 
class nsNPAPIPluginStreamListener; 
class nsIPluginInstanceOwner;

class nsNPAPITimer
{
public:
  NPP npp;
  uint32_t id;
  nsCOMPtr<nsITimer> timer;
  void (*callback)(NPP npp, uint32_t timerID);
};

class nsNPAPIPluginInstance : public nsIPluginInstance
{
private:
  typedef mozilla::PluginLibrary PluginLibrary;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPLUGININSTANCE

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
  PRPackedBool mWantsAllNetworkStreams;
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
