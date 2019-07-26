









#ifndef nsFrameLoader_h_
#define nsFrameLoader_h_

#include "nsIDocShell.h"
#include "nsStringFwd.h"
#include "nsIFrameLoader.h"
#include "nsPoint.h"
#include "nsSize.h"
#include "nsIURI.h"
#include "nsAutoPtr.h"
#include "nsFrameMessageManager.h"
#include "mozilla/dom/Element.h"
#include "mozilla/Attributes.h"
#include "FrameMetrics.h"
#include "nsStubMutationObserver.h"

class nsIURI;
class nsSubDocumentFrame;
class nsView;
class nsIInProcessContentFrameMessageManager;
class AutoResetInShow;
class nsITabParent;
class nsIDocShellTreeItem;
class nsIDocShellTreeOwner;
class nsIDocShellTreeNode;
class mozIApplication;

namespace mozilla {
namespace dom {
class PBrowserParent;
class TabParent;
struct StructuredCloneData;
}

namespace layout {
class RenderFrameParent;
}
}

#ifdef MOZ_WIDGET_GTK2
typedef struct _GtkWidget GtkWidget;
#endif
#ifdef MOZ_WIDGET_QT
class QX11EmbedContainer;
#endif










class nsContentView MOZ_FINAL : public nsIContentView
{
public:
  typedef mozilla::layers::FrameMetrics::ViewID ViewID;
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTENTVIEW
 
  struct ViewConfig {
    ViewConfig()
      : mScrollOffset(0, 0)
      , mXScale(1.0)
      , mYScale(1.0)
    {}

    

    bool operator==(const ViewConfig& aOther) const
    {
      return (mScrollOffset == aOther.mScrollOffset &&
              mXScale == aOther.mXScale &&
              mYScale == aOther.mYScale);
    }

    
    
    
    
    
    
    nsPoint mScrollOffset;
    
    
    
    
    
    float mXScale;
    float mYScale;
  };

  nsContentView(nsFrameLoader* aFrameLoader, ViewID aScrollId,
                ViewConfig aConfig = ViewConfig())
    : mViewportSize(0, 0)
    , mContentSize(0, 0)
    , mParentScaleX(1.0)
    , mParentScaleY(1.0)
    , mFrameLoader(aFrameLoader)
    , mScrollId(aScrollId)
    , mConfig(aConfig)
  {}

  bool IsRoot() const;

  ViewID GetId() const
  {
    return mScrollId;
  }

  ViewConfig GetViewConfig() const
  {
    return mConfig;
  }

  nsSize mViewportSize;
  nsSize mContentSize;
  float mParentScaleX;
  float mParentScaleY;

  nsFrameLoader* mFrameLoader;  

private:
  nsresult Update(const ViewConfig& aConfig);

  ViewID mScrollId;
  ViewConfig mConfig;
};


class nsFrameLoader MOZ_FINAL : public nsIFrameLoader,
                                public nsIContentViewManager,
                                public nsStubMutationObserver,
                                public mozilla::dom::ipc::MessageManagerCallback
{
  friend class AutoResetInShow;
  typedef mozilla::dom::PBrowserParent PBrowserParent;
  typedef mozilla::dom::TabParent TabParent;
  typedef mozilla::layout::RenderFrameParent RenderFrameParent;

protected:
  nsFrameLoader(mozilla::dom::Element* aOwner, bool aNetworkCreated);

public:
  ~nsFrameLoader() {
    mNeedsAsyncDestroy = true;
    if (mMessageManager) {
      mMessageManager->Disconnect();
    }
    nsFrameLoader::Destroy();
  }

  bool AsyncScrollEnabled() const
  {
    return !!(mRenderMode & RENDER_MODE_ASYNC_SCROLL);
  }

  static nsFrameLoader* Create(mozilla::dom::Element* aOwner,
                               bool aNetworkCreated);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsFrameLoader, nsIFrameLoader)
  NS_DECL_NSIFRAMELOADER
  NS_DECL_NSICONTENTVIEWMANAGER
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  NS_HIDDEN_(nsresult) CheckForRecursiveLoad(nsIURI* aURI);
  nsresult ReallyStartLoading();
  void Finalize();
  nsIDocShell* GetExistingDocShell() { return mDocShell; }
  nsIDOMEventTarget* GetTabChildGlobalAsEventTarget();
  nsresult CreateStaticClone(nsIFrameLoader* aDest);

  


  virtual bool DoLoadFrameScript(const nsAString& aURL);
  virtual bool DoSendAsyncMessage(const nsAString& aMessage,
                                  const mozilla::dom::StructuredCloneData& aData);
  virtual bool CheckPermission(const nsAString& aPermission);
  virtual bool CheckManifestURL(const nsAString& aManifestURL);
  virtual bool CheckAppHasPermission(const nsAString& aPermission);

  



  bool Show(int32_t marginWidth, int32_t marginHeight,
              int32_t scrollbarPrefX, int32_t scrollbarPrefY,
              nsSubDocumentFrame* frame);

  


  void MarginsChanged(uint32_t aMarginWidth, uint32_t aMarginHeight);

  




  void Hide();

  nsresult CloneForStatic(nsIFrameLoader* aOriginal);

  
  
  
  nsresult SwapWithOtherLoader(nsFrameLoader* aOther,
                               nsRefPtr<nsFrameLoader>& aFirstToSwap,
                               nsRefPtr<nsFrameLoader>& aSecondToSwap);

  
  void DestroyChild();

  



  nsIFrame* GetPrimaryFrameOfOwningContent() const
  {
    return mOwnerContent ? mOwnerContent->GetPrimaryFrame() : nullptr;
  }

  



  nsIDocument* GetOwnerDoc() const
  { return mOwnerContent ? mOwnerContent->OwnerDoc() : nullptr; }

  PBrowserParent* GetRemoteBrowser();

  













  RenderFrameParent* GetCurrentRemoteFrame() const
  {
    return mCurrentRemoteFrame;
  }

  




  void SetCurrentRemoteFrame(RenderFrameParent* aFrame)
  {
    mCurrentRemoteFrame = aFrame;
  }
  nsFrameMessageManager* GetFrameMessageManager() { return mMessageManager; }

  mozilla::dom::Element* GetOwnerContent() { return mOwnerContent; }
  bool ShouldClipSubdocument() { return mClipSubdocument; }

  bool ShouldClampScrollPosition() { return mClampScrollPosition; }

  







  void SetRemoteBrowser(nsITabParent* aTabParent);

  









  void SetDetachedSubdocView(nsView* aDetachedView,
                             nsIDocument* aContainerDoc);

  



  nsView* GetDetachedSubdocView(nsIDocument** aContainerDoc) const;

