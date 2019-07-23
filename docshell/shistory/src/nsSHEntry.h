






































#ifndef nsSHEntry_h
#define nsSHEntry_h


#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsString.h"
#include "nsVoidArray.h"


#include "nsIContentViewer.h"
#include "nsIInputStream.h"
#include "nsILayoutHistoryState.h"
#include "nsISHEntry.h"
#include "nsISHContainer.h"
#include "nsIURI.h"
#include "nsIEnumerator.h"
#include "nsIHistoryEntry.h"
#include "nsRect.h"
#include "nsSupportsArray.h"
#include "nsIMutationObserver.h"

class nsSHEntry : public nsISHEntry,
                  public nsISHContainer,
                  public nsIMutationObserver
{
public: 
  nsSHEntry();
  nsSHEntry(const nsSHEntry &other);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIHISTORYENTRY
  NS_DECL_NSISHENTRY
  NS_DECL_NSISHCONTAINER
  NS_DECL_NSIMUTATIONOBSERVER

  void DropPresentationState();

private:
  ~nsSHEntry();
  void DocumentMutated();

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
  PRUint32                        mPageIdentifier;
  PRInt32                         mScrollPositionX;
  PRInt32                         mScrollPositionY;
  PRPackedBool                    mIsFrameNavigation;
  PRPackedBool                    mSaveLayoutState;
  PRPackedBool                    mExpired;
  PRPackedBool                    mSticky;
  nsCString                       mContentType;
  nsCOMPtr<nsISupports>           mCacheKey;
  nsISHEntry *                    mParent;  
  nsCOMPtr<nsISupports>           mWindowState;
  nsRect                          mViewerBounds;
  nsCOMArray<nsIDocShellTreeItem> mChildShells;
  nsCOMPtr<nsISupportsArray>      mRefreshURIList;
  nsCOMPtr<nsISupports>           mOwner;
};

#endif 
