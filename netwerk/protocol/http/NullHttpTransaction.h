





#ifndef mozilla_net_NullHttpTransaction_h
#define mozilla_net_NullHttpTransaction_h

#include "nsAHttpTransaction.h"
#include "mozilla/Attributes.h"

class nsAHttpConnection;
class nsHttpRequestHead;
class nsHttpConnectionInfo;






namespace mozilla { namespace net {

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

  
  bool IsNullTransaction() { return true; }

private:

  nsresult mStatus;
  uint32_t mCaps;
  nsRefPtr<nsAHttpConnection> mConnection;
  nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
  nsRefPtr<nsHttpConnectionInfo> mConnectionInfo;
  nsHttpRequestHead *mRequestHead;
  bool mIsDone;
};

}} 

#endif 
