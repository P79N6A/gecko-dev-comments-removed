













































#ifdef MOZ_IPC
#  include "base/basictypes.h"
#endif

#include "prenv.h"

#include "nsIDOMHTMLIFrameElement.h"
#include "nsIDOMHTMLFrameElement.h"
#include "nsIDOMWindow.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIContent.h"
#include "nsIContentViewer.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIWebNavigation.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDocShellLoadInfo.h"
#include "nsIBaseWindow.h"
#include "nsContentUtils.h"
#include "nsUnicharUtils.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScrollable.h"
#include "nsFrameLoader.h"
#include "nsIDOMEventTarget.h"
#include "nsIFrame.h"
#include "nsIFrameFrame.h"
#include "nsDOMError.h"
#include "nsPresShellIterator.h"
#include "nsGUIEvent.h"
#include "nsEventDispatcher.h"
#include "nsISHistory.h"
#include "nsISHistoryInternal.h"
#include "nsIDOMNSHTMLDocument.h"
#include "nsLayoutUtils.h"

#include "nsIURI.h"
#include "nsIURL.h"
#include "nsNetUtil.h"

#include "nsGkAtoms.h"
#include "nsINameSpaceManager.h"

#include "nsThreadUtils.h"
#include "nsIView.h"

#ifdef MOZ_WIDGET_GTK2
#include "mozcontainer.h"

#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#endif

#ifdef MOZ_IPC
#include "ContentProcessParent.h"
#include "TabParent.h"

using namespace mozilla;
using namespace mozilla::dom;
#endif

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







#define MAX_SAME_URL_CONTENT_FRAMES 1








#define MAX_DEPTH_CONTENT_FRAMES 10

NS_IMPL_CYCLE_COLLECTION_1(nsFrameLoader, mDocShell)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsFrameLoader)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsFrameLoader)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsFrameLoader)
  NS_INTERFACE_MAP_ENTRY(nsIFrameLoader)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

nsFrameLoader*
nsFrameLoader::Create(nsIContent* aOwner)
{
  NS_ENSURE_TRUE(aOwner, nsnull);
  nsIDocument* doc = aOwner->GetCurrentDoc();
  NS_ENSURE_TRUE(doc && !doc->GetDisplayDocument() &&
                 !doc->IsLoadedAsData(), nsnull);

  return new nsFrameLoader(aOwner);
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
  if (!doc) {
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

  NS_ENSURE_SUCCESS(rv, rv);
  return LoadURI(uri);
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
  NS_ENSURE_STATE(mURIToLoad && mOwnerContent && mOwnerContent->IsInDoc());

  nsresult rv = MaybeCreateDocShell();
  if (NS_FAILED(rv)) {
    return rv;
  }

#ifdef MOZ_IPC
  if (mRemoteFrame) {
    if (!mChildProcess) {
      TryNewProcess();
    }

    if (!mChildProcess) {
      NS_WARNING("Couldn't create child process for iframe.");
      return NS_ERROR_FAILURE;
    }

    
    mChildProcess->LoadURL(mURIToLoad);
    return NS_OK;
  }
#endif

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
#ifdef DEBUG
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to load the URL");
  }
#endif
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
#ifdef MOZ_IPC
  if (mRemoteFrame) {
    return NS_OK;
  }
#endif
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
#ifdef MOZ_IPC
    if (mRemoteFrame) {
      NS_WARNING("No docshells for remote frames!");
      return NS_ERROR_NOT_AVAILABLE;
    }
#endif
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

bool
nsFrameLoader::Show(PRInt32 marginWidth, PRInt32 marginHeight,
                    PRInt32 scrollbarPrefX, PRInt32 scrollbarPrefY,
                    nsIFrameFrame* frame)
{
  nsContentType contentType;

  nsresult rv = MaybeCreateDocShell();
  if (NS_FAILED(rv)) {
    return false;
  }

#ifdef MOZ_IPC
  if (mRemoteFrame) {
    contentType = eContentTypeUI;
  }
  else
#endif
  {
    if (!mDocShell)
      return false;

    nsCOMPtr<nsIPresShell> presShell;
    mDocShell->GetPresShell(getter_AddRefs(presShell));
    if (presShell)
      return true;

    mDocShell->SetMarginWidth(marginWidth);
    mDocShell->SetMarginHeight(marginHeight);

    nsCOMPtr<nsIScrollable> sc = do_QueryInterface(mDocShell);
    if (sc) {
      sc->SetDefaultScrollbarPreferences(nsIScrollable::ScrollOrientation_X,
                                         scrollbarPrefX);
      sc->SetDefaultScrollbarPreferences(nsIScrollable::ScrollOrientation_Y,
                                         scrollbarPrefY);
    }


    nsCOMPtr<nsIDocShellTreeItem> treeItem = do_QueryInterface(mDocShell);
    NS_ASSERTION(treeItem,
                 "Found a nsIDocShell that isn't a nsIDocShellTreeItem.");

    PRInt32 itemType;
    treeItem->GetItemType(&itemType);

    if (itemType == nsIDocShellTreeItem::typeChrome)
      contentType = eContentTypeUI;
    else {
      nsCOMPtr<nsIDocShellTreeItem> sameTypeParent;
      treeItem->GetSameTypeParent(getter_AddRefs(sameTypeParent));
      contentType = sameTypeParent ? eContentTypeContentFrame : eContentTypeContent;
    }
  }

  nsIView* view = frame->CreateViewAndWidget(contentType);
  if (!view)
    return false;

#ifdef MOZ_IPC
  if (mRemoteFrame) {
    return ShowRemoteFrame(frame, view);
  }
#endif

  nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(mDocShell);
  NS_ASSERTION(baseWindow, "Found a nsIDocShell that isn't a nsIBaseWindow.");
  baseWindow->InitWindow(nsnull, view->GetWidget(), 0, 0, 10, 10);
  
  
  
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
        doc->SetDesignMode(NS_LITERAL_STRING("off"));
        doc->SetDesignMode(NS_LITERAL_STRING("on"));
      }
    }
  }

  return true;
}

