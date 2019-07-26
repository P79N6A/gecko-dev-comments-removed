










#include "base/basictypes.h"

#include "prenv.h"

#include "mozIApplication.h"
#include "nsIDOMHTMLIFrameElement.h"
#include "nsIDOMHTMLFrameElement.h"
#include "nsIDOMMozBrowserFrame.h"
#include "nsIDOMWindow.h"
#include "nsIPresShell.h"
#include "nsIContent.h"
#include "nsIContentViewer.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMFile.h"
#include "nsPIDOMWindow.h"
#include "nsIWebNavigation.h"
#include "nsIWebProgress.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDocShellLoadInfo.h"
#include "nsIDOMApplicationRegistry.h"
#include "nsIBaseWindow.h"
#include "nsContentUtils.h"
#include "nsIXPConnect.h"
#include "nsIJSContextStack.h"
#include "nsUnicharUtils.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScrollable.h"
#include "nsFrameLoader.h"
#include "nsIDOMEventTarget.h"
#include "nsIFrame.h"
#include "nsIScrollableFrame.h"
#include "nsSubDocumentFrame.h"
#include "nsError.h"
#include "nsGUIEvent.h"
#include "nsEventDispatcher.h"
#include "nsISHistory.h"
#include "nsISHistoryInternal.h"
#include "nsIDocShellHistory.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIXULWindow.h"
#include "nsIEditor.h"
#include "nsIMozBrowserFrame.h"
#include "nsIPermissionManager.h"

#include "nsLayoutUtils.h"
#include "nsView.h"
#include "nsAsyncDOMEvent.h"

#include "nsIURI.h"
#include "nsIURL.h"
#include "nsNetUtil.h"

#include "nsGkAtoms.h"
#include "nsINameSpaceManager.h"

#include "nsThreadUtils.h"

#include "nsIDOMChromeWindow.h"
#include "nsInProcessTabChildGlobal.h"

#include "Layers.h"

#include "AppProcessChecker.h"
#include "ContentParent.h"
#include "TabParent.h"
#include "mozilla/GuardObjects.h"
#include "mozilla/Preferences.h"
#include "mozilla/unused.h"
#include "mozilla/dom/Element.h"
#include "mozilla/layout/RenderFrameParent.h"
#include "nsIAppsService.h"
#include "GeckoProfiler.h"

#include "jsapi.h"
#include "mozilla/dom/HTMLIFrameElement.h"
#include "nsSandboxFlags.h"

#include "mozilla/dom/StructuredCloneUtils.h"

#ifdef MOZ_XUL
#include "nsXULPopupManager.h"
#endif

using namespace mozilla;
using namespace mozilla::hal;
using namespace mozilla::dom;
using namespace mozilla::dom::ipc;
using namespace mozilla::layers;
using namespace mozilla::layout;
typedef FrameMetrics::ViewID ViewID;

class nsAsyncDocShellDestroyer : public nsRunnable
{
public:
  nsAsyncDocShellDestroyer(nsIDocShell* aDocShell)
    : mDocShell(aDocShell)
  {
  }

  NS_IMETHOD Run()
  {
    nsCOMPtr<nsIBaseWindow> base_win(do_QueryInterface(mDocShell));
    if (base_win) {
      base_win->Destroy();
    }
    return NS_OK;
  }
  nsRefPtr<nsIDocShell> mDocShell;
};

NS_IMPL_ISUPPORTS1(nsContentView, nsIContentView)

bool
nsContentView::IsRoot() const
{
  return mScrollId == FrameMetrics::ROOT_SCROLL_ID;
}

nsresult
nsContentView::Update(const ViewConfig& aConfig)
{
  if (aConfig == mConfig) {
    return NS_OK;
  }
  mConfig = aConfig;

  
  
  if (!mFrameLoader) {
    if (IsRoot()) {
      
      
      
      return NS_OK;
    } else {
      
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  if (RenderFrameParent* rfp = mFrameLoader->GetCurrentRemoteFrame()) {
    rfp->ContentViewScaleChanged(this);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsContentView::ScrollTo(float aXpx, float aYpx)
{
  ViewConfig config(mConfig);
  config.mScrollOffset = nsPoint(nsPresContext::CSSPixelsToAppUnits(aXpx),
                                 nsPresContext::CSSPixelsToAppUnits(aYpx));
  return Update(config);
}

NS_IMETHODIMP
nsContentView::ScrollBy(float aDXpx, float aDYpx)
{
  ViewConfig config(mConfig);
  config.mScrollOffset.MoveBy(nsPresContext::CSSPixelsToAppUnits(aDXpx),
                              nsPresContext::CSSPixelsToAppUnits(aDYpx));
  return Update(config);
}

NS_IMETHODIMP
nsContentView::SetScale(float aXScale, float aYScale)
{
  ViewConfig config(mConfig);
  config.mXScale = aXScale;
  config.mYScale = aYScale;
  return Update(config);
}

NS_IMETHODIMP
nsContentView::GetScrollX(float* aViewScrollX)
{
  *aViewScrollX = nsPresContext::AppUnitsToFloatCSSPixels(
    mConfig.mScrollOffset.x);
  return NS_OK;
}

NS_IMETHODIMP
nsContentView::GetScrollY(float* aViewScrollY)
{
  *aViewScrollY = nsPresContext::AppUnitsToFloatCSSPixels(
    mConfig.mScrollOffset.y);
  return NS_OK;
}

NS_IMETHODIMP
nsContentView::GetViewportWidth(float* aWidth)
{
  *aWidth = nsPresContext::AppUnitsToFloatCSSPixels(mViewportSize.width);
  return NS_OK;
}

NS_IMETHODIMP
nsContentView::GetViewportHeight(float* aHeight)
{
  *aHeight = nsPresContext::AppUnitsToFloatCSSPixels(mViewportSize.height);
  return NS_OK;
}

NS_IMETHODIMP
nsContentView::GetContentWidth(float* aWidth)
{
  *aWidth = nsPresContext::AppUnitsToFloatCSSPixels(mContentSize.width);
  return NS_OK;
}

NS_IMETHODIMP
nsContentView::GetContentHeight(float* aHeight)
{
  *aHeight = nsPresContext::AppUnitsToFloatCSSPixels(mContentSize.height);
  return NS_OK;
}

NS_IMETHODIMP
nsContentView::GetId(nsContentViewId* aId)
{
  NS_ASSERTION(sizeof(nsContentViewId) == sizeof(ViewID),
               "ID size for XPCOM ID and internal ID type are not the same!");
  *aId = mScrollId;
  return NS_OK;
}







#define MAX_SAME_URL_CONTENT_FRAMES 1








#define MAX_DEPTH_CONTENT_FRAMES 10

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsFrameLoader)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mDocShell)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mMessageManager)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mChildMessageManager)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsFrameLoader)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDocShell)
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "nsFrameLoader::mMessageManager");
  cb.NoteXPCOMChild(static_cast<nsIContentFrameMessageManager*>(tmp->mMessageManager.get()));
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mChildMessageManager)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsFrameLoader)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsFrameLoader)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsFrameLoader)
  NS_INTERFACE_MAP_ENTRY(nsIFrameLoader)
  NS_INTERFACE_MAP_ENTRY(nsIContentViewManager)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIFrameLoader)
NS_INTERFACE_MAP_END

nsFrameLoader::nsFrameLoader(Element* aOwner, bool aNetworkCreated)
  : mOwnerContent(aOwner)
  , mAppIdSentToPermissionManager(nsIScriptSecurityManager::NO_APP_ID)
  , mDetachedSubdocViews(nullptr)
  , mDepthTooGreat(false)
  , mIsTopLevelContent(false)
  , mDestroyCalled(false)
  , mNeedsAsyncDestroy(false)
  , mInSwap(false)
  , mInShow(false)
  , mHideCalled(false)
  , mNetworkCreated(aNetworkCreated)
  , mDelayRemoteDialogs(false)
  , mRemoteBrowserShown(false)
  , mRemoteFrame(false)
  , mClipSubdocument(true)
  , mClampScrollPosition(true)
  , mRemoteBrowserInitialized(false)
  , mObservingOwnerContent(false)
  , mCurrentRemoteFrame(nullptr)
  , mRemoteBrowser(nullptr)
  , mRenderMode(RENDER_MODE_DEFAULT)
  , mEventMode(EVENT_MODE_NORMAL_DISPATCH)
{
  ResetPermissionManagerStatus();
}

nsFrameLoader*
nsFrameLoader::Create(Element* aOwner, bool aNetworkCreated)
{
  NS_ENSURE_TRUE(aOwner, nullptr);
  nsIDocument* doc = aOwner->OwnerDoc();
  NS_ENSURE_TRUE(!doc->GetDisplayDocument() &&
                 ((!doc->IsLoadedAsData() && aOwner->GetCurrentDoc()) ||
                   doc->IsStaticDocument()),
                 nullptr);

  return new nsFrameLoader(aOwner, aNetworkCreated);
}

NS_IMETHODIMP
nsFrameLoader::LoadFrame()
{
  NS_ENSURE_TRUE(mOwnerContent, NS_ERROR_NOT_INITIALIZED);

  nsAutoString src;
  GetURL(src);

  src.Trim(" \t\n\r");

  if (src.IsEmpty()) {
    src.AssignLiteral("about:blank");
  }

  nsIDocument* doc = mOwnerContent->OwnerDoc();
  if (doc->IsStaticDocument()) {
    return NS_OK;
  }

  nsCOMPtr<nsIURI> base_uri = mOwnerContent->GetBaseURI();
  const nsAFlatCString &doc_charset = doc->GetDocumentCharacterSet();
  const char *charset = doc_charset.IsEmpty() ? nullptr : doc_charset.get();

  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), src, charset, base_uri);

  
  if (rv == NS_ERROR_MALFORMED_URI) {
    rv = NS_NewURI(getter_AddRefs(uri), NS_LITERAL_STRING("about:blank"),
                   charset, base_uri);
  }

  if (NS_SUCCEEDED(rv)) {
    rv = LoadURI(uri);
  }
  
  if (NS_FAILED(rv)) {
    FireErrorEvent();

    return rv;
  }

  return NS_OK;
}

