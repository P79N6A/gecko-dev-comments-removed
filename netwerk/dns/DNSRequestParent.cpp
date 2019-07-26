





#include "mozilla/net/DNSRequestParent.h"
#include "nsIDNSService.h"
#include "nsNetCID.h"
#include "nsThreadUtils.h"
#include "nsIServiceManager.h"
#include "nsICancelable.h"
#include "nsIDNSRecord.h"
#include "nsHostResolver.h"
#include "mozilla/unused.h"

using namespace mozilla::ipc;

namespace mozilla {
namespace net {

DNSRequestParent::DNSRequestParent()
  : mIPCClosed(false)
{

}

DNSRequestParent::~DNSRequestParent()
{

}

void
DNSRequestParent::DoAsyncResolve(const nsACString &hostname, uint32_t flags)
{
  nsresult rv;
  mFlags = flags;
  nsCOMPtr<nsIDNSService> dns = do_GetService(NS_DNSSERVICE_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    nsCOMPtr<nsICancelable> unused;
    rv = dns->AsyncResolve(hostname, flags, this, mainThread,
                           getter_AddRefs(unused));
  }

  if (NS_FAILED(rv) && !mIPCClosed) {
    unused << Send__delete__(this, DNSRequestResponse(rv));
  }
}

void
DNSRequestParent::ActorDestroy(ActorDestroyReason why)
{
  
  
  
  mIPCClosed = true;
}




NS_IMPL_ISUPPORTS1(DNSRequestParent,
                   nsIDNSListener)





NS_IMETHODIMP
DNSRequestParent::OnLookupComplete(nsICancelable *request,
                                   nsIDNSRecord  *rec,
                                   nsresult       status)
{
  if (mIPCClosed) {
    
    return NS_OK;
  }

  if (NS_SUCCEEDED(status)) {
    MOZ_ASSERT(rec);

    nsAutoCString cname;
    if (mFlags & nsHostResolver::RES_CANON_NAME) {
      rec->GetCanonicalName(cname);
    }

    
    NetAddrArray array;
    NetAddr addr;
    while (NS_SUCCEEDED(rec->GetNextAddr(80, &addr))) {
      array.AppendElement(addr);
    }

    unused << Send__delete__(this, DNSRequestResponse(DNSRecord(cname, array)));
  } else {
    unused << Send__delete__(this, DNSRequestResponse(status));
  }

  return NS_OK;
}



}} 
