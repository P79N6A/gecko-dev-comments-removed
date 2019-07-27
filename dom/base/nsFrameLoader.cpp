










#include "base/basictypes.h"

#include "prenv.h"

#include "mozIApplication.h"
#include "nsIDOMHTMLIFrameElement.h"
#include "nsIDOMHTMLFrameElement.h"
#include "nsIDOMMozBrowserFrame.h"
#include "nsIDOMWindow.h"
#include "nsIPresShell.h"
#include "nsIContentInlines.h"
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
#include "nsIBaseWindow.h"
#include "nsContentUtils.h"
#include "nsIXPConnect.h"
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
#include "nsISHistory.h"
#include "nsISHistoryInternal.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIXULWindow.h"
#include "nsIEditor.h"
#include "nsIMozBrowserFrame.h"
#include "nsIPermissionManager.h"
#include "nsISHistory.h"
#include "nsNullPrincipal.h"
#include "nsIScriptError.h"
#include "nsGlobalWindow.h"
#include "nsPIWindowRoot.h"
#include "nsLayoutUtils.h"
#include "nsView.h"

#include "nsIURI.h"
#include "nsIURL.h"
#include "nsNetUtil.h"

#include "nsGkAtoms.h"
#include "nsNameSpaceManager.h"

#include "nsThreadUtils.h"

#include "nsIDOMChromeWindow.h"
#include "nsInProcessTabChildGlobal.h"

#include "Layers.h"
#include "ClientLayerManager.h"

#include "AppProcessChecker.h"
#include "ContentParent.h"
#include "TabParent.h"
#include "mozilla/plugins/PPluginWidgetParent.h"
#include "../plugins/ipc/PluginWidgetParent.h"
#include "mozilla/AsyncEventDispatcher.h"
#include "mozilla/GuardObjects.h"
#include "mozilla/Preferences.h"
#include "mozilla/unused.h"
#include "mozilla/dom/Element.h"
#include "mozilla/jsipc/CrossProcessObjectWrappers.h"
#include "mozilla/layout/RenderFrameParent.h"
#include "nsIAppsService.h"
#include "GeckoProfiler.h"

#include "jsapi.h"
#include "mozilla/dom/HTMLIFrameElement.h"
#include "nsSandboxFlags.h"
#include "mozilla/layers/CompositorChild.h"

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







#define MAX_SAME_URL_CONTENT_FRAMES 1








#define MAX_DEPTH_CONTENT_FRAMES 10

NS_IMPL_CYCLE_COLLECTION(nsFrameLoader, mDocShell, mMessageManager, mChildMessageManager)
NS_IMPL_CYCLE_COLLECTING_ADDREF(nsFrameLoader)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsFrameLoader)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsFrameLoader)
  NS_INTERFACE_MAP_ENTRY(nsIFrameLoader)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIFrameLoader)
NS_INTERFACE_MAP_END

nsFrameLoader::nsFrameLoader(Element* aOwner, bool aNetworkCreated)
  : mOwnerContent(aOwner)
  , mAppIdSentToPermissionManager(nsIScriptSecurityManager::NO_APP_ID)
  , mDetachedSubdocViews(nullptr)
  , mIsPrerendered(false)
  , mDepthTooGreat(false)
  , mIsTopLevelContent(false)
  , mDestroyCalled(false)
  , mNeedsAsyncDestroy(false)
  , mInSwap(false)
  , mInShow(false)
  , mHideCalled(false)
  , mNetworkCreated(aNetworkCreated)
  , mRemoteBrowserShown(false)
  , mRemoteFrame(false)
  , mClipSubdocument(true)
  , mClampScrollPosition(true)
  , mObservingOwnerContent(false)
  , mVisible(true)
  , mCurrentRemoteFrame(nullptr)
  , mRemoteBrowser(nullptr)
  , mChildID(0)
  , mEventMode(EVENT_MODE_NORMAL_DISPATCH)
{
  ResetPermissionManagerStatus();
}

nsFrameLoader::~nsFrameLoader()
{
  if (mMessageManager) {
    mMessageManager->Disconnect();
  }
  MOZ_RELEASE_ASSERT(mDestroyCalled);
}

nsFrameLoader*
nsFrameLoader::Create(Element* aOwner, bool aNetworkCreated)
{
  NS_ENSURE_TRUE(aOwner, nullptr);
  nsIDocument* doc = aOwner->OwnerDoc();
  NS_ENSURE_TRUE(!doc->IsResourceDoc() &&
                 ((!doc->IsLoadedAsData() && aOwner->GetComposedDoc()) ||
                   doc->IsStaticDocument()),
                 nullptr);

  return new nsFrameLoader(aOwner, aNetworkCreated);
}

