






































#ifndef nsNPAPIPluginInstance_h_
#define nsNPAPIPluginInstance_h_

#include "nsCOMPtr.h"
#include "nsTArray.h"
#include "nsIPlugin.h"
#include "nsIPluginInstance.h"
#include "nsIPluginInstancePeer.h"
#include "nsIPluginTagInfo2.h"
#include "nsIPluginInstanceInternal.h"
#include "nsPIDOMWindow.h"

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

class nsNPAPIPluginInstance : public nsIPluginInstance,
                              public nsIPluginInstanceInternal
{
public:

    NS_DECL_ISUPPORTS
    NS_DECL_NSIPLUGININSTANCE

    

    virtual JSObject *GetJSObject(JSContext *cx);

    virtual nsresult GetFormValue(nsAString& aValue);

    virtual void PushPopupsEnabledState(PRBool aEnabled);
    virtual void PopPopupsEnabledState();

    virtual PRUint16 GetPluginAPIVersion();

    virtual void DefineJavaProperties();

    

    nsresult GetNPP(NPP * aNPP);

    
    nsresult GetCallbacks(const NPPluginFuncs ** aCallbacks);

    NPError SetWindowless(PRBool aWindowless);

    NPError SetTransparent(PRBool aTransparent);

    NPError SetWantsAllNetworkStreams(PRBool aWantsAllNetworkStreams);

#ifdef XP_MACOSX
    void SetDrawingModel(NPDrawingModel aModel);
    NPDrawingModel GetDrawingModel();
#endif

    nsresult NewNotifyStream(nsIPluginStreamListener** listener, 
                             void* notifyData, 
                             PRBool aCallNotify,
                             const char * aURL);

    nsNPAPIPluginInstance(NPPluginFuncs* callbacks, PRLibrary* aLibrary);

    
    virtual ~nsNPAPIPluginInstance(void);

    
    PRBool IsStarted(void);

    
    nsresult SetCached(PRBool aCache) { mCached = aCache; return NS_OK; }

    
    nsIPluginInstancePeer *Peer()
    {
        return mPeer;
    }

    already_AddRefed<nsPIDOMWindow> GetDOMWindow();

    nsresult PrivateModeStateChanged();
protected:

    nsresult InitializePlugin(nsIPluginInstancePeer* peer);

    
    nsresult GetValueInternal(NPPVariable variable, void* value);

    
    nsCOMPtr<nsIPluginInstancePeer> mPeer;

    
    
    
    NPPluginFuncs* fCallbacks;

    
    
    NPP_t fNPP;

#ifdef XP_MACOSX
    NPDrawingModel mDrawingModel;
#endif

    
    
    PRPackedBool  mWindowless;
    PRPackedBool  mTransparent;
    PRPackedBool  mStarted;
    PRPackedBool  mCached;
    PRPackedBool  mIsJavaPlugin;
    PRPackedBool  mWantsAllNetworkStreams;

public:
    
    PRPackedBool  mInPluginInitCall;
    PRLibrary* fLibrary;
    nsInstanceStream *mStreams;

    nsTArray<PopupControlState> mPopupStates;

    nsMIMEType mMIMEType;
};

#endif 
