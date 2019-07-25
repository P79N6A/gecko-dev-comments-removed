






































#ifndef nsNPAPIPluginInstance_h_
#define nsNPAPIPluginInstance_h_

#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsIPluginInstance.h"
#include "nsPIDOMWindow.h"
#include "nsITimer.h"
#include "nsIPluginTagInfo.h"

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

  nsresult GetNPP(NPP * aNPP);

  
  nsresult GetCallbacks(const NPPluginFuncs ** aCallbacks);

  NPError SetWindowless(PRBool aWindowless);

  NPError SetWindowlessLocal(PRBool aWindowlessLocal);

  NPError SetTransparent(PRBool aTransparent);

  NPError SetWantsAllNetworkStreams(PRBool aWantsAllNetworkStreams);

#ifdef XP_MACOSX
  void SetDrawingModel(NPDrawingModel aModel);
  void SetEventModel(NPEventModel aModel);
#endif

  nsresult NewNotifyStream(nsIPluginStreamListener** listener, 
                           void* notifyData, 
                           PRBool aCallNotify,
                           const char * aURL);

  nsNPAPIPluginInstance(NPPluginFuncs* callbacks, PluginLibrary* aLibrary);

  
  virtual ~nsNPAPIPluginInstance();

  
  bool IsRunning() {
    return RUNNING == mRunning;
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

  
  nsTArray<nsNPAPIPluginStreamListener*> *PStreamListeners();
  
  nsTArray<nsPluginStreamListenerPeer*> *BStreamListeners();

protected:
  nsresult InitializePlugin();

  nsresult GetTagType(nsPluginTagType *result);
  nsresult GetAttributes(PRUint16& n, const char*const*& names,
                         const char*const*& values);
  nsresult GetParameters(PRUint16& n, const char*const*& names,
                         const char*const*& values);
  nsresult GetMode(PRInt32 *result);

  
  
  
  NPPluginFuncs* mCallbacks;

  
  
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

public:
  
  PRPackedBool mInPluginInitCall;
  PluginLibrary* mLibrary;

private:
  
  nsTArray<nsNPAPIPluginStreamListener*> mPStreamListeners;
  
  
  nsTArray<nsPluginStreamListenerPeer*> mBStreamListeners;

  nsTArray<PopupControlState> mPopupStates;

  char* mMIMEType;

  
  
  nsIPluginInstanceOwner *mOwner;

  nsTArray<nsNPAPITimer*> mTimers;

  
  void* mCurrentPluginEvent;

  
  
  mozilla::TimeStamp mStopTime;
};

#endif 
