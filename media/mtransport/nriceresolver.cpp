










































#include "nspr.h"
#include "prnetdb.h"

#include "mozilla/Assertions.h"

extern "C" {
#include "nr_api.h"
#include "async_timer.h"
#include "nr_resolver.h"
#include "transport_addr.h"
}

#include "mozilla/net/DNS.h" 
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIDNSService.h"
#include "nsIDNSListener.h"
#include "nsIDNSRecord.h"
#include "nsNetCID.h"
#include "nsCOMPtr.h"
#include "nriceresolver.h"
#include "nr_socket_prsock.h"
#include "mtransport/runnable_utils.h"


#include "logging.h"

namespace mozilla {

MOZ_MTLOG_MODULE("mtransport")

NrIceResolver::NrIceResolver() :
    vtbl_(new nr_resolver_vtbl())
#ifdef DEBUG
    , allocated_resolvers_(0)
#endif
{
  vtbl_->destroy = &NrIceResolver::destroy;
  vtbl_->resolve = &NrIceResolver::resolve;
  vtbl_->cancel = &NrIceResolver::cancel;
}

NrIceResolver::~NrIceResolver() {
  MOZ_ASSERT(!allocated_resolvers_);
  delete vtbl_;
}

nsresult NrIceResolver::Init() {
  nsresult rv;

  sts_thread_ = do_GetService(NS_SOCKETTRANSPORTSERVICE_CONTRACTID, &rv);
  MOZ_ASSERT(NS_SUCCEEDED(rv));
  dns_ = do_GetService(NS_DNSSERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    MOZ_MTLOG(PR_LOG_ERROR, "Could not acquire DNS service");
  }
  return rv;
}

nr_resolver *NrIceResolver::AllocateResolver() {
  nr_resolver *resolver;

  int r = nr_resolver_create_int((void *)this, vtbl_, &resolver);
  MOZ_ASSERT(!r);
  if(r) {
    MOZ_MTLOG(PR_LOG_ERROR, "nr_resolver_create_int failed");
    return nullptr;
  }
  
  
  AddRef();
#ifdef DEBUG
  ++allocated_resolvers_;
#endif
  return resolver;
}

void NrIceResolver::DestroyResolver() {
#ifdef DEBUG
  --allocated_resolvers_;
#endif
  
  Release();
}

int NrIceResolver::destroy(void **objp) {
  if (!objp || !*objp)
    return 0;
  NrIceResolver *resolver = static_cast<NrIceResolver *>(*objp);
  *objp = 0;
  resolver->DestroyResolver();
  return 0;
}

int NrIceResolver::resolve(void *obj,
                           nr_resolver_resource *resource,
                           int (*cb)(void *cb_arg, nr_transport_addr *addr),
                           void *cb_arg,
                           void **handle) {
  MOZ_ASSERT(obj);
  return static_cast<NrIceResolver *>(obj)->resolve(resource, cb, cb_arg, handle);
}

int NrIceResolver::resolve(nr_resolver_resource *resource,
                           int (*cb)(void *cb_arg, nr_transport_addr *addr),
                           void *cb_arg,
                           void **handle) {
  int _status;
  MOZ_ASSERT(allocated_resolvers_ > 0);
  ASSERT_ON_THREAD(sts_thread_);
  nsCOMPtr<PendingResolution> pr;

  if (resource->transport_protocol != IPPROTO_UDP) {
    MOZ_MTLOG(PR_LOG_ERROR, "Only UDP is supported.");
    ABORT(R_NOT_FOUND);
  }
  pr = new PendingResolution(sts_thread_, resource->port? resource->port : 3478,
                             cb, cb_arg);
  if (NS_FAILED(dns_->AsyncResolve(nsAutoCString(resource->domain_name),
                                   nsIDNSService::RESOLVE_DISABLE_IPV6, pr,
                                   sts_thread_, getter_AddRefs(pr->request_)))) {
    MOZ_MTLOG(PR_LOG_ERROR, "AsyncResolve failed.");
    ABORT(R_NOT_FOUND);
  }
  
  
  
  
  
  
  *handle = pr.forget().get();

  _status=0;
abort:
  return _status;
}

nsresult NrIceResolver::PendingResolution::OnLookupComplete(
    nsICancelable *request, nsIDNSRecord *record, nsresult status) {
  ASSERT_ON_THREAD(thread_);
  
  
  if (!canceled_) {
    nr_transport_addr *cb_addr = nullptr;
    nr_transport_addr ta;
    
    if (NS_SUCCEEDED(status)) {
      net::NetAddr na;
      if (NS_SUCCEEDED(record->GetNextAddr(port_, &na))) {
        MOZ_ALWAYS_TRUE (nr_netaddr_to_transport_addr(&na, &ta) == 0);
        cb_addr = &ta;
      }
    }
    cb_(cb_arg_, cb_addr);
    Release();
  }
  return NS_OK;
}

int NrIceResolver::cancel(void *obj, void *handle) {
  MOZ_ALWAYS_TRUE(obj);
  MOZ_ASSERT(handle);
  ASSERT_ON_THREAD(static_cast<NrIceResolver *>(obj)->sts_thread_);
  return static_cast<PendingResolution *>(handle)->cancel();
}

int NrIceResolver::PendingResolution::cancel() {
  request_->Cancel (NS_ERROR_ABORT);
  canceled_ = true; 
  Release();
  return 0;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(NrIceResolver::PendingResolution, nsIDNSListener);
}  
