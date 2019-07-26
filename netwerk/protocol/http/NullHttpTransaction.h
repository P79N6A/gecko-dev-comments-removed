





#ifndef mozilla_net_NullHttpTransaction_h
#define mozilla_net_NullHttpTransaction_h

#include "nsAHttpTransaction.h"
#include "mozilla/Attributes.h"






namespace mozilla { namespace net {

class nsAHttpConnection;
class nsHttpConnectionInfo;
class nsHttpRequestHead;

class NullHttpTransaction MOZ_FINAL : public nsAHttpTransaction
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSAHTTPTRANSACTION

  NullHttpTransaction(nsHttpConnectionInfo *ci,
                      nsIInterfaceRequestor *callbacks,
                      uint32_t caps);
  ~NullHttpTransaction();

  nsHttpConnectionInfo *ConnectionInfo() { return mConnectionInfo; }

  
  bool IsNullTransaction() MOZ_OVERRIDE MOZ_FINAL { return true; }
  bool ResponseTimeoutEnabled() const MOZ_OVERRIDE MOZ_FINAL {return true; }
  PRIntervalTime ResponseTimeout() MOZ_OVERRIDE MOZ_FINAL
  {
    return PR_SecondsToInterval(15);
  }

private:

  nsresult mStatus;
  uint32_t mCaps;
  
  
  
  
  
  uint32_t mCapsToClear;
  nsRefPtr<nsAHttpConnection> mConnection;
  nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
  nsRefPtr<nsHttpConnectionInfo> mConnectionInfo;
  nsHttpRequestHead *mRequestHead;
  bool mIsDone;
};

}} 

#endif 
