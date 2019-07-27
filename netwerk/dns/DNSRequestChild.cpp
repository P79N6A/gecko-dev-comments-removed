





#include "mozilla/net/ChildDNSService.h"
#include "mozilla/net/DNSRequestChild.h"
#include "mozilla/net/NeckoChild.h"
#include "mozilla/unused.h"
#include "nsIDNSRecord.h"
#include "nsHostResolver.h"
#include "nsTArray.h"
#include "nsNetAddr.h"
#include "nsIThread.h"
#include "nsThreadUtils.h"

using namespace mozilla::ipc;

namespace mozilla {
namespace net {






class ChildDNSRecord : public nsIDNSRecord
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIDNSRECORD

  ChildDNSRecord(const DNSRecord& reply, uint16_t flags);

private:
  virtual ~ChildDNSRecord();

  nsCString mCanonicalName;
  nsTArray<NetAddr> mAddresses;
  uint32_t mCurrent; 
  uint32_t mLength;  
  uint16_t mFlags;
};

NS_IMPL_ISUPPORTS(ChildDNSRecord, nsIDNSRecord)

ChildDNSRecord::ChildDNSRecord(const DNSRecord& reply, uint16_t flags)
  : mCurrent(0)
  , mFlags(flags)
{
  mCanonicalName = reply.canonicalName();

  
  const nsTArray<NetAddr>& addrs = reply.addrs();
  uint32_t i = 0;
  mLength = addrs.Length();
  for (; i < mLength; i++) {
    mAddresses.AppendElement(addrs[i]);
  }
}

ChildDNSRecord::~ChildDNSRecord()
{
}





NS_IMETHODIMP
ChildDNSRecord::GetCanonicalName(nsACString &result)
{
  if (!(mFlags & nsHostResolver::RES_CANON_NAME)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  result = mCanonicalName;
  return NS_OK;
}

NS_IMETHODIMP
ChildDNSRecord::GetNextAddr(uint16_t port, NetAddr *addr)
{
  if (mCurrent >= mLength) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  memcpy(addr, &mAddresses[mCurrent++], sizeof(NetAddr));

  
  addr->inet.port = htons(port);

  return NS_OK;
}


NS_IMETHODIMP
ChildDNSRecord::GetScriptableNextAddr(uint16_t port, nsINetAddr **result)
{
  NetAddr addr;
  nsresult rv = GetNextAddr(port, &addr);
  if (NS_FAILED(rv)) return rv;

  NS_ADDREF(*result = new nsNetAddr(&addr));

  return NS_OK;
}


NS_IMETHODIMP
ChildDNSRecord::GetNextAddrAsString(nsACString &result)
{
  NetAddr addr;
  nsresult rv = GetNextAddr(0, &addr);
  if (NS_FAILED(rv)) {
    return rv;
  }

  char buf[kIPv6CStrBufSize];
  if (NetAddrToString(&addr, buf, sizeof(buf))) {
    result.Assign(buf);
    return NS_OK;
  }
  NS_ERROR("NetAddrToString failed unexpectedly");
  return NS_ERROR_FAILURE; 
}

NS_IMETHODIMP
ChildDNSRecord::HasMore(bool *result)
{
  *result = mCurrent < mLength;
  return NS_OK;
}

NS_IMETHODIMP
ChildDNSRecord::Rewind()
{
  mCurrent = 0;
  return NS_OK;
}

NS_IMETHODIMP
ChildDNSRecord::ReportUnusable(uint16_t aPort)
{
  
  
  return NS_OK;
}





class CancelDNSRequestEvent : public nsRunnable
{
public:
  CancelDNSRequestEvent(DNSRequestChild* aDnsReq, nsresult aReason)
    : mDnsRequest(aDnsReq)
    , mReasonForCancel(aReason)
  {}

  NS_IMETHOD Run()
  {
    if (mDnsRequest->mIPCOpen) {
      
      mDnsRequest->SendCancelDNSRequest(mDnsRequest->mHost, mDnsRequest->mFlags,
                                      mReasonForCancel);
    }
    return NS_OK;
  }
private:
  nsRefPtr<DNSRequestChild> mDnsRequest;
  nsresult mReasonForCancel;
};





DNSRequestChild::DNSRequestChild(const nsCString& aHost,
                                 const uint32_t& aFlags,
                                 nsIDNSListener *aListener,
                                 nsIEventTarget *target)
  : mListener(aListener)
  , mTarget(target)
  , mResultStatus(NS_OK)
  , mHost(aHost)
  , mFlags(aFlags)
  , mIPCOpen(false)
{
}

void
DNSRequestChild::StartRequest()
{
  
  if (!NS_IsMainThread()) {
    NS_DispatchToMainThread(
      NS_NewRunnableMethod(this, &DNSRequestChild::StartRequest));
    return;
  }

  
  gNeckoChild->SendPDNSRequestConstructor(this, mHost, mFlags);
  mIPCOpen = true;

  
  AddIPDLReference();
}

void
DNSRequestChild::CallOnLookupComplete()
{
  MOZ_ASSERT(mListener);
  mListener->OnLookupComplete(this, mResultRecord, mResultStatus);
}

bool
DNSRequestChild::RecvLookupCompleted(const DNSRequestResponse& reply)
{
  mIPCOpen = false;
  MOZ_ASSERT(mListener);

  switch (reply.type()) {
  case DNSRequestResponse::TDNSRecord: {
    mResultRecord = new ChildDNSRecord(reply.get_DNSRecord(), mFlags);
    break;
  }
  case DNSRequestResponse::Tnsresult: {
    mResultStatus = reply.get_nsresult();
    break;
  }
  default:
    NS_NOTREACHED("unknown type");
    return false;
  }

  MOZ_ASSERT(NS_IsMainThread());

  bool targetIsMain = false;
  if (!mTarget) {
    targetIsMain = true;
  } else {
    mTarget->IsOnCurrentThread(&targetIsMain);
  }

  if (targetIsMain) {
    CallOnLookupComplete();
  } else {
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &DNSRequestChild::CallOnLookupComplete);
    mTarget->Dispatch(event, NS_DISPATCH_NORMAL);
  }

  unused << Send__delete__(this);

  return true;
}

void
DNSRequestChild::ReleaseIPDLReference()
{
  
  nsRefPtr<ChildDNSService> dnsServiceChild =
    dont_AddRef(ChildDNSService::GetSingleton());
  dnsServiceChild->NotifyRequestDone(this);

  Release();
}

void
DNSRequestChild::ActorDestroy(ActorDestroyReason why)
{
  mIPCOpen = false;
}





NS_IMPL_ISUPPORTS(DNSRequestChild,
                  nsICancelable)





NS_IMETHODIMP
DNSRequestChild::Cancel(nsresult reason)
{
  if(mIPCOpen) {
    
    NS_DispatchToMainThread(
      new CancelDNSRequestEvent(this, reason));
  }
  return NS_OK;
}


}} 
