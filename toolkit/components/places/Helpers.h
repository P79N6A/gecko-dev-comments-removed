




#ifndef mozilla_places_Helpers_h_
#define mozilla_places_Helpers_h_





#include "mozilla/storage.h"
#include "nsIURI.h"
#include "nsThreadUtils.h"
#include "nsProxyRelease.h"
#include "prtime.h"
#include "mozilla/Telemetry.h"

namespace mozilla {
namespace places {




class AsyncStatementCallback : public mozIStorageStatementCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGESTATEMENTCALLBACK
  AsyncStatementCallback() {}

protected:
  virtual ~AsyncStatementCallback() {}
};





#define NS_DECL_ASYNCSTATEMENTCALLBACK \
  NS_IMETHOD HandleResult(mozIStorageResultSet *) override; \
  NS_IMETHOD HandleCompletion(uint16_t) override;






class URIBinder 
{
public:
  
  static nsresult Bind(mozIStorageStatement* statement,
                       int32_t index,
                       nsIURI* aURI);
  
  static nsresult Bind(mozIStorageStatement* statement,
                       int32_t index,
                       const nsACString& aURLString);
  
  static nsresult Bind(mozIStorageStatement* statement,
                       const nsACString& aName,
                       nsIURI* aURI);
  
  static nsresult Bind(mozIStorageStatement* statement,
                       const nsACString& aName,
                       const nsACString& aURLString);
  
  static nsresult Bind(mozIStorageBindingParams* aParams,
                       int32_t index,
                       nsIURI* aURI);
  
  static nsresult Bind(mozIStorageBindingParams* aParams,
                       int32_t index,
                       const nsACString& aURLString);
  
  static nsresult Bind(mozIStorageBindingParams* aParams,
                       const nsACString& aName,
                       nsIURI* aURI);
  
  static nsresult Bind(mozIStorageBindingParams* aParams,
                       const nsACString& aName,
                       const nsACString& aURLString);
};























nsresult GetReversedHostname(nsIURI* aURI, nsString& aRevHost);




void GetReversedHostname(const nsString& aForward, nsString& aRevHost);









void ReverseString(const nsString& aInput, nsString& aReversed);






nsresult GenerateGUID(nsCString& _guid);








bool IsValidGUID(const nsACString& aGUID);









void TruncateTitle(const nsACString& aTitle, nsACString& aTrimmed);








PRTime RoundToMilliseconds(PRTime aTime);






PRTime RoundedPRNow();




template<typename StatementType>
class FinalizeStatementCacheProxy : public nsRunnable
{
public:
  









  FinalizeStatementCacheProxy(
    mozilla::storage::StatementCache<StatementType>& aStatementCache,
    nsISupports* aOwner
  )
  : mStatementCache(aStatementCache)
  , mOwner(aOwner)
  , mCallingThread(do_GetCurrentThread())
  {
  }

  NS_IMETHOD Run()
  {
    mStatementCache.FinalizeStatements();
    
    (void)NS_ProxyRelease(mCallingThread, mOwner);
    return NS_OK;
  }

protected:
  mozilla::storage::StatementCache<StatementType>& mStatementCache;
  nsCOMPtr<nsISupports> mOwner;
  nsCOMPtr<nsIThread> mCallingThread;
};







void ForceWALCheckpoint();











bool GetHiddenState(bool aIsRedirect,
                    uint32_t aTransitionType);




class PlacesEvent : public nsRunnable
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIRUNNABLE

  explicit PlacesEvent(const char* aTopic);
protected:
  ~PlacesEvent() {}
  void Notify();

  const char* const mTopic;
};




class AsyncStatementCallbackNotifier : public AsyncStatementCallback
{
public:
  explicit AsyncStatementCallbackNotifier(const char* aTopic)
    : mTopic(aTopic)
  {
  }

  NS_IMETHOD HandleCompletion(uint16_t aReason);

private:
  const char* mTopic;
};




class AsyncStatementTelemetryTimer : public AsyncStatementCallback
{
public:
  explicit AsyncStatementTelemetryTimer(Telemetry::ID aHistogramId,
                                        TimeStamp aStart = TimeStamp::Now())
    : mHistogramId(aHistogramId)
    , mStart(aStart)
  {
  }

  NS_IMETHOD HandleCompletion(uint16_t aReason);

private:
  const Telemetry::ID mHistogramId;
  const TimeStamp mStart;
};

} 
} 

#endif 