void
nsFrameLoader::FireErrorEvent()
{
  if (mOwnerContent) {
    nsRefPtr<nsAsyncDOMEvent> event =
      new nsLoadBlockingAsyncDOMEvent(mOwnerContent, NS_LITERAL_STRING("error"),
                                      false, false);
    event->PostDOMEvent();
  }
}

NS_IMETHODIMP
nsFrameLoader::LoadURI(nsIURI* aURI)
{
  if (!aURI)
    return NS_ERROR_INVALID_POINTER;
  NS_ENSURE_STATE(!mDestroyCalled && mOwnerContent);

  nsCOMPtr<nsIDocument> doc = mOwnerContent->OwnerDoc();

  nsresult rv = CheckURILoad(aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  mURIToLoad = aURI;
  rv = doc->InitializeFrameLoader(this);
  if (NS_FAILED(rv)) {
    mURIToLoad = nullptr;
  }
  return rv;
}

nsresult
nsFrameLoader::ReallyStartLoading()
{
  nsresult rv = ReallyStartLoadingInternal();
  if (NS_FAILED(rv)) {
    FireErrorEvent();
  }
  
  return rv;
}

nsresult
nsFrameLoader::ReallyStartLoadingInternal()
{
  NS_ENSURE_STATE(mURIToLoad && mOwnerContent && mOwnerContent->IsInDoc());

  PROFILER_LABEL("nsFrameLoader", "ReallyStartLoading");

  nsresult rv = MaybeCreateDocShell();
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (mRemoteFrame) {
    if (!mRemoteBrowser) {
      TryRemoteBrowser();

      if (!mRemoteBrowser) {
        NS_WARNING("Couldn't create child process for iframe.");
        return NS_ERROR_FAILURE;
      }
    }

    if (mRemoteBrowserShown || ShowRemoteFrame(nsIntSize(0, 0))) {
      
      mRemoteBrowser->LoadURL(mURIToLoad);
    } else {
      NS_WARNING("[nsFrameLoader] ReallyStartLoadingInternal tried but couldn't show remote browser.\n");
    }

    return NS_OK;
  }

  NS_ASSERTION(mDocShell,
               "MaybeCreateDocShell succeeded with a null mDocShell");

  
  rv = CheckURILoad(mURIToLoad);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDocShellLoadInfo> loadInfo;
  mDocShell->CreateLoadInfo(getter_AddRefs(loadInfo));
  NS_ENSURE_TRUE(loadInfo, NS_ERROR_FAILURE);

  
  
  uint32_t sandboxFlags = 0;
  uint32_t parentSandboxFlags = mOwnerContent->OwnerDoc()->GetSandboxFlags();

  HTMLIFrameElement* iframe = HTMLIFrameElement::FromContent(mOwnerContent);

  if (iframe) {
    sandboxFlags = iframe->GetSandboxFlags();
  }

  if (sandboxFlags || parentSandboxFlags) {
    
    sandboxFlags |= parentSandboxFlags;
    mDocShell->SetSandboxFlags(sandboxFlags);
  }

  
  
  
  
  
  
  
  loadInfo->SetOwner(mOwnerContent->NodePrincipal());

  nsCOMPtr<nsIURI> referrer;
  rv = mOwnerContent->NodePrincipal()->GetURI(getter_AddRefs(referrer));
  NS_ENSURE_SUCCESS(rv, rv);

  loadInfo->SetReferrer(referrer);

  
  int32_t flags = nsIWebNavigation::LOAD_FLAGS_NONE;

  
  if (OwnerIsBrowserFrame()) {
    flags = nsIWebNavigation::LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP |
            nsIWebNavigation::LOAD_FLAGS_DISALLOW_INHERIT_OWNER;
  }

  
  bool tmpState = mNeedsAsyncDestroy;
  mNeedsAsyncDestroy = true;
  rv = mDocShell->LoadURI(mURIToLoad, loadInfo, flags, false);
  mNeedsAsyncDestroy = tmpState;
  mURIToLoad = nullptr;
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsFrameLoader::CheckURILoad(nsIURI* aURI)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsIScriptSecurityManager *secMan = nsContentUtils::GetSecurityManager();

  
  nsIPrincipal* principal = mOwnerContent->NodePrincipal();

  
  nsresult rv =
    secMan->CheckLoadURIWithPrincipal(principal, aURI,
                                      nsIScriptSecurityManager::STANDARD);
  if (NS_FAILED(rv)) {
    return rv; 
  }

  
  rv = MaybeCreateDocShell();
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (mRemoteFrame) {
    return NS_OK;
  }
  return CheckForRecursiveLoad(aURI);
}

NS_IMETHODIMP
nsFrameLoader::GetDocShell(nsIDocShell **aDocShell)
{
  *aDocShell = nullptr;
  nsresult rv = NS_OK;

  
  
  
  if (mOwnerContent) {
    nsresult rv = MaybeCreateDocShell();
    if (NS_FAILED(rv))
      return rv;
    if (mRemoteFrame) {
      NS_WARNING("No docshells for remote frames!");
      return rv;
    }
    NS_ASSERTION(mDocShell,
                 "MaybeCreateDocShell succeeded, but null mDocShell");
  }

  *aDocShell = mDocShell;
  NS_IF_ADDREF(*aDocShell);

  return rv;
}

void
nsFrameLoader::Finalize()
{
  nsCOMPtr<nsIBaseWindow> base_win(do_QueryInterface(mDocShell));
  if (base_win) {
    base_win->Destroy();
  }
  mDocShell = nullptr;
}

static void
FirePageHideEvent(nsIDocShellTreeItem* aItem,
                  nsIDOMEventTarget* aChromeEventHandler)
{
  nsCOMPtr<nsIDOMDocument> doc = do_GetInterface(aItem);
  nsCOMPtr<nsIDocument> internalDoc = do_QueryInterface(doc);
  NS_ASSERTION(internalDoc, "What happened here?");
  internalDoc->OnPageHide(true, aChromeEventHandler);

  int32_t childCount = 0;
  aItem->GetChildCount(&childCount);
  nsAutoTArray<nsCOMPtr<nsIDocShellTreeItem>, 8> kids;
  kids.AppendElements(childCount);
  for (int32_t i = 0; i < childCount; ++i) {
    aItem->GetChildAt(i, getter_AddRefs(kids[i]));
  }

  for (uint32_t i = 0; i < kids.Length(); ++i) {
    if (kids[i]) {
      FirePageHideEvent(kids[i], aChromeEventHandler);
    }
  }
}





static void
FirePageShowEvent(nsIDocShellTreeItem* aItem,
                  nsIDOMEventTarget* aChromeEventHandler,
                  bool aFireIfShowing)
{
  int32_t childCount = 0;
  aItem->GetChildCount(&childCount);
  nsAutoTArray<nsCOMPtr<nsIDocShellTreeItem>, 8> kids;
  kids.AppendElements(childCount);
  for (int32_t i = 0; i < childCount; ++i) {
    aItem->GetChildAt(i, getter_AddRefs(kids[i]));
  }

  for (uint32_t i = 0; i < kids.Length(); ++i) {
    if (kids[i]) {
      FirePageShowEvent(kids[i], aChromeEventHandler, aFireIfShowing);
    }
  }

  nsCOMPtr<nsIDOMDocument> doc = do_GetInterface(aItem);
  nsCOMPtr<nsIDocument> internalDoc = do_QueryInterface(doc);
  NS_ASSERTION(internalDoc, "What happened here?");
  if (internalDoc->IsShowing() == aFireIfShowing) {
    internalDoc->OnPageShow(true, aChromeEventHandler);
  }
}

static void
SetTreeOwnerAndChromeEventHandlerOnDocshellTree(nsIDocShellTreeItem* aItem,
                                                nsIDocShellTreeOwner* aOwner,
                                                nsIDOMEventTarget* aHandler)
{
  NS_PRECONDITION(aItem, "Must have item");

  aItem->SetTreeOwner(aOwner);

  int32_t childCount = 0;
  aItem->GetChildCount(&childCount);
  for (int32_t i = 0; i < childCount; ++i) {
    nsCOMPtr<nsIDocShellTreeItem> item;
    aItem->GetChildAt(i, getter_AddRefs(item));
    if (aHandler) {
      nsCOMPtr<nsIDocShell> shell(do_QueryInterface(item));
      shell->SetChromeEventHandler(aHandler);
    }
    SetTreeOwnerAndChromeEventHandlerOnDocshellTree(item, aOwner, aHandler);
  }
}










bool
nsFrameLoader::AddTreeItemToTreeOwner(nsIDocShellTreeItem* aItem,
                                      nsIDocShellTreeOwner* aOwner,
                                      int32_t aParentType,
                                      nsIDocShellTreeNode* aParentNode)
{
  NS_PRECONDITION(aItem, "Must have docshell treeitem");
  NS_PRECONDITION(mOwnerContent, "Must have owning content");
  
  nsAutoString value;
  bool isContent = false;
  mOwnerContent->GetAttr(kNameSpaceID_None, TypeAttrName(), value);

  
  
  

  isContent = value.LowerCaseEqualsLiteral("content") ||
    StringBeginsWith(value, NS_LITERAL_STRING("content-"),
                     nsCaseInsensitiveStringComparator());

  
  
  nsCOMPtr<nsIDOMMozBrowserFrame> mozbrowser =
    do_QueryInterface(mOwnerContent);
  if (mozbrowser) {
    bool isMozbrowser = false;
    mozbrowser->GetMozbrowser(&isMozbrowser);
    isContent |= isMozbrowser;
  }

  if (isContent) {
    

    aItem->SetItemType(nsIDocShellTreeItem::typeContent);
  } else {
    
    
    

    aItem->SetItemType(aParentType);
  }

  
  if (aParentNode) {
    aParentNode->AddChild(aItem);
  }

  bool retval = false;
  if (aParentType == nsIDocShellTreeItem::typeChrome && isContent) {
    retval = true;

    bool is_primary = value.LowerCaseEqualsLiteral("content-primary");

    if (aOwner) {
      bool is_targetable = is_primary ||
        value.LowerCaseEqualsLiteral("content-targetable");
      mOwnerContent->AddMutationObserver(this);
      mObservingOwnerContent = true;
      aOwner->ContentShellAdded(aItem, is_primary, is_targetable, value);
    }
  }

  return retval;
}

static bool
AllDescendantsOfType(nsIDocShellTreeItem* aParentItem, int32_t aType)
{
  int32_t childCount = 0;
  aParentItem->GetChildCount(&childCount);

  for (int32_t i = 0; i < childCount; ++i) {
    nsCOMPtr<nsIDocShellTreeItem> kid;
    aParentItem->GetChildAt(i, getter_AddRefs(kid));

    int32_t kidType;
    kid->GetItemType(&kidType);
    if (kidType != aType || !AllDescendantsOfType(kid, aType)) {
      return false;
    }
  }

  return true;
}





class NS_STACK_CLASS AutoResetInShow {
  private:
    nsFrameLoader* mFrameLoader;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
  public:
    AutoResetInShow(nsFrameLoader* aFrameLoader MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
      : mFrameLoader(aFrameLoader)
    {
      MOZ_GUARD_OBJECT_NOTIFIER_INIT;
    }
    ~AutoResetInShow() { mFrameLoader->mInShow = false; }
};


bool
nsFrameLoader::Show(int32_t marginWidth, int32_t marginHeight,
                    int32_t scrollbarPrefX, int32_t scrollbarPrefY,
                    nsSubDocumentFrame* frame)
{
  if (mInShow) {
    return false;
  }
  
  AutoResetInShow resetInShow(this);
  mInShow = true;

  nsresult rv = MaybeCreateDocShell();
  if (NS_FAILED(rv)) {
    return false;
  }

  if (!mRemoteFrame) {
    if (!mDocShell)
      return false;

    mDocShell->SetMarginWidth(marginWidth);
    mDocShell->SetMarginHeight(marginHeight);

    nsCOMPtr<nsIScrollable> sc = do_QueryInterface(mDocShell);
    if (sc) {
      sc->SetDefaultScrollbarPreferences(nsIScrollable::ScrollOrientation_X,
                                         scrollbarPrefX);
      sc->SetDefaultScrollbarPreferences(nsIScrollable::ScrollOrientation_Y,
                                         scrollbarPrefY);
    }

    nsCOMPtr<nsIPresShell> presShell = mDocShell->GetPresShell();
    if (presShell) {
      
      
      nsIFrame* rootScrollFrame = presShell->GetRootScrollFrame();
      if (rootScrollFrame) {
        presShell->FrameNeedsReflow(rootScrollFrame, nsIPresShell::eResize,
                                    NS_FRAME_IS_DIRTY);
      }
      return true;
    }
  }

  nsIntSize size = frame->GetSubdocumentSize();
  if (mRemoteFrame) {
    return ShowRemoteFrame(size, frame);
  }

  nsView* view = frame->EnsureInnerView();
  if (!view)
    return false;

  nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(mDocShell);
  NS_ASSERTION(baseWindow, "Found a nsIDocShell that isn't a nsIBaseWindow.");
  baseWindow->InitWindow(nullptr, view->GetWidget(), 0, 0,
                         size.width, size.height);
  
  
  
  baseWindow->Create();
  baseWindow->SetVisibility(true);

  
  
  
  
  nsCOMPtr<nsIPresShell> presShell = mDocShell->GetPresShell();
  if (presShell) {
    nsCOMPtr<nsIDOMHTMLDocument> doc =
      do_QueryInterface(presShell->GetDocument());

    if (doc) {
      nsAutoString designMode;
      doc->GetDesignMode(designMode);

      if (designMode.EqualsLiteral("on")) {
        
        
        nsCOMPtr<nsIEditor> editor;
        nsresult rv = mDocShell->GetEditor(getter_AddRefs(editor));
        NS_ENSURE_SUCCESS(rv, false);

        doc->SetDesignMode(NS_LITERAL_STRING("off"));
        doc->SetDesignMode(NS_LITERAL_STRING("on"));
      } else {
        
        bool editable = false,
             hasEditingSession = false;
        mDocShell->GetEditable(&editable);
        mDocShell->GetHasEditingSession(&hasEditingSession);
        nsCOMPtr<nsIEditor> editor;
        mDocShell->GetEditor(getter_AddRefs(editor));
        if (editable && hasEditingSession && editor) {
          editor->PostCreate();
        }
      }
    }
  }

  mInShow = false;
  if (mHideCalled) {
    mHideCalled = false;
    Hide();
    return false;
  }
  return true;
}

void
nsFrameLoader::MarginsChanged(uint32_t aMarginWidth,
                              uint32_t aMarginHeight)
{
  
  if (mRemoteFrame)
    return;

  
  
  
  if (!mDocShell)
    return;

  
  mDocShell->SetMarginWidth(aMarginWidth);
  mDocShell->SetMarginHeight(aMarginHeight);

  
  nsRefPtr<nsPresContext> presContext;
  mDocShell->GetPresContext(getter_AddRefs(presContext));
  if (presContext)
    presContext->RebuildAllStyleData(nsChangeHint(0));
}

bool
nsFrameLoader::ShowRemoteFrame(const nsIntSize& size,
                               nsSubDocumentFrame *aFrame)
{
  NS_ASSERTION(mRemoteFrame, "ShowRemote only makes sense on remote frames.");

  if (!mRemoteBrowser) {
    TryRemoteBrowser();

    if (!mRemoteBrowser) {
      NS_ERROR("Couldn't create child process.");
      return false;
    }
  }

  
  
  
  if (!mRemoteBrowserShown) {
    if (!mOwnerContent ||
        !mOwnerContent->GetCurrentDoc()) {
      return false;
    }

    nsRefPtr<layers::LayerManager> layerManager =
      nsContentUtils::LayerManagerForDocument(mOwnerContent->GetCurrentDoc());
    if (!layerManager) {
      
      return false;
    }

    mRemoteBrowser->Show(size);
    mRemoteBrowserShown = true;

    EnsureMessageManager();

    nsCOMPtr<nsIObserverService> os = services::GetObserverService();
    if (OwnerIsBrowserOrAppFrame() && os && !mRemoteBrowserInitialized) {
      os->NotifyObservers(NS_ISUPPORTS_CAST(nsIFrameLoader*, this),
                          "remote-browser-frame-shown", NULL);
      mRemoteBrowserInitialized = true;
    }
  } else {
    nsRect dimensions;
    NS_ENSURE_SUCCESS(GetWindowDimensions(dimensions), false);

    
    if (!aFrame || !(aFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
      mRemoteBrowser->UpdateDimensions(dimensions, size);
    }
  }

  return true;
}

void
nsFrameLoader::Hide()
{
  if (mHideCalled) {
    return;
  }
  if (mInShow) {
    mHideCalled = true;
    return;
  }

  if (!mDocShell)
    return;

  nsCOMPtr<nsIContentViewer> contentViewer;
  mDocShell->GetContentViewer(getter_AddRefs(contentViewer));
  if (contentViewer)
    contentViewer->SetSticky(false);

  nsCOMPtr<nsIBaseWindow> baseWin = do_QueryInterface(mDocShell);
  NS_ASSERTION(baseWin,
               "Found an nsIDocShell which doesn't implement nsIBaseWindow.");
  baseWin->SetVisibility(false);
  baseWin->SetParentWidget(nullptr);
}

nsresult
nsFrameLoader::SwapWithOtherLoader(nsFrameLoader* aOther,
                                   nsRefPtr<nsFrameLoader>& aFirstToSwap,
                                   nsRefPtr<nsFrameLoader>& aSecondToSwap)
{
  NS_PRECONDITION((aFirstToSwap == this && aSecondToSwap == aOther) ||
                  (aFirstToSwap == aOther && aSecondToSwap == this),
                  "Swapping some sort of random loaders?");
  NS_ENSURE_STATE(!mInShow && !aOther->mInShow);

  Element* ourContent = mOwnerContent;
  Element* otherContent = aOther->mOwnerContent;

  if (!ourContent || !otherContent) {
    
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  bool equal;
  nsresult rv =
    ourContent->NodePrincipal()->Equals(otherContent->NodePrincipal(), &equal);
  if (NS_FAILED(rv) || !equal) {
    
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsCOMPtr<nsIDocShell> ourDocshell = GetExistingDocShell();
  nsCOMPtr<nsIDocShell> otherDocshell = aOther->GetExistingDocShell();
  if (!ourDocshell || !otherDocshell) {
    
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  
  
  nsCOMPtr<nsIDocShellTreeItem> ourRootTreeItem, otherRootTreeItem;
  ourDocshell->GetSameTypeRootTreeItem(getter_AddRefs(ourRootTreeItem));
  otherDocshell->GetSameTypeRootTreeItem(getter_AddRefs(otherRootTreeItem));
  nsCOMPtr<nsIWebNavigation> ourRootWebnav =
    do_QueryInterface(ourRootTreeItem);
  nsCOMPtr<nsIWebNavigation> otherRootWebnav =
    do_QueryInterface(otherRootTreeItem);

  if (!ourRootWebnav || !otherRootWebnav) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsCOMPtr<nsISHistory> ourHistory;
  nsCOMPtr<nsISHistory> otherHistory;
  ourRootWebnav->GetSessionHistory(getter_AddRefs(ourHistory));
  otherRootWebnav->GetSessionHistory(getter_AddRefs(otherHistory));

  if ((ourRootTreeItem != ourDocshell || otherRootTreeItem != otherDocshell) &&
      (ourHistory || otherHistory)) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  
  
  int32_t ourType = nsIDocShellTreeItem::typeChrome;
  int32_t otherType = nsIDocShellTreeItem::typeChrome;
  ourDocshell->GetItemType(&ourType);
  otherDocshell->GetItemType(&otherType);
  if (ourType != otherType) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  
  
  
  if (ourType != nsIDocShellTreeItem::typeContent &&
      (!AllDescendantsOfType(ourDocshell, ourType) ||
       !AllDescendantsOfType(otherDocshell, otherType))) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  
  
  
  nsCOMPtr<nsIDocShellTreeOwner> ourOwner, otherOwner;
  ourDocshell->GetTreeOwner(getter_AddRefs(ourOwner));
  otherDocshell->GetTreeOwner(getter_AddRefs(otherOwner));
  

  nsCOMPtr<nsIDocShellTreeItem> ourParentItem, otherParentItem;
  ourDocshell->GetParent(getter_AddRefs(ourParentItem));
  otherDocshell->GetParent(getter_AddRefs(otherParentItem));
  if (!ourParentItem || !otherParentItem) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  int32_t ourParentType = nsIDocShellTreeItem::typeContent;
  int32_t otherParentType = nsIDocShellTreeItem::typeContent;
  ourParentItem->GetItemType(&ourParentType);
  otherParentItem->GetItemType(&otherParentType);
  if (ourParentType != otherParentType) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsCOMPtr<nsPIDOMWindow> ourWindow = do_GetInterface(ourDocshell);
  nsCOMPtr<nsPIDOMWindow> otherWindow = do_GetInterface(otherDocshell);

  nsCOMPtr<nsIDOMElement> ourFrameElement =
    ourWindow->GetFrameElementInternal();
  nsCOMPtr<nsIDOMElement> otherFrameElement =
    otherWindow->GetFrameElementInternal();

  nsCOMPtr<nsIDOMEventTarget> ourChromeEventHandler =
    do_QueryInterface(ourWindow->GetChromeEventHandler());
  nsCOMPtr<nsIDOMEventTarget> otherChromeEventHandler =
    do_QueryInterface(otherWindow->GetChromeEventHandler());

  NS_ASSERTION(SameCOMIdentity(ourFrameElement, ourContent) &&
               SameCOMIdentity(otherFrameElement, otherContent) &&
               SameCOMIdentity(ourChromeEventHandler, ourContent) &&
               SameCOMIdentity(otherChromeEventHandler, otherContent),
               "How did that happen, exactly?");

  nsCOMPtr<nsIDocument> ourChildDocument =
    do_QueryInterface(ourWindow->GetExtantDocument());
  nsCOMPtr<nsIDocument> otherChildDocument =
    do_QueryInterface(otherWindow->GetExtantDocument());
  if (!ourChildDocument || !otherChildDocument) {
    
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsCOMPtr<nsIDocument> ourParentDocument =
    ourChildDocument->GetParentDocument();
  nsCOMPtr<nsIDocument> otherParentDocument =
    otherChildDocument->GetParentDocument();

  
  nsIDocument* ourDoc = ourContent->GetCurrentDoc();
  nsIDocument* otherDoc = otherContent->GetCurrentDoc();
  if (!ourDoc || !otherDoc) {
    
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_ASSERTION(ourDoc == ourParentDocument, "Unexpected parent document");
  NS_ASSERTION(otherDoc == otherParentDocument, "Unexpected parent document");

  nsIPresShell* ourShell = ourDoc->GetShell();
  nsIPresShell* otherShell = otherDoc->GetShell();
  if (!ourShell || !otherShell) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  if (ourDocshell->GetIsBrowserElement() !=
      otherDocshell->GetIsBrowserElement() ||
      ourDocshell->GetIsApp() != otherDocshell->GetIsApp()) {
      return NS_ERROR_NOT_IMPLEMENTED;
  }

  if (mInSwap || aOther->mInSwap) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  mInSwap = aOther->mInSwap = true;

  
  
  
  FirePageShowEvent(ourDocshell, ourChromeEventHandler, false);
  FirePageShowEvent(otherDocshell, otherChromeEventHandler, false);
  FirePageHideEvent(ourDocshell, ourChromeEventHandler);
  FirePageHideEvent(otherDocshell, otherChromeEventHandler);
  
  nsIFrame* ourFrame = ourContent->GetPrimaryFrame();
  nsIFrame* otherFrame = otherContent->GetPrimaryFrame();
  if (!ourFrame || !otherFrame) {
    mInSwap = aOther->mInSwap = false;
    FirePageShowEvent(ourDocshell, ourChromeEventHandler, true);
    FirePageShowEvent(otherDocshell, otherChromeEventHandler, true);
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsSubDocumentFrame* ourFrameFrame = do_QueryFrame(ourFrame);
  if (!ourFrameFrame) {
    mInSwap = aOther->mInSwap = false;
    FirePageShowEvent(ourDocshell, ourChromeEventHandler, true);
    FirePageShowEvent(otherDocshell, otherChromeEventHandler, true);
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  rv = ourFrameFrame->BeginSwapDocShells(otherFrame);
  if (NS_FAILED(rv)) {
    mInSwap = aOther->mInSwap = false;
    FirePageShowEvent(ourDocshell, ourChromeEventHandler, true);
    FirePageShowEvent(otherDocshell, otherChromeEventHandler, true);
    return rv;
  }

  
  
  ourParentItem->RemoveChild(ourDocshell);
  otherParentItem->RemoveChild(otherDocshell);
  if (ourType == nsIDocShellTreeItem::typeContent) {
    ourOwner->ContentShellRemoved(ourDocshell);
    otherOwner->ContentShellRemoved(otherDocshell);
  }
  
  ourParentItem->AddChild(otherDocshell);
  otherParentItem->AddChild(ourDocshell);

  
  ourDocshell->SetChromeEventHandler(otherChromeEventHandler);
  otherDocshell->SetChromeEventHandler(ourChromeEventHandler);
  
  
  SetTreeOwnerAndChromeEventHandlerOnDocshellTree(ourDocshell, otherOwner,
    ourType == nsIDocShellTreeItem::typeContent ? otherChromeEventHandler : nullptr);
  SetTreeOwnerAndChromeEventHandlerOnDocshellTree(otherDocshell, ourOwner,
    ourType == nsIDocShellTreeItem::typeContent ? ourChromeEventHandler : nullptr);

  
  
  
  SetOwnerContent(otherContent);
  aOther->SetOwnerContent(ourContent);

  AddTreeItemToTreeOwner(ourDocshell, otherOwner, otherParentType, nullptr);
  aOther->AddTreeItemToTreeOwner(otherDocshell, ourOwner, ourParentType,
                                 nullptr);

  
  
  
  
  ourParentDocument->SetSubDocumentFor(ourContent, nullptr);
  otherParentDocument->SetSubDocumentFor(otherContent, nullptr);
  ourParentDocument->SetSubDocumentFor(ourContent, otherChildDocument);
  otherParentDocument->SetSubDocumentFor(otherContent, ourChildDocument);

  ourWindow->SetFrameElementInternal(otherFrameElement);
  otherWindow->SetFrameElementInternal(ourFrameElement);

  nsRefPtr<nsFrameMessageManager> ourMessageManager = mMessageManager;
  nsRefPtr<nsFrameMessageManager> otherMessageManager = aOther->mMessageManager;
  
  if (mChildMessageManager) {
    nsInProcessTabChildGlobal* tabChild =
      static_cast<nsInProcessTabChildGlobal*>(mChildMessageManager.get());
    tabChild->SetOwner(otherContent);
    tabChild->SetChromeMessageManager(otherMessageManager);
  }
  if (aOther->mChildMessageManager) {
    nsInProcessTabChildGlobal* otherTabChild =
      static_cast<nsInProcessTabChildGlobal*>(aOther->mChildMessageManager.get());
    otherTabChild->SetOwner(ourContent);
    otherTabChild->SetChromeMessageManager(ourMessageManager);
  }
  
  nsFrameMessageManager* ourParentManager = mMessageManager ?
    mMessageManager->GetParentManager() : nullptr;
  nsFrameMessageManager* otherParentManager = aOther->mMessageManager ?
    aOther->mMessageManager->GetParentManager() : nullptr;
  JSContext* thisCx =
    mMessageManager ? mMessageManager->GetJSContext() : nullptr;
  JSContext* otherCx = 
    aOther->mMessageManager ? aOther->mMessageManager->GetJSContext() : nullptr;
  if (mMessageManager) {
    mMessageManager->RemoveFromParent();
    mMessageManager->SetJSContext(otherCx);
    mMessageManager->SetParentManager(otherParentManager);
    mMessageManager->SetCallback(aOther, false);
  }
  if (aOther->mMessageManager) {
    aOther->mMessageManager->RemoveFromParent();
    aOther->mMessageManager->SetJSContext(thisCx);
    aOther->mMessageManager->SetParentManager(ourParentManager);
    aOther->mMessageManager->SetCallback(this, false);
  }
  mMessageManager.swap(aOther->mMessageManager);

  aFirstToSwap.swap(aSecondToSwap);

  
  nsCOMPtr<nsISHistoryInternal> ourInternalHistory =
    do_QueryInterface(ourHistory);
  nsCOMPtr<nsISHistoryInternal> otherInternalHistory =
    do_QueryInterface(otherHistory);
  if (ourInternalHistory) {
    ourInternalHistory->EvictAllContentViewers();
  }
  if (otherInternalHistory) {
    otherInternalHistory->EvictAllContentViewers();
  }

  NS_ASSERTION(ourFrame == ourContent->GetPrimaryFrame() &&
               otherFrame == otherContent->GetPrimaryFrame(),
               "changed primary frame");

  ourFrameFrame->EndSwapDocShells(otherFrame);

  
  
  
  
  
  ourShell->BackingScaleFactorChanged();
  otherShell->BackingScaleFactorChanged();

  ourParentDocument->FlushPendingNotifications(Flush_Layout);
  otherParentDocument->FlushPendingNotifications(Flush_Layout);

  FirePageShowEvent(ourDocshell, otherChromeEventHandler, true);
  FirePageShowEvent(otherDocshell, ourChromeEventHandler, true);

  mInSwap = aOther->mInSwap = false;
  return NS_OK;
}

void
nsFrameLoader::DestroyChild()
{
  if (mRemoteBrowser) {
    mRemoteBrowser->SetOwnerElement(nullptr);
    mRemoteBrowser->Destroy();
    mRemoteBrowser = nullptr;
  }
}

NS_IMETHODIMP
nsFrameLoader::Destroy()
{
  if (mDestroyCalled) {
    return NS_OK;
  }
  mDestroyCalled = true;

  if (mMessageManager) {
    mMessageManager->Disconnect();
  }
  if (mChildMessageManager) {
    static_cast<nsInProcessTabChildGlobal*>(mChildMessageManager.get())->Disconnect();
  }

  nsCOMPtr<nsIDocument> doc;
  bool dynamicSubframeRemoval = false;
  if (mOwnerContent) {
    doc = mOwnerContent->OwnerDoc();
    dynamicSubframeRemoval = !mIsTopLevelContent && !doc->InUnlinkOrDeletion();
    doc->SetSubDocumentFor(mOwnerContent, nullptr);

    SetOwnerContent(nullptr);
  }
  DestroyChild();

  
  if (dynamicSubframeRemoval) {
    nsCOMPtr<nsIDocShellHistory> dhistory = do_QueryInterface(mDocShell);
    if (dhistory) {
      dhistory->RemoveFromSessionHistory();
    }
  }

  
  if (mIsTopLevelContent) {
    if (mDocShell) {
      nsCOMPtr<nsIDocShellTreeItem> parentItem;
      mDocShell->GetParent(getter_AddRefs(parentItem));
      nsCOMPtr<nsIDocShellTreeOwner> owner = do_GetInterface(parentItem);
      if (owner) {
        owner->ContentShellRemoved(mDocShell);
      }
    }
  }
  
  
  nsCOMPtr<nsPIDOMWindow> win_private(do_GetInterface(mDocShell));
  if (win_private) {
    win_private->SetFrameElementInternal(nullptr);
  }

  if ((mNeedsAsyncDestroy || !doc ||
       NS_FAILED(doc->FinalizeFrameLoader(this))) && mDocShell) {
    nsCOMPtr<nsIRunnable> event = new nsAsyncDocShellDestroyer(mDocShell);
    NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);
    NS_DispatchToCurrentThread(event);

    
    

    mDocShell = nullptr;
  }

  

  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::GetDepthTooGreat(bool* aDepthTooGreat)
{
  *aDepthTooGreat = mDepthTooGreat;
  return NS_OK;
}

void
nsFrameLoader::SetOwnerContent(Element* aContent)
{
  if (mObservingOwnerContent) {
    mObservingOwnerContent = false;
    mOwnerContent->RemoveMutationObserver(this);
  }
  mOwnerContent = aContent;
  if (RenderFrameParent* rfp = GetCurrentRemoteFrame()) {
    rfp->OwnerContentChanged(aContent);
  }

  ResetPermissionManagerStatus();
}

bool
nsFrameLoader::OwnerIsBrowserOrAppFrame()
{
  nsCOMPtr<nsIMozBrowserFrame> browserFrame = do_QueryInterface(mOwnerContent);
  return browserFrame ? browserFrame->GetReallyIsBrowserOrApp() : false;
}

bool
nsFrameLoader::OwnerIsAppFrame()
{
  nsCOMPtr<nsIMozBrowserFrame> browserFrame = do_QueryInterface(mOwnerContent);
  return browserFrame ? browserFrame->GetReallyIsApp() : false;
}

bool
nsFrameLoader::OwnerIsBrowserFrame()
{
  return OwnerIsBrowserOrAppFrame() && !OwnerIsAppFrame();
}

void
nsFrameLoader::GetOwnerAppManifestURL(nsAString& aOut)
{
  aOut.Truncate();
  nsCOMPtr<nsIMozBrowserFrame> browserFrame = do_QueryInterface(mOwnerContent);
  if (browserFrame) {
    browserFrame->GetAppManifestURL(aOut);
  }
}

already_AddRefed<mozIApplication>
nsFrameLoader::GetOwnApp()
{
  nsAutoString manifest;
  GetOwnerAppManifestURL(manifest);
  if (manifest.IsEmpty()) {
    return nullptr;
  }

  nsCOMPtr<nsIAppsService> appsService = do_GetService(APPS_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(appsService, nullptr);

  nsCOMPtr<mozIDOMApplication> domApp;
  appsService->GetAppByManifestURL(manifest, getter_AddRefs(domApp));

  nsCOMPtr<mozIApplication> app = do_QueryInterface(domApp);
  MOZ_ASSERT_IF(domApp, app);
  return app.forget();
}

already_AddRefed<mozIApplication>
nsFrameLoader::GetContainingApp()
{
  
  uint32_t appId = mOwnerContent->NodePrincipal()->GetAppId();
  MOZ_ASSERT(appId != nsIScriptSecurityManager::UNKNOWN_APP_ID);

  if (appId == nsIScriptSecurityManager::NO_APP_ID ||
      appId == nsIScriptSecurityManager::UNKNOWN_APP_ID) {
    return nullptr;
  }

  nsCOMPtr<nsIAppsService> appsService = do_GetService(APPS_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(appsService, nullptr);

  nsCOMPtr<mozIDOMApplication> domApp;
  appsService->GetAppByLocalId(appId, getter_AddRefs(domApp));
  MOZ_ASSERT(domApp);

  nsCOMPtr<mozIApplication> app = do_QueryInterface(domApp);
  MOZ_ASSERT_IF(domApp, app);
  return app.forget();
}

bool
nsFrameLoader::ShouldUseRemoteProcess()
{
  if (PR_GetEnv("MOZ_DISABLE_OOP_TABS") ||
      Preferences::GetBool("dom.ipc.tabs.disabled", false)) {
    return false;
  }

  
  
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    return false;
  }

  
  
  if (OwnerIsBrowserOrAppFrame() &&
      !mOwnerContent->HasAttr(kNameSpaceID_None, nsGkAtoms::Remote)) {

    return Preferences::GetBool("dom.ipc.browser_frames.oop_by_default", false);
  }

  
  
  return (OwnerIsBrowserOrAppFrame() ||
          mOwnerContent->GetNameSpaceID() == kNameSpaceID_XUL) &&
         mOwnerContent->AttrValueIs(kNameSpaceID_None,
                                    nsGkAtoms::Remote,
                                    nsGkAtoms::_true,
                                    eCaseMatters);
}

nsresult
nsFrameLoader::MaybeCreateDocShell()
{
  if (mDocShell) {
    return NS_OK;
  }
  if (mRemoteFrame) {
    return NS_OK;
  }
  NS_ENSURE_STATE(!mDestroyCalled);

  if (ShouldUseRemoteProcess()) {
    mRemoteFrame = true;
    return NS_OK;
  }

  
  
  
  nsIDocument* doc = mOwnerContent->OwnerDoc();
  if (!(doc->IsStaticDocument() || mOwnerContent->IsInDoc())) {
    return NS_ERROR_UNEXPECTED;
  }

  if (doc->IsResourceDoc() || !doc->IsActive()) {
    
    
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsISupports> container =
    doc->GetContainer();
  nsCOMPtr<nsIWebNavigation> parentAsWebNav = do_QueryInterface(container);
  NS_ENSURE_STATE(parentAsWebNav);

  
  mDocShell = do_CreateInstance("@mozilla.org/docshell;1");
  NS_ENSURE_TRUE(mDocShell, NS_ERROR_FAILURE);

  if (!mNetworkCreated) {
    nsCOMPtr<nsIDocShellHistory> history = do_QueryInterface(mDocShell);
    if (history) {
      history->SetCreatedDynamically(true);
    }
  }

  
  NS_ENSURE_TRUE(mDocShell, NS_ERROR_FAILURE);
  nsAutoString frameName;

  int32_t namespaceID = mOwnerContent->GetNameSpaceID();
  if (namespaceID == kNameSpaceID_XHTML && !mOwnerContent->IsInHTMLDocument()) {
    mOwnerContent->GetAttr(kNameSpaceID_None, nsGkAtoms::id, frameName);
  } else {
    mOwnerContent->GetAttr(kNameSpaceID_None, nsGkAtoms::name, frameName);
    
    
    if (frameName.IsEmpty() && namespaceID == kNameSpaceID_XUL) {
      mOwnerContent->GetAttr(kNameSpaceID_None, nsGkAtoms::id, frameName);
    }
  }

  if (!frameName.IsEmpty()) {
    mDocShell->SetName(frameName.get());
  }

  
  
  

  nsCOMPtr<nsIDocShellTreeNode> parentAsNode(do_QueryInterface(parentAsWebNav));
  if (parentAsNode) {
    
    

    nsCOMPtr<nsIDocShellTreeItem> parentAsItem =
      do_QueryInterface(parentAsNode);

    int32_t parentType;
    parentAsItem->GetItemType(&parentType);

    
    
    nsCOMPtr<nsIDocShellTreeOwner> parentTreeOwner;
    parentAsItem->GetTreeOwner(getter_AddRefs(parentTreeOwner));
    NS_ENSURE_STATE(parentTreeOwner);
    mIsTopLevelContent =
      AddTreeItemToTreeOwner(mDocShell, parentTreeOwner, parentType,
                             parentAsNode);

    
    
    nsCOMPtr<nsIDOMEventTarget> chromeEventHandler;

    if (parentType == nsIDocShellTreeItem::typeChrome) {
      
      

      chromeEventHandler = do_QueryInterface(mOwnerContent);
      NS_ASSERTION(chromeEventHandler,
                   "This mContent should implement this.");
    } else {
      nsCOMPtr<nsIDocShell> parentShell(do_QueryInterface(parentAsNode));

      
      

      parentShell->GetChromeEventHandler(getter_AddRefs(chromeEventHandler));
    }

    mDocShell->SetChromeEventHandler(chromeEventHandler);
  }

  
  
  
  

  
  nsCOMPtr<nsIDOMElement> frame_element(do_QueryInterface(mOwnerContent));
  NS_ASSERTION(frame_element, "frame loader owner element not a DOM element!");

  nsCOMPtr<nsPIDOMWindow> win_private(do_GetInterface(mDocShell));
  nsCOMPtr<nsIBaseWindow> base_win(do_QueryInterface(mDocShell));
  if (win_private) {
    win_private->SetFrameElementInternal(frame_element);
  }

  
  
  
  if (NS_FAILED(base_win->Create()) || !win_private) {
    
    NS_WARNING("Something wrong when creating the docshell for a frameloader!");
    return NS_ERROR_FAILURE;
  }

  EnsureMessageManager();

  if (OwnerIsAppFrame()) {
    
    MOZ_ASSERT(!OwnerIsBrowserFrame());

    nsCOMPtr<mozIApplication> ownApp = GetOwnApp();
    MOZ_ASSERT(ownApp);
    uint32_t ownAppId = nsIScriptSecurityManager::NO_APP_ID;
    if (ownApp) {
      NS_ENSURE_SUCCESS(ownApp->GetLocalId(&ownAppId), NS_ERROR_FAILURE);
    }

    mDocShell->SetIsApp(ownAppId);
  }

  if (OwnerIsBrowserFrame()) {
    
    MOZ_ASSERT(!OwnerIsAppFrame());

    nsCOMPtr<mozIApplication> containingApp = GetContainingApp();
    uint32_t containingAppId = nsIScriptSecurityManager::NO_APP_ID;
    if (containingApp) {
      NS_ENSURE_SUCCESS(containingApp->GetLocalId(&containingAppId),
                        NS_ERROR_FAILURE);
    }
    mDocShell->SetIsBrowserInsideApp(containingAppId);
  }

  if (OwnerIsBrowserOrAppFrame()) {
    nsCOMPtr<nsIObserverService> os = services::GetObserverService();
    if (os) {
      os->NotifyObservers(NS_ISUPPORTS_CAST(nsIFrameLoader*, this),
                          "in-process-browser-or-app-frame-shown", NULL);
    }

    if (mMessageManager) {
      mMessageManager->LoadFrameScript(
        NS_LITERAL_STRING("chrome://global/content/BrowserElementChild.js"),
         true);
    }
  }

  return NS_OK;
}

void
nsFrameLoader::GetURL(nsString& aURI)
{
  aURI.Truncate();

  if (mOwnerContent->Tag() == nsGkAtoms::object) {
    mOwnerContent->GetAttr(kNameSpaceID_None, nsGkAtoms::data, aURI);
  } else {
    mOwnerContent->GetAttr(kNameSpaceID_None, nsGkAtoms::src, aURI);
  }
}

nsresult
nsFrameLoader::CheckForRecursiveLoad(nsIURI* aURI)
{
  nsresult rv;

  mDepthTooGreat = false;
  rv = MaybeCreateDocShell();
  if (NS_FAILED(rv)) {
    return rv;
  }
  NS_ASSERTION(!mRemoteFrame,
               "Shouldn't call CheckForRecursiveLoad on remote frames.");
  if (!mDocShell) {
    return NS_ERROR_FAILURE;
  }

  
  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  mDocShell->GetTreeOwner(getter_AddRefs(treeOwner));
  NS_WARN_IF_FALSE(treeOwner,
                   "Trying to load a new url to a docshell without owner!");
  NS_ENSURE_STATE(treeOwner);
  
  
  int32_t ourType;
  rv = mDocShell->GetItemType(&ourType);
  if (NS_SUCCEEDED(rv) && ourType != nsIDocShellTreeItem::typeContent) {
    
    
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIDocShellTreeItem> parentAsItem;
  mDocShell->GetSameTypeParent(getter_AddRefs(parentAsItem));
  int32_t depth = 0;
  while (parentAsItem) {
    ++depth;
    
    if (depth >= MAX_DEPTH_CONTENT_FRAMES) {
      mDepthTooGreat = true;
      NS_WARNING("Too many nested content frames so giving up");

      return NS_ERROR_UNEXPECTED; 
    }

    nsCOMPtr<nsIDocShellTreeItem> temp;
    temp.swap(parentAsItem);
    temp->GetSameTypeParent(getter_AddRefs(parentAsItem));
  }
  
  
  int32_t matchCount = 0;
  mDocShell->GetSameTypeParent(getter_AddRefs(parentAsItem));
  while (parentAsItem) {
    
    nsCOMPtr<nsIWebNavigation> parentAsNav(do_QueryInterface(parentAsItem));
    if (parentAsNav) {
      
      nsCOMPtr<nsIURI> parentURI;
      parentAsNav->GetCurrentURI(getter_AddRefs(parentURI));
      if (parentURI) {
        
        bool equal;
        rv = aURI->EqualsExceptRef(parentURI, &equal);
        NS_ENSURE_SUCCESS(rv, rv);
        
        if (equal) {
          matchCount++;
          if (matchCount >= MAX_SAME_URL_CONTENT_FRAMES) {
            NS_WARNING("Too many nested content frames have the same url (recursion?) so giving up");
            return NS_ERROR_UNEXPECTED;
          }
        }
      }
    }
    nsCOMPtr<nsIDocShellTreeItem> temp;
    temp.swap(parentAsItem);
    temp->GetSameTypeParent(getter_AddRefs(parentAsItem));
  }

  return NS_OK;
}

nsresult
nsFrameLoader::GetWindowDimensions(nsRect& aRect)
{
  
  nsIDocument* doc = mOwnerContent->GetDocument();
  if (!doc) {
    return NS_ERROR_FAILURE;
  }

  if (doc->GetDisplayDocument()) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIWebNavigation> parentAsWebNav =
    do_GetInterface(doc->GetScriptGlobalObject());

  if (!parentAsWebNav) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDocShellTreeItem> parentAsItem(do_QueryInterface(parentAsWebNav));

  nsCOMPtr<nsIDocShellTreeOwner> parentOwner;
  if (NS_FAILED(parentAsItem->GetTreeOwner(getter_AddRefs(parentOwner))) ||
      !parentOwner) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIBaseWindow> treeOwnerAsWin(do_GetInterface(parentOwner));
  treeOwnerAsWin->GetPosition(&aRect.x, &aRect.y);
  treeOwnerAsWin->GetSize(&aRect.width, &aRect.height);
  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::UpdatePositionAndSize(nsSubDocumentFrame *aIFrame)
{
  if (mRemoteFrame) {
    if (mRemoteBrowser) {
      nsIntSize size = aIFrame->GetSubdocumentSize();
      nsRect dimensions;
      NS_ENSURE_SUCCESS(GetWindowDimensions(dimensions), NS_ERROR_FAILURE);
      mRemoteBrowser->UpdateDimensions(dimensions, size);
    }
    return NS_OK;
  }
  return UpdateBaseWindowPositionAndSize(aIFrame);
}

nsresult
nsFrameLoader::UpdateBaseWindowPositionAndSize(nsSubDocumentFrame *aIFrame)
{
  nsCOMPtr<nsIDocShell> docShell;
  GetDocShell(getter_AddRefs(docShell));
  nsCOMPtr<nsIBaseWindow> baseWindow(do_QueryInterface(docShell));

  
  if (baseWindow) {
    int32_t x = 0;
    int32_t y = 0;

    nsWeakFrame weakFrame(aIFrame);

    baseWindow->GetPositionAndSize(&x, &y, nullptr, nullptr);

    if (!weakFrame.IsAlive()) {
      
      return NS_OK;
    }

    nsIntSize size = aIFrame->GetSubdocumentSize();

    baseWindow->SetPositionAndSize(x, y, size.width, size.height, false);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::GetRenderMode(uint32_t* aRenderMode)
{
  *aRenderMode = mRenderMode;
  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::SetRenderMode(uint32_t aRenderMode)
{
  if (aRenderMode == mRenderMode) {
    return NS_OK;
  }

  mRenderMode = aRenderMode;
  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::GetEventMode(uint32_t* aEventMode)
{
  *aEventMode = mEventMode;
  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::SetEventMode(uint32_t aEventMode)
{
  mEventMode = aEventMode;
  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::GetClipSubdocument(bool* aResult)
{
  *aResult = mClipSubdocument;
  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::SetClipSubdocument(bool aClip)
{
  mClipSubdocument = aClip;
  nsIFrame* frame = GetPrimaryFrameOfOwningContent();
  if (frame) {
    frame->InvalidateFrame();
    frame->PresContext()->PresShell()->
      FrameNeedsReflow(frame, nsIPresShell::eResize, NS_FRAME_IS_DIRTY);
    nsSubDocumentFrame* subdocFrame = do_QueryFrame(frame);
    if (subdocFrame) {
      nsIFrame* subdocRootFrame = subdocFrame->GetSubdocumentRootFrame();
      if (subdocRootFrame) {
        nsIFrame* subdocRootScrollFrame = subdocRootFrame->PresContext()->PresShell()->
          GetRootScrollFrame();
        if (subdocRootScrollFrame) {
          frame->PresContext()->PresShell()->
            FrameNeedsReflow(frame, nsIPresShell::eResize, NS_FRAME_IS_DIRTY);
        }
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::GetClampScrollPosition(bool* aResult)
{
  *aResult = mClampScrollPosition;
  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::SetClampScrollPosition(bool aClamp)
{
  mClampScrollPosition = aClamp;

  
  if (aClamp) {
    nsIFrame* frame = GetPrimaryFrameOfOwningContent();
    if (frame) {
      nsSubDocumentFrame* subdocFrame = do_QueryFrame(frame);
      if (subdocFrame) {
        nsIFrame* subdocRootFrame = subdocFrame->GetSubdocumentRootFrame();
        if (subdocRootFrame) {
          nsIScrollableFrame* subdocRootScrollFrame = subdocRootFrame->PresContext()->PresShell()->
            GetRootScrollFrameAsScrollable();
          if (subdocRootScrollFrame) {
            subdocRootScrollFrame->ScrollTo(subdocRootScrollFrame->GetScrollPosition(), nsIScrollableFrame::INSTANT);
          }
        }
      }
    }
  }
  return NS_OK;
}

bool
nsFrameLoader::TryRemoteBrowser()
{
  NS_ASSERTION(!mRemoteBrowser, "TryRemoteBrowser called with a remote browser already?");

  nsIDocument* doc = mOwnerContent->GetDocument();
  if (!doc) {
    return false;
  }

  if (doc->GetDisplayDocument()) {
    
    return false;
  }

  nsCOMPtr<nsIWebNavigation> parentAsWebNav =
    do_GetInterface(doc->GetScriptGlobalObject());

  if (!parentAsWebNav) {
    return false;
  }

  nsCOMPtr<nsIDocShellTreeItem> parentAsItem(do_QueryInterface(parentAsWebNav));

  
  if (!OwnerIsBrowserOrAppFrame()) {
    int32_t parentType;
    parentAsItem->GetItemType(&parentType);

    if (parentType != nsIDocShellTreeItem::typeChrome) {
      return false;
    }

    if (!mOwnerContent->IsXUL()) {
      return false;
    }

    nsAutoString value;
    mOwnerContent->GetAttr(kNameSpaceID_None, nsGkAtoms::type, value);

    if (!value.LowerCaseEqualsLiteral("content") &&
        !StringBeginsWith(value, NS_LITERAL_STRING("content-"),
                          nsCaseInsensitiveStringComparator())) {
      return false;
    }
  }

  uint32_t chromeFlags = 0;
  nsCOMPtr<nsIDocShellTreeOwner> parentOwner;
  if (NS_FAILED(parentAsItem->GetTreeOwner(getter_AddRefs(parentOwner))) ||
      !parentOwner) {
    return false;
  }
  nsCOMPtr<nsIXULWindow> window(do_GetInterface(parentOwner));
  if (!window) {
    return false;
  }
  if (NS_FAILED(window->GetChromeFlags(&chromeFlags))) {
    return false;
  }

  PROFILER_LABEL("nsFrameLoader", "CreateRemoteBrowser");

  MutableTabContext context;
  nsCOMPtr<mozIApplication> ownApp = GetOwnApp();
  nsCOMPtr<mozIApplication> containingApp = GetContainingApp();
  ScrollingBehavior scrollingBehavior = DEFAULT_SCROLLING;
  if (mOwnerContent->AttrValueIs(kNameSpaceID_None,
                                 nsGkAtoms::mozasyncpanzoom,
                                 nsGkAtoms::_true,
                                 eCaseMatters)) {
    scrollingBehavior = ASYNC_PAN_ZOOM;
  }
  if (ownApp) {
    context.SetTabContextForAppFrame(ownApp, containingApp, scrollingBehavior);
  } else if (OwnerIsBrowserFrame()) {
    
    context.SetTabContextForBrowserFrame(containingApp, scrollingBehavior);
  }

  nsCOMPtr<nsIDOMElement> ownerElement = do_QueryInterface(mOwnerContent);
  mRemoteBrowser = ContentParent::CreateBrowserOrApp(context, ownerElement);
  if (mRemoteBrowser) {
    nsCOMPtr<nsIDocShellTreeItem> rootItem;
    parentAsItem->GetRootTreeItem(getter_AddRefs(rootItem));
    nsCOMPtr<nsIDOMWindow> rootWin = do_GetInterface(rootItem);
    nsCOMPtr<nsIDOMChromeWindow> rootChromeWin = do_QueryInterface(rootWin);
    NS_ABORT_IF_FALSE(rootChromeWin, "How did we not get a chrome window here?");

    nsCOMPtr<nsIBrowserDOMWindow> browserDOMWin;
    rootChromeWin->GetBrowserDOMWindow(getter_AddRefs(browserDOMWin));
    mRemoteBrowser->SetBrowserDOMWindow(browserDOMWin);

    mChildHost = static_cast<ContentParent*>(mRemoteBrowser->Manager());
  }
  return true;
}

mozilla::dom::PBrowserParent*
nsFrameLoader::GetRemoteBrowser()
{
  return mRemoteBrowser;
}

NS_IMETHODIMP
nsFrameLoader::ActivateRemoteFrame() {
  if (mRemoteBrowser) {
    mRemoteBrowser->Activate();
    return NS_OK;
  }
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsFrameLoader::DeactivateRemoteFrame() {
  if (mRemoteBrowser) {
    mRemoteBrowser->Deactivate();
    return NS_OK;
  }
  return NS_ERROR_UNEXPECTED;
}

NS_IMETHODIMP
nsFrameLoader::SendCrossProcessMouseEvent(const nsAString& aType,
                                          float aX,
                                          float aY,
                                          int32_t aButton,
                                          int32_t aClickCount,
                                          int32_t aModifiers,
                                          bool aIgnoreRootScrollFrame)
{
  if (mRemoteBrowser) {
    mRemoteBrowser->SendMouseEvent(aType, aX, aY, aButton,
                                   aClickCount, aModifiers,
                                   aIgnoreRootScrollFrame);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsFrameLoader::ActivateFrameEvent(const nsAString& aType,
                                  bool aCapture)
{
  if (mRemoteBrowser) {
    return mRemoteBrowser->SendActivateFrameEvent(nsString(aType), aCapture) ?
      NS_OK : NS_ERROR_NOT_AVAILABLE;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsFrameLoader::SendCrossProcessKeyEvent(const nsAString& aType,
                                        int32_t aKeyCode,
                                        int32_t aCharCode,
                                        int32_t aModifiers,
                                        bool aPreventDefault)
{
  if (mRemoteBrowser) {
    mRemoteBrowser->SendKeyEvent(aType, aKeyCode, aCharCode, aModifiers,
                                 aPreventDefault);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsFrameLoader::GetDelayRemoteDialogs(bool* aRetVal)
{
  *aRetVal = mDelayRemoteDialogs;
  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::SetDelayRemoteDialogs(bool aDelay)
{
  if (mRemoteBrowser && mDelayRemoteDialogs && !aDelay) {
    nsRefPtr<nsIRunnable> ev =
      NS_NewRunnableMethod(mRemoteBrowser,
                           &mozilla::dom::TabParent::HandleDelayedDialogs);
    NS_DispatchToCurrentThread(ev);
  }
  mDelayRemoteDialogs = aDelay;
  return NS_OK;
}

nsresult
nsFrameLoader::CreateStaticClone(nsIFrameLoader* aDest)
{
  nsFrameLoader* dest = static_cast<nsFrameLoader*>(aDest);
  dest->MaybeCreateDocShell();
  NS_ENSURE_STATE(dest->mDocShell);

  nsCOMPtr<nsIDOMDocument> dummy = do_GetInterface(dest->mDocShell);
  nsCOMPtr<nsIContentViewer> viewer;
  dest->mDocShell->GetContentViewer(getter_AddRefs(viewer));
  NS_ENSURE_STATE(viewer);

  nsCOMPtr<nsIDocShell> origDocShell;
  GetDocShell(getter_AddRefs(origDocShell));
  nsCOMPtr<nsIDOMDocument> domDoc = do_GetInterface(origDocShell);

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
  NS_ENSURE_STATE(doc);
  nsCOMPtr<nsIDocument> clonedDoc = doc->CreateStaticClone(dest->mDocShell);
  nsCOMPtr<nsIDOMDocument> clonedDOMDoc = do_QueryInterface(clonedDoc);

  viewer->SetDOMDocument(clonedDOMDoc);
  return NS_OK;
}

bool
nsFrameLoader::DoLoadFrameScript(const nsAString& aURL)
{
  mozilla::dom::PBrowserParent* tabParent = GetRemoteBrowser();
  if (tabParent) {
    return tabParent->SendLoadRemoteScript(nsString(aURL));
  }
  nsRefPtr<nsInProcessTabChildGlobal> tabChild =
    static_cast<nsInProcessTabChildGlobal*>(GetTabChildGlobalAsEventTarget());
  if (tabChild) {
    tabChild->LoadFrameScript(aURL);
  }
  return true;
}

class nsAsyncMessageToChild : public nsRunnable
{
public:
  nsAsyncMessageToChild(nsFrameLoader* aFrameLoader,
                        const nsAString& aMessage,
                        const StructuredCloneData& aData)
    : mFrameLoader(aFrameLoader), mMessage(aMessage)
  {
    if (aData.mDataLength && !mData.copy(aData.mData, aData.mDataLength)) {
      NS_RUNTIMEABORT("OOM");
    }
    mClosure = aData.mClosure;
  }

  NS_IMETHOD Run()
  {
    nsInProcessTabChildGlobal* tabChild =
      static_cast<nsInProcessTabChildGlobal*>(mFrameLoader->mChildMessageManager.get());
    if (tabChild && tabChild->GetInnerManager()) {
      nsFrameScriptCx cx(static_cast<nsIDOMEventTarget*>(tabChild), tabChild);

      StructuredCloneData data;
      data.mData = mData.data();
      data.mDataLength = mData.nbytes();
      data.mClosure = mClosure;

      nsRefPtr<nsFrameMessageManager> mm = tabChild->GetInnerManager();
      mm->ReceiveMessage(static_cast<nsIDOMEventTarget*>(tabChild), mMessage,
                         false, &data, nullptr, nullptr, nullptr);
    }
    return NS_OK;
  }
  nsRefPtr<nsFrameLoader> mFrameLoader;
  nsString mMessage;
  JSAutoStructuredCloneBuffer mData;
  StructuredCloneClosure mClosure;
};

bool
nsFrameLoader::DoSendAsyncMessage(const nsAString& aMessage,
                                  const StructuredCloneData& aData)
{
  PBrowserParent* tabParent = GetRemoteBrowser();
  if (tabParent) {
    ClonedMessageData data;
    ContentParent* cp = static_cast<ContentParent*>(tabParent->Manager());
    if (!BuildClonedMessageDataForParent(cp, aData, data)) {
      return false;
    }
    return tabParent->SendAsyncMessage(nsString(aMessage), data);
  }

  if (mChildMessageManager) {
    nsRefPtr<nsIRunnable> ev = new nsAsyncMessageToChild(this, aMessage, aData);
    NS_DispatchToCurrentThread(ev);
    return true;
  }

  
  return false;
}

bool
nsFrameLoader::CheckPermission(const nsAString& aPermission)
{
  return AssertAppProcessPermission(GetRemoteBrowser(),
                                    NS_ConvertUTF16toUTF8(aPermission).get());
}

bool
nsFrameLoader::CheckManifestURL(const nsAString& aManifestURL)
{
  return AssertAppProcessManifestURL(GetRemoteBrowser(),
                                     NS_ConvertUTF16toUTF8(aManifestURL).get());
}

bool
nsFrameLoader::CheckAppHasPermission(const nsAString& aPermission)
{
  return AssertAppHasPermission(GetRemoteBrowser(),
                                NS_ConvertUTF16toUTF8(aPermission).get());
}

NS_IMETHODIMP
nsFrameLoader::GetMessageManager(nsIMessageSender** aManager)
{
  EnsureMessageManager();
  if (mMessageManager) {
    CallQueryInterface(mMessageManager, aManager);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::GetContentViewsIn(float aXPx, float aYPx,
                                 float aTopSize, float aRightSize,
                                 float aBottomSize, float aLeftSize,
                                 uint32_t* aLength,
                                 nsIContentView*** aResult)
{
  nscoord x = nsPresContext::CSSPixelsToAppUnits(aXPx - aLeftSize);
  nscoord y = nsPresContext::CSSPixelsToAppUnits(aYPx - aTopSize);
  nscoord w = nsPresContext::CSSPixelsToAppUnits(aLeftSize + aRightSize) + 1;
  nscoord h = nsPresContext::CSSPixelsToAppUnits(aTopSize + aBottomSize) + 1;
  nsRect target(x, y, w, h);

  nsIFrame* frame = GetPrimaryFrameOfOwningContent();

  nsTArray<ViewID> ids;
  nsLayoutUtils::GetRemoteContentIds(frame, target, ids, true);
  if (ids.Length() == 0 || !GetCurrentRemoteFrame()) {
    *aResult = nullptr;
    *aLength = 0;
    return NS_OK;
  }

  nsIContentView** result = reinterpret_cast<nsIContentView**>(
    NS_Alloc(ids.Length() * sizeof(nsIContentView*)));

  for (uint32_t i = 0; i < ids.Length(); i++) {
    nsIContentView* view = GetCurrentRemoteFrame()->GetContentView(ids[i]);
    NS_ABORT_IF_FALSE(view, "Retrieved ID from RenderFrameParent, it should be valid!");
    nsRefPtr<nsIContentView>(view).forget(&result[i]);
  }

  *aResult = result;
  *aLength = ids.Length();

  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::GetRootContentView(nsIContentView** aContentView)
{
  RenderFrameParent* rfp = GetCurrentRemoteFrame();
  if (!rfp) {
    *aContentView = nullptr;
    return NS_OK;
  }

  nsContentView* view = rfp->GetContentView();
  NS_ABORT_IF_FALSE(view, "Should always be able to create root scrollable!");
  nsRefPtr<nsIContentView>(view).forget(aContentView);

  return NS_OK;
}

nsresult
nsFrameLoader::EnsureMessageManager()
{
  NS_ENSURE_STATE(mOwnerContent);

  nsresult rv = MaybeCreateDocShell();
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (!mIsTopLevelContent && !OwnerIsBrowserOrAppFrame() && !mRemoteFrame) {
    return NS_OK;
  }

  if (mMessageManager) {
    if (ShouldUseRemoteProcess()) {
      mMessageManager->SetCallback(mRemoteBrowserShown ? this : nullptr);
    }
    return NS_OK;
  }

  nsIScriptContext* sctx = mOwnerContent->GetContextForEventHandlers(&rv);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_STATE(sctx);
  AutoPushJSContext cx(sctx->GetNativeContext());
  NS_ENSURE_STATE(cx);

  nsCOMPtr<nsIDOMChromeWindow> chromeWindow =
    do_QueryInterface(GetOwnerDoc()->GetWindow());
  nsCOMPtr<nsIMessageBroadcaster> parentManager;
  if (chromeWindow) {
    chromeWindow->GetMessageManager(getter_AddRefs(parentManager));
  }

  if (ShouldUseRemoteProcess()) {
    mMessageManager = new nsFrameMessageManager(mRemoteBrowserShown ? this : nullptr,
                                                static_cast<nsFrameMessageManager*>(parentManager.get()),
                                                cx,
                                                MM_CHROME);
  } else {
    mMessageManager = new nsFrameMessageManager(nullptr,
                                                static_cast<nsFrameMessageManager*>(parentManager.get()),
                                                cx,
                                                MM_CHROME);

    mChildMessageManager =
      new nsInProcessTabChildGlobal(mDocShell, mOwnerContent, mMessageManager);
    
    mMessageManager->SetCallback(this);
  }
  return NS_OK;
}

nsIDOMEventTarget*
nsFrameLoader::GetTabChildGlobalAsEventTarget()
{
  return static_cast<nsInProcessTabChildGlobal*>(mChildMessageManager.get());
}

NS_IMETHODIMP
nsFrameLoader::GetOwnerElement(nsIDOMElement **aElement)
{
  nsCOMPtr<nsIDOMElement> ownerElement = do_QueryInterface(mOwnerContent);
  ownerElement.forget(aElement);
  return NS_OK;
}

void
nsFrameLoader::SetRemoteBrowser(nsITabParent* aTabParent)
{
  MOZ_ASSERT(!mRemoteBrowser);
  MOZ_ASSERT(!mCurrentRemoteFrame);
  mRemoteFrame = true;
  mRemoteBrowser = static_cast<TabParent*>(aTabParent);

  ShowRemoteFrame(nsIntSize(0, 0));
}

void
nsFrameLoader::SetDetachedSubdocView(nsView* aDetachedViews,
                                     nsIDocument* aContainerDoc)
{
  mDetachedSubdocViews = aDetachedViews;
  mContainerDocWhileDetached = aContainerDoc;
}

nsView*
nsFrameLoader::GetDetachedSubdocView(nsIDocument** aContainerDoc) const
{
  NS_IF_ADDREF(*aContainerDoc = mContainerDocWhileDetached);
  return mDetachedSubdocViews;
}

 void
nsFrameLoader::AttributeChanged(nsIDocument* aDocument,
                                mozilla::dom::Element* aElement,
                                int32_t      aNameSpaceID,
                                nsIAtom*     aAttribute,
                                int32_t      aModType)
{
  MOZ_ASSERT(mObservingOwnerContent);
  
  MOZ_ASSERT(!mRemoteBrowser);

  if (aNameSpaceID != kNameSpaceID_None || aAttribute != TypeAttrName()) {
    return;
  }

  if (aElement != mOwnerContent) {
    return;
  }

  
  

  
  
  
  if (!mDocShell) {
    return;
  }

  nsCOMPtr<nsIDocShellTreeItem> parentItem;
  mDocShell->GetParent(getter_AddRefs(parentItem));
  if (!parentItem) {
    return;
  }

  int32_t parentType;
  parentItem->GetItemType(&parentType);

  if (parentType != nsIDocShellTreeItem::typeChrome) {
    return;
  }

  nsCOMPtr<nsIDocShellTreeOwner> parentTreeOwner;
  parentItem->GetTreeOwner(getter_AddRefs(parentTreeOwner));
  if (!parentTreeOwner) {
    return;
  }

  nsAutoString value;
  aElement->GetAttr(kNameSpaceID_None, TypeAttrName(), value);

  bool is_primary = value.LowerCaseEqualsLiteral("content-primary");

#ifdef MOZ_XUL
  
  if (!is_primary) {
    nsXULPopupManager* pm = nsXULPopupManager::GetInstance();
    if (pm)
      pm->HidePopupsInDocShell(mDocShell);
  }
#endif

  parentTreeOwner->ContentShellRemoved(mDocShell);
  if (value.LowerCaseEqualsLiteral("content") ||
      StringBeginsWith(value, NS_LITERAL_STRING("content-"),
                       nsCaseInsensitiveStringComparator())) {
    bool is_targetable = is_primary ||
      value.LowerCaseEqualsLiteral("content-targetable");

    parentTreeOwner->ContentShellAdded(mDocShell, is_primary,
                                       is_targetable, value);
  }
}

void
nsFrameLoader::ResetPermissionManagerStatus()
{
  
  
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    return;
  }

  
  
  
  
  uint32_t appId = nsIScriptSecurityManager::NO_APP_ID;
  if (OwnerIsAppFrame()) {
    
    MOZ_ASSERT(!OwnerIsBrowserFrame());

    nsCOMPtr<mozIApplication> ownApp = GetOwnApp();
    MOZ_ASSERT(ownApp);
    uint32_t ownAppId = nsIScriptSecurityManager::NO_APP_ID;
    if (ownApp && NS_SUCCEEDED(ownApp->GetLocalId(&ownAppId))) {
      appId = ownAppId;
    }
  }

  if (OwnerIsBrowserFrame()) {
    
    MOZ_ASSERT(!OwnerIsAppFrame());

    nsCOMPtr<mozIApplication> containingApp = GetContainingApp();
    uint32_t containingAppId = nsIScriptSecurityManager::NO_APP_ID;
    if (containingApp && NS_SUCCEEDED(containingApp->GetLocalId(&containingAppId))) {
      appId = containingAppId;
    }
  }

  
  if (appId == mAppIdSentToPermissionManager) {
    return;
  }

  nsCOMPtr<nsIPermissionManager> permMgr = do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);
  if (!permMgr) {
    NS_ERROR("No PermissionManager available!");
    return;
  }

  
  if (mAppIdSentToPermissionManager != nsIScriptSecurityManager::NO_APP_ID) {
    permMgr->ReleaseAppId(mAppIdSentToPermissionManager);
    mAppIdSentToPermissionManager = nsIScriptSecurityManager::NO_APP_ID;
  }

  
  if (appId != nsIScriptSecurityManager::NO_APP_ID) {
    mAppIdSentToPermissionManager = appId;
    permMgr->AddrefAppId(mAppIdSentToPermissionManager);
  }
}

