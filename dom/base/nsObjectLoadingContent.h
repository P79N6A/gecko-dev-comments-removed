











#ifndef NSOBJECTLOADINGCONTENT_H_
#define NSOBJECTLOADINGCONTENT_H_

#include "mozilla/Attributes.h"
#include "nsImageLoadingContent.h"
#include "nsIStreamListener.h"
#include "nsIChannelEventSink.h"
#include "nsIObjectLoadingContent.h"
#include "nsIRunnable.h"
#include "nsIThreadInternal.h"
#include "nsIFrame.h"
#include "nsIFrameLoader.h"

class nsAsyncInstantiateEvent;
class nsStopPluginRunnable;
class AutoSetInstantiatingToFalse;
class nsPluginFrame;
class nsFrameLoader;
class nsXULElement;
class nsPluginInstanceOwner;

namespace mozilla {
namespace dom {
template<typename T> class Sequence;
struct MozPluginParameter;
}
}

class nsObjectLoadingContent : public nsImageLoadingContent
                             , public nsIStreamListener
                             , public nsIFrameLoaderOwner
                             , public nsIObjectLoadingContent
                             , public nsIChannelEventSink
{
  friend class AutoSetInstantiatingToFalse;
  friend class AutoSetLoadingToFalse;
  friend class CheckPluginStopEvent;
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
      
      eFallbackVulnerableNoUpdate = nsIObjectLoadingContent::PLUGIN_VULNERABLE_NO_UPDATE,
      
      
      eFallbackPlayPreview = nsIObjectLoadingContent::PLUGIN_PLAY_PREVIEW
    };

    nsObjectLoadingContent();
    virtual ~nsObjectLoadingContent();

    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIFRAMELOADEROWNER
    NS_DECL_NSIOBJECTLOADINGCONTENT
    NS_DECL_NSICHANNELEVENTSINK

    



    mozilla::EventStates ObjectState() const;

    ObjectType Type() const { return mType; }

    void SetIsNetworkCreated(bool aNetworkCreated)
    {
      mNetworkCreated = aNetworkCreated;
    }

    






    
    void GetPluginAttributes(nsTArray<mozilla::dom::MozPluginParameter>& aAttributes);

    
    void GetPluginParameters(nsTArray<mozilla::dom::MozPluginParameter>& aParameters);

    






    nsresult InstantiatePluginInstance(bool aIsLoading = false);

    



    void NotifyOwnerDocumentActivityChanged();

    












    
    void SetupProtoChain(JSContext* aCx, JS::Handle<JSObject*> aObject);

    
    void TeardownProtoChain();

    
    bool DoResolve(JSContext* aCx, JS::Handle<JSObject*> aObject,
                   JS::Handle<jsid> aId,
                   JS::MutableHandle<JSPropertyDescriptor> aDesc);
    
    
    static bool MayResolve(jsid aId);

    
    void GetOwnPropertyNames(JSContext* aCx, nsTArray<nsString>& ,
                             mozilla::ErrorResult& aRv);

    
    nsIDocument* GetContentDocument();
    void GetActualType(nsAString& aType) const
    {
      CopyUTF8toUTF16(mContentType, aType);
    }
    uint32_t DisplayedType() const
    {
      return mType;
    }
    uint32_t GetContentTypeForMIMEType(const nsAString& aMIMEType)
    {
      return GetTypeOfContent(NS_ConvertUTF16toUTF8(aMIMEType));
    }
    void PlayPlugin(mozilla::ErrorResult& aRv)
    {
      aRv = PlayPlugin();
    }
    void Reload(bool aClearActivation, mozilla::ErrorResult& aRv)
    {
      aRv = Reload(aClearActivation);
    }
    bool Activated() const
    {
      return mActivated;
    }
    nsIURI* GetSrcURI() const
    {
      return mURI;
    }
  
    



    uint32_t DefaultFallbackType();

    uint32_t PluginFallbackType() const
    {
      return mFallbackType;
    }
    bool HasRunningPlugin() const
    {
      return !!mInstanceOwner;
    }
    void CancelPlayPreview(mozilla::ErrorResult& aRv)
    {
      aRv = CancelPlayPreview();
    }
    void SwapFrameLoaders(nsXULElement& aOtherOwner, mozilla::ErrorResult& aRv)
    {
      aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
    }
    void LegacyCall(JSContext* aCx, JS::Handle<JS::Value> aThisVal,
                    const mozilla::dom::Sequence<JS::Value>& aArguments,
                    JS::MutableHandle<JS::Value> aRetval,
                    mozilla::ErrorResult& aRv);

    uint32_t GetRunID(mozilla::ErrorResult& aRv)
    {
      uint32_t runID;
      nsresult rv = GetRunID(&runID);
      if (NS_FAILED(rv)) {
        aRv.Throw(rv);
        return 0;
      }

      return runID;
    }

  protected:
    

































    nsresult LoadObject(bool aNotify,
                        bool aForceLoad = false);

    enum Capabilities {
      eSupportImages       = 1u << 0, 
      eSupportPlugins      = 1u << 1, 
      eSupportDocuments    = 1u << 2, 
                                        
                                        
      eSupportSVG          = 1u << 3, 
      eSupportClassID      = 1u << 4, 

      
      
      
      
      
      eAllowPluginSkipChannel  = 1u << 5
    };

    






    virtual uint32_t GetCapabilities() const;

    


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
      
      
      eParamChannelChanged     = 1u << 0,
      
      
      eParamStateChanged       = 1u << 1,
      
      
      
      
      
      
      eParamContentTypeChanged = 1u << 2
    };

    














    void GetNestedParams(nsTArray<mozilla::dom::MozPluginParameter>& aParameters,
                         bool aIgnoreCodebase);

    void BuildParametersArray();

    






    void LoadFallback(FallbackType aType, bool aNotify);

    




    nsresult LoadObject(bool aNotify,
                        bool aForceLoad,
                        nsIRequest *aLoadingChannel);

    

























    ParameterUpdateFlags UpdateObjectParameters(bool aJavaURI = false);

    


    void QueueCheckPluginStopEvent();

    void NotifyContentObjectWrapper();

    


    nsresult OpenChannel();

    


    nsresult CloseChannel();

    





    bool ShouldPlay(FallbackType &aReason, bool aIgnoreCurrentType);

    


    bool CheckJavaCodebase();

    






    bool CheckLoadPolicy(int16_t *aContentPolicy);

    







    bool CheckProcessPolicy(int16_t *aContentPolicy);

    




    bool IsSupportedDocument(const nsCString& aType);

    



    bool MakePluginListener();

    







    void UnloadObject(bool aResetState = true);

    









    void NotifyStateChanged(ObjectType aOldType,
                            mozilla::EventStates aOldState,
                            bool aSync, bool aNotify);

    







    ObjectType GetTypeOfContent(const nsCString& aMIMEType);

    



    nsPluginFrame* GetExistingFrame();

    
    class SetupProtoChainRunner final : public nsIRunnable
    {
      ~SetupProtoChainRunner();
    public:
      NS_DECL_ISUPPORTS

      explicit SetupProtoChainRunner(nsObjectLoadingContent* aContent);

      NS_IMETHOD Run() override;

    private:
      
      
      nsRefPtr<nsIObjectLoadingContent> mContent;
    };

    
    nsresult ScriptRequestPluginInstance(JSContext* aCx,
                                         nsNPAPIPluginInstance** aResult);

    
    static nsresult GetPluginJSObject(JSContext *cx,
                                      JS::Handle<JSObject*> obj,
                                      nsNPAPIPluginInstance *plugin_inst,
                                      JS::MutableHandle<JSObject*> plugin_obj,
                                      JS::MutableHandle<JSObject*> plugin_proto);

    
    nsCOMPtr<nsIStreamListener> mFinalListener;

    
    nsRefPtr<nsFrameLoader>     mFrameLoader;

    
    nsCOMPtr<nsIRunnable>       mPendingInstantiateEvent;

    
    nsCOMPtr<nsIRunnable>       mPendingCheckPluginStopEvent;

    
    
    
    
    
    nsCString                   mContentType;

    
    
    nsCString                   mOriginalContentType;

    
    
    nsCOMPtr<nsIChannel>        mChannel;

    
    
    
    nsCOMPtr<nsIURI>            mURI;

    
    
    nsCOMPtr<nsIURI>            mOriginalURI;

    
    
    nsCOMPtr<nsIURI>            mBaseURI;



    
    ObjectType                  mType           : 8;
    
    FallbackType                mFallbackType : 8;

    uint32_t                    mRunID;
    bool                        mHasRunID;

    
    
    
    bool                        mChannelLoaded    : 1;

    
    
    bool                        mInstantiating : 1;

    
    
    
    bool                        mNetworkCreated : 1;

    
    
    bool                        mActivated : 1;

    
    bool                        mPlayPreviewCanceled : 1;

    
    bool                        mIsStopping : 1;

    
    bool                        mIsLoading : 1;

    
    
    bool                        mScriptRequested : 1;

    nsWeakFrame                 mPrintFrame;

    nsRefPtr<nsPluginInstanceOwner> mInstanceOwner;
    nsTArray<mozilla::dom::MozPluginParameter> mCachedAttributes;
    nsTArray<mozilla::dom::MozPluginParameter> mCachedParameters;
};

#endif
