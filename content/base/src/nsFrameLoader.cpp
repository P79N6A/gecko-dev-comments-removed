













































#include "base/basictypes.h"

#include "prenv.h"

#include "nsIDOMHTMLIFrameElement.h"
#include "nsIDOMHTMLFrameElement.h"
#include "nsIDOMWindow.h"
#include "nsIPresShell.h"
#include "nsIContent.h"
#include "nsIContentViewer.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIWebNavigation.h"
#include "nsIWebProgress.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDocShellLoadInfo.h"
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
#include "nsSubDocumentFrame.h"
#include "nsDOMError.h"
#include "nsGUIEvent.h"
#include "nsEventDispatcher.h"
#include "nsISHistory.h"
#include "nsISHistoryInternal.h"
#include "nsIDocShellHistory.h"
#include "nsIDOMNSHTMLDocument.h"
#include "nsIXULWindow.h"
#include "nsIEditor.h"
#include "nsIEditorDocShell.h"

#include "nsLayoutUtils.h"
#include "nsIView.h"
#include "nsPLDOMEvent.h"

#include "nsIURI.h"
#include "nsIURL.h"
#include "nsNetUtil.h"

#include "nsGkAtoms.h"
#include "nsINameSpaceManager.h"

#include "nsThreadUtils.h"
#include "nsIContentViewer.h"
#include "nsIView.h"

#include "nsIDOMChromeWindow.h"
#include "nsInProcessTabChildGlobal.h"
#include "mozilla/AutoRestore.h"
#include "mozilla/unused.h"

#include "Layers.h"

#include "ContentParent.h"
#include "TabParent.h"
#include "mozilla/layout/RenderFrameParent.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::layers;
typedef FrameMetrics::ViewID ViewID;

#include "jsapi.h"

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

static void InvalidateFrame(nsIFrame* aFrame)
{
  nsRect rect = nsRect(nsPoint(0, 0), aFrame->GetRect().Size());
  
  
  
  
  aFrame->InvalidateWithFlags(rect, nsIFrame::INVALIDATE_NO_THEBES_LAYERS);
}

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

  
  
  if (!mOwnerContent) {
    if (IsRoot()) {
      
      
      
      return NS_OK;
    } else {
      
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  nsIFrame* frame = mOwnerContent->GetPrimaryFrame();

  
  
  InvalidateFrame(frame);
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

NS_IMPL_CYCLE_COLLECTION_CLASS(nsFrameLoader)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsFrameLoader)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mDocShell)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mMessageManager)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mChildMessageManager)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsFrameLoader)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mDocShell)
  NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(cb, "nsFrameLoader::mMessageManager");
  cb.NoteXPCOMChild(static_cast<nsIContentFrameMessageManager*>(tmp->mMessageManager.get()));
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mChildMessageManager)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsFrameLoader)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsFrameLoader)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsFrameLoader)
  NS_INTERFACE_MAP_ENTRY(nsIFrameLoader)
  NS_INTERFACE_MAP_ENTRY(nsIContentViewManager)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIFrameLoader)
NS_INTERFACE_MAP_END

nsFrameLoader::nsFrameLoader(nsIContent *aOwner, PRBool aNetworkCreated)
  : mOwnerContent(aOwner)
  , mDepthTooGreat(PR_FALSE)
  , mIsTopLevelContent(PR_FALSE)
  , mDestroyCalled(PR_FALSE)
  , mNeedsAsyncDestroy(PR_FALSE)
  , mInSwap(PR_FALSE)
  , mInShow(PR_FALSE)
  , mHideCalled(PR_FALSE)
  , mNetworkCreated(aNetworkCreated)
  , mDelayRemoteDialogs(PR_FALSE)
  , mRemoteBrowserShown(PR_FALSE)
  , mRemoteFrame(false)
  , mCurrentRemoteFrame(nsnull)
  , mRemoteBrowser(nsnull)
  , mRenderMode(RENDER_MODE_DEFAULT)
{
}

nsFrameLoader*
nsFrameLoader::Create(nsIContent* aOwner, PRBool aNetworkCreated)
{
  NS_ENSURE_TRUE(aOwner, nsnull);
  nsIDocument* doc = aOwner->GetOwnerDoc();
  NS_ENSURE_TRUE(doc && !doc->GetDisplayDocument() &&
                 ((!doc->IsLoadedAsData() && aOwner->GetCurrentDoc()) ||
                   doc->IsStaticDocument()),
                 nsnull);

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

  nsIDocument* doc = mOwnerContent->GetOwnerDoc();
  if (!doc || doc->IsStaticDocument()) {
    return NS_OK;
  }

  nsCOMPtr<nsIURI> base_uri = mOwnerContent->GetBaseURI();
  const nsAFlatCString &doc_charset = doc->GetDocumentCharacterSet();
  const char *charset = doc_charset.IsEmpty() ? nsnull : doc_charset.get();

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
    nsRefPtr<nsPLDOMEvent> event =
      new nsLoadBlockingPLDOMEvent(mOwnerContent, NS_LITERAL_STRING("error"),
                                   PR_FALSE, PR_FALSE);
    event->PostDOMEvent();
  }
}