#ifdef MOZ_IPC
bool
nsFrameLoader::ShowRemoteFrame(nsIFrameFrame* frame, nsIView* view)
{
  NS_ASSERTION(mRemoteFrame, "ShowRemote only makes sense on remote frames.");

  TryNewProcess();
  if (!mChildProcess) {
    NS_ERROR("Couldn't create child process.");
    return false;
  }

  nsIWidget* w = view->GetWidget();
  if (!w) {
    NS_ERROR("Our view doesn't have a widget. Totally stuffed!");
    return false;
  }

  nsIntSize size = GetSubDocumentSize(frame->GetFrame());

#ifdef XP_WIN
  HWND parentwin =
    static_cast<HWND>(w->GetNativeData(NS_NATIVE_WINDOW));

  mChildProcess->SendcreateWidget(parentwin);
#elif defined(MOZ_WIDGET_GTK2)
  GdkWindow* parent_win =
    static_cast<GdkWindow*>(w->GetNativeData(NS_NATIVE_WINDOW));

  gpointer user_data = nsnull;
  gdk_window_get_user_data(parent_win, &user_data);

  MozContainer* parentMozContainer = MOZ_CONTAINER(user_data);
  GtkContainer* container = GTK_CONTAINER(parentMozContainer);

  
  mRemoteSocket = gtk_socket_new();
  gtk_widget_set_parent_window(mRemoteSocket, parent_win);
  gtk_container_add(container, mRemoteSocket);
  gtk_widget_realize(mRemoteSocket);

  
  GtkAllocation alloc = { 0, 0, size.width, size.height };
  gtk_widget_size_allocate(mRemoteSocket, &alloc);

  gtk_widget_show(mRemoteSocket);
  GdkNativeWindow id = gtk_socket_get_id(GTK_SOCKET(mRemoteSocket));
  mChildProcess->SendcreateWidget(id);

#else
#error TODO for this platform
#endif

  mChildProcess->Move(0, 0, size.width, size.height);

  return true;
}
#endif

