











#ifndef NSOBJECTLOADINGCONTENT_H_
#define NSOBJECTLOADINGCONTENT_H_

#include "nsImageLoadingContent.h"
#include "nsIStreamListener.h"
#include "nsFrameLoader.h"
#include "nsIInterfaceRequestor.h"
#include "nsIChannelEventSink.h"
#include "nsIObjectLoadingContent.h"
#include "nsIRunnable.h"
#include "nsPluginInstanceOwner.h"
#include "nsIThreadInternal.h"
#include "nsIFrame.h"

class nsAsyncInstantiateEvent;
class nsStopPluginRunnable;
class AutoNotifier;
class AutoFallback;
class AutoSetInstantiatingToFalse;
class nsObjectFrame;

class nsObjectLoadingContent : public nsImageLoadingContent
                             , public nsIStreamListener
                             , public nsIFrameLoaderOwner
                             , public nsIObjectLoadingContent
                             , public nsIInterfaceRequestor
                             , public nsIChannelEventSink
{
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
    enum FallbackType {
      
      eFallbackUnsupported = nsIObjectLoadingContent::PLUGIN_UNSUPPORTED,
      
      eFallbackAlternate = nsIObjectLoadingContent::PLUGIN_ALTERNATE,
      
      eFallbackDisabled = nsIObjectLoadingContent::PLUGIN_DISABLED,
      
      eFallbackBlocklisted = nsIObjectLoadingContent::PLUGIN_BLOCKLISTED,
      
      eFallbackOutdated = nsIObjectLoadingContent::PLUGIN_OUTDATED,
      
      eFallbackCrashed = nsIObjectLoadingContent::PLUGIN_CRASHED,
      
      eFallbackSuppressed = nsIObjectLoadingContent::PLUGIN_SUPPRESSED,
      
      eFallbackUserDisabled = nsIObjectLoadingContent::PLUGIN_USER_DISABLED,
      
      eFallbackClickToPlay = nsIObjectLoadingContent::PLUGIN_CLICK_TO_PLAY,
      
      eFallbackVulnerableUpdatable = nsIObjectLoadingContent::PLUGIN_VULNERABLE_UPDATABLE,
      
      eFallbackVulnerableNoUpdate = nsIObjectLoadingContent::PLUGIN_VULNERABLE_NO_UPDATE
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

    






    void LoadFallback(FallbackType aType, bool aNotify);

    




    nsresult LoadObject(bool aNotify,
                        bool aForceLoad,
                        nsIRequest *aLoadingChannel);

    





















    ParameterUpdateFlags UpdateObjectParameters();

    void NotifyContentObjectWrapper();

    




    nsresult OpenChannel(PRInt32 aPolicyType);

    


    nsresult CloseChannel();

    




    bool ShouldPlay(FallbackType &aReason);

    












    bool CheckURILoad(nsIURI *aURI,
                      PRInt16 *aContentPolicy,
                      PRInt32 aContentPolicyType);

    











    bool CheckObjectURIs(PRInt16 *aContentPolicy, PRInt32 aContentPolicyType);

    




    bool IsSupportedDocument(const nsCString& aType);

    







    void UnloadObject(bool aResetState = true);

    









    void NotifyStateChanged(ObjectType aOldType, nsEventStates aOldState,
                            bool aSync, bool aNotify);

    




    void FirePluginError(FallbackType aFallbackType);

    







    ObjectType GetTypeOfContent(const nsCString& aMIMEType);

    



    nsObjectFrame* GetExistingFrame();

    
    nsCOMPtr<nsIStreamListener> mFinalListener;

    
    nsRefPtr<nsFrameLoader>     mFrameLoader;

    
    nsIRunnable                *mPendingInstantiateEvent;

    
    
    
    
    
    nsCString                   mContentType;

    
    
    nsCString                   mOriginalContentType;

    
    
    nsCOMPtr<nsIChannel>        mChannel;

    
    
    
    nsCOMPtr<nsIURI>            mURI;

    
    
    nsCOMPtr<nsIURI>            mOriginalURI;

    
    
    nsCOMPtr<nsIURI>            mBaseURI;



    
    ObjectType                  mType           : 8;
    
    FallbackType                mFallbackType : 8;

    
    
    bool                        mChannelLoaded    : 1;

    
    
    bool                        mInstantiating : 1;

    
    
    
    bool                        mNetworkCreated : 1;

    
    
    bool                        mActivated : 1;

    
    bool                        mIsStopping : 1;

    
    bool                        mIsLoading : 1;

    
    
    
    
    
    bool mSrcStreamLoading;


    nsWeakFrame                 mPrintFrame;

    nsRefPtr<nsPluginInstanceOwner> mInstanceOwner;
};

#endif
