






































#ifndef nsSHEntry_h
#define nsSHEntry_h


#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsString.h"
#include "nsAutoPtr.h"


#include "nsIContentViewer.h"
#include "nsIInputStream.h"
#include "nsILayoutHistoryState.h"
#include "nsISHEntry.h"
#include "nsISHContainer.h"
#include "nsIURI.h"
#include "nsIEnumerator.h"
#include "nsIHistoryEntry.h"
#include "nsRect.h"
#include "nsISupportsArray.h"
#include "nsIMutationObserver.h"
#include "nsExpirationTracker.h"
#include "nsDocShellEditorData.h"

class nsSHEntry : public nsISHEntry,
                  public nsISHContainer,
                  public nsIMutationObserver,
                  public nsISHEntryInternal
{
public: 
  nsSHEntry();
  nsSHEntry(const nsSHEntry &other);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIHISTORYENTRY
  NS_DECL_NSISHENTRY
  NS_DECL_NSISHENTRYINTERNAL
  NS_DECL_NSISHCONTAINER
  NS_DECL_NSIMUTATIONOBSERVER

  void DropPresentationState();

  void Expire();
  
  nsExpirationState *GetExpirationState() { return &mExpirationState; }
  
  static nsresult Startup();
  static void Shutdown();
  
private:
  ~nsSHEntry();

  nsCOMPtr<nsIURI>                mURI;
  nsCOMPtr<nsIURI>                mReferrerURI;
  nsCOMPtr<nsIContentViewer>      mContentViewer;
  nsCOMPtr<nsIDocument>           mDocument; 
  nsString                        mTitle;
  nsCOMPtr<nsIInputStream>        mPostData;
  nsCOMPtr<nsILayoutHistoryState> mLayoutHistoryState;
  nsCOMArray<nsISHEntry>          mChildren;
  PRUint32                        mLoadType;
  PRUint32                        mID;
  PRInt64                         mDocIdentifier;
  PRInt32                         mScrollPositionX;
  PRInt32                         mScrollPositionY;
  PRPackedBool                    mIsFrameNavigation;
  PRPackedBool                    mSaveLayoutState;
  PRPackedBool                    mExpired;
  PRPackedBool                    mSticky;
  PRPackedBool                    mDynamicallyCreated;
  nsCString                       mContentType;
  nsCOMPtr<nsISupports>           mCacheKey;
  nsISHEntry *                    mParent;  
  nsCOMPtr<nsISupports>           mWindowState;
  nsIntRect                       mViewerBounds;
  nsCOMArray<nsIDocShellTreeItem> mChildShells;
  nsCOMPtr<nsISupportsArray>      mRefreshURIList;
  nsCOMPtr<nsISupports>           mOwner;
  nsExpirationState               mExpirationState;
  nsAutoPtr<nsDocShellEditorData> mEditorData;
  nsString                        mStateData;
  PRUint64                        mDocShellID;
  PRUint32                        mLastTouched;
};

#endif 