private:

  void SetOwnerContent(mozilla::dom::Element* aContent);

  bool ShouldUseRemoteProcess();

  




  bool OwnerIsBrowserOrAppFrame();

  



  bool OwnerIsAppFrame();

  


  bool OwnerIsBrowserFrame();

  



  already_AddRefed<mozIApplication> GetOwnApp();

  



  already_AddRefed<mozIApplication> GetContainingApp();

  



  nsresult MaybeCreateDocShell();
  nsresult EnsureMessageManager();
  NS_HIDDEN_(void) GetURL(nsString& aURL);

  
  nsresult GetWindowDimensions(nsRect& aRect);

  
  
  NS_HIDDEN_(nsresult) UpdateBaseWindowPositionAndSize(nsSubDocumentFrame *aIFrame);
  nsresult CheckURILoad(nsIURI* aURI);
  void FireErrorEvent();
  nsresult ReallyStartLoadingInternal();

  
  bool TryRemoteBrowser();

  
  bool ShowRemoteFrame(const nsIntSize& size,
                       nsSubDocumentFrame *aFrame = nullptr);

  bool AddTreeItemToTreeOwner(nsIDocShellTreeItem* aItem,
                              nsIDocShellTreeOwner* aOwner,
                              int32_t aParentType,
                              nsIDocShellTreeNode* aParentNode);

  nsIAtom* TypeAttrName() const {
    return mOwnerContent->IsXUL() ? nsGkAtoms::type : nsGkAtoms::mozframetype;
  }

  
  
  void ResetPermissionManagerStatus();

  nsCOMPtr<nsIDocShell> mDocShell;
  nsCOMPtr<nsIURI> mURIToLoad;
  mozilla::dom::Element* mOwnerContent; 

  
  uint32_t mAppIdSentToPermissionManager;

public:
  
  nsRefPtr<nsFrameMessageManager> mMessageManager;
  nsCOMPtr<nsIInProcessContentFrameMessageManager> mChildMessageManager;
private:
  
  
  nsView* mDetachedSubdocViews;
  
  
  
  
  
  nsCOMPtr<nsIDocument> mContainerDocWhileDetached;

  bool mDepthTooGreat : 1;
  bool mIsTopLevelContent : 1;
  bool mDestroyCalled : 1;
  bool mNeedsAsyncDestroy : 1;
  bool mInSwap : 1;
  bool mInShow : 1;
  bool mHideCalled : 1;
  
  
  
  bool mNetworkCreated : 1;

  bool mDelayRemoteDialogs : 1;
  bool mRemoteBrowserShown : 1;
  bool mRemoteFrame : 1;
  bool mClipSubdocument : 1;
  bool mClampScrollPosition : 1;
  bool mRemoteBrowserInitialized : 1;
  bool mObservingOwnerContent : 1;

  
  nsCOMPtr<nsIObserver> mChildHost;
  RenderFrameParent* mCurrentRemoteFrame;
  TabParent* mRemoteBrowser;

  
  
  
  uint32_t mRenderMode;

  
  
  uint32_t mEventMode;
};

#endif
