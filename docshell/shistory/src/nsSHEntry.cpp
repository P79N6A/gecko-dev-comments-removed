






































#ifdef DEBUG_bryner
#define DEBUG_PAGE_CACHE
#endif


#include "nsSHEntry.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsIDocShellLoadInfo.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsAutoPtr.h"
#include "nsThreadUtils.h"
#include "nsIWebNavigation.h"
#include "nsISHistory.h"
#include "nsISHistoryInternal.h"
#include "nsDocShellEditorData.h"
#include "nsIDocShell.h"


#define CONTENT_VIEWER_TIMEOUT_SECONDS 30*60

typedef nsExpirationTracker<nsSHEntry,3> HistoryTrackerBase;
class HistoryTracker : public HistoryTrackerBase {
public:
  
  HistoryTracker() : HistoryTrackerBase((CONTENT_VIEWER_TIMEOUT_SECONDS/2)*1000) {}
  
protected:
  virtual void NotifyExpired(nsSHEntry* aObj) {
    RemoveObject(aObj);
    aObj->Expire();
  }
};

static HistoryTracker *gHistoryTracker = nsnull;
static PRUint32 gEntryID = 0;

nsresult nsSHEntry::Startup()
{
  gHistoryTracker = new HistoryTracker();
  return gHistoryTracker ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

void nsSHEntry::Shutdown()
{
  delete gHistoryTracker;
  gHistoryTracker = nsnull;
}

static void StopTrackingEntry(nsSHEntry *aEntry)
{
  if (aEntry->GetExpirationState()->IsTracked()) {
    gHistoryTracker->RemoveObject(aEntry);
  }
}






nsSHEntry::nsSHEntry() 
  : mLoadType(0)
  , mID(gEntryID++)
  , mPageIdentifier(mID)
  , mScrollPositionX(0)
  , mScrollPositionY(0)
  , mIsFrameNavigation(PR_FALSE)
  , mSaveLayoutState(PR_TRUE)
  , mExpired(PR_FALSE)
  , mSticky(PR_TRUE)
  , mParent(nsnull)
  , mViewerBounds(0, 0, 0, 0)
{
}

nsSHEntry::nsSHEntry(const nsSHEntry &other)
  : mURI(other.mURI)
  , mReferrerURI(other.mReferrerURI)
  
  , mTitle(other.mTitle)
  , mPostData(other.mPostData)
  , mLayoutHistoryState(other.mLayoutHistoryState)
  , mLoadType(0)         
  , mID(other.mID)
  , mPageIdentifier(other.mPageIdentifier)
  , mScrollPositionX(0)  
  , mScrollPositionY(0)  
  , mIsFrameNavigation(other.mIsFrameNavigation)
  , mSaveLayoutState(other.mSaveLayoutState)
  , mExpired(other.mExpired)
  , mSticky(PR_TRUE)
  
  , mCacheKey(other.mCacheKey)
  , mParent(other.mParent)
  , mViewerBounds(0, 0, 0, 0)
  , mOwner(other.mOwner)
{
}

static PRBool
ClearParentPtr(nsISHEntry* aEntry, void* )
{
  if (aEntry) {
    aEntry->SetParent(nsnull);
  }
  return PR_TRUE;
}

nsSHEntry::~nsSHEntry()
{
  StopTrackingEntry(this);

  
  
  mChildren.EnumerateForwards(ClearParentPtr, nsnull);
  mChildren.Clear();

  nsCOMPtr<nsIContentViewer> viewer = mContentViewer;
  DropPresentationState();
  if (viewer) {
    viewer->Destroy();
  }

  mEditorData = nsnull;

#ifdef DEBUG
  
  nsExpirationTracker<nsSHEntry,3>::Iterator iterator(gHistoryTracker);
  nsSHEntry* elem;
  while ((elem = iterator.Next()) != nsnull) {
    NS_ASSERTION(elem != this, "Found dead entry still in the tracker!");
  }
#endif
}





NS_IMPL_ISUPPORTS4(nsSHEntry, nsISHContainer, nsISHEntry, nsIHistoryEntry,
                   nsIMutationObserver)





NS_IMETHODIMP nsSHEntry::SetScrollPosition(PRInt32 x, PRInt32 y)
{
  mScrollPositionX = x;
  mScrollPositionY = y;
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::GetScrollPosition(PRInt32 *x, PRInt32 *y)
{
  *x = mScrollPositionX;
  *y = mScrollPositionY;
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::GetURI(nsIURI** aURI)
{
  *aURI = mURI;
  NS_IF_ADDREF(*aURI);
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::SetURI(nsIURI* aURI)
{
  mURI = aURI;
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::GetReferrerURI(nsIURI **aReferrerURI)
{
  *aReferrerURI = mReferrerURI;
  NS_IF_ADDREF(*aReferrerURI);
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::SetReferrerURI(nsIURI *aReferrerURI)
{
  mReferrerURI = aReferrerURI;
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::SetContentViewer(nsIContentViewer *aViewer)
{
  NS_PRECONDITION(!aViewer || !mContentViewer, "SHEntry already contains viewer");

  if (mContentViewer || !aViewer) {
    DropPresentationState();
  }

  mContentViewer = aViewer;

  if (mContentViewer) {
    gHistoryTracker->AddObject(this);

    nsCOMPtr<nsIDOMDocument> domDoc;
    mContentViewer->GetDOMDocument(getter_AddRefs(domDoc));
    
    
    mDocument = do_QueryInterface(domDoc);
    if (mDocument) {
      mDocument->SetShellsHidden(PR_TRUE);
      mDocument->AddMutationObserver(this);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::GetContentViewer(nsIContentViewer **aResult)
{
  *aResult = mContentViewer;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::GetAnyContentViewer(nsISHEntry **aOwnerEntry,
                               nsIContentViewer **aResult)
{
  
  
  
  GetContentViewer(aResult);
  if (*aResult) {
#ifdef DEBUG_PAGE_CACHE 
    printf("Found content viewer\n");
#endif
    *aOwnerEntry = this;
    NS_ADDREF(*aOwnerEntry);
    return NS_OK;
  }
  
  for (PRInt32 i = 0; i < mChildren.Count(); i++) {
    nsISHEntry* child = mChildren[i];
    if (child) {
#ifdef DEBUG_PAGE_CACHE
      printf("Evaluating SHEntry child %d\n", i);
#endif
      child->GetAnyContentViewer(aOwnerEntry, aResult);
      if (*aResult) {
        return NS_OK;
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::SetSticky(PRBool aSticky)
{
  mSticky = aSticky;
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::GetSticky(PRBool *aSticky)
{
  *aSticky = mSticky;
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::GetTitle(PRUnichar** aTitle)
{
  
  if (mTitle.IsEmpty() && mURI) {
    
    nsCAutoString spec;
    if (NS_SUCCEEDED(mURI->GetSpec(spec)))
      AppendUTF8toUTF16(spec, mTitle);
  }

  *aTitle = ToNewUnicode(mTitle);
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::SetTitle(const nsAString &aTitle)
{
  mTitle = aTitle;
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::GetPostData(nsIInputStream** aResult)
{
  *aResult = mPostData;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::SetPostData(nsIInputStream* aPostData)
{
  mPostData = aPostData;
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::GetLayoutHistoryState(nsILayoutHistoryState** aResult)
{
  *aResult = mLayoutHistoryState;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::SetLayoutHistoryState(nsILayoutHistoryState* aState)
{
  mLayoutHistoryState = aState;
  if (mLayoutHistoryState)
    mLayoutHistoryState->SetScrollPositionOnly(!mSaveLayoutState);

  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::GetLoadType(PRUint32 * aResult)
{
  *aResult = mLoadType;
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::SetLoadType(PRUint32  aLoadType)
{
  mLoadType = aLoadType;
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::GetID(PRUint32 * aResult)
{
  *aResult = mID;
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::SetID(PRUint32  aID)
{
  mID = aID;
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::GetPageIdentifier(PRUint32 * aResult)
{
  *aResult = mPageIdentifier;
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::SetPageIdentifier(PRUint32 aPageIdentifier)
{
  mPageIdentifier = aPageIdentifier;
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::GetIsSubFrame(PRBool * aFlag)
{
  *aFlag = mIsFrameNavigation;
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::SetIsSubFrame(PRBool  aFlag)
{
  mIsFrameNavigation = aFlag;
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::GetCacheKey(nsISupports** aResult)
{
  *aResult = mCacheKey;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::SetCacheKey(nsISupports* aCacheKey)
{
  mCacheKey = aCacheKey;
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::GetSaveLayoutStateFlag(PRBool * aFlag)
{
  *aFlag = mSaveLayoutState;
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::SetSaveLayoutStateFlag(PRBool  aFlag)
{
  mSaveLayoutState = aFlag;
  if (mLayoutHistoryState)
    mLayoutHistoryState->SetScrollPositionOnly(!aFlag);

  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::GetExpirationStatus(PRBool * aFlag)
{
  *aFlag = mExpired;
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::SetExpirationStatus(PRBool  aFlag)
{
  mExpired = aFlag;
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::GetContentType(nsACString& aContentType)
{
  aContentType = mContentType;
  return NS_OK;
}

NS_IMETHODIMP nsSHEntry::SetContentType(const nsACString& aContentType)
{
  mContentType = aContentType;
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::Create(nsIURI * aURI, const nsAString &aTitle,
                  nsIInputStream * aInputStream,
                  nsILayoutHistoryState * aLayoutHistoryState,
                  nsISupports * aCacheKey, const nsACString& aContentType,
                  nsISupports* aOwner)
{
  mURI = aURI;
  mTitle = aTitle;
  mPostData = aInputStream;
  mCacheKey = aCacheKey;
  mContentType = aContentType;
  mOwner = aOwner;
    
  
  mLoadType = (PRUint32) nsIDocShellLoadInfo::loadHistory;

  
  
  
  mIsFrameNavigation = PR_FALSE;

  
  mSaveLayoutState = PR_TRUE;
  mLayoutHistoryState = aLayoutHistoryState;

  
  mExpired = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::Clone(nsISHEntry ** aResult)
{
  *aResult = new nsSHEntry(*this);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::GetParent(nsISHEntry ** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = mParent;
  NS_IF_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::SetParent(nsISHEntry * aParent)
{
  




  mParent = aParent;
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::SetWindowState(nsISupports *aState)
{
  mWindowState = aState;
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::GetWindowState(nsISupports **aState)
{
  NS_IF_ADDREF(*aState = mWindowState);
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::SetViewerBounds(const nsIntRect &aBounds)
{
  mViewerBounds = aBounds;
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::GetViewerBounds(nsIntRect &aBounds)
{
  aBounds = mViewerBounds;
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::GetOwner(nsISupports **aOwner)
{
  NS_IF_ADDREF(*aOwner = mOwner);
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::SetOwner(nsISupports *aOwner)
{
  mOwner = aOwner;
  return NS_OK;
}





NS_IMETHODIMP 
nsSHEntry::GetChildCount(PRInt32 * aCount)
{
  *aCount = mChildren.Count();
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::AddChild(nsISHEntry * aChild, PRInt32 aOffset)
{
  NS_ENSURE_TRUE(aChild, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(aChild->SetParent(this), NS_ERROR_FAILURE);

  
  
  
  
  
  
  
  
  
  NS_ASSERTION(aOffset < (mChildren.Count()+1023), "Large frames array!\n");

  if (aOffset < mChildren.Count()) {
    nsISHEntry* oldChild = mChildren.ObjectAt(aOffset);
    if (oldChild && oldChild != aChild) {
      NS_ERROR("Adding child where we already have a child?  "
               "This will likely misbehave");
      oldChild->SetParent(nsnull);
    }
  }
  
  
  mChildren.ReplaceObjectAt(aChild, aOffset);

  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::RemoveChild(nsISHEntry * aChild)
{
  NS_ENSURE_TRUE(aChild, NS_ERROR_FAILURE);
  PRBool childRemoved = mChildren.RemoveObject(aChild);
  if (childRemoved)
    aChild->SetParent(nsnull);
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::GetChildAt(PRInt32 aIndex, nsISHEntry ** aResult)
{
  if (aIndex >= 0 && aIndex < mChildren.Count()) {
    *aResult = mChildren[aIndex];
    
    
    NS_IF_ADDREF(*aResult);
  } else {
    *aResult = nsnull;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::AddChildShell(nsIDocShellTreeItem *aShell)
{
  NS_ASSERTION(aShell, "Null child shell added to history entry");
  mChildShells.AppendObject(aShell);
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::ChildShellAt(PRInt32 aIndex, nsIDocShellTreeItem **aShell)
{
  NS_IF_ADDREF(*aShell = mChildShells.SafeObjectAt(aIndex));
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::ClearChildShells()
{
  mChildShells.Clear();
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::GetRefreshURIList(nsISupportsArray **aList)
{
  NS_IF_ADDREF(*aList = mRefreshURIList);
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::SetRefreshURIList(nsISupportsArray *aList)
{
  mRefreshURIList = aList;
  return NS_OK;
}

NS_IMETHODIMP
nsSHEntry::SyncPresentationState()
{
  if (mContentViewer && mWindowState) {
    
    return NS_OK;
  }

  DropPresentationState();

  return NS_OK;
}

void
nsSHEntry::DropPresentationState()
{
  nsRefPtr<nsSHEntry> kungFuDeathGrip = this;

  if (mDocument) {
    mDocument->SetShellsHidden(PR_FALSE);
    mDocument->RemoveMutationObserver(this);
    mDocument = nsnull;
  }
  if (mContentViewer)
    mContentViewer->ClearHistoryEntry();

  StopTrackingEntry(this);
  mContentViewer = nsnull;
  mSticky = PR_TRUE;
  mWindowState = nsnull;
  mViewerBounds.SetRect(0, 0, 0, 0);
  mChildShells.Clear();
  mRefreshURIList = nsnull;
}

void
nsSHEntry::Expire()
{
  
  
  if (!mContentViewer)
    return;
  nsCOMPtr<nsISupports> container;
  mContentViewer->GetContainer(getter_AddRefs(container));
  nsCOMPtr<nsIDocShellTreeItem> treeItem = do_QueryInterface(container);
  if (!treeItem)
    return;
  
  
  nsCOMPtr<nsIDocShellTreeItem> root;
  treeItem->GetSameTypeRootTreeItem(getter_AddRefs(root));
  nsCOMPtr<nsIWebNavigation> webNav = do_QueryInterface(root);
  nsCOMPtr<nsISHistory> history;
  webNav->GetSessionHistory(getter_AddRefs(history));
  nsCOMPtr<nsISHistoryInternal> historyInt = do_QueryInterface(history);
  if (!historyInt)
    return;
  historyInt->EvictExpiredContentViewerForEntry(this);
}





void
nsSHEntry::NodeWillBeDestroyed(const nsINode* aNode)
{
  NS_NOTREACHED("Document destroyed while we're holding a strong ref to it");
}

void
nsSHEntry::CharacterDataWillChange(nsIDocument* aDocument,
                                   nsIContent* aContent,
                                   CharacterDataChangeInfo* aInfo)
{
}

void
nsSHEntry::CharacterDataChanged(nsIDocument* aDocument,
                                nsIContent* aContent,
                                CharacterDataChangeInfo* aInfo)
{
  DocumentMutated();
}

void
nsSHEntry::AttributeWillChange(nsIDocument* aDocument,
                               nsIContent* aContent,
                               PRInt32 aNameSpaceID,
                               nsIAtom* aAttribute,
                               PRInt32 aModType)
{
}

void
nsSHEntry::AttributeChanged(nsIDocument* aDocument,
                            nsIContent* aContent,
                            PRInt32 aNameSpaceID,
                            nsIAtom* aAttribute,
                            PRInt32 aModType)
{
  DocumentMutated();
}

void
nsSHEntry::ContentAppended(nsIDocument* aDocument,
                        nsIContent* aContainer,
                        PRInt32 aNewIndexInContainer)
{
  DocumentMutated();
}

void
nsSHEntry::ContentInserted(nsIDocument* aDocument,
                           nsIContent* aContainer,
                           nsIContent* aChild,
                           PRInt32 aIndexInContainer)
{
  DocumentMutated();
}

void
nsSHEntry::ContentRemoved(nsIDocument* aDocument,
                          nsIContent* aContainer,
                          nsIContent* aChild,
                          PRInt32 aIndexInContainer)
{
  DocumentMutated();
}

void
nsSHEntry::ParentChainChanged(nsIContent *aContent)
{
}

class DestroyViewerEvent : public nsRunnable
{
public:
  DestroyViewerEvent(nsIContentViewer* aViewer, nsIDocument* aDocument)
    : mViewer(aViewer),
      mDocument(aDocument)
  {}

  NS_IMETHOD Run()
  {
    if (mViewer)
      mViewer->Destroy();
    return NS_OK;
  }

  nsCOMPtr<nsIContentViewer> mViewer;
  nsCOMPtr<nsIDocument> mDocument;
};

void
nsSHEntry::DocumentMutated()
{
  NS_ASSERTION(mContentViewer && mDocument,
               "we shouldn't still be observing the doc");

  
  

  nsCOMPtr<nsIRunnable> evt =
      new DestroyViewerEvent(mContentViewer, mDocument);
  nsresult rv = NS_DispatchToCurrentThread(evt);
  if (NS_FAILED(rv)) {
    NS_WARNING("failed to dispatch DestroyViewerEvent");
  }
  else {
    
    
    
    DropPresentationState();
  }
  
  
}

nsDocShellEditorData*
nsSHEntry::ForgetEditorData()
{
  return mEditorData.forget();
}

void
nsSHEntry::SetEditorData(nsDocShellEditorData* aData)
{
  NS_ASSERTION(!(aData && mEditorData),
               "We're going to overwrite an owning ref!");
  if (mEditorData != aData)
    mEditorData = aData;
}

PRBool
nsSHEntry::HasDetachedEditor()
{
  return mEditorData != nsnull;
}

