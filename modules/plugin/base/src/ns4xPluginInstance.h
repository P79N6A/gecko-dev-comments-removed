






































#ifndef ns4xPluginInstance_h__
#define ns4xPluginInstance_h__

#define _UINT32



#ifndef HPUX11
#define _INT32
#endif 

#include "nsCOMPtr.h"
#include "nsVoidArray.h"
#include "nsIPlugin.h"
#include "nsIPluginInstance.h"
#include "nsIPluginInstancePeer.h"
#include "nsIPluginTagInfo2.h"
#include "nsIScriptablePlugin.h"
#include "nsIPluginInstanceInternal.h"

#include "npupp.h"
#include "prlink.h"  



class ns4xPluginStreamListener;
class nsPIDOMWindow;

struct nsInstanceStream
{
    nsInstanceStream *mNext;
    ns4xPluginStreamListener *mPluginStreamListener;

    nsInstanceStream();
    ~nsInstanceStream();
};

class ns4xPluginInstance : public nsIPluginInstance,
                           public nsIScriptablePlugin,
                           public nsIPluginInstanceInternal
{
public:

    NS_DECL_ISUPPORTS
    NS_DECL_NSIPLUGININSTANCE
    NS_DECL_NSISCRIPTABLEPLUGIN

    
    

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

#ifdef XP_MACOSX
    void SetDrawingModel(NPDrawingModel aModel);
    NPDrawingModel GetDrawingModel();
#endif

    nsresult NewNotifyStream(nsIPluginStreamListener** listener, 
                             void* notifyData, 
                             PRBool aCallNotify,
                             const char * aURL);

    



    ns4xPluginInstance(NPPluginFuncs* callbacks, PRLibrary* aLibrary);

    
    virtual ~ns4xPluginInstance(void);

    
    PRBool IsStarted(void);

    
    nsresult SetCached(PRBool aCache) { mCached = aCache; return NS_OK; }

    
    nsIPluginInstancePeer *Peer()
    {
        return mPeer;
    }

    already_AddRefed<nsPIDOMWindow> GetDOMWindow();

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

public:
    
    
    PRPackedBool  mInPluginInitCall;
    PRLibrary* fLibrary;
    nsInstanceStream *mStreams;

    nsVoidArray mPopupStates;
};

#endif 