NS_IMETHODIMP
nsFrameLoader::LoadURI(nsIURI* aURI)
{
  if (!aURI)
    return NS_ERROR_INVALID_POINTER;
  NS_ENSURE_STATE(!mDestroyCalled && mOwnerContent);

  nsCOMPtr<nsIDocument> doc = mOwnerContent->GetOwnerDoc();
  if (!doc) {
    return NS_OK;
  }

  nsresult rv = CheckURILoad(aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  mURIToLoad = aURI;
  rv = doc->InitializeFrameLoader(this);
  if (NS_FAILED(rv)) {
    mURIToLoad = nsnull;
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

    
    mRemoteBrowser->LoadURL(mURIToLoad);
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
  rv = mOwnerContent->NodePrincipal()->GetURI(getter_AddRefs(referrer));
  NS_ENSURE_SUCCESS(rv, rv);

  loadInfo->SetReferrer(referrer);

  
  PRBool tmpState = mNeedsAsyncDestroy;
  mNeedsAsyncDestroy = PR_TRUE;
  rv = mDocShell->LoadURI(mURIToLoad, loadInfo,
                          nsIWebNavigation::LOAD_FLAGS_NONE, PR_FALSE);
  mNeedsAsyncDestroy = tmpState;
  mURIToLoad = nsnull;
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
  *aDocShell = nsnull;

  
  
  
  if (mOwnerContent) {
    nsresult rv = MaybeCreateDocShell();
    if (NS_FAILED(rv))
      return rv;
    if (mRemoteFrame) {
      NS_WARNING("No docshells for remote frames!");
      return NS_ERROR_NOT_AVAILABLE;
    }
    NS_ASSERTION(mDocShell,
                 "MaybeCreateDocShell succeeded, but null mDocShell");
  }

  *aDocShell = mDocShell;
  NS_IF_ADDREF(*aDocShell);

  return NS_OK;
}

void
nsFrameLoader::Finalize()
{
  nsCOMPtr<nsIBaseWindow> base_win(do_QueryInterface(mDocShell));
  if (base_win) {
    base_win->Destroy();
  }
  mDocShell = nsnull;
}

static void
FirePageHideEvent(nsIDocShellTreeItem* aItem,
                  nsIDOMEventTarget* aChromeEventHandler)
{
  nsCOMPtr<nsIDOMDocument> doc = do_GetInterface(aItem);
  nsCOMPtr<nsIDocument> internalDoc = do_QueryInterface(doc);
  NS_ASSERTION(internalDoc, "What happened here?");
  internalDoc->OnPageHide(PR_TRUE, aChromeEventHandler);

  PRInt32 childCount = 0;
  aItem->GetChildCount(&childCount);
  nsAutoTArray<nsCOMPtr<nsIDocShellTreeItem>, 8> kids;
  kids.AppendElements(childCount);
  for (PRInt32 i = 0; i < childCount; ++i) {
    aItem->GetChildAt(i, getter_AddRefs(kids[i]));
  }

  for (PRUint32 i = 0; i < kids.Length(); ++i) {
    if (kids[i]) {
      FirePageHideEvent(kids[i], aChromeEventHandler);
    }
  }
}





static void
FirePageShowEvent(nsIDocShellTreeItem* aItem,
                  nsIDOMEventTarget* aChromeEventHandler,
                  PRBool aFireIfShowing)
{
  PRInt32 childCount = 0;
  aItem->GetChildCount(&childCount);
  nsAutoTArray<nsCOMPtr<nsIDocShellTreeItem>, 8> kids;
  kids.AppendElements(childCount);
  for (PRInt32 i = 0; i < childCount; ++i) {
    aItem->GetChildAt(i, getter_AddRefs(kids[i]));
  }

  for (PRUint32 i = 0; i < kids.Length(); ++i) {
    if (kids[i]) {
      FirePageShowEvent(kids[i], aChromeEventHandler, aFireIfShowing);
    }
  }

  nsCOMPtr<nsIDOMDocument> doc = do_GetInterface(aItem);
  nsCOMPtr<nsIDocument> internalDoc = do_QueryInterface(doc);
  NS_ASSERTION(internalDoc, "What happened here?");
  if (internalDoc->IsShowing() == aFireIfShowing) {
    internalDoc->OnPageShow(PR_TRUE, aChromeEventHandler);
  }
}

static void
SetTreeOwnerAndChromeEventHandlerOnDocshellTree(nsIDocShellTreeItem* aItem,
                                                nsIDocShellTreeOwner* aOwner,
                                                nsIDOMEventTarget* aHandler)
{
  NS_PRECONDITION(aItem, "Must have item");

  aItem->SetTreeOwner(aOwner);
  nsCOMPtr<nsIDocShell> shell(do_QueryInterface(aItem));
  shell->SetChromeEventHandler(aHandler);

  PRInt32 childCount = 0;
  aItem->GetChildCount(&childCount);
  for (PRInt32 i = 0; i < childCount; ++i) {
    nsCOMPtr<nsIDocShellTreeItem> item;
    aItem->GetChildAt(i, getter_AddRefs(item));
    SetTreeOwnerAndChromeEventHandlerOnDocshellTree(item, aOwner, aHandler);
  }
}











static PRBool
AddTreeItemToTreeOwner(nsIDocShellTreeItem* aItem, nsIContent* aOwningContent,
                       nsIDocShellTreeOwner* aOwner, PRInt32 aParentType,
                       nsIDocShellTreeNode* aParentNode)
{
  NS_PRECONDITION(aItem, "Must have docshell treeitem");
  NS_PRECONDITION(aOwningContent, "Must have owning content");
  
  nsAutoString value;
  PRBool isContent = PR_FALSE;

  if (aOwningContent->IsXUL()) {
      aOwningContent->GetAttr(kNameSpaceID_None, nsGkAtoms::type, value);
  }

  
  
  

  isContent = value.LowerCaseEqualsLiteral("content") ||
    StringBeginsWith(value, NS_LITERAL_STRING("content-"),
                     nsCaseInsensitiveStringComparator());

  if (isContent) {
    

    aItem->SetItemType(nsIDocShellTreeItem::typeContent);
  } else {
    
    
    

    aItem->SetItemType(aParentType);
  }

  
  if (aParentNode) {
    aParentNode->AddChild(aItem);
  }

  PRBool retval = PR_FALSE;
  if (aParentType == nsIDocShellTreeItem::typeChrome && isContent) {
    retval = PR_TRUE;

    PRBool is_primary = value.LowerCaseEqualsLiteral("content-primary");

    if (aOwner) {
      PRBool is_targetable = is_primary ||
        value.LowerCaseEqualsLiteral("content-targetable");
      aOwner->ContentShellAdded(aItem, is_primary, is_targetable, value);
    }
  }

  return retval;
}

static PRBool
AllDescendantsOfType(nsIDocShellTreeItem* aParentItem, PRInt32 aType)
{
  PRInt32 childCount = 0;
  aParentItem->GetChildCount(&childCount);

  for (PRInt32 i = 0; i < childCount; ++i) {
    nsCOMPtr<nsIDocShellTreeItem> kid;
    aParentItem->GetChildAt(i, getter_AddRefs(kid));

    PRInt32 kidType;
    kid->GetItemType(&kidType);
    if (kidType != aType || !AllDescendantsOfType(kid, aType)) {
      return PR_FALSE;
    }
  }

  return PR_TRUE;
}





class NS_STACK_CLASS AutoResetInShow {
  private:
    nsFrameLoader* mFrameLoader;
    MOZILLA_DECL_USE_GUARD_OBJECT_NOTIFIER
  public:
    AutoResetInShow(nsFrameLoader* aFrameLoader MOZILLA_GUARD_OBJECT_NOTIFIER_PARAM)
      : mFrameLoader(aFrameLoader)
    {
      MOZILLA_GUARD_OBJECT_NOTIFIER_INIT;
    }
    ~AutoResetInShow() { mFrameLoader->mInShow = PR_FALSE; }
};


PRBool
nsFrameLoader::Show(PRInt32 marginWidth, PRInt32 marginHeight,
                    PRInt32 scrollbarPrefX, PRInt32 scrollbarPrefY,
                    nsSubDocumentFrame* frame)
{
  if (mInShow) {
    return PR_FALSE;
  }
  
  AutoResetInShow resetInShow(this);
  mInShow = PR_TRUE;

  nsresult rv = MaybeCreateDocShell();
  if (NS_FAILED(rv)) {
    return PR_FALSE;
  }

  if (!mRemoteFrame) {
    if (!mDocShell)
      return PR_FALSE;
    nsCOMPtr<nsIPresShell> presShell;
    mDocShell->GetPresShell(getter_AddRefs(presShell));
    if (presShell)
      return PR_TRUE;

    mDocShell->SetMarginWidth(marginWidth);
    mDocShell->SetMarginHeight(marginHeight);

    nsCOMPtr<nsIScrollable> sc = do_QueryInterface(mDocShell);
    if (sc) {
      sc->SetDefaultScrollbarPreferences(nsIScrollable::ScrollOrientation_X,
                                         scrollbarPrefX);
      sc->SetDefaultScrollbarPreferences(nsIScrollable::ScrollOrientation_Y,
                                         scrollbarPrefY);
    }
  }

  nsIView* view = frame->EnsureInnerView();
  if (!view)
    return PR_FALSE;

  if (mRemoteFrame) {
    return ShowRemoteFrame(GetSubDocumentSize(frame));
  }

  nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(mDocShell);
  NS_ASSERTION(baseWindow, "Found a nsIDocShell that isn't a nsIBaseWindow.");
  nsIntSize size;
  if (!(frame->GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    
    
    size = GetSubDocumentSize(frame);
  } else {
    
    
    size.SizeTo(10, 10);
  }
  baseWindow->InitWindow(nsnull, view->GetWidget(), 0, 0,
                         size.width, size.height);
  
  
  
  baseWindow->Create();
  baseWindow->SetVisibility(PR_TRUE);

  
  
  
  
  nsCOMPtr<nsIPresShell> presShell;
  mDocShell->GetPresShell(getter_AddRefs(presShell));
  if (presShell) {
    nsCOMPtr<nsIDOMNSHTMLDocument> doc =
      do_QueryInterface(presShell->GetDocument());

    if (doc) {
      nsAutoString designMode;
      doc->GetDesignMode(designMode);

      if (designMode.EqualsLiteral("on")) {
        
        
        nsCOMPtr<nsIEditorDocShell> editorDocshell = do_QueryInterface(mDocShell);
        nsCOMPtr<nsIEditor> editor;
        nsresult rv = editorDocshell->GetEditor(getter_AddRefs(editor));
        NS_ENSURE_SUCCESS(rv, PR_FALSE);

        doc->SetDesignMode(NS_LITERAL_STRING("off"));
        doc->SetDesignMode(NS_LITERAL_STRING("on"));
      } else {
        
        nsCOMPtr<nsIEditorDocShell> editorDocshell = do_QueryInterface(mDocShell);
        if (editorDocshell) {
          PRBool editable = PR_FALSE,
                 hasEditingSession = PR_FALSE;
          editorDocshell->GetEditable(&editable);
          editorDocshell->GetHasEditingSession(&hasEditingSession);
          nsCOMPtr<nsIEditor> editor;
          editorDocshell->GetEditor(getter_AddRefs(editor));
          if (editable && hasEditingSession && editor) {
            editor->PostCreate();
          }
        }
      }
    }
  }

  mInShow = PR_FALSE;
  if (mHideCalled) {
    mHideCalled = PR_FALSE;
    Hide();
    return PR_FALSE;
  }
  return PR_TRUE;
}

bool
nsFrameLoader::ShowRemoteFrame(const nsIntSize& size)
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
    mRemoteBrowser->Show(size);
    mRemoteBrowserShown = PR_TRUE;

    EnsureMessageManager();
  } else {
    mRemoteBrowser->Move(size);
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
    mHideCalled = PR_TRUE;
    return;
  }

  if (!mDocShell)
    return;

  nsCOMPtr<nsIContentViewer> contentViewer;
  mDocShell->GetContentViewer(getter_AddRefs(contentViewer));
  if (contentViewer)
    contentViewer->SetSticky(PR_FALSE);

  nsCOMPtr<nsIBaseWindow> baseWin = do_QueryInterface(mDocShell);
  NS_ASSERTION(baseWin,
               "Found an nsIDocShell which doesn't implement nsIBaseWindow.");
  baseWin->SetVisibility(PR_FALSE);
  baseWin->SetParentWidget(nsnull);
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

  nsIContent* ourContent = mOwnerContent;
  nsIContent* otherContent = aOther->mOwnerContent;

  if (!ourContent || !otherContent) {
    
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  PRBool equal;
  nsresult rv =
    ourContent->NodePrincipal()->Equals(otherContent->NodePrincipal(), &equal);
  if (NS_FAILED(rv) || !equal) {
    
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsCOMPtr<nsIDocShell> ourDochell = GetExistingDocShell();
  nsCOMPtr<nsIDocShell> otherDocshell = aOther->GetExistingDocShell();
  if (!ourDochell || !otherDocshell) {
    
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  
  
  nsCOMPtr<nsIDocShellTreeItem> ourTreeItem = do_QueryInterface(ourDochell);
  nsCOMPtr<nsIDocShellTreeItem> otherTreeItem =
    do_QueryInterface(otherDocshell);
  nsCOMPtr<nsIDocShellTreeItem> ourRootTreeItem, otherRootTreeItem;
  ourTreeItem->GetSameTypeRootTreeItem(getter_AddRefs(ourRootTreeItem));
  otherTreeItem->GetSameTypeRootTreeItem(getter_AddRefs(otherRootTreeItem));
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

  if ((ourRootTreeItem != ourTreeItem || otherRootTreeItem != otherTreeItem) &&
      (ourHistory || otherHistory)) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  
  PRInt32 ourType = nsIDocShellTreeItem::typeChrome;
  PRInt32 otherType = nsIDocShellTreeItem::typeChrome;
  ourTreeItem->GetItemType(&ourType);
  otherTreeItem->GetItemType(&otherType);
  if (ourType != otherType) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  
  
  
  if (ourType != nsIDocShellTreeItem::typeContent &&
      (!AllDescendantsOfType(ourTreeItem, ourType) ||
       !AllDescendantsOfType(otherTreeItem, otherType))) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  
  
  
  nsCOMPtr<nsIDocShellTreeOwner> ourOwner, otherOwner;
  ourTreeItem->GetTreeOwner(getter_AddRefs(ourOwner));
  otherTreeItem->GetTreeOwner(getter_AddRefs(otherOwner));
  

  nsCOMPtr<nsIDocShellTreeItem> ourParentItem, otherParentItem;
  ourTreeItem->GetParent(getter_AddRefs(ourParentItem));
  otherTreeItem->GetParent(getter_AddRefs(otherParentItem));
  if (!ourParentItem || !otherParentItem) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  PRInt32 ourParentType = nsIDocShellTreeItem::typeContent;
  PRInt32 otherParentType = nsIDocShellTreeItem::typeContent;
  ourParentItem->GetItemType(&ourParentType);
  otherParentItem->GetItemType(&otherParentType);
  if (ourParentType != otherParentType) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsCOMPtr<nsPIDOMWindow> ourWindow = do_GetInterface(ourDochell);
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

  if (mInSwap || aOther->mInSwap) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  mInSwap = aOther->mInSwap = PR_TRUE;

  
  
  
  FirePageShowEvent(ourTreeItem, ourChromeEventHandler, PR_FALSE);
  FirePageShowEvent(otherTreeItem, otherChromeEventHandler, PR_FALSE);
  FirePageHideEvent(ourTreeItem, ourChromeEventHandler);
  FirePageHideEvent(otherTreeItem, otherChromeEventHandler);
  
  nsIFrame* ourFrame = ourContent->GetPrimaryFrame();
  nsIFrame* otherFrame = otherContent->GetPrimaryFrame();
  if (!ourFrame || !otherFrame) {
    mInSwap = aOther->mInSwap = PR_FALSE;
    FirePageShowEvent(ourTreeItem, ourChromeEventHandler, PR_TRUE);
    FirePageShowEvent(otherTreeItem, otherChromeEventHandler, PR_TRUE);
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsSubDocumentFrame* ourFrameFrame = do_QueryFrame(ourFrame);
  if (!ourFrameFrame) {
    mInSwap = aOther->mInSwap = PR_FALSE;
    FirePageShowEvent(ourTreeItem, ourChromeEventHandler, PR_TRUE);
    FirePageShowEvent(otherTreeItem, otherChromeEventHandler, PR_TRUE);
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  rv = ourFrameFrame->BeginSwapDocShells(otherFrame);
  if (NS_FAILED(rv)) {
    mInSwap = aOther->mInSwap = PR_FALSE;
    FirePageShowEvent(ourTreeItem, ourChromeEventHandler, PR_TRUE);
    FirePageShowEvent(otherTreeItem, otherChromeEventHandler, PR_TRUE);
    return rv;
  }

  
  
  ourParentItem->RemoveChild(ourTreeItem);
  otherParentItem->RemoveChild(otherTreeItem);
  if (ourType == nsIDocShellTreeItem::typeContent) {
    ourOwner->ContentShellRemoved(ourTreeItem);
    otherOwner->ContentShellRemoved(otherTreeItem);
  }
  
  ourParentItem->AddChild(otherTreeItem);
  otherParentItem->AddChild(ourTreeItem);

  
  SetTreeOwnerAndChromeEventHandlerOnDocshellTree(ourTreeItem, otherOwner,
                                                  otherChromeEventHandler);
  SetTreeOwnerAndChromeEventHandlerOnDocshellTree(otherTreeItem, ourOwner,
                                                  ourChromeEventHandler);

  AddTreeItemToTreeOwner(ourTreeItem, otherContent, otherOwner,
                         otherParentType, nsnull);
  AddTreeItemToTreeOwner(otherTreeItem, ourContent, ourOwner, ourParentType,
                         nsnull);

  
  
  
  
  ourParentDocument->SetSubDocumentFor(ourContent, nsnull);
  otherParentDocument->SetSubDocumentFor(otherContent, nsnull);
  ourParentDocument->SetSubDocumentFor(ourContent, otherChildDocument);
  otherParentDocument->SetSubDocumentFor(otherContent, ourChildDocument);

  ourWindow->SetFrameElementInternal(otherFrameElement);
  otherWindow->SetFrameElementInternal(ourFrameElement);

  SetOwnerContent(otherContent);
  aOther->SetOwnerContent(ourContent);

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
    mMessageManager->GetParentManager() : nsnull;
  nsFrameMessageManager* otherParentManager = aOther->mMessageManager ?
    aOther->mMessageManager->GetParentManager() : nsnull;
  if (mMessageManager) {
    mMessageManager->Disconnect();
    mMessageManager->SetParentManager(otherParentManager);
    mMessageManager->SetCallbackData(aOther, PR_FALSE);
  }
  if (aOther->mMessageManager) {
    aOther->mMessageManager->Disconnect();
    aOther->mMessageManager->SetParentManager(ourParentManager);
    aOther->mMessageManager->SetCallbackData(this, PR_FALSE);
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

  ourParentDocument->FlushPendingNotifications(Flush_Layout);
  otherParentDocument->FlushPendingNotifications(Flush_Layout);

  FirePageShowEvent(ourTreeItem, otherChromeEventHandler, PR_TRUE);
  FirePageShowEvent(otherTreeItem, ourChromeEventHandler, PR_TRUE);

  mInSwap = aOther->mInSwap = PR_FALSE;
  return NS_OK;
}

void
nsFrameLoader::DestroyChild()
{
  if (mRemoteBrowser) {
    mRemoteBrowser->SetOwnerElement(nsnull);
    mRemoteBrowser->Destroy();
    mRemoteBrowser = nsnull;
  }
}

NS_IMETHODIMP
nsFrameLoader::Destroy()
{
  if (mDestroyCalled) {
    return NS_OK;
  }
  mDestroyCalled = PR_TRUE;

  if (mMessageManager) {
    mMessageManager->Disconnect();
  }
  if (mChildMessageManager) {
    static_cast<nsInProcessTabChildGlobal*>(mChildMessageManager.get())->Disconnect();
  }

  nsCOMPtr<nsIDocument> doc;
  PRBool dynamicSubframeRemoval = PR_FALSE;
  if (mOwnerContent) {
    doc = mOwnerContent->GetOwnerDoc();

    if (doc) {
      dynamicSubframeRemoval = !mIsTopLevelContent && !doc->InUnlinkOrDeletion();
      doc->SetSubDocumentFor(mOwnerContent, nsnull);
    }

    SetOwnerContent(nsnull);
  }
  DestroyChild();

  
  if (dynamicSubframeRemoval) {
    nsCOMPtr<nsIDocShellHistory> dhistory = do_QueryInterface(mDocShell);
    if (dhistory) {
      dhistory->RemoveFromSessionHistory();
    }
  }

  
  if (mIsTopLevelContent) {
    nsCOMPtr<nsIDocShellTreeItem> ourItem = do_QueryInterface(mDocShell);
    if (ourItem) {
      nsCOMPtr<nsIDocShellTreeItem> parentItem;
      ourItem->GetParent(getter_AddRefs(parentItem));
      nsCOMPtr<nsIDocShellTreeOwner> owner = do_GetInterface(parentItem);
      if (owner) {
        owner->ContentShellRemoved(ourItem);
      }
    }
  }
  
  
  nsCOMPtr<nsPIDOMWindow> win_private(do_GetInterface(mDocShell));
  if (win_private) {
    win_private->SetFrameElementInternal(nsnull);
  }

  if ((mNeedsAsyncDestroy || !doc ||
       NS_FAILED(doc->FinalizeFrameLoader(this))) && mDocShell) {
    nsCOMPtr<nsIRunnable> event = new nsAsyncDocShellDestroyer(mDocShell);
    NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);
    NS_DispatchToCurrentThread(event);

    
    

    mDocShell = nsnull;
  }

  

  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::GetDepthTooGreat(PRBool* aDepthTooGreat)
{
  *aDepthTooGreat = mDepthTooGreat;
  return NS_OK;
}

void
nsFrameLoader::SetOwnerContent(nsIContent* aContent)
{
  mOwnerContent = aContent;
  if (RenderFrameParent* rfp = GetCurrentRemoteFrame()) {
    rfp->OwnerContentChanged(aContent);
  }
}

bool
nsFrameLoader::ShouldUseRemoteProcess()
{
  
  
  

  if (PR_GetEnv("MOZ_DISABLE_OOP_TABS")) {
    return false;
  }

  PRBool remoteDisabled = nsContentUtils::GetBoolPref("dom.ipc.tabs.disabled",
                                                      PR_FALSE);
  if (remoteDisabled) {
    return false;
  }

  static nsIAtom* const *const remoteValues[] = {
    &nsGkAtoms::_false,
    &nsGkAtoms::_true,
    nsnull
  };

  switch (mOwnerContent->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::Remote,
                                         remoteValues, eCaseMatters)) {
  case 0:
    return false;
  case 1:
    return true;
  }

  PRBool remoteEnabled = nsContentUtils::GetBoolPref("dom.ipc.tabs.enabled",
                                                     PR_FALSE);
  return (bool) remoteEnabled;
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

  
  
  
  nsIDocument* doc = mOwnerContent->GetOwnerDoc();
  if (!doc || !(doc->IsStaticDocument() || mOwnerContent->IsInDoc())) {
    return NS_ERROR_UNEXPECTED;
  }

  if (doc->GetDisplayDocument() || !doc->IsActive()) {
    
    
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsISupports> container =
    doc->GetContainer();
  nsCOMPtr<nsIWebNavigation> parentAsWebNav = do_QueryInterface(container);

  
  mDocShell = do_CreateInstance("@mozilla.org/docshell;1");
  NS_ENSURE_TRUE(mDocShell, NS_ERROR_FAILURE);

  if (!mNetworkCreated) {
    nsCOMPtr<nsIDocShellHistory> history = do_QueryInterface(mDocShell);
    if (history) {
      history->SetCreatedDynamically(PR_TRUE);
    }
  }

  
  nsCOMPtr<nsIDocShellTreeItem> docShellAsItem(do_QueryInterface(mDocShell));
  NS_ENSURE_TRUE(docShellAsItem, NS_ERROR_FAILURE);
  nsAutoString frameName;

  PRInt32 namespaceID = mOwnerContent->GetNameSpaceID();
  if (namespaceID == kNameSpaceID_XHTML && !mOwnerContent->IsInHTMLDocument()) {
    mOwnerContent->GetAttr(kNameSpaceID_None, nsGkAtoms::id, frameName);
  } else {
    mOwnerContent->GetAttr(kNameSpaceID_None, nsGkAtoms::name, frameName);
    
    
    if (frameName.IsEmpty() && namespaceID == kNameSpaceID_XUL) {
      mOwnerContent->GetAttr(kNameSpaceID_None, nsGkAtoms::id, frameName);
    }
  }

  if (!frameName.IsEmpty()) {
    docShellAsItem->SetName(frameName.get());
  }

  
  
  

  nsCOMPtr<nsIDocShellTreeNode> parentAsNode(do_QueryInterface(parentAsWebNav));
  if (parentAsNode) {
    
    

    nsCOMPtr<nsIDocShellTreeItem> parentAsItem =
      do_QueryInterface(parentAsNode);

    PRInt32 parentType;
    parentAsItem->GetItemType(&parentType);

    
    
    nsCOMPtr<nsIDocShellTreeOwner> parentTreeOwner;
    parentAsItem->GetTreeOwner(getter_AddRefs(parentTreeOwner));
    mIsTopLevelContent =
      AddTreeItemToTreeOwner(docShellAsItem, mOwnerContent, parentTreeOwner,
                             parentType, parentAsNode);

    
    
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

  mDepthTooGreat = PR_FALSE;
  rv = MaybeCreateDocShell();
  if (NS_FAILED(rv)) {
    return rv;
  }
  NS_ASSERTION(!mRemoteFrame,
               "Shouldn't call CheckForRecursiveLoad on remote frames.");
  if (!mDocShell) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDocShellTreeItem> treeItem = do_QueryInterface(mDocShell);
  NS_ASSERTION(treeItem, "docshell must be a treeitem!");

  
  nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
  treeItem->GetTreeOwner(getter_AddRefs(treeOwner));
  NS_WARN_IF_FALSE(treeOwner,
                   "Trying to load a new url to a docshell without owner!");
  NS_ENSURE_STATE(treeOwner);
  
  
  PRInt32 ourType;
  rv = treeItem->GetItemType(&ourType);
  if (NS_SUCCEEDED(rv) && ourType != nsIDocShellTreeItem::typeContent) {
    
    
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIDocShellTreeItem> parentAsItem;
  treeItem->GetSameTypeParent(getter_AddRefs(parentAsItem));
  PRInt32 depth = 0;
  while (parentAsItem) {
    ++depth;
    
    if (depth >= MAX_DEPTH_CONTENT_FRAMES) {
      mDepthTooGreat = PR_TRUE;
      NS_WARNING("Too many nested content frames so giving up");

      return NS_ERROR_UNEXPECTED; 
    }

    nsCOMPtr<nsIDocShellTreeItem> temp;
    temp.swap(parentAsItem);
    temp->GetSameTypeParent(getter_AddRefs(parentAsItem));
  }
  
  
  
  nsCOMPtr<nsIURI> cloneURI;
  rv = aURI->Clone(getter_AddRefs(cloneURI));
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  nsCOMPtr<nsIURL> cloneURL(do_QueryInterface(cloneURI)); 
  if (cloneURL) {
    rv = cloneURL->SetRef(EmptyCString());
    NS_ENSURE_SUCCESS(rv,rv);
  }

  PRInt32 matchCount = 0;
  treeItem->GetSameTypeParent(getter_AddRefs(parentAsItem));
  while (parentAsItem) {
    
    nsCOMPtr<nsIWebNavigation> parentAsNav(do_QueryInterface(parentAsItem));
    if (parentAsNav) {
      
      nsCOMPtr<nsIURI> parentURI;
      parentAsNav->GetCurrentURI(getter_AddRefs(parentURI));
      if (parentURI) {
        nsCOMPtr<nsIURI> parentClone;
        rv = parentURI->Clone(getter_AddRefs(parentClone));
        NS_ENSURE_SUCCESS(rv, rv);
        nsCOMPtr<nsIURL> parentURL(do_QueryInterface(parentClone));
        if (parentURL) {
          rv = parentURL->SetRef(EmptyCString());
          NS_ENSURE_SUCCESS(rv,rv);
        }

        PRBool equal;
        rv = cloneURI->Equals(parentClone, &equal);
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

NS_IMETHODIMP
nsFrameLoader::UpdatePositionAndSize(nsIFrame *aIFrame)
{
  if (mRemoteFrame) {
    if (mRemoteBrowser) {
      nsIntSize size = GetSubDocumentSize(aIFrame);
      mRemoteBrowser->Move(size);
    }
    return NS_OK;
  }
  return UpdateBaseWindowPositionAndSize(aIFrame);
}

nsresult
nsFrameLoader::UpdateBaseWindowPositionAndSize(nsIFrame *aIFrame)
{
  nsCOMPtr<nsIDocShell> docShell;
  GetDocShell(getter_AddRefs(docShell));
  nsCOMPtr<nsIBaseWindow> baseWindow(do_QueryInterface(docShell));

  
  if (baseWindow) {
    PRInt32 x = 0;
    PRInt32 y = 0;

    nsWeakFrame weakFrame(aIFrame);

    baseWindow->GetPositionAndSize(&x, &y, nsnull, nsnull);

    if (!weakFrame.IsAlive()) {
      
      return NS_OK;
    }

    nsIntSize size = GetSubDocumentSize(aIFrame);

    baseWindow->SetPositionAndSize(x, y, size.width, size.height, PR_FALSE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::GetRenderMode(PRUint32* aRenderMode)
{
  *aRenderMode = mRenderMode;
  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::SetRenderMode(PRUint32 aRenderMode)
{
  if (aRenderMode == mRenderMode) {
    return NS_OK;
  }

  mRenderMode = aRenderMode;
  InvalidateFrame(GetPrimaryFrameOfOwningContent());
  return NS_OK;
}

nsIntSize
nsFrameLoader::GetSubDocumentSize(const nsIFrame *aIFrame)
{
  nsSize docSizeAppUnits;
  nsPresContext* presContext = aIFrame->PresContext();
  nsCOMPtr<nsIDOMHTMLFrameElement> frameElem = 
    do_QueryInterface(aIFrame->GetContent());
  if (frameElem) {
    docSizeAppUnits = aIFrame->GetSize();
  } else {
    docSizeAppUnits = aIFrame->GetContentRect().Size();
  }
  return nsIntSize(presContext->AppUnitsToDevPixels(docSizeAppUnits.width),
                   presContext->AppUnitsToDevPixels(docSizeAppUnits.height));
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

  PRInt32 parentType;
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

  PRUint32 chromeFlags = 0;
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

  ContentParent* parent = ContentParent::GetSingleton();
  NS_ASSERTION(parent->IsAlive(), "Process parent should be alive; something is very wrong!");
  mRemoteBrowser = parent->CreateTab(chromeFlags);
  if (mRemoteBrowser) {
    nsCOMPtr<nsIDOMElement> element = do_QueryInterface(mOwnerContent);
    mRemoteBrowser->SetOwnerElement(element);

    nsCOMPtr<nsIDocShellTreeItem> rootItem;
    parentAsItem->GetRootTreeItem(getter_AddRefs(rootItem));
    nsCOMPtr<nsIDOMWindow> rootWin = do_GetInterface(rootItem);
    nsCOMPtr<nsIDOMChromeWindow> rootChromeWin = do_QueryInterface(rootWin);
    NS_ABORT_IF_FALSE(rootChromeWin, "How did we not get a chrome window here?");

    nsCOMPtr<nsIBrowserDOMWindow> browserDOMWin;
    rootChromeWin->GetBrowserDOMWindow(getter_AddRefs(browserDOMWin));
    mRemoteBrowser->SetBrowserDOMWindow(browserDOMWin);
    
    mChildHost = parent;
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
nsFrameLoader::SendCrossProcessMouseEvent(const nsAString& aType,
                                          float aX,
                                          float aY,
                                          PRInt32 aButton,
                                          PRInt32 aClickCount,
                                          PRInt32 aModifiers,
                                          PRBool aIgnoreRootScrollFrame)
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
                                  PRBool aCapture)
{
  if (mRemoteBrowser) {
    return mRemoteBrowser->SendActivateFrameEvent(nsString(aType), aCapture) ?
      NS_OK : NS_ERROR_NOT_AVAILABLE;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsFrameLoader::SendCrossProcessKeyEvent(const nsAString& aType,
                                        PRInt32 aKeyCode,
                                        PRInt32 aCharCode,
                                        PRInt32 aModifiers,
                                        PRBool aPreventDefault)
{
  if (mRemoteBrowser) {
    mRemoteBrowser->SendKeyEvent(aType, aKeyCode, aCharCode, aModifiers,
                                 aPreventDefault);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsFrameLoader::GetDelayRemoteDialogs(PRBool* aRetVal)
{
  *aRetVal = mDelayRemoteDialogs;
  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::SetDelayRemoteDialogs(PRBool aDelay)
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

bool LoadScript(void* aCallbackData, const nsAString& aURL)
{
  mozilla::dom::PBrowserParent* tabParent =
    static_cast<nsFrameLoader*>(aCallbackData)->GetRemoteBrowser();
  if (tabParent) {
    return tabParent->SendLoadRemoteScript(nsString(aURL));
  }
  nsFrameLoader* fl = static_cast<nsFrameLoader*>(aCallbackData);
  nsRefPtr<nsInProcessTabChildGlobal> tabChild =
    static_cast<nsInProcessTabChildGlobal*>(fl->GetTabChildGlobalAsEventTarget());
  if (tabChild) {
    tabChild->LoadFrameScript(aURL);
  }
  return true;
}

class nsAsyncMessageToChild : public nsRunnable
{
public:
  nsAsyncMessageToChild(nsFrameLoader* aFrameLoader,
                        const nsAString& aMessage, const nsAString& aJSON)
    : mFrameLoader(aFrameLoader), mMessage(aMessage), mJSON(aJSON) {}

  NS_IMETHOD Run()
  {
    nsInProcessTabChildGlobal* tabChild =
      static_cast<nsInProcessTabChildGlobal*>(mFrameLoader->mChildMessageManager.get());
    if (tabChild && tabChild->GetInnerManager()) {
      tabChild->GetInnerManager()->
        ReceiveMessage(static_cast<nsPIDOMEventTarget*>(tabChild), mMessage,
                       PR_FALSE, mJSON, nsnull, nsnull);
    }
    return NS_OK;
  }
  nsRefPtr<nsFrameLoader> mFrameLoader;
  nsString mMessage;
  nsString mJSON;
};

bool SendAsyncMessageToChild(void* aCallbackData,
                             const nsAString& aMessage,
                             const nsAString& aJSON)
{
  mozilla::dom::PBrowserParent* tabParent =
    static_cast<nsFrameLoader*>(aCallbackData)->GetRemoteBrowser();
  if (tabParent) {
    return tabParent->SendAsyncMessage(nsString(aMessage), nsString(aJSON));
  }
  nsRefPtr<nsIRunnable> ev =
    new nsAsyncMessageToChild(static_cast<nsFrameLoader*>(aCallbackData),
                              aMessage, aJSON);
  NS_DispatchToCurrentThread(ev);
  return true;
}

NS_IMETHODIMP
nsFrameLoader::GetMessageManager(nsIChromeFrameMessageManager** aManager)
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
                                 PRUint32* aLength,
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
    *aResult = nsnull;
    *aLength = 0;
    return NS_OK;
  }

  nsIContentView** result = reinterpret_cast<nsIContentView**>(
    NS_Alloc(ids.Length() * sizeof(nsIContentView*)));

  for (PRUint32 i = 0; i < ids.Length(); i++) {
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
    *aContentView = nsnull;
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

  if (!mIsTopLevelContent && !mRemoteFrame) {
    return NS_OK;
  }

  if (mMessageManager) {
    if (ShouldUseRemoteProcess()) {
      mMessageManager->SetCallbackData(mRemoteBrowserShown ? this : nsnull);
    }
    return NS_OK;
  }

  nsIScriptContext* sctx = mOwnerContent->GetContextForEventHandlers(&rv);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_STATE(sctx);
  JSContext* cx = static_cast<JSContext*>(sctx->GetNativeContext());
  NS_ENSURE_STATE(cx);

  nsCOMPtr<nsIDOMChromeWindow> chromeWindow =
    do_QueryInterface(mOwnerContent->GetOwnerDoc()->GetWindow());
  NS_ENSURE_STATE(chromeWindow);
  nsCOMPtr<nsIChromeFrameMessageManager> parentManager;
  chromeWindow->GetMessageManager(getter_AddRefs(parentManager));

  if (ShouldUseRemoteProcess()) {
    mMessageManager = new nsFrameMessageManager(PR_TRUE,
                                                nsnull,
                                                SendAsyncMessageToChild,
                                                LoadScript,
                                                mRemoteBrowserShown ? this : nsnull,
                                                static_cast<nsFrameMessageManager*>(parentManager.get()),
                                                cx);
    NS_ENSURE_TRUE(mMessageManager, NS_ERROR_OUT_OF_MEMORY);
  } else
  {

    mMessageManager = new nsFrameMessageManager(PR_TRUE,
                                                nsnull,
                                                SendAsyncMessageToChild,
                                                LoadScript,
                                                nsnull,
                                                static_cast<nsFrameMessageManager*>(parentManager.get()),
                                                cx);
    NS_ENSURE_TRUE(mMessageManager, NS_ERROR_OUT_OF_MEMORY);
    mChildMessageManager =
      new nsInProcessTabChildGlobal(mDocShell, mOwnerContent, mMessageManager);
    mMessageManager->SetCallbackData(this);
  }
  return NS_OK;
}

nsPIDOMEventTarget*
nsFrameLoader::GetTabChildGlobalAsEventTarget()
{
  return static_cast<nsInProcessTabChildGlobal*>(mChildMessageManager.get());
}
