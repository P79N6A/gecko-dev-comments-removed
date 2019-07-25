





#ifndef mozilla_net_NullHttpTransaction_h
#define mozilla_net_NullHttpTransaction_h

#include "nsAHttpTransaction.h"
#include "nsAHttpConnection.h"
#include "nsIInterfaceRequestor.h"
#include "nsIEventTarget.h"
#include "nsHttpConnectionInfo.h"
#include "nsHttpRequestHead.h"
#include "mozilla/Attributes.h"






namespace mozilla { namespace net {

class NullHttpTransaction MOZ_FINAL : public nsAHttpTransaction
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSAHTTPTRANSACTION

  NullHttpTransaction(nsHttpConnectionInfo *ci,
                      nsIInterfaceRequestor *callbacks,
                      nsIEventTarget *target,
                      uint8_t caps);
  ~NullHttpTransaction();

  nsHttpConnectionInfo *ConnectionInfo() { return mConnectionInfo; }

  
  bool IsNullTransaction() { return true; }

private:

  nsresult mStatus;
  uint8_t  mCaps;
  nsRefPtr<nsAHttpConnection> mConnection;
  nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
  nsCOMPtr<nsIEventTarget> mEventTarget;
  nsRefPtr<nsHttpConnectionInfo> mConnectionInfo;
  nsHttpRequestHead *mRequestHead;
  bool mIsDone;
};

}} 

#endif 
