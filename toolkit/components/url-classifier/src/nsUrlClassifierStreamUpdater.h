





































#ifndef nsUrlClassifierStreamUpdater_h_
#define nsUrlClassifierStreamUpdater_h_

#include <nsISupportsUtils.h>

#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsIUrlClassifierStreamUpdater.h"
#include "nsIStreamListener.h"
#include "nsNetUtil.h"


class nsIURI;

class nsUrlClassifierStreamUpdater : public nsIUrlClassifierStreamUpdater,
                                     public nsIObserver
{
public:
  nsUrlClassifierStreamUpdater();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIURLCLASSIFIERSTREAMUPDATER
  NS_DECL_NSIOBSERVER

  
  
  void DownloadDone();

private:
  
  ~nsUrlClassifierStreamUpdater() {}

  
  nsUrlClassifierStreamUpdater(nsUrlClassifierStreamUpdater&);

  PRBool mIsUpdating;
  PRBool mInitialized;
  nsCOMPtr<nsIURI> mUpdateUrl;
  nsCOMPtr<nsIStreamListener> mListener;
  nsCOMPtr<nsIChannel> mChannel;
};

#endif 
