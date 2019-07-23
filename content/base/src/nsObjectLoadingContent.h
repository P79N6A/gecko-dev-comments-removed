











































#ifndef NSOBJECTLOADINGCONTENT_H_
#define NSOBJECTLOADINGCONTENT_H_

#include "nsImageLoadingContent.h"
#include "nsIStreamListener.h"
#include "nsFrameLoader.h"
#include "nsIInterfaceRequestor.h"
#include "nsIChannelEventSink.h"
#include "nsIObjectLoadingContent.h"
#include "nsIRunnable.h"

#include "nsWeakReference.h"

struct nsAsyncInstantiateEvent;
class  AutoNotifier;
class  AutoFallback;
class  AutoSetInstantiatingToFalse;



















class nsObjectLoadingContent : public nsImageLoadingContent
                             , public nsIStreamListener
                             , public nsIFrameLoaderOwner
                             , public nsIObjectLoadingContent
                             , public nsIInterfaceRequestor
                             , public nsIChannelEventSink
                             
                             
                             , public nsSupportsWeakReference
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

    




    PRInt32 ObjectState() const;

  protected:
    













    nsresult LoadObject(const nsAString& aURI,
                        PRBool aNotify,
                        const nsCString& aTypeHint = EmptyCString(),
                        PRBool aForceLoad = PR_FALSE);
    


























    nsresult LoadObject(nsIURI* aURI,
                        PRBool aNotify,
                        const nsCString& aTypeHint = EmptyCString(),
                        PRBool aForceLoad = PR_FALSE);

    enum Capabilities {
      eSupportImages    = PR_BIT(0), 
      eSupportPlugins   = PR_BIT(1), 
      eSupportDocuments = PR_BIT(2), 
                                     
                                     
#ifdef MOZ_SVG
      eSupportSVG       = PR_BIT(3), 
#endif
      eSupportClassID   = PR_BIT(4), 
      eOverrideServerType = PR_BIT(5) 
                                      
    };

    





    virtual PRUint32 GetCapabilities() const;

    


    void Fallback(PRBool aNotify);

    





    void RemovedFromDocument();

    void Traverse(nsCycleCollectionTraversalCallback &cb);

  private:
    


    static PRBool IsSuccessfulRequest(nsIRequest* aRequest);

    


    static PRBool CanHandleURI(nsIURI* aURI);

    


    PRBool IsSupportedDocument(const nsCString& aType);

    




    void UnloadContent();

    








    void NotifyStateChanged(ObjectType aOldType, PRInt32 aOldState,
                            PRBool aSync);

    



    static void FirePluginError(nsIContent* thisContent, PRBool blocklisted);

    ObjectType GetTypeOfContent(const nsCString& aMIMEType);

    





    nsresult TypeForClassID(const nsAString& aClassID, nsACString& aType);

    




    void GetObjectBaseURI(nsIContent* thisContent, nsIURI** aURI);

    






    nsIObjectFrame* GetFrame(PRBool aFlushLayout);

    



    nsresult TryInstantiate(const nsACString& aMIMEType, nsIURI* aURI);

    




    nsresult Instantiate(nsIObjectFrame* aFrame, const nsACString& aMIMEType, nsIURI* aURI);

    




    static PRBool ShouldShowDefaultPlugin(nsIContent* aContent,
                                          const nsCString& aContentType);

    enum PluginSupportState {
      ePluginUnsupported,  
      ePluginDisabled,     
                           
      ePluginBlocklisted,  
      ePluginOtherState    
                           
    };

    






    static PluginSupportState
      GetPluginSupportState(nsIContent* aContent,
                            const nsCString& aContentType);

    






    static PluginSupportState
      GetPluginDisabledState(const nsCString& aContentType);

    


    nsCOMPtr<nsIStreamListener> mFinalListener;

    


    nsRefPtr<nsFrameLoader>     mFrameLoader;

    


    nsIRunnable                *mPendingInstantiateEvent;

    


    nsCString                   mContentType;

    



    nsIChannel*                 mChannel;

    
    nsCOMPtr<nsIURI>            mURI;

    


    ObjectType                  mType          : 16;

    



    PRBool                      mInstantiating : 1;
    
    PRBool                      mUserDisabled  : 1;
    PRBool                      mSuppressed    : 1;
    
    PRBool                      mTypeUnsupported:1;

    friend struct nsAsyncInstantiateEvent;
};


#endif
