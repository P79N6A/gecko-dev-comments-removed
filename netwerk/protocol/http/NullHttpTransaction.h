





#ifndef mozilla_net_NullHttpTransaction_h
#define mozilla_net_NullHttpTransaction_h

#include "nsAHttpTransaction.h"
#include "mozilla/Attributes.h"






class nsIHttpActivityObserver;

namespace mozilla { namespace net {

class nsAHttpConnection;
class nsHttpConnectionInfo;
class nsHttpRequestHead;


#define NS_NULLHTTPTRANSACTION_IID \
{ 0x6c445340, 0x3b82, 0x4345, {0x8e, 0xfa, 0x49, 0x02, 0xc3, 0xb8, 0x80, 0x5a }}

class NullHttpTransaction : public nsAHttpTransaction
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_NULLHTTPTRANSACTION_IID)
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSAHTTPTRANSACTION

  NullHttpTransaction(nsHttpConnectionInfo *ci,
                      nsIInterfaceRequestor *callbacks,
                      uint32_t caps);

  bool Claim();

  
  bool IsNullTransaction() override final { return true; }
  NullHttpTransaction *QueryNullTransaction() override final { return this; }
  bool ResponseTimeoutEnabled() const override final {return true; }
  PRIntervalTime ResponseTimeout() override final
  {
    return PR_SecondsToInterval(15);
  }

protected:
  virtual ~NullHttpTransaction();

private:
  nsresult mStatus;
protected:
  uint32_t mCaps;
  nsHttpRequestHead *mRequestHead;
private:
  
  
  
  
  
  
  
  Atomic<uint32_t> mCapsToClear;
  bool mIsDone;
  bool mClaimed;

protected:
  nsRefPtr<nsAHttpConnection> mConnection;
  nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
  nsRefPtr<nsHttpConnectionInfo> mConnectionInfo;
  nsCOMPtr<nsIHttpActivityObserver> mActivityDistributor;
};

NS_DEFINE_STATIC_IID_ACCESSOR(NullHttpTransaction, NS_NULLHTTPTRANSACTION_IID)

}} 

#endif 
