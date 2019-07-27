




#ifndef mozilla_dom_FetchDriver_h
#define mozilla_dom_FetchDriver_h

#include "nsIStreamListener.h"
#include "nsRefPtr.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class InternalRequest;
class InternalResponse;

class FetchDriverObserver
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(FetchDriverObserver);
  virtual void OnResponseAvailable(InternalResponse* aResponse) = 0;

protected:
  virtual ~FetchDriverObserver()
  { };
};

class FetchDriver MOZ_FINAL
{
  NS_INLINE_DECL_REFCOUNTING(FetchDriver)
public:
  explicit FetchDriver(InternalRequest* aRequest);
  NS_IMETHOD Fetch(FetchDriverObserver* aObserver);

private:
  nsRefPtr<InternalRequest> mRequest;
  nsRefPtr<FetchDriverObserver> mObserver;
  uint32_t mFetchRecursionCount;

  FetchDriver() = delete;
  FetchDriver(const FetchDriver&) = delete;
  FetchDriver& operator=(const FetchDriver&) = delete;
  ~FetchDriver();

  nsresult Fetch(bool aCORSFlag);
  nsresult ContinueFetch(bool aCORSFlag);
  nsresult BasicFetch();
  nsresult FailWithNetworkError();
  nsresult BeginResponse(InternalResponse* aResponse);
  nsresult SucceedWithResponse();
};

} 
} 

#endif 
