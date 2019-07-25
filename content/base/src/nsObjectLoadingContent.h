











































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

class nsAsyncInstantiateEvent;
class AutoNotifier;
class AutoFallback;
class AutoSetInstantiatingToFalse;

enum PluginSupportState {
  ePluginUnsupported,  
  ePluginDisabled,     
  ePluginBlocklisted,  
  ePluginOutdated,     
  ePluginOtherState,   
  ePluginCrashed
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

  public:
    
    
    enum ObjectType {
      eType_Loading  = TYPE_LOADING,  
      eType_Image    = TYPE_IMAGE,    
      eType_Plugin   = TYPE_PLUGIN,   
      eType_Document = TYPE_DOCUMENT, 
      eType_Null     = TYPE_NULL      
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

    ObjectType Type() { return mType; }

    




    nsEventStates ObjectState() const;

    void SetIsNetworkCreated(bool aNetworkCreated)
    {
      mNetworkCreated = aNetworkCreated;
    }
  protected:
    













    nsresult LoadObject(const nsAString& aURI,
                        bool aNotify,
                        const nsCString& aTypeHint = EmptyCString(),
                        bool aForceLoad = false);
    


























    nsresult LoadObject(nsIURI* aURI,
                        bool aNotify,
                        const nsCString& aTypeHint = EmptyCString(),
                        bool aForceLoad = false);

    enum Capabilities {
      eSupportImages    = PR_BIT(0), 
      eSupportPlugins   = PR_BIT(1), 
      eSupportDocuments = PR_BIT(2), 
                                     
                                     
      eSupportSVG       = PR_BIT(3), 
      eSupportClassID   = PR_BIT(4), 
      eOverrideServerType = PR_BIT(5) 
                                      
    };

    





    virtual PRUint32 GetCapabilities() const;

    


    void Fallback(bool aNotify);

    





    void RemovedFromDocument();

    static void Traverse(nsObjectLoadingContent *tmp,
                         nsCycleCollectionTraversalCallback &cb);

    void CreateStaticClone(nsObjectLoadingContent* aDest) const;
  private:
    


    static bool IsSuccessfulRequest(nsIRequest* aRequest);

    


    static bool CanHandleURI(nsIURI* aURI);

    


    bool IsSupportedDocument(const nsCString& aType);

    




    void UnloadContent();

    









    void NotifyStateChanged(ObjectType aOldType, nsEventStates aOldState,
                            bool aSync, bool aNotify);

    



    static void FirePluginError(nsIContent* thisContent, PluginSupportState state);

    ObjectType GetTypeOfContent(const nsCString& aMIMEType);

    





    nsresult TypeForClassID(const nsAString& aClassID, nsACString& aType);

    




    void GetObjectBaseURI(nsIContent* thisContent, nsIURI** aURI);


    













    enum FlushType {
      eFlushContent,
      eFlushLayout,
      eDontFlush
    };
    nsIObjectFrame* GetExistingFrame(FlushType aFlushType);

    




    void HandleBeingBlockedByContentPolicy(nsresult aStatus,
                                           PRInt16 aRetval);

    




    nsresult TryInstantiate(const nsACString& aMIMEType, nsIURI* aURI);

    






    nsresult Instantiate(nsIObjectFrame* aFrame, const nsACString& aMIMEType, nsIURI* aURI);

    






    static PluginSupportState
      GetPluginSupportState(nsIContent* aContent,
                            const nsCString& aContentType);

    






    static PluginSupportState
      GetPluginDisabledState(const nsCString& aContentType);

    




    static void
      UpdateFallbackState(nsIContent* aContent, AutoFallback& fallback,
                          const nsCString& aTypeHint);

    


    nsCOMPtr<nsIStreamListener> mFinalListener;

    


    nsRefPtr<nsFrameLoader>     mFrameLoader;

    


    nsIRunnable                *mPendingInstantiateEvent;

    


    nsCString                   mContentType;

    



    nsIChannel*                 mChannel;

    
    nsCOMPtr<nsIURI>            mURI;

    


    ObjectType                  mType          : 16;

    



    bool                        mInstantiating : 1;
    
    bool                        mUserDisabled  : 1;
    bool                        mSuppressed    : 1;

    
    
    
    bool                        mNetworkCreated : 1;

    
    PluginSupportState          mFallbackReason;

    nsWeakFrame                 mPrintFrame;

    friend class nsAsyncInstantiateEvent;
};


#endif
