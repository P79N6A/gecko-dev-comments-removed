









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
#include "nsStubMutationObserver.h"
#include "Units.h"

class nsIURI;
class nsSubDocumentFrame;
class nsView;
class nsIInProcessContentFrameMessageManager;
class AutoResetInShow;
class nsITabParent;
class nsIDocShellTreeItem;
class nsIDocShellTreeOwner;
class mozIApplication;

namespace mozilla {
namespace dom {
class ContentParent;
class PBrowserParent;
class TabParent;
struct StructuredCloneData;
}

namespace layout {
class RenderFrameParent;
}
}

#if defined(MOZ_WIDGET_GTK)
typedef struct _GtkWidget GtkWidget;
#endif

class nsFrameLoader final : public nsIFrameLoader,
                            public nsStubMutationObserver,
                            public mozilla::dom::ipc::MessageManagerCallback
{
  friend class AutoResetInShow;
  typedef mozilla::dom::PBrowserParent PBrowserParent;
  typedef mozilla::dom::TabParent TabParent;
  typedef mozilla::layout::RenderFrameParent RenderFrameParent;

protected:
  nsFrameLoader(mozilla::dom::Element* aOwner, bool aNetworkCreated);

  ~nsFrameLoader();

public:
  static nsFrameLoader* Create(mozilla::dom::Element* aOwner,
                               bool aNetworkCreated);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsFrameLoader, nsIFrameLoader)
  NS_DECL_NSIFRAMELOADER
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
  nsresult CheckForRecursiveLoad(nsIURI* aURI);
  nsresult ReallyStartLoading();
  void StartDestroy();
  void DestroyDocShell();
  void DestroyComplete();
  nsIDocShell* GetExistingDocShell() { return mDocShell; }
  mozilla::dom::EventTarget* GetTabChildGlobalAsEventTarget();
  nsresult CreateStaticClone(nsIFrameLoader* aDest);

  


  virtual bool DoLoadMessageManagerScript(const nsAString& aURL,
                                          bool aRunInGlobalScope) override;
  virtual bool DoSendAsyncMessage(JSContext* aCx,
                                  const nsAString& aMessage,
                                  const mozilla::dom::StructuredCloneData& aData,
                                  JS::Handle<JSObject *> aCpows,
                                  nsIPrincipal* aPrincipal) override;
  virtual bool CheckPermission(const nsAString& aPermission) override;
  virtual bool CheckManifestURL(const nsAString& aManifestURL) override;
  virtual bool CheckAppHasPermission(const nsAString& aPermission) override;

  



  bool Show(int32_t marginWidth, int32_t marginHeight,
              int32_t scrollbarPrefX, int32_t scrollbarPrefY,
              nsSubDocumentFrame* frame);

  


  void MarginsChanged(uint32_t aMarginWidth, uint32_t aMarginHeight);

  




  void Hide();

  nsresult CloneForStatic(nsIFrameLoader* aOriginal);

  
  
  
  nsresult SwapWithOtherLoader(nsFrameLoader* aOther,
                               nsRefPtr<nsFrameLoader>& aFirstToSwap,
                               nsRefPtr<nsFrameLoader>& aSecondToSwap);

  nsresult SwapWithOtherRemoteLoader(nsFrameLoader* aOther,
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

  




  void ApplySandboxFlags(uint32_t sandboxFlags);

  void GetURL(nsString& aURL);

  void ActivateUpdateHitRegion();
  void DeactivateUpdateHitRegion();

  
  nsresult GetWindowDimensions(nsIntRect& aRect);

private:

  void SetOwnerContent(mozilla::dom::Element* aContent);

  bool ShouldUseRemoteProcess();

  




  bool OwnerIsBrowserOrAppFrame();

  



  bool OwnerIsWidget();

  



  bool OwnerIsAppFrame();

  


  bool OwnerIsBrowserFrame();

  



  void GetOwnerAppManifestURL(nsAString& aOut);

  



  already_AddRefed<mozIApplication> GetOwnApp();

  



  already_AddRefed<mozIApplication> GetContainingApp();

  



  nsresult MaybeCreateDocShell();
  nsresult EnsureMessageManager();

  
  
  void UpdateBaseWindowPositionAndSize(nsSubDocumentFrame *aIFrame);
  nsresult CheckURILoad(nsIURI* aURI);
  void FireErrorEvent();
  nsresult ReallyStartLoadingInternal();

  
  bool TryRemoteBrowser();

  
  bool ShowRemoteFrame(const mozilla::ScreenIntSize& size,
                       nsSubDocumentFrame *aFrame = nullptr);

  bool AddTreeItemToTreeOwner(nsIDocShellTreeItem* aItem,
                              nsIDocShellTreeOwner* aOwner,
                              int32_t aParentType,
                              nsIDocShell* aParentNode);

  nsIAtom* TypeAttrName() const {
    return mOwnerContent->IsXULElement()
             ? nsGkAtoms::type : nsGkAtoms::mozframetype;
  }

  
  
  void ResetPermissionManagerStatus();

  void InitializeBrowserAPI();

  nsCOMPtr<nsIDocShell> mDocShell;
  nsCOMPtr<nsIURI> mURIToLoad;
  mozilla::dom::Element* mOwnerContent; 

  
  
  
  nsRefPtr<mozilla::dom::Element> mOwnerContentStrong;

  
  uint32_t mAppIdSentToPermissionManager;

public:
  
  nsRefPtr<nsFrameMessageManager> mMessageManager;
  nsCOMPtr<nsIInProcessContentFrameMessageManager> mChildMessageManager;
private:
  
  
  nsView* mDetachedSubdocViews;
  
  
  
  
  
  nsCOMPtr<nsIDocument> mContainerDocWhileDetached;

  bool mIsPrerendered : 1;
  bool mDepthTooGreat : 1;
  bool mIsTopLevelContent : 1;
  bool mDestroyCalled : 1;
  bool mNeedsAsyncDestroy : 1;
  bool mInSwap : 1;
  bool mInShow : 1;
  bool mHideCalled : 1;
  
  
  
  bool mNetworkCreated : 1;

  bool mRemoteBrowserShown : 1;
  bool mRemoteFrame : 1;
  bool mClipSubdocument : 1;
  bool mClampScrollPosition : 1;
  bool mObservingOwnerContent : 1;

  
  
  
  bool mVisible : 1;

  
  
  nsRefPtr<mozilla::dom::nsIContentParent> mContentParent;
  RenderFrameParent* mCurrentRemoteFrame;
  TabParent* mRemoteBrowser;
  uint64_t mChildID;

  
  
  uint32_t mEventMode;
};

#endif
