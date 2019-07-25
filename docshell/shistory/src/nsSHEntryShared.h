



#ifndef nsSHEntryShared_h__
#define nsSHEntryShared_h__

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsIBFCacheEntry.h"
#include "nsIMutationObserver.h"
#include "nsExpirationTracker.h"
#include "nsRect.h"
#include "nsString.h"
#include "mozilla/Attributes.h"

class nsSHEntry;
class nsISHEntry;
class nsIDocument;
class nsIContentViewer;
class nsIDocShellTreeItem;
class nsILayoutHistoryState;
class nsISupportsArray;
class nsDocShellEditorData;







class nsSHEntryShared MOZ_FINAL : public nsIBFCacheEntry,
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

    

    
    
    uint64_t                        mDocShellID;
    nsCOMArray<nsIDocShellTreeItem> mChildShells;
    nsCOMPtr<nsISupports>           mOwner;
    nsCString                       mContentType;
    bool                            mIsFrameNavigation;
    bool                            mSaveLayoutState;
    bool                            mSticky;
    bool                            mDynamicallyCreated;
    nsCOMPtr<nsISupports>           mCacheKey;
    uint32_t                        mLastTouched;

    
    
    uint64_t                        mID;
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
