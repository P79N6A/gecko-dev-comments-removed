





































#ifndef mozilla_places_Helpers_h_
#define mozilla_places_Helpers_h_





#include "mozilla/storage.h"
#include "nsIURI.h"
#include "nsThreadUtils.h"
#include "nsProxyRelease.h"

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
  NS_IMETHOD HandleResult(mozIStorageResultSet *); \
  NS_IMETHOD HandleCompletion(PRUint16);






class URIBinder 
{
public:
  
  static nsresult Bind(mozIStorageStatement* statement,
                       PRInt32 index,
                       nsIURI* aURI);
  
  static nsresult Bind(mozIStorageStatement* statement,
                       PRInt32 index,
                       const nsACString& aURLString);
  
  static nsresult Bind(mozIStorageStatement* statement,
                       const nsACString& aName,
                       nsIURI* aURI);
  
  static nsresult Bind(mozIStorageStatement* statement,
                       const nsACString& aName,
                       const nsACString& aURLString);
  
  static nsresult Bind(mozIStorageBindingParams* aParams,
                       PRInt32 index,
                       nsIURI* aURI);
  
  static nsresult Bind(mozIStorageBindingParams* aParams,
                       PRInt32 index,
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








bool IsValidGUID(const nsCString& aGUID);









void TruncateTitle(const nsACString& aTitle, nsACString& aTrimmed);




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
                    PRUint32 aTransitionType);




class PlacesEvent : public nsRunnable
                  , public mozIStorageCompletionCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  NS_DECL_MOZISTORAGECOMPLETIONCALLBACK

  PlacesEvent(const char* aTopic);
protected:
  void Notify();

  const char* const mTopic;
};

} 
} 

#endif 
