





#ifndef DNSListenerProxy_h__
#define DNSListenerProxy_h__

#include "nsIDNSListener.h"
#include "nsIDNSRecord.h"
#include "nsProxyRelease.h"
#include "nsThreadUtils.h"

class nsIEventTarget;
class nsICancelable;

namespace mozilla {
namespace net {

class DNSListenerProxy MOZ_FINAL
    : public nsIDNSListener
    , public nsIDNSListenerProxy
{
public:
  DNSListenerProxy(nsIDNSListener* aListener, nsIEventTarget* aTargetThread)
    
    
    
    
    : mListener(new nsMainThreadPtrHolder<nsIDNSListener>(aListener, false))
    , mTargetThread(aTargetThread)
  { }

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIDNSLISTENER
  NS_DECL_NSIDNSLISTENERPROXY

  class OnLookupCompleteRunnable : public nsRunnable
  {
  public:
    OnLookupCompleteRunnable(const nsMainThreadPtrHandle<nsIDNSListener>& aListener,
                             nsICancelable* aRequest,
                             nsIDNSRecord* aRecord,
                             nsresult aStatus)
      : mListener(aListener)
      , mRequest(aRequest)
      , mRecord(aRecord)
      , mStatus(aStatus)
    { }

    NS_DECL_NSIRUNNABLE

  private:
    nsMainThreadPtrHandle<nsIDNSListener> mListener;
    nsCOMPtr<nsICancelable> mRequest;
    nsCOMPtr<nsIDNSRecord> mRecord;
    nsresult mStatus;
  };

private:
  ~DNSListenerProxy() {}

  nsMainThreadPtrHandle<nsIDNSListener> mListener;
  nsCOMPtr<nsIEventTarget> mTargetThread;
};


} 
} 
#endif 
