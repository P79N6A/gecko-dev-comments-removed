






































#ifndef nsSHEntry_h
#define nsSHEntry_h


#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsString.h"


#include "nsIInputStream.h"
#include "nsISHEntry.h"
#include "nsISHContainer.h"
#include "nsIURI.h"
#include "nsIHistoryEntry.h"

class nsSHEntryShared;

class nsSHEntry : public nsISHEntry,
                  public nsISHContainer,
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

  void DropPresentationState();

  static nsresult Startup();
  static void Shutdown();
  
private:
  ~nsSHEntry();

  
  
  nsRefPtr<nsSHEntryShared> mShared;

  
  nsCOMPtr<nsIURI>         mURI;
  nsCOMPtr<nsIURI>         mReferrerURI;
  nsString                 mTitle;
  nsCOMPtr<nsIInputStream> mPostData;
  PRUint32                 mLoadType;
  PRUint32                 mID;
  PRInt32                  mScrollPositionX;
  PRInt32                  mScrollPositionY;
  nsISHEntry*              mParent;
  nsCOMArray<nsISHEntry>   mChildren;
  bool                     mURIWasModified;
  nsCOMPtr<nsIStructuredCloneContainer> mStateData;
};

#endif 
