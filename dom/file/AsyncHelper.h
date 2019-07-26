





#ifndef mozilla_dom_file_asynchelper_h__
#define mozilla_dom_file_asynchelper_h__

#include "FileCommon.h"

#include "nsIRequest.h"
#include "nsIRunnable.h"

class nsIRequestObserver;

BEGIN_FILE_NAMESPACE






class AsyncHelper : public nsIRunnable,
                    public nsIRequest
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSIREQUEST

  nsresult
  AsyncWork(nsIRequestObserver* aObserver, nsISupports* aCtxt);

protected:
  AsyncHelper(nsISupports* aStream)
  : mStream(aStream),
    mStatus(NS_OK)
  { }

  virtual ~AsyncHelper()
  { }

  virtual nsresult
  DoStreamWork(nsISupports* aStream) = 0;

private:
  nsCOMPtr<nsISupports> mStream;
  nsCOMPtr<nsIRequestObserver> mObserver;

  nsresult mStatus;
};

END_FILE_NAMESPACE

#endif 