NS_IMETHODIMP
nsFrameLoader::LoadFrame()
{
  NS_ENSURE_TRUE(mOwnerContent, NS_ERROR_NOT_INITIALIZED);

  nsAutoString src;

  bool isSrcdoc = mOwnerContent->IsHTMLElement(nsGkAtoms::iframe) &&
                  mOwnerContent->HasAttr(kNameSpaceID_None, nsGkAtoms::srcdoc);
  if (isSrcdoc) {
    src.AssignLiteral("about:srcdoc");
  }
  else {
    GetURL(src);

    src.Trim(" \t\n\r");

    if (src.IsEmpty()) {
      
      
      
      if (mOwnerContent->IsXULElement() &&
          mOwnerContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::nodefaultsrc,
                                     nsGkAtoms::_true, eCaseMatters)) {
        return NS_OK;
      }
      src.AssignLiteral("about:blank");
    }
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
  if (!mOwnerContent) {
    return;
  }
  nsRefPtr<AsyncEventDispatcher > loadBlockingAsyncDispatcher =
    new LoadBlockingAsyncEventDispatcher(mOwnerContent,
                                         NS_LITERAL_STRING("error"),
                                         false, false);
  loadBlockingAsyncDispatcher->PostDOMEvent();
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

NS_IMETHODIMP
nsFrameLoader::SetIsPrerendered()
{
  MOZ_ASSERT(!mDocShell, "Please call SetIsPrerendered before docShell is created");
  mIsPrerendered = true;

  return NS_OK;
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

class DelayedStartLoadingRunnable : public nsRunnable
{
public:
  explicit DelayedStartLoadingRunnable(nsFrameLoader* aFrameLoader)
    : mFrameLoader(aFrameLoader)
  {
  }

  NS_IMETHOD Run()
  {
    
    mFrameLoader->ReallyStartLoading();

    
    
    
    nsIFrame* frame = mFrameLoader->GetPrimaryFrameOfOwningContent();
    if (!frame) {
      return NS_OK;
    }
    frame->InvalidateFrame();
    frame->PresContext()->PresShell()->
      FrameNeedsReflow(frame, nsIPresShell::eResize, NS_FRAME_IS_DIRTY);

    return NS_OK;
  }

private:
  nsRefPtr<nsFrameLoader> mFrameLoader;
};

nsresult
nsFrameLoader::ReallyStartLoadingInternal()
{
  NS_ENSURE_STATE(mURIToLoad && mOwnerContent && mOwnerContent->IsInComposedDoc());

  PROFILER_LABEL("nsFrameLoader", "ReallyStartLoading",
    js::ProfileEntry::Category::OTHER);

  nsresult rv = MaybeCreateDocShell();
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (mRemoteFrame) {
    if (!mRemoteBrowser && !TryRemoteBrowser()) {
        NS_WARNING("Couldn't create child process for iframe.");
        return NS_ERROR_FAILURE;
    }

    
    EnsureMessageManager();

    
    mRemoteBrowser->LoadURL(mURIToLoad);
    
    if (!mRemoteBrowserShown && !ShowRemoteFrame(ScreenIntSize(0, 0))) {
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

  
  
  
  
  
  
  
  loadInfo->SetOwner(mOwnerContent->NodePrincipal());

  nsCOMPtr<nsIURI> referrer;
  
  nsAutoString srcdoc;
  bool isSrcdoc = mOwnerContent->IsHTMLElement(nsGkAtoms::iframe) &&
                  mOwnerContent->GetAttr(kNameSpaceID_None, nsGkAtoms::srcdoc,
                                         srcdoc);

  if (isSrcdoc) {
    nsAutoString referrerStr;
    mOwnerContent->OwnerDoc()->GetReferrer(referrerStr);
    rv = NS_NewURI(getter_AddRefs(referrer), referrerStr);

    loadInfo->SetSrcdocData(srcdoc);
    nsCOMPtr<nsIURI> baseURI = mOwnerContent->GetBaseURI();
    loadInfo->SetBaseURI(baseURI);
  }
  else {
    rv = mOwnerContent->NodePrincipal()->GetURI(getter_AddRefs(referrer));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  if (referrer) {
    bool isNullPrincipalScheme;
    rv = referrer->SchemeIs(NS_NULLPRINCIPAL_SCHEME, &isNullPrincipalScheme);
    if (NS_SUCCEEDED(rv) && !isNullPrincipalScheme) {
      loadInfo->SetReferrer(referrer);
    }
  }

  loadInfo->SetReferrerPolicy(mOwnerContent->OwnerDoc()->GetReferrerPolicy());

  
  int32_t flags = nsIWebNavigation::LOAD_FLAGS_NONE;

  
  if (OwnerIsBrowserFrame()) {
    flags = nsIWebNavigation::LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP |
            nsIWebNavigation::LOAD_FLAGS_DISALLOW_INHERIT_OWNER;
  }

  
  bool tmpState = mNeedsAsyncDestroy;
  mNeedsAsyncDestroy = true;
  nsCOMPtr<nsIURI> uriToLoad = mURIToLoad;
  rv = mDocShell->LoadURI(uriToLoad, loadInfo, flags, false);
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

static void
FirePageHideEvent(nsIDocShellTreeItem* aItem,
                  EventTarget* aChromeEventHandler)
{
  nsCOMPtr<nsIDocument> doc = aItem->GetDocument();
  NS_ASSERTION(doc, "What happened here?");
  doc->OnPageHide(true, aChromeEventHandler);

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
                  EventTarget* aChromeEventHandler,
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

  nsCOMPtr<nsIDocument> doc = aItem->GetDocument();
  NS_ASSERTION(doc, "What happened here?");
  if (doc->IsShowing() == aFireIfShowing) {
    doc->OnPageShow(true, aChromeEventHandler);
  }
}

static void
SetTreeOwnerAndChromeEventHandlerOnDocshellTree(nsIDocShellTreeItem* aItem,
                                                nsIDocShellTreeOwner* aOwner,
                                                EventTarget* aHandler)
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
                                      nsIDocShell* aParentNode)
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

    if (kid->ItemType() != aType || !AllDescendantsOfType(kid, aType)) {
      return false;
    }
  }

  return true;
}





class MOZ_STACK_CLASS AutoResetInShow {
  private:
    nsFrameLoader* mFrameLoader;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
  public:
    explicit AutoResetInShow(nsFrameLoader* aFrameLoader MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
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

  ScreenIntSize size = frame->GetSubdocumentSize();
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
  NS_ENSURE_TRUE(mDocShell, false);

  
  
  
  
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
    presContext->RebuildAllStyleData(nsChangeHint(0), eRestyle_Subtree);
}

bool
nsFrameLoader::ShowRemoteFrame(const ScreenIntSize& size,
                               nsSubDocumentFrame *aFrame)
{
  NS_ASSERTION(mRemoteFrame, "ShowRemote only makes sense on remote frames.");

  if (!mRemoteBrowser && !TryRemoteBrowser()) {
    NS_ERROR("Couldn't create child process.");
    return false;
  }

  
  
  
  if (!mRemoteBrowserShown) {
    if (!mOwnerContent ||
        !mOwnerContent->GetComposedDoc()) {
      return false;
    }

    nsRefPtr<layers::LayerManager> layerManager =
      nsContentUtils::LayerManagerForDocument(mOwnerContent->GetComposedDoc());
    if (!layerManager) {
      
      return false;
    }

    nsPIDOMWindow* win = mOwnerContent->OwnerDoc()->GetWindow();
    bool parentIsActive = false;
    if (win) {
      nsCOMPtr<nsPIWindowRoot> windowRoot =
        static_cast<nsGlobalWindow*>(win)->GetTopWindowRoot();
      if (windowRoot) {
        nsPIDOMWindow* topWin = windowRoot->GetWindow();
        parentIsActive = topWin && topWin->IsActive();
      }
    }
    mRemoteBrowser->Show(size, parentIsActive);
    mRemoteBrowserShown = true;

    EnsureMessageManager();

    InitializeBrowserAPI();
    nsCOMPtr<nsIObserverService> os = services::GetObserverService();
    if (os) {
      os->NotifyObservers(NS_ISUPPORTS_CAST(nsIFrameLoader*, this),
                          "remote-browser-shown", nullptr);
    }
  } else {
    nsIntRect dimensions;
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
nsFrameLoader::SwapWithOtherRemoteLoader(nsFrameLoader* aOther,
                                         nsRefPtr<nsFrameLoader>& aFirstToSwap,
                                         nsRefPtr<nsFrameLoader>& aSecondToSwap)
{
  MOZ_ASSERT(NS_IsMainThread());

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

  nsIDocument* ourDoc = ourContent->GetComposedDoc();
  nsIDocument* otherDoc = otherContent->GetComposedDoc();
  if (!ourDoc || !otherDoc) {
    
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsIPresShell* ourShell = ourDoc->GetShell();
  nsIPresShell* otherShell = otherDoc->GetShell();
  if (!ourShell || !otherShell) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  if (mInSwap || aOther->mInSwap) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  mInSwap = aOther->mInSwap = true;

  nsIFrame* ourFrame = ourContent->GetPrimaryFrame();
  nsIFrame* otherFrame = otherContent->GetPrimaryFrame();
  if (!ourFrame || !otherFrame) {
    mInSwap = aOther->mInSwap = false;
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsSubDocumentFrame* ourFrameFrame = do_QueryFrame(ourFrame);
  if (!ourFrameFrame) {
    mInSwap = aOther->mInSwap = false;
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  rv = ourFrameFrame->BeginSwapDocShells(otherFrame);
  if (NS_FAILED(rv)) {
    mInSwap = aOther->mInSwap = false;
    return rv;
  }

  mRemoteBrowser->SwapLayerTreeObservers(aOther->mRemoteBrowser);

  nsCOMPtr<nsIBrowserDOMWindow> otherBrowserDOMWindow =
    aOther->mRemoteBrowser->GetBrowserDOMWindow();
  nsCOMPtr<nsIBrowserDOMWindow> browserDOMWindow =
    mRemoteBrowser->GetBrowserDOMWindow();

  if (!otherBrowserDOMWindow || !browserDOMWindow) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  aOther->mRemoteBrowser->SetBrowserDOMWindow(browserDOMWindow);
  mRemoteBrowser->SetBrowserDOMWindow(otherBrowserDOMWindow);

  
  const nsTArray<mozilla::plugins::PPluginWidgetParent*>& plugins =
    aOther->mRemoteBrowser->ManagedPPluginWidgetParent();
  nsPIDOMWindow* newWin = ourDoc->GetWindow();
  if (newWin) {
    nsRefPtr<nsIWidget> newParent = ((nsGlobalWindow*)newWin)->GetMainWidget();
    for (uint32_t idx = 0; idx < plugins.Length(); ++idx) {
      static_cast<mozilla::plugins::PluginWidgetParent*>(plugins[idx])->SetParent(newParent);
    }
  }

  SetOwnerContent(otherContent);
  aOther->SetOwnerContent(ourContent);

  mRemoteBrowser->SetOwnerElement(otherContent);
  aOther->mRemoteBrowser->SetOwnerElement(ourContent);

  nsRefPtr<nsFrameMessageManager> ourMessageManager = mMessageManager;
  nsRefPtr<nsFrameMessageManager> otherMessageManager = aOther->mMessageManager;
  
  if (mMessageManager) {
    mMessageManager->SetCallback(aOther);
  }
  if (aOther->mMessageManager) {
    aOther->mMessageManager->SetCallback(this);
  }
  mMessageManager.swap(aOther->mMessageManager);

  aFirstToSwap.swap(aSecondToSwap);

  ourFrameFrame->EndSwapDocShells(otherFrame);

  ourDoc->FlushPendingNotifications(Flush_Layout);
  otherDoc->FlushPendingNotifications(Flush_Layout);

  mInSwap = aOther->mInSwap = false;
  return NS_OK;
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

  if (mRemoteFrame && aOther->mRemoteFrame) {
    return SwapWithOtherRemoteLoader(aOther, aFirstToSwap, aSecondToSwap);
  }

  if (mRemoteFrame || aOther->mRemoteFrame) {
    NS_WARNING("Swapping remote and non-remote frames is not currently supported");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

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

  
  
  
  int32_t ourType = ourDocshell->ItemType();
  int32_t otherType = otherDocshell->ItemType();
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

  
  int32_t ourParentType = ourParentItem->ItemType();
  int32_t otherParentType = otherParentItem->ItemType();
  if (ourParentType != otherParentType) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsCOMPtr<nsPIDOMWindow> ourWindow = ourDocshell->GetWindow();
  nsCOMPtr<nsPIDOMWindow> otherWindow = otherDocshell->GetWindow();

  nsCOMPtr<Element> ourFrameElement =
    ourWindow->GetFrameElementInternal();
  nsCOMPtr<Element> otherFrameElement =
    otherWindow->GetFrameElementInternal();

  nsCOMPtr<EventTarget> ourChromeEventHandler =
    do_QueryInterface(ourWindow->GetChromeEventHandler());
  nsCOMPtr<EventTarget> otherChromeEventHandler =
    do_QueryInterface(otherWindow->GetChromeEventHandler());

  nsCOMPtr<EventTarget> ourEventTarget = ourWindow->GetParentTarget();
  nsCOMPtr<EventTarget> otherEventTarget = otherWindow->GetParentTarget();

  NS_ASSERTION(SameCOMIdentity(ourFrameElement, ourContent) &&
               SameCOMIdentity(otherFrameElement, otherContent) &&
               SameCOMIdentity(ourChromeEventHandler, ourContent) &&
               SameCOMIdentity(otherChromeEventHandler, otherContent),
               "How did that happen, exactly?");

  nsCOMPtr<nsIDocument> ourChildDocument = ourWindow->GetExtantDoc();
  nsCOMPtr<nsIDocument> otherChildDocument = otherWindow ->GetExtantDoc();
  if (!ourChildDocument || !otherChildDocument) {
    
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsCOMPtr<nsIDocument> ourParentDocument =
    ourChildDocument->GetParentDocument();
  nsCOMPtr<nsIDocument> otherParentDocument =
    otherChildDocument->GetParentDocument();

  
  nsIDocument* ourDoc = ourContent->GetComposedDoc();
  nsIDocument* otherDoc = otherContent->GetComposedDoc();
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

  
  
  
  FirePageShowEvent(ourDocshell, ourEventTarget, false);
  FirePageShowEvent(otherDocshell, otherEventTarget, false);
  FirePageHideEvent(ourDocshell, ourEventTarget);
  FirePageHideEvent(otherDocshell, otherEventTarget);
  
  nsIFrame* ourFrame = ourContent->GetPrimaryFrame();
  nsIFrame* otherFrame = otherContent->GetPrimaryFrame();
  if (!ourFrame || !otherFrame) {
    mInSwap = aOther->mInSwap = false;
    FirePageShowEvent(ourDocshell, ourEventTarget, true);
    FirePageShowEvent(otherDocshell, otherEventTarget, true);
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsSubDocumentFrame* ourFrameFrame = do_QueryFrame(ourFrame);
  if (!ourFrameFrame) {
    mInSwap = aOther->mInSwap = false;
    FirePageShowEvent(ourDocshell, ourEventTarget, true);
    FirePageShowEvent(otherDocshell, otherEventTarget, true);
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  rv = ourFrameFrame->BeginSwapDocShells(otherFrame);
  if (NS_FAILED(rv)) {
    mInSwap = aOther->mInSwap = false;
    FirePageShowEvent(ourDocshell, ourEventTarget, true);
    FirePageShowEvent(otherDocshell, otherEventTarget, true);
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
  
  if (mMessageManager) {
    mMessageManager->SetCallback(aOther);
  }
  if (aOther->mMessageManager) {
    aOther->mMessageManager->SetCallback(this);
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

  FirePageShowEvent(ourDocshell, ourEventTarget, true);
  FirePageShowEvent(otherDocshell, otherEventTarget, true);

  mInSwap = aOther->mInSwap = false;
  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::Destroy()
{
  StartDestroy();
  return NS_OK;
}

class nsFrameLoaderDestroyRunnable : public nsRunnable
{
  enum DestroyPhase
  {
    
    eDestroyDocShell,
    eWaitForUnloadMessage,
    eDestroyComplete
  };

  nsRefPtr<nsFrameLoader> mFrameLoader;
  DestroyPhase mPhase;

public:
  explicit nsFrameLoaderDestroyRunnable(nsFrameLoader* aFrameLoader)
   : mFrameLoader(aFrameLoader), mPhase(eDestroyDocShell) {}

  NS_IMETHODIMP Run() override;
};

void
nsFrameLoader::StartDestroy()
{
  
  
  

  if (mDestroyCalled) {
    return;
  }
  mDestroyCalled = true;

  
  
  if (mMessageManager) {
    mMessageManager->Close();
  }

  
  
  
  if (mChildMessageManager || mRemoteBrowser) {
    mOwnerContentStrong = mOwnerContent;
    if (mRemoteBrowser) {
      mRemoteBrowser->CacheFrameLoader(this);
    }
  }

  
  
  if (mRemoteBrowser) {
    mRemoteBrowser->RemoveWindowListeners();
  }

  nsCOMPtr<nsIDocument> doc;
  bool dynamicSubframeRemoval = false;
  if (mOwnerContent) {
    doc = mOwnerContent->OwnerDoc();
    dynamicSubframeRemoval = !mIsTopLevelContent && !doc->InUnlinkOrDeletion();
    doc->SetSubDocumentFor(mOwnerContent, nullptr);

    SetOwnerContent(nullptr);
  }

  
  if (dynamicSubframeRemoval) {
    if (mDocShell) {
      mDocShell->RemoveFromSessionHistory();
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

  
  if (mDocShell) {
    nsCOMPtr<nsPIDOMWindow> win_private(mDocShell->GetWindow());
    if (win_private) {
      win_private->SetFrameElementInternal(nullptr);
    }
  }

  nsCOMPtr<nsIRunnable> destroyRunnable = new nsFrameLoaderDestroyRunnable(this);
  if (mNeedsAsyncDestroy || !doc ||
      NS_FAILED(doc->FinalizeFrameLoader(this, destroyRunnable))) {
    NS_DispatchToCurrentThread(destroyRunnable);
  }
}

nsresult
nsFrameLoaderDestroyRunnable::Run()
{
  switch (mPhase) {
  case eDestroyDocShell:
    mFrameLoader->DestroyDocShell();

    
    
    
    
    
    if (mFrameLoader->mChildMessageManager) {
      
      
      
      
      
      mPhase = eWaitForUnloadMessage;
      NS_DispatchToCurrentThread(this);
    }
    break;

   case eWaitForUnloadMessage:
     
     
     
     
     
     
     mPhase = eDestroyComplete;
     NS_DispatchToCurrentThread(this);
     break;

   case eDestroyComplete:
     
     
     
     mFrameLoader->DestroyComplete();
     break;
  }

  return NS_OK;
}

void
nsFrameLoader::DestroyDocShell()
{
  
  
  

  
  
  
  if (mRemoteBrowser) {
    mRemoteBrowser->Destroy();
  }

  
  if (mChildMessageManager) {
    static_cast<nsInProcessTabChildGlobal*>(mChildMessageManager.get())->FireUnloadEvent();
  }

  
  nsCOMPtr<nsIBaseWindow> base_win(do_QueryInterface(mDocShell));
  if (base_win) {
    base_win->Destroy();
  }
  mDocShell = nullptr;

  if (mChildMessageManager) {
    
    static_cast<nsInProcessTabChildGlobal*>(mChildMessageManager.get())->DisconnectEventListeners();
  }
}

void
nsFrameLoader::DestroyComplete()
{
  
  
  
  

  
  if (mChildMessageManager || mRemoteBrowser) {
    mOwnerContentStrong = nullptr;
    if (mRemoteBrowser) {
      mRemoteBrowser->CacheFrameLoader(nullptr);
    }
  }

  
  if (mRemoteBrowser) {
    mRemoteBrowser->SetOwnerElement(nullptr);
    mRemoteBrowser->Destroy();
    mRemoteBrowser = nullptr;
  }

  if (mMessageManager) {
    mMessageManager->Disconnect();
  }

  if (mChildMessageManager) {
    static_cast<nsInProcessTabChildGlobal*>(mChildMessageManager.get())->Disconnect();
  }
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


NS_IMETHODIMP
nsFrameLoader::GetOwnerIsBrowserOrAppFrame(bool* aResult)
{
  *aResult = OwnerIsBrowserOrAppFrame();
  return NS_OK;
}

bool
nsFrameLoader::OwnerIsWidget()
{
  nsCOMPtr<nsIMozBrowserFrame> browserFrame = do_QueryInterface(mOwnerContent);
  return browserFrame ? browserFrame->GetReallyIsWidget() : false;
}



NS_IMETHODIMP
nsFrameLoader::GetOwnerIsWidget(bool* aResult)
{
  *aResult = OwnerIsWidget();
  return NS_OK;
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

  nsCOMPtr<mozIApplication> app;
  appsService->GetAppByManifestURL(manifest, getter_AddRefs(app));

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

  nsCOMPtr<mozIApplication> app;
  appsService->GetAppByLocalId(appId, getter_AddRefs(app));

  return app.forget();
}

bool
nsFrameLoader::ShouldUseRemoteProcess()
{
  if (PR_GetEnv("MOZ_DISABLE_OOP_TABS") ||
      Preferences::GetBool("dom.ipc.tabs.disabled", false)) {
    return false;
  }

  
  
  if (XRE_GetProcessType() == GeckoProcessType_Content &&
      !CompositorChild::ChildProcessHasCompositor()) {
    return false;
  }

  if (XRE_GetProcessType() == GeckoProcessType_Content &&
      !(PR_GetEnv("MOZ_NESTED_OOP_TABS") ||
        Preferences::GetBool("dom.ipc.tabs.nested.enabled", false))) {
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
  if (!(doc->IsStaticDocument() || mOwnerContent->IsInComposedDoc())) {
    return NS_ERROR_UNEXPECTED;
  }

  if (doc->IsResourceDoc() || !doc->IsActive()) {
    
    
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsIDocShell> docShell = doc->GetDocShell();
  nsCOMPtr<nsIWebNavigation> parentAsWebNav = do_QueryInterface(docShell);
  NS_ENSURE_STATE(parentAsWebNav);

  
  mDocShell = do_CreateInstance("@mozilla.org/docshell;1");
  NS_ENSURE_TRUE(mDocShell, NS_ERROR_FAILURE);

  if (mIsPrerendered) {
    nsresult rv = mDocShell->SetIsPrerendered(true);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  
  uint32_t sandboxFlags = 0;
  HTMLIFrameElement* iframe = HTMLIFrameElement::FromContent(mOwnerContent);
  if (iframe) {
    sandboxFlags = iframe->GetSandboxFlags();
  }
  ApplySandboxFlags(sandboxFlags);

  if (!mNetworkCreated) {
    if (mDocShell) {
      mDocShell->SetCreatedDynamically(true);
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
    mDocShell->SetName(frameName);
  }

  
  
  

  int32_t parentType = docShell->ItemType();

  
  
  nsCOMPtr<nsIDocShellTreeOwner> parentTreeOwner;
  docShell->GetTreeOwner(getter_AddRefs(parentTreeOwner));
  NS_ENSURE_STATE(parentTreeOwner);
  mIsTopLevelContent =
    AddTreeItemToTreeOwner(mDocShell, parentTreeOwner, parentType, docShell);

  
  
  nsCOMPtr<nsIDOMEventTarget> chromeEventHandler;

  if (parentType == nsIDocShellTreeItem::typeChrome) {
    
    

    chromeEventHandler = do_QueryInterface(mOwnerContent);
    NS_ASSERTION(chromeEventHandler,
                 "This mContent should implement this.");
  } else {
    
    

    docShell->GetChromeEventHandler(getter_AddRefs(chromeEventHandler));
  }

  mDocShell->SetChromeEventHandler(chromeEventHandler);

  
  
  
  

  
  nsCOMPtr<Element> frame_element = mOwnerContent;
  NS_ASSERTION(frame_element, "frame loader owner element not a DOM element!");

  nsCOMPtr<nsPIDOMWindow> win_private(mDocShell->GetWindow());
  nsCOMPtr<nsIBaseWindow> base_win(do_QueryInterface(mDocShell));
  if (win_private) {
    win_private->SetFrameElementInternal(frame_element);
  }

  
  
  
  if (NS_FAILED(base_win->Create()) || !win_private) {
    
    NS_WARNING("Something wrong when creating the docshell for a frameloader!");
    return NS_ERROR_FAILURE;
  }

  if (mIsTopLevelContent &&
      mOwnerContent->IsXULElement(nsGkAtoms::browser) &&
      !mOwnerContent->HasAttr(kNameSpaceID_None, nsGkAtoms::disablehistory)) {
    nsresult rv;
    nsCOMPtr<nsISHistory> sessionHistory =
      do_CreateInstance(NS_SHISTORY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIWebNavigation> webNav(do_QueryInterface(mDocShell));
    webNav->SetSessionHistory(sessionHistory);
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

  InitializeBrowserAPI();
  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  if (os) {
    os->NotifyObservers(NS_ISUPPORTS_CAST(nsIFrameLoader*, this),
                        "inprocess-browser-shown", nullptr);
  }

  if (OwnerIsBrowserOrAppFrame() && mMessageManager) {
    mMessageManager->LoadFrameScript(
      NS_LITERAL_STRING("chrome://global/content/BrowserElementChild.js"),
       true,
       true);
    
    nsCOMPtr<nsIDocShellTreeItem> item = do_GetInterface(docShell);
    nsAutoString name;
    if (mOwnerContent->GetAttr(kNameSpaceID_None, nsGkAtoms::name, name)) {
      item->SetName(name);
    }
    mDocShell->SetFullscreenAllowed(
      mOwnerContent->HasAttr(kNameSpaceID_None, nsGkAtoms::allowfullscreen) ||
      mOwnerContent->HasAttr(kNameSpaceID_None, nsGkAtoms::mozallowfullscreen));
    bool isPrivate = mOwnerContent->HasAttr(kNameSpaceID_None, nsGkAtoms::mozprivatebrowsing);
    if (isPrivate) {
      bool nonBlank;
      mDocShell->GetHasLoadedNonBlankURI(&nonBlank);
      if (nonBlank) {
        nsContentUtils::ReportToConsoleNonLocalized(
          NS_LITERAL_STRING("We should not switch to Private Browsing after loading a document."),
          nsIScriptError::warningFlag,
          NS_LITERAL_CSTRING("mozprivatebrowsing"),
          nullptr);
      } else {
        nsCOMPtr<nsILoadContext> context = do_GetInterface(mDocShell);
        context->SetUsePrivateBrowsing(true);
      }
    }
  }

  return NS_OK;
}

void
nsFrameLoader::GetURL(nsString& aURI)
{
  aURI.Truncate();

  if (mOwnerContent->IsHTMLElement(nsGkAtoms::object)) {
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
  
  if (mDocShell->ItemType() != nsIDocShellTreeItem::typeContent) {
    
    
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

  
  
  
  
  
  nsAutoCString buffer;
  rv = aURI->GetScheme(buffer);
  if (NS_SUCCEEDED(rv) && buffer.EqualsLiteral("about")) {
    rv = aURI->GetPath(buffer);
    if (NS_SUCCEEDED(rv) && buffer.EqualsLiteral("srcdoc")) {
      
      return NS_OK;
    }
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
nsFrameLoader::GetWindowDimensions(nsIntRect& aRect)
{
  
  nsIDocument* doc = mOwnerContent->GetComposedDoc();
  if (!doc) {
    return NS_ERROR_FAILURE;
  }

  if (doc->IsResourceDoc()) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsPIDOMWindow> win = doc->GetWindow();
  if (!win) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDocShellTreeItem> parentAsItem(win->GetDocShell());
  if (!parentAsItem) {
    return NS_ERROR_FAILURE;
  }

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
      ScreenIntSize size = aIFrame->GetSubdocumentSize();
      nsIntRect dimensions;
      NS_ENSURE_SUCCESS(GetWindowDimensions(dimensions), NS_ERROR_FAILURE);
      mRemoteBrowser->UpdateDimensions(dimensions, size);
    }
    return NS_OK;
  }
  UpdateBaseWindowPositionAndSize(aIFrame);
  return NS_OK;
}

void
nsFrameLoader::UpdateBaseWindowPositionAndSize(nsSubDocumentFrame *aIFrame)
{
  nsCOMPtr<nsIDocShell> docShell;
  GetDocShell(getter_AddRefs(docShell));
  nsCOMPtr<nsIBaseWindow> baseWindow(do_QueryInterface(docShell));

  
  if (baseWindow) {
    int32_t x = 0;
    int32_t y = 0;

    nsWeakFrame weakFrame(aIFrame);

    baseWindow->GetPosition(&x, &y);

    if (!weakFrame.IsAlive()) {
      
      return;
    }

    ScreenIntSize size = aIFrame->GetSubdocumentSize();

    baseWindow->SetPositionAndSize(x, y, size.width, size.height, false);
  }
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
  return NS_OK;
}

bool
nsFrameLoader::TryRemoteBrowser()
{
  NS_ASSERTION(!mRemoteBrowser, "TryRemoteBrowser called with a remote browser already?");

  
  
  
  nsIDocument* doc = mOwnerContent->GetComposedDoc();
  if (!doc) {
    return false;
  }

  if (doc->IsResourceDoc()) {
    
    return false;
  }

  nsCOMPtr<nsPIDOMWindow> parentWin = doc->GetWindow();
  if (!parentWin) {
    return false;
  }

  nsCOMPtr<nsIDocShell> parentDocShell = parentWin->GetDocShell();
  if (!parentDocShell) {
    return false;
  }

  TabParent* openingTab = TabParent::GetFrom(parentDocShell->GetOpener());
  ContentParent* openerContentParent = nullptr;

  if (openingTab &&
      openingTab->Manager() &&
      openingTab->Manager()->IsContentParent()) {
    openerContentParent = openingTab->Manager()->AsContentParent();
  }

  
  if (!OwnerIsBrowserOrAppFrame()) {
    if (parentDocShell->ItemType() != nsIDocShellTreeItem::typeChrome) {
      return false;
    }

    if (!mOwnerContent->IsXULElement()) {
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
  if (NS_FAILED(parentDocShell->GetTreeOwner(getter_AddRefs(parentOwner))) ||
      !parentOwner) {
    return false;
  }
  nsCOMPtr<nsIXULWindow> window(do_GetInterface(parentOwner));
  if (window && NS_FAILED(window->GetChromeFlags(&chromeFlags))) {
    return false;
  }

  PROFILER_LABEL("nsFrameLoader", "CreateRemoteBrowser",
    js::ProfileEntry::Category::OTHER);

  MutableTabContext context;
  nsCOMPtr<mozIApplication> ownApp = GetOwnApp();
  nsCOMPtr<mozIApplication> containingApp = GetContainingApp();

  bool rv = true;
  if (ownApp) {
    rv = context.SetTabContextForAppFrame(ownApp, containingApp);
  } else if (OwnerIsBrowserFrame()) {
    
    rv = context.SetTabContextForBrowserFrame(containingApp);
  } else {
    rv = context.SetTabContextForNormalFrame();
  }
  NS_ENSURE_TRUE(rv, false);

  nsCOMPtr<Element> ownerElement = mOwnerContent;
  mRemoteBrowser = ContentParent::CreateBrowserOrApp(context, ownerElement, openerContentParent);
  if (!mRemoteBrowser) {
    return false;
  }

  mContentParent = mRemoteBrowser->Manager();
  mChildID = mRemoteBrowser->Manager()->ChildID();

  nsCOMPtr<nsIDocShellTreeItem> rootItem;
  parentDocShell->GetRootTreeItem(getter_AddRefs(rootItem));
  nsCOMPtr<nsIDOMWindow> rootWin = rootItem->GetWindow();
  nsCOMPtr<nsIDOMChromeWindow> rootChromeWin = do_QueryInterface(rootWin);

  if (rootChromeWin) {
    nsCOMPtr<nsIBrowserDOMWindow> browserDOMWin;
    rootChromeWin->GetBrowserDOMWindow(getter_AddRefs(browserDOMWin));
    mRemoteBrowser->SetBrowserDOMWindow(browserDOMWin);
  }

  if (mOwnerContent->AttrValueIs(kNameSpaceID_None,
                                 nsGkAtoms::mozpasspointerevents,
                                 nsGkAtoms::_true,
                                 eCaseMatters)) {
    unused << mRemoteBrowser->SendSetUpdateHitRegion(true);
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

void
nsFrameLoader::ActivateUpdateHitRegion() {
  if (mRemoteBrowser) {
    unused << mRemoteBrowser->SendSetUpdateHitRegion(true);
  }
}

void
nsFrameLoader::DeactivateUpdateHitRegion() {
  if (mRemoteBrowser) {
    unused << mRemoteBrowser->SendSetUpdateHitRegion(false);
  }
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

nsresult
nsFrameLoader::CreateStaticClone(nsIFrameLoader* aDest)
{
  nsFrameLoader* dest = static_cast<nsFrameLoader*>(aDest);
  dest->MaybeCreateDocShell();
  NS_ENSURE_STATE(dest->mDocShell);

  nsCOMPtr<nsIDocument> dummy = dest->mDocShell->GetDocument();
  nsCOMPtr<nsIContentViewer> viewer;
  dest->mDocShell->GetContentViewer(getter_AddRefs(viewer));
  NS_ENSURE_STATE(viewer);

  nsCOMPtr<nsIDocShell> origDocShell;
  GetDocShell(getter_AddRefs(origDocShell));
  NS_ENSURE_STATE(origDocShell);

  nsCOMPtr<nsIDocument> doc = origDocShell->GetDocument();
  NS_ENSURE_STATE(doc);

  nsCOMPtr<nsIDocument> clonedDoc = doc->CreateStaticClone(dest->mDocShell);
  nsCOMPtr<nsIDOMDocument> clonedDOMDoc = do_QueryInterface(clonedDoc);

  viewer->SetDOMDocument(clonedDOMDoc);
  return NS_OK;
}

bool
nsFrameLoader::DoLoadMessageManagerScript(const nsAString& aURL, bool aRunInGlobalScope)
{
  auto* tabParent = TabParent::GetFrom(GetRemoteBrowser());
  if (tabParent) {
    return tabParent->SendLoadRemoteScript(nsString(aURL), aRunInGlobalScope);
  }
  nsRefPtr<nsInProcessTabChildGlobal> tabChild =
    static_cast<nsInProcessTabChildGlobal*>(GetTabChildGlobalAsEventTarget());
  if (tabChild) {
    tabChild->LoadFrameScript(aURL, aRunInGlobalScope);
  }
  return true;
}

class nsAsyncMessageToChild : public nsSameProcessAsyncMessageBase,
                              public nsRunnable
{
public:
  nsAsyncMessageToChild(JSContext* aCx,
                        nsFrameLoader* aFrameLoader,
                        const nsAString& aMessage,
                        const StructuredCloneData& aData,
                        JS::Handle<JSObject *> aCpows,
                        nsIPrincipal* aPrincipal)
    : nsSameProcessAsyncMessageBase(aCx, aMessage, aData, aCpows, aPrincipal)
    , mFrameLoader(aFrameLoader)
  {
  }

  NS_IMETHOD Run()
  {
    nsInProcessTabChildGlobal* tabChild =
      static_cast<nsInProcessTabChildGlobal*>(mFrameLoader->mChildMessageManager.get());
    if (tabChild && tabChild->GetInnerManager()) {
      nsCOMPtr<nsIXPConnectJSObjectHolder> kungFuDeathGrip(tabChild->GetGlobal());
      ReceiveMessage(static_cast<EventTarget*>(tabChild),
                     tabChild->GetInnerManager());
    }
    return NS_OK;
  }
  nsRefPtr<nsFrameLoader> mFrameLoader;
};

bool
nsFrameLoader::DoSendAsyncMessage(JSContext* aCx,
                                  const nsAString& aMessage,
                                  const StructuredCloneData& aData,
                                  JS::Handle<JSObject *> aCpows,
                                  nsIPrincipal* aPrincipal)
{
  TabParent* tabParent = mRemoteBrowser;
  if (tabParent) {
    ClonedMessageData data;
    nsIContentParent* cp = tabParent->Manager();
    if (!BuildClonedMessageDataForParent(cp, aData, data)) {
      return false;
    }
    InfallibleTArray<mozilla::jsipc::CpowEntry> cpows;
    if (aCpows && !cp->GetCPOWManager()->Wrap(aCx, aCpows, &cpows)) {
      return false;
    }
    return tabParent->SendAsyncMessage(nsString(aMessage), data, cpows,
                                       IPC::Principal(aPrincipal));
  }

  if (mChildMessageManager) {
    nsCOMPtr<nsIRunnable> ev = new nsAsyncMessageToChild(aCx, this, aMessage,
                                                         aData, aCpows,
                                                         aPrincipal);
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
    nsRefPtr<nsFrameMessageManager> mm(mMessageManager);
    mm.forget(aManager);
    return NS_OK;
  }
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

  if (!mIsTopLevelContent &&
      !OwnerIsBrowserOrAppFrame() &&
      !mRemoteFrame &&
      !(mOwnerContent->IsXULElement() &&
        mOwnerContent->AttrValueIs(kNameSpaceID_None,
                                   nsGkAtoms::forcemessagemanager,
                                   nsGkAtoms::_true, eCaseMatters))) {
    return NS_OK;
  }

  bool useRemoteProcess = ShouldUseRemoteProcess();
  if (mMessageManager) {
    if (useRemoteProcess && mRemoteBrowser) {
      mMessageManager->InitWithCallback(this);
    }
    return NS_OK;
  }

  nsCOMPtr<nsIDOMChromeWindow> chromeWindow =
    do_QueryInterface(GetOwnerDoc()->GetWindow());
  nsCOMPtr<nsIMessageBroadcaster> parentManager;

  if (chromeWindow) {
    nsAutoString messagemanagergroup;
    if (mOwnerContent->IsXULElement() &&
        mOwnerContent->GetAttr(kNameSpaceID_None,
                               nsGkAtoms::messagemanagergroup,
                               messagemanagergroup)) {
      chromeWindow->GetGroupMessageManager(messagemanagergroup, getter_AddRefs(parentManager));
    }

    if (!parentManager) {
      chromeWindow->GetMessageManager(getter_AddRefs(parentManager));
    }
  }

  if (useRemoteProcess) {
    mMessageManager = new nsFrameMessageManager(mRemoteBrowser ? this : nullptr,
                                                static_cast<nsFrameMessageManager*>(parentManager.get()),
                                                MM_CHROME);
  } else {
    mMessageManager = new nsFrameMessageManager(nullptr,
                                                static_cast<nsFrameMessageManager*>(parentManager.get()),
                                                MM_CHROME);

    mChildMessageManager =
      new nsInProcessTabChildGlobal(mDocShell, mOwnerContent, mMessageManager);
    
    mMessageManager->InitWithCallback(this);
  }
  return NS_OK;
}

EventTarget*
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

NS_IMETHODIMP
nsFrameLoader::GetChildID(uint64_t* aChildID)
{
  *aChildID = mChildID;
  return NS_OK;
}

void
nsFrameLoader::SetRemoteBrowser(nsITabParent* aTabParent)
{
  MOZ_ASSERT(!mRemoteBrowser);
  MOZ_ASSERT(!mCurrentRemoteFrame);
  mRemoteFrame = true;
  mRemoteBrowser = TabParent::GetFrom(aTabParent);
  mChildID = mRemoteBrowser ? mRemoteBrowser->Manager()->ChildID() : 0;
  ShowRemoteFrame(ScreenIntSize(0, 0));
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
nsFrameLoader::ApplySandboxFlags(uint32_t sandboxFlags)
{
  if (mDocShell) {
    uint32_t parentSandboxFlags = mOwnerContent->OwnerDoc()->GetSandboxFlags();

    
    sandboxFlags |= parentSandboxFlags;
    mDocShell->SetSandboxFlags(sandboxFlags);
  }
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

  if (parentItem->ItemType() != nsIDocShellTreeItem::typeChrome) {
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

  nsCOMPtr<nsIPermissionManager> permMgr = services::GetPermissionManager();
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




NS_IMETHODIMP
nsFrameLoader::RequestNotifyAfterRemotePaint()
{
  
  if (mRemoteBrowser) {
    unused << mRemoteBrowser->SendRequestNotifyAfterRemotePaint();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::RequestNotifyLayerTreeReady()
{
  if (mRemoteBrowser) {
    return mRemoteBrowser->RequestNotifyLayerTreeReady() ? NS_OK : NS_ERROR_NOT_AVAILABLE;
  }

  if (!mOwnerContent) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsRefPtr<AsyncEventDispatcher> event =
    new AsyncEventDispatcher(mOwnerContent,
                             NS_LITERAL_STRING("MozLayerTreeReady"),
                             true, false);
  event->PostDOMEvent();

  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::RequestNotifyLayerTreeCleared()
{
  if (mRemoteBrowser) {
    return mRemoteBrowser->RequestNotifyLayerTreeCleared() ? NS_OK : NS_ERROR_NOT_AVAILABLE;
  }

  if (!mOwnerContent) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsRefPtr<AsyncEventDispatcher> event =
    new AsyncEventDispatcher(mOwnerContent,
                             NS_LITERAL_STRING("MozLayerTreeCleared"),
                             true, false);
  event->PostDOMEvent();

  return NS_OK;
}

 NS_IMETHODIMP
nsFrameLoader::SetVisible(bool aVisible)
{
  if (mVisible == aVisible) {
    return NS_OK;
  }

  mVisible = aVisible;
  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  if (os) {
    os->NotifyObservers(NS_ISUPPORTS_CAST(nsIFrameLoader*, this),
                        "frameloader-visible-changed", nullptr);
  }
  return NS_OK;
}

 NS_IMETHODIMP
nsFrameLoader::GetVisible(bool* aVisible)
{
  *aVisible = mVisible;
  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::GetTabParent(nsITabParent** aTabParent)
{
  nsCOMPtr<nsITabParent> tp = mRemoteBrowser;
  tp.forget(aTabParent);
  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::GetLoadContext(nsILoadContext** aLoadContext)
{
  nsCOMPtr<nsILoadContext> loadContext;
  if (mRemoteBrowser) {
    loadContext = mRemoteBrowser->GetLoadContext();
  } else {
    nsCOMPtr<nsIDocShell> docShell;
    GetDocShell(getter_AddRefs(docShell));
    loadContext = do_GetInterface(docShell);
  }
  loadContext.forget(aLoadContext);
  return NS_OK;
}

void
nsFrameLoader::InitializeBrowserAPI()
{
  nsCOMPtr<nsIMozBrowserFrame> browserFrame = do_QueryInterface(mOwnerContent);
  if (browserFrame) {
    browserFrame->InitializeBrowserAPI();
  }
}
