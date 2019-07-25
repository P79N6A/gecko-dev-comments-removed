










































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
#include "Layers.h"
#include "mozilla/dom/Element.h"

class nsIURI;
class nsSubDocumentFrame;
class nsIView;
class nsIInProcessContentFrameMessageManager;
class AutoResetInShow;

namespace mozilla {
namespace dom {
class PBrowserParent;
class TabParent;
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










class nsContentView : public nsIContentView
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

  nsFrameLoader* mFrameLoader;  

private:
  nsresult Update(const ViewConfig& aConfig);

  ViewID mScrollId;
  ViewConfig mConfig;
};


class nsFrameLoader : public nsIFrameLoader,
                      public nsIContentViewManager
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
  NS_HIDDEN_(nsresult) CheckForRecursiveLoad(nsIURI* aURI);
  nsresult ReallyStartLoading();
  void Finalize();
  nsIDocShell* GetExistingDocShell() { return mDocShell; }
  nsIDOMEventTarget* GetTabChildGlobalAsEventTarget();
  nsresult CreateStaticClone(nsIFrameLoader* aDest);

  



  bool Show(PRInt32 marginWidth, PRInt32 marginHeight,
              PRInt32 scrollbarPrefX, PRInt32 scrollbarPrefY,
              nsSubDocumentFrame* frame);

  


  void MarginsChanged(PRUint32 aMarginWidth, PRUint32 aMarginHeight);

  




  void Hide();

  nsresult CloneForStatic(nsIFrameLoader* aOriginal);

  
  
  
  nsresult SwapWithOtherLoader(nsFrameLoader* aOther,
                               nsRefPtr<nsFrameLoader>& aFirstToSwap,
                               nsRefPtr<nsFrameLoader>& aSecondToSwap);

  
  void DestroyChild();

  



  nsIFrame* GetPrimaryFrameOfOwningContent() const
  {
    return mOwnerContent ? mOwnerContent->GetPrimaryFrame() : nsnull;
  }

  



  nsIDocument* GetOwnerDoc() const
  { return mOwnerContent ? mOwnerContent->GetOwnerDoc() : nsnull; }

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
  void SetOwnerContent(mozilla::dom::Element* aContent);

private:

  bool ShouldUseRemoteProcess();

  



  nsresult MaybeCreateDocShell();
  nsresult EnsureMessageManager();
  NS_HIDDEN_(void) GetURL(nsString& aURL);

  
  NS_HIDDEN_(nsIntSize) GetSubDocumentSize(const nsIFrame *aIFrame);
  nsresult GetWindowDimensions(nsRect& aRect);

  
  
  NS_HIDDEN_(nsresult) UpdateBaseWindowPositionAndSize(nsIFrame *aIFrame);
  nsresult CheckURILoad(nsIURI* aURI);
  void FireErrorEvent();
  nsresult ReallyStartLoadingInternal();

  
  bool TryRemoteBrowser();

  
  bool ShowRemoteFrame(const nsIntSize& size);

  nsCOMPtr<nsIDocShell> mDocShell;
  nsCOMPtr<nsIURI> mURIToLoad;
  mozilla::dom::Element* mOwnerContent; 
public:
  
  nsRefPtr<nsFrameMessageManager> mMessageManager;
  nsCOMPtr<nsIInProcessContentFrameMessageManager> mChildMessageManager;
private:
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
  bool mRemoteFrame;
  
  nsCOMPtr<nsIObserver> mChildHost;
  RenderFrameParent* mCurrentRemoteFrame;
  TabParent* mRemoteBrowser;

  
  
  
  PRUint32 mRenderMode;

  
  
  PRUint32 mEventMode;
};

#endif
