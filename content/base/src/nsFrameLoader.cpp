












































#include "prenv.h"

#include "nsIDOMHTMLIFrameElement.h"
#include "nsIDOMHTMLFrameElement.h"
#include "nsIDOMWindow.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIContent.h"
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

#include "nsIURI.h"
#include "nsIURL.h"
#include "nsNetUtil.h"

#include "nsGkAtoms.h"
#include "nsINameSpaceManager.h"

#include "nsThreadUtils.h"
#include "nsIView.h"

#ifdef MOZ_IPC
#include "TabParent.h"

using namespace mozilla;
using namespace mozilla::tabs;
#endif

#ifdef MOZ_WIDGET_GTK2
#include "mozcontainer.h"

#include <gdk/gdkx.h>
#include <gtk/gtk.h>
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

#ifdef MOZ_IPC
  if (!mTriedNewProcess) {
    TryNewProcess();
    mTriedNewProcess = PR_TRUE;
  }

  if (mChildProcess) {
    
    mChildProcess->LoadURL(mURIToLoad);
    return NS_OK;
  }
#endif
  
  
  nsresult rv = CheckURILoad(mURIToLoad);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = EnsureDocShell();
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

  
  return CheckForRecursiveLoad(aURI);
}

NS_IMETHODIMP
nsFrameLoader::GetDocShell(nsIDocShell **aDocShell)
{
  *aDocShell = nsnull;

  
  
  
  if (mOwnerContent) {
    nsresult rv = EnsureDocShell();
    NS_ENSURE_SUCCESS(rv, rv);
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

  if (aOwningContent->IsNodeOfType(nsINode::eXUL)) {
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

nsresult
nsFrameLoader::EnsureDocShell()
{
  if (mDocShell) {
    return NS_OK;
  }
  NS_ENSURE_STATE(!mDestroyCalled);

  
  
  
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
  mDepthTooGreat = PR_FALSE;
  nsresult rv = EnsureDocShell();
  NS_ENSURE_SUCCESS(rv, rv);

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

#ifdef MOZ_IPC
PRBool
nsFrameLoader::TryNewProcess()
{
  if (PR_GetEnv("MOZ_DISABLE_OOP_TABS")) {
      return PR_FALSE;
  }

  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (!prefs) {
      return PR_FALSE;
  }

  PRBool oopTabsEnabled = PR_FALSE;
  prefs->GetBoolPref("dom.ipc.tabs.enabled", &oopTabsEnabled);

  if (!oopTabsEnabled) {
      return PR_FALSE;
  }

  nsIDocument* doc = mOwnerContent->GetDocument();
  if (!doc) {
    return PR_FALSE;
  }

  if (doc->GetDisplayDocument()) {
    
    return PR_FALSE;
  }

  nsCOMPtr<nsIWebNavigation> parentAsWebNav =
    do_GetInterface(doc->GetScriptGlobalObject());

  if (!parentAsWebNav) {
    return PR_FALSE;
  }

  nsCOMPtr<nsIDocShellTreeItem> parentAsItem(do_QueryInterface(parentAsWebNav));

  PRInt32 parentType;
  parentAsItem->GetItemType(&parentType);

  if (parentType != nsIDocShellTreeItem::typeChrome) {
    return PR_FALSE;
  }

  if (!mOwnerContent->IsNodeOfType(nsINode::eXUL)) {
    return PR_FALSE;
  }

  NS_ERROR("trying to start new process");
  nsAutoString value;
  mOwnerContent->GetAttr(kNameSpaceID_None, nsGkAtoms::type, value);

  if (!value.LowerCaseEqualsLiteral("content") &&
      !StringBeginsWith(value, NS_LITERAL_STRING("content-"),
                        nsCaseInsensitiveStringComparator())) {
    return PR_FALSE;
  }

  

  
  doc->FlushPendingNotifications(Flush_Layout);
  nsIFrame* ourFrame =
    doc->GetPrimaryShell()->GetPrimaryFrameFor(mOwnerContent);
  nsIView* ancestorView = ourFrame->GetView();

  nsIView* firstChild = ancestorView->GetFirstChild();
  if (!firstChild) {
    NS_ERROR("no first child");
    return PR_FALSE;
  }

  nsIWidget* w = firstChild->GetWidget();
  if (!w) {
    NS_ERROR("we're stuffed!");
    return PR_FALSE;
  }
  
  

  nsPresContext* presContext = ourFrame->PresContext();

#ifdef XP_WIN
  HWND parentwin =
    static_cast<HWND>(w->GetNativeData(NS_NATIVE_WINDOW));

  mChildProcess = new TabParent(parentwin);
  mChildProcess->Move(0, 0,
                      presContext->AppUnitsToDevPixels(ourFrame->GetSize().width),
                      presContext->AppUnitsToDevPixels(ourFrame->GetSize().height));
                      
#elif defined(MOZ_WIDGET_GTK2)
  GdkWindow* parent_win =
    static_cast<GdkWindow*>(w->GetNativeData(NS_NATIVE_WINDOW));
  
  gpointer user_data = nsnull;
  gdk_window_get_user_data(parent_win, &user_data);

  MozContainer* parentMozContainer = MOZ_CONTAINER(user_data);
  GtkContainer* container = GTK_CONTAINER(parentMozContainer);

  
  GtkWidget* socket = gtk_socket_new();
  gtk_widget_set_parent_window(socket, parent_win);
  gtk_container_add(container, socket);
  gtk_widget_realize(socket);

  
  GtkAllocation alloc;
  alloc.x = 0;                  
  alloc.y = 0;
  alloc.width = presContext->AppUnitsToDevPixels(ourFrame->GetSize().width);
  alloc.height = presContext->AppUnitsToDevPixels(ourFrame->GetSize().height);
  gtk_widget_size_allocate(socket, &alloc);

  gtk_widget_show(socket);

  GdkNativeWindow id = gtk_socket_get_id((GtkSocket*)socket);

  mChildProcess = new TabParent(id);

  mChildProcess->Move(0, 0, alloc.width, alloc.height);

#else
#error TODO for this platform
#endif

  return PR_TRUE;
}
#endif
