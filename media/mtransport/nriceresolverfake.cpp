










































#include "nspr.h"
#include "prnetdb.h"

#include "mozilla/Assertions.h"

extern "C" {
#include "nr_api.h"
#include "async_timer.h"
#include "nr_resolver.h"
#include "transport_addr.h"
}

#include "nriceresolverfake.h"
#include "nr_socket_prsock.h"

namespace mozilla {

NrIceResolverFake::NrIceResolverFake() :
    vtbl_(new nr_resolver_vtbl), addrs_(), delay_ms_(100),
    allocated_resolvers_(0) {
  vtbl_->destroy = &NrIceResolverFake::destroy;
  vtbl_->resolve = &NrIceResolverFake::resolve;
  vtbl_->cancel = &NrIceResolverFake::cancel;
}

NrIceResolverFake::~NrIceResolverFake() {
  MOZ_ASSERT(allocated_resolvers_ == 0);
  delete vtbl_;
}


nr_resolver *NrIceResolverFake::AllocateResolver() {
  nr_resolver *resolver;

  int r = nr_resolver_create_int((void *)this,
                                 vtbl_, &resolver);
  MOZ_ASSERT(!r);
  if(r)
    return nullptr;

  ++allocated_resolvers_;

  return resolver;
}

void NrIceResolverFake::DestroyResolver() {
  --allocated_resolvers_;
}

int NrIceResolverFake::destroy(void **objp) {
  if (!objp || !*objp)
    return 0;

  NrIceResolverFake *fake = static_cast<NrIceResolverFake *>(*objp);
  *objp = 0;

  fake->DestroyResolver();

  return 0;
}

int NrIceResolverFake::resolve(void *obj,
                               nr_resolver_resource *resource,
                               int (*cb)(void *cb_arg,
                                         nr_transport_addr *addr),
                               void *cb_arg,
                               void **handle) {
  int r,_status;

  MOZ_ASSERT(obj);
  NrIceResolverFake *fake = static_cast<NrIceResolverFake *>(obj);

  MOZ_ASSERT(fake->allocated_resolvers_ > 0);

  PendingResolution *pending = new PendingResolution(fake,
                                                     resource->domain_name,
                                                     resource->port ?
                                                     resource->port : 3478,
                                                     cb, cb_arg);

  if ((r=NR_ASYNC_TIMER_SET(fake->delay_ms_,NrIceResolverFake::resolve_cb,
                            (void *)pending, &pending->timer_handle_))) {
    delete pending;
    ABORT(r);
  }
  *handle = pending;

  _status=0;
abort:
  return(_status);
}

void NrIceResolverFake::resolve_cb(NR_SOCKET s, int how, void *cb_arg) {
  MOZ_ASSERT(cb_arg);
  PendingResolution *pending = static_cast<PendingResolution *>(cb_arg);

  const PRNetAddr *addr=pending->resolver_->Resolve(pending->hostname_);

  if (addr) {
    nr_transport_addr transport_addr;

    int r = nr_praddr_to_transport_addr(addr, &transport_addr, 0);
    MOZ_ASSERT(!r);
    if (r)
      goto abort;

    r=nr_transport_addr_set_port(&transport_addr, pending->port_);
    MOZ_ASSERT(!r);
    if (r)
      goto abort;

    
    r=nr_transport_addr_fmt_addr_string(&transport_addr);
    MOZ_ASSERT(!r);
    if (r)
      goto abort;

    pending->cb_(pending->cb_arg_, &transport_addr);
    delete pending;
    return;
  }

abort:
  
  pending->cb_(pending->cb_arg_, nullptr);

  delete pending;
}

int NrIceResolverFake::cancel(void *obj, void *handle) {
  MOZ_ASSERT(obj);
  MOZ_ASSERT(static_cast<NrIceResolverFake *>(obj)->allocated_resolvers_ > 0);

  PendingResolution *pending = static_cast<PendingResolution *>(handle);

  NR_async_timer_cancel(pending->timer_handle_);
  delete pending;

  return(0);
}


}  
