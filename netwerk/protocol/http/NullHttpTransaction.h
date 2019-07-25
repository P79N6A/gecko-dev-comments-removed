






































#ifndef mozilla_net_NullHttpTransaction_h
#define mozilla_net_NullHttpTransaction_h

#include "nsAHttpTransaction.h"
#include "nsAHttpConnection.h"
#include "nsIInterfaceRequestor.h"
#include "nsIEventTarget.h"
#include "nsHttpConnectionInfo.h"
#include "nsHttpRequestHead.h"






namespace mozilla { namespace net {

class NullHttpTransaction : public nsAHttpTransaction
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSAHTTPTRANSACTION

  NullHttpTransaction(nsHttpConnectionInfo *ci,
                      nsIInterfaceRequestor *callbacks,
                      nsIEventTarget *target,
                      PRUint8 caps);
  ~NullHttpTransaction();

  PRUint8 Caps() { return mCaps; }
  nsHttpConnectionInfo *ConnectionInfo() { return mConnectionInfo; }

private:

  nsresult mStatus;
  PRUint8  mCaps;
  nsRefPtr<nsAHttpConnection> mConnection;
  nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
  nsCOMPtr<nsIEventTarget> mEventTarget;
  nsRefPtr<nsHttpConnectionInfo> mConnectionInfo;
  nsHttpRequestHead *mRequestHead;
};

}} 

#endif 
