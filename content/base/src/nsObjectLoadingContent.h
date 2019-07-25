











#ifndef NSOBJECTLOADINGCONTENT_H_
#define NSOBJECTLOADINGCONTENT_H_

#include "nsImageLoadingContent.h"
#include "nsIStreamListener.h"
#include "nsFrameLoader.h"
#include "nsIInterfaceRequestor.h"
#include "nsIChannelEventSink.h"
#include "nsIObjectLoadingContent.h"
#include "nsIRunnable.h"
#include "nsIFrame.h"
#include "nsPluginInstanceOwner.h"
#include "nsIThreadInternal.h"

class nsAsyncInstantiateEvent;
class nsStopPluginRunnable;
class AutoNotifier;
class AutoFallback;
class AutoSetInstantiatingToFalse;
class nsObjectFrame;

enum PluginSupportState {
  ePluginUnsupported,  
  ePluginDisabled,     
  ePluginBlocklisted,  
  ePluginOutdated,     
  ePluginOtherState,   
  ePluginCrashed,
  ePluginClickToPlay,  
  ePluginVulnerableUpdatable, 
  ePluginVulnerableNoUpdate   
};

class nsObjectLoadingContent : public nsImageLoadingContent
                             , public nsIStreamListener
                             , public nsIFrameLoaderOwner
                             , public nsIObjectLoadingContent
                             , public nsIInterfaceRequestor
                             , public nsIChannelEventSink
{
  friend class AutoNotifier;
  friend class AutoFallback;
  friend class AutoSetInstantiatingToFalse;
  friend class AutoSetLoadingToFalse;
  friend class InDocCheckEvent;
  friend class nsStopPluginRunnable;
  friend class nsAsyncInstantiateEvent;

  public:
    
    
    enum ObjectType {
      
      eType_Loading        = TYPE_LOADING,
      
      eType_Image          = TYPE_IMAGE,
      
      eType_Plugin         = TYPE_PLUGIN,
      
      eType_Document       = TYPE_DOCUMENT,
      
      
      eType_Null           = TYPE_NULL
    };

    nsObjectLoadingContent();
    virtual ~nsObjectLoadingContent();

    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIFRAMELOADEROWNER
    NS_DECL_NSIOBJECTLOADINGCONTENT
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSICHANNELEVENTSINK

#ifdef HAVE_CPP_AMBIGUITY_RESOLVING_USING
    
    using nsImageLoadingContent::OnStartRequest;
    using nsImageLoadingContent::OnDataAvailable;
    using nsImageLoadingContent::OnStopRequest;
#endif

    



    nsEventStates ObjectState() const;

    ObjectType Type() { return mType; }

    void SetIsNetworkCreated(bool aNetworkCreated)
    {
      mNetworkCreated = aNetworkCreated;
    }

    



    nsresult InstantiatePluginInstance();

    



    void NotifyOwnerDocumentActivityChanged();

    



    bool SrcStreamLoading() { return mSrcStreamLoading; };

  protected:
    

































    nsresult LoadObject(bool aNotify,
                        bool aForceLoad = false);

    enum Capabilities {
      eSupportImages       = PR_BIT(0), 
      eSupportPlugins      = PR_BIT(1), 
      eSupportDocuments    = PR_BIT(2), 
                                        
                                        
      eSupportSVG          = PR_BIT(3), 
      eSupportClassID      = PR_BIT(4), 

      
      
      
      
      
      
      eAllowPluginSkipChannel  = PR_BIT(5)
    };

    






    virtual PRUint32 GetCapabilities() const;

    





    void Fallback(bool aNotify);

    


    void DestroyContent();

    static void Traverse(nsObjectLoadingContent *tmp,
                         nsCycleCollectionTraversalCallback &cb);

    void CreateStaticClone(nsObjectLoadingContent* aDest) const;

    void DoStopPlugin(nsPluginInstanceOwner* aInstanceOwner, bool aDelayedStop,
                      bool aForcedReentry = false);

    nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                        nsIContent* aBindingParent,
                        bool aCompileEventHandler);
    void UnbindFromTree(bool aDeep = true,
                        bool aNullParent = true);

  private:
    
    enum ParameterUpdateFlags {
      eParamNoChange           = 0,
      
      
      eParamChannelChanged     = PR_BIT(0),
      
      
      eParamStateChanged       = PR_BIT(1)
    };

    




    nsresult LoadObject(bool aNotify,
                        bool aForceLoad,
                        nsIRequest *aLoadingChannel);

    





















    ParameterUpdateFlags UpdateObjectParameters();

    void NotifyContentObjectWrapper();

    




    nsresult OpenChannel(PRInt32 aPolicyType);

    


    nsresult CloseChannel();

    












    bool CheckURILoad(nsIURI *aURI,
                      PRInt16 *aContentPolicy,
                      PRInt32 aContentPolicyType);

    











    bool CheckObjectURIs(PRInt16 *aContentPolicy, PRInt32 aContentPolicyType);

    




    bool IsSupportedDocument(const nsCString& aType);

    





    void UnloadContent();

    









    void NotifyStateChanged(ObjectType aOldType, nsEventStates aOldState,
                            bool aSync, bool aNotify);

    




    void FirePluginError(PluginSupportState state);

    







    ObjectType GetTypeOfContent(const nsCString& aMIMEType);

    







    nsresult TypeForClassID(const nsAString& aClassID, nsACString& aType);

    



    nsObjectFrame* GetExistingFrame();

    




    void HandleBeingBlockedByContentPolicy(nsresult aStatus,
                                           PRInt16 aRetval);

    






    PluginSupportState GetPluginSupportState(nsIContent* aContent,
                                             const nsCString& aContentType);

    






    PluginSupportState GetPluginDisabledState(const nsCString& aContentType);

    




    void UpdateFallbackState(nsIContent* aContent, AutoFallback& fallback,
                             const nsCString& aTypeHint);

    


    nsresult IsPluginEnabledForType(const nsCString& aMIMEType);

    




    bool IsPluginEnabledByExtension(nsIURI* uri, nsCString& mimeType);

    
    nsCOMPtr<nsIStreamListener> mFinalListener;

    
    nsRefPtr<nsFrameLoader>     mFrameLoader;

    
    nsIRunnable                *mPendingInstantiateEvent;

    
    
    
    
    
    nsCString                   mContentType;

    
    
    nsCString                   mOriginalContentType;

    
    
    nsCOMPtr<nsIChannel>            mChannel;

    
    
    
    nsCOMPtr<nsIURI>            mURI;

    
    
    nsCOMPtr<nsIURI>            mOriginalURI;

    
    
    nsCOMPtr<nsIURI>            mBaseURI;


    
    ObjectType                  mType          : 16;

    
    bool                        mLoaded           : 1;

    
    
    bool                        mChannelLoaded    : 1;

    
    
    bool                        mInstantiating : 1;

    
    bool                        mUserDisabled  : 1;
    
    bool                        mSuppressed    : 1;

    
    
    
    bool                        mNetworkCreated : 1;

    
    
    
    
    bool                        mCTPPlayable    : 1;

    
    
    bool                        mActivated : 1;

    
    bool                        mIsStopping : 1;

    
    bool                        mIsLoading : 1;

    
    
    
    
    
    bool mSrcStreamLoading;

    
    PluginSupportState          mFallbackReason;

    nsWeakFrame                 mPrintFrame;

    nsRefPtr<nsPluginInstanceOwner> mInstanceOwner;
};

#endif
