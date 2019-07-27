





#ifndef mozilla_dom_AsyncHelper_h
#define mozilla_dom_AsyncHelper_h

#include "nsCOMPtr.h"
#include "nsIRequest.h"
#include "nsIRunnable.h"

class nsIRequestObserver;

namespace mozilla {
namespace dom {






class AsyncHelper : public nsIRunnable,
                    public nsIRequest
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_NSIREQUEST

  nsresult
  AsyncWork(nsIRequestObserver* aObserver, nsISupports* aCtxt);

protected:
  explicit AsyncHelper(nsISupports* aStream)
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

} 
} 

#endif 
