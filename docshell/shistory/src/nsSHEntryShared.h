




































#ifndef nsSHEntryShared_h__
#define nsSHEntryShared_h__

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsIBFCacheEntry.h"
#include "nsIMutationObserver.h"
#include "nsExpirationTracker.h"
#include "nsRect.h"

class nsSHEntry;
class nsISHEntry;
class nsIDocument;
class nsIContentViewer;
class nsIDocShellTreeItem;
class nsILayoutHistoryState;
class nsISupportsArray;
class nsDocShellEditorData;







class nsSHEntryShared : public nsIBFCacheEntry,
                        public nsIMutationObserver
{
  public:
    static void Startup();
    static void Shutdown();

    nsSHEntryShared();
    ~nsSHEntryShared();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIMUTATIONOBSERVER
    NS_DECL_NSIBFCACHEENTRY

  private:
    friend class nsSHEntry;

    friend class HistoryTracker;
    friend class nsExpirationTracker<nsSHEntryShared, 3>;
    nsExpirationState *GetExpirationState() { return &mExpirationState; }

    static already_AddRefed<nsSHEntryShared> Duplicate(nsSHEntryShared *aEntry);

    void RemoveFromExpirationTracker();
    void Expire();
    nsresult SyncPresentationState();
    void DropPresentationState();

    nsresult SetContentViewer(nsIContentViewer *aViewer);

    

    
    
    PRUint64                        mDocShellID;
    nsCOMArray<nsIDocShellTreeItem> mChildShells;
    nsCOMPtr<nsISupports>           mOwner;
    nsISHEntry*                     mParent;
    nsCString                       mContentType;
    bool                            mIsFrameNavigation;
    bool                            mSaveLayoutState;
    bool                            mSticky;
    bool                            mDynamicallyCreated;
    nsCOMPtr<nsISupports>           mCacheKey;
    PRUint32                        mLastTouched;

    
    
    PRUint64                        mID;
    nsCOMPtr<nsIContentViewer>      mContentViewer;
    nsCOMPtr<nsIDocument>           mDocument;
    nsCOMPtr<nsILayoutHistoryState> mLayoutHistoryState;
    bool                            mExpired;
    nsCOMPtr<nsISupports>           mWindowState;
    nsIntRect                       mViewerBounds;
    nsCOMPtr<nsISupportsArray>      mRefreshURIList;
    nsExpirationState               mExpirationState;
    nsAutoPtr<nsDocShellEditorData> mEditorData;
};

#endif
