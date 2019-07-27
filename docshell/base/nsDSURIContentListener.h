





#ifndef nsDSURIContentListener_h__
#define nsDSURIContentListener_h__

#include "nsCOMPtr.h"
#include "nsIURIContentListener.h"
#include "nsWeakReference.h"

class nsDocShell;
class nsIWebNavigationInfo;
class nsIHttpChannel;
class nsAString;

class nsDSURIContentListener MOZ_FINAL
  : public nsIURIContentListener
  , public nsSupportsWeakReference
{
  friend class nsDocShell;

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIURICONTENTLISTENER

  nsresult Init();

protected:
  explicit nsDSURIContentListener(nsDocShell* aDocShell);
  virtual ~nsDSURIContentListener();

  void DropDocShellReference()
  {
    mDocShell = nullptr;
    mExistingJPEGRequest = nullptr;
    mExistingJPEGStreamListener = nullptr;
  }

  
  
  bool CheckFrameOptions(nsIRequest* aRequest);
  bool CheckOneFrameOptionsPolicy(nsIHttpChannel* aHttpChannel,
                                  const nsAString& aPolicy);

  enum XFOHeader
  {
    eDENY,
    eSAMEORIGIN,
    eALLOWFROM
  };

  void ReportXFOViolation(nsIDocShellTreeItem* aTopDocShellItem,
                          nsIURI* aThisURI,
                          XFOHeader aHeader);

protected:
  nsDocShell* mDocShell;
  
  nsCOMPtr<nsIStreamListener> mExistingJPEGStreamListener;
  nsCOMPtr<nsIChannel> mExistingJPEGRequest;

  
  
  
  nsWeakPtr mWeakParentContentListener;
  nsIURIContentListener* mParentContentListener;

  nsCOMPtr<nsIWebNavigationInfo> mNavInfo;
};

#endif 
