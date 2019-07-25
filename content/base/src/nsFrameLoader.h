










































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

class nsIContent;
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

    

    PRBool operator==(const ViewConfig& aOther) const
    {
      return (mScrollOffset == aOther.mScrollOffset &&
              mXScale == aOther.mXScale &&
              mYScale == aOther.mYScale);
    }

    
    
    
    
    
    
    nsPoint mScrollOffset;
    
    
    
    
    
    float mXScale;
    float mYScale;
  };

  nsContentView(nsIContent* aOwnerContent, ViewID aScrollId,
                ViewConfig aConfig = ViewConfig())
    : mViewportSize(0, 0)
    , mContentSize(0, 0)
    , mOwnerContent(aOwnerContent)
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

  nsIContent *mOwnerContent; 

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
  nsFrameLoader(nsIContent *aOwner, PRBool aNetworkCreated);

public:
  ~nsFrameLoader() {
    mNeedsAsyncDestroy = PR_TRUE;
    if (mMessageManager) {
      mMessageManager->Disconnect();
    }
    nsFrameLoader::Destroy();
  }

  PRBool AsyncScrollEnabled() const
  {
    return !!(mRenderMode & RENDER_MODE_ASYNC_SCROLL);
  }

  static nsFrameLoader* Create(nsIContent* aOwner, PRBool aNetworkCreated);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsFrameLoader, nsIFrameLoader)
  NS_DECL_NSIFRAMELOADER
  NS_DECL_NSICONTENTVIEWMANAGER
  NS_HIDDEN_(nsresult) CheckForRecursiveLoad(nsIURI* aURI);
  nsresult ReallyStartLoading();
  void Finalize();
  nsIDocShell* GetExistingDocShell() { return mDocShell; }
  nsPIDOMEventTarget* GetTabChildGlobalAsEventTarget();
  nsresult CreateStaticClone(nsIFrameLoader* aDest);

  



  PRBool Show(PRInt32 marginWidth, PRInt32 marginHeight,
              PRInt32 scrollbarPrefX, PRInt32 scrollbarPrefY,
              nsSubDocumentFrame* frame);

  




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

  nsIContent* GetOwnerContent() { return mOwnerContent; }
  void SetOwnerContent(nsIContent* aContent);

private:

  bool ShouldUseRemoteProcess();

  



  nsresult MaybeCreateDocShell();
  nsresult EnsureMessageManager();
  NS_HIDDEN_(void) GetURL(nsString& aURL);

  
  NS_HIDDEN_(nsIntSize) GetSubDocumentSize(const nsIFrame *aIFrame);

  
  
  NS_HIDDEN_(nsresult) UpdateBaseWindowPositionAndSize(nsIFrame *aIFrame);
  nsresult CheckURILoad(nsIURI* aURI);
  void FireErrorEvent();
  nsresult ReallyStartLoadingInternal();

  
  bool TryRemoteBrowser();

  
  bool ShowRemoteFrame(const nsIntSize& size);

  nsCOMPtr<nsIDocShell> mDocShell;
  nsCOMPtr<nsIURI> mURIToLoad;
  nsIContent *mOwnerContent; 
public:
  
  nsRefPtr<nsFrameMessageManager> mMessageManager;
  nsCOMPtr<nsIInProcessContentFrameMessageManager> mChildMessageManager;
private:
  PRPackedBool mDepthTooGreat : 1;
  PRPackedBool mIsTopLevelContent : 1;
  PRPackedBool mDestroyCalled : 1;
  PRPackedBool mNeedsAsyncDestroy : 1;
  PRPackedBool mInSwap : 1;
  PRPackedBool mInShow : 1;
  PRPackedBool mHideCalled : 1;
  
  
  
  PRPackedBool mNetworkCreated : 1;

  PRPackedBool mDelayRemoteDialogs : 1;
  PRPackedBool mRemoteBrowserShown : 1;
  bool mRemoteFrame;
  
  nsCOMPtr<nsIObserver> mChildHost;
  RenderFrameParent* mCurrentRemoteFrame;
  TabParent* mRemoteBrowser;

  
  
  
  PRUint32 mRenderMode;
};

#endif
