






































#ifndef nsNPAPIPluginInstance_h_
#define nsNPAPIPluginInstance_h_

#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsIPlugin.h"
#include "nsIPluginInstance.h"
#include "nsIPluginTagInfo.h"
#include "nsPIDOMWindow.h"
#include "nsIPluginInstanceOwner.h"
#include "nsITimer.h"

#include "npfunctions.h"
#include "prlink.h"

class nsNPAPIPluginStreamListener;
class nsPIDOMWindow;

struct nsInstanceStream
{
  nsInstanceStream *mNext;
  nsNPAPIPluginStreamListener *mPluginStreamListener;

  nsInstanceStream();
  ~nsInstanceStream();
};

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

  nsNPAPIPluginInstance(NPPluginFuncs* callbacks, PRLibrary* aLibrary);

  
  virtual ~nsNPAPIPluginInstance();

  
  PRBool IsStarted();

  
  nsresult SetCached(PRBool aCache) { mCached = aCache; return NS_OK; }

  already_AddRefed<nsPIDOMWindow> GetDOMWindow();

  nsresult PrivateModeStateChanged();

  nsresult GetDOMElement(nsIDOMElement* *result);

  nsNPAPITimer* TimerWithID(uint32_t id, PRUint32* index);
  uint32_t      ScheduleTimer(uint32_t interval, NPBool repeat, void (*timerFunc)(NPP npp, uint32_t timerID));
  void          UnscheduleTimer(uint32_t timerID);
  NPError       PopUpContextMenu(NPMenu* menu);
  NPBool        ConvertPoint(double sourceX, double sourceY, NPCoordinateSpace sourceSpace, double *destX, double *destY, NPCoordinateSpace destSpace);
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

  
  
  PRPackedBool mWindowless;
  PRPackedBool mWindowlessLocal;
  PRPackedBool mTransparent;
  PRPackedBool mStarted;
  PRPackedBool mCached;
  PRPackedBool mWantsAllNetworkStreams;

public:
  
  PRPackedBool mInPluginInitCall;
  PRLibrary* mLibrary;
  nsInstanceStream *mStreams;

private:
  nsTArray<PopupControlState> mPopupStates;

  char* mMIMEType;

  
  
  nsIPluginInstanceOwner *mOwner;

  nsTArray<nsNPAPITimer*> mTimers;

  
  void* mCurrentPluginEvent;
};

#endif 