void
nsFrameLoader::Hide()
{
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

  nsPresShellIterator iter1(ourDoc);
  nsPresShellIterator iter2(otherDoc);
  if (iter1.HasMoreThanOneShell() || iter2.HasMoreThanOneShell()) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsIPresShell* ourShell = ourDoc->GetPrimaryShell();
  nsIPresShell* otherShell = otherDoc->GetPrimaryShell();
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
  
  nsIFrame* ourFrame = ourShell->GetPrimaryFrameFor(ourContent);
  nsIFrame* otherFrame = otherShell->GetPrimaryFrameFor(otherContent);
  if (!ourFrame || !otherFrame) {
    mInSwap = aOther->mInSwap = PR_FALSE;
    FirePageShowEvent(ourTreeItem, ourChromeEventHandler, PR_TRUE);
    FirePageShowEvent(otherTreeItem, otherChromeEventHandler, PR_TRUE);
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsIFrameFrame* ourFrameFrame = do_QueryFrame(ourFrame);
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

  mOwnerContent = otherContent;
  aOther->mOwnerContent = ourContent;

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

  
  if (ourFrame == ourShell->GetPrimaryFrameFor(ourContent) &&
      otherFrame == otherShell->GetPrimaryFrameFor(otherContent)) {
    ourFrameFrame->EndSwapDocShells(otherFrame);
  }

  ourParentDocument->FlushPendingNotifications(Flush_Layout);
  otherParentDocument->FlushPendingNotifications(Flush_Layout);
  
  FirePageShowEvent(ourTreeItem, otherChromeEventHandler, PR_TRUE);
  FirePageShowEvent(otherTreeItem, ourChromeEventHandler, PR_TRUE);

  mInSwap = aOther->mInSwap = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsFrameLoader::Destroy()
{
  if (mDestroyCalled) {
    return NS_OK;
  }
  mDestroyCalled = PR_TRUE;

  nsCOMPtr<nsIDocument> doc;
  if (mOwnerContent) {
    doc = mOwnerContent->GetOwnerDoc();

    if (doc) {
      doc->SetSubDocumentFor(mOwnerContent, nsnull);
    }

    mOwnerContent = nsnull;
  }
#ifdef MOZ_IPC
  if (mChildProcess) {
    mChildProcess->SetOwnerElement(nsnull);
  }
#endif

  
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

#ifdef MOZ_IPC
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
#endif

nsresult
nsFrameLoader::MaybeCreateDocShell()
{
  if (mDocShell) {
    return NS_OK;
  }
#ifdef MOZ_IPC
  if (mRemoteFrame) {
    return NS_OK;
  }
#endif
  NS_ENSURE_STATE(!mDestroyCalled);

#ifdef MOZ_IPC
  if (ShouldUseRemoteProcess()) {
    mRemoteFrame = true;
    return NS_OK;
  }
#endif

  
  
  
  nsIDocument* doc = mOwnerContent->GetDocument();
  if (!doc) {
    return NS_ERROR_UNEXPECTED;
  }

  if (doc->GetDisplayDocument()) {
    
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsIWebNavigation> parentAsWebNav =
    do_GetInterface(doc->GetScriptGlobalObject());

  
  mDocShell = do_CreateInstance("@mozilla.org/docshell;1");
  NS_ENSURE_TRUE(mDocShell, NS_ERROR_FAILURE);

  
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
#ifdef MOZ_IPC
  NS_ASSERTION(!mRemoteFrame,
               "Shouldn't call CheckForRecursiveLoad on remote frames.");
#endif
  if (!mDocShell) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDocShellTreeItem> treeItem = do_QueryInterface(mDocShell);
  NS_ASSERTION(treeItem, "docshell must be a treeitem!");
  
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
#ifdef MOZ_IPC
  if (mRemoteFrame) {
    if (mChildProcess) {
      nsIntSize size = GetSubDocumentSize(aIFrame);

#ifdef MOZ_WIDGET_GTK2
      if (mRemoteSocket) {
        GtkAllocation alloc = {0, 0, size.width, size.height };
        gtk_widget_size_allocate(mRemoteSocket, &alloc);
      }
#endif

      mChildProcess->Move(0, 0, size.width, size.height);
    }
    return NS_OK;
  }
#endif
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

nsIntSize
nsFrameLoader::GetSubDocumentSize(const nsIFrame *aIFrame)
{
  nsAutoDisableGetUsedXAssertions disableAssert;
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

#ifdef MOZ_IPC
bool
nsFrameLoader::TryNewProcess()
{
  NS_ASSERTION(!mChildProcess, "TryNewProcess called with a process already?");

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

  mChildProcess = ContentProcessParent::GetSingleton()->CreateTab();
  if (mChildProcess) {
    nsCOMPtr<nsIDOMElement> element = do_QueryInterface(mOwnerContent);
    mChildProcess->SetOwnerElement(element);
  }
  return true;
}
#endif

#ifdef MOZ_IPC
mozilla::dom::PIFrameEmbeddingParent*
nsFrameLoader::GetChildProcess()
{
  return mChildProcess;
}
#endif

NS_IMETHODIMP
nsFrameLoader::ActivateRemoteFrame() {
#ifdef MOZ_IPC
  if (mChildProcess) {
    mChildProcess->Activate();
    return NS_OK;
  }
#endif
  return NS_ERROR_UNEXPECTED;
}
