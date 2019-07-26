










































#ifndef nriceresolverfake_h__
#define nriceresolverfake_h__

#include <map>
#include <string>

#include "nspr.h"
#include "prnetdb.h"
#include "mozilla/NullPtr.h"

typedef struct nr_resolver_ nr_resolver;
typedef struct nr_resolver_vtbl_ nr_resolver_vtbl;
typedef struct nr_transport_addr_ nr_transport_addr;
typedef struct nr_resolver_resource_ nr_resolver_resource;

namespace mozilla {

class NrIceResolverFake {
 public:
  NrIceResolverFake();
  ~NrIceResolverFake();

  void SetAddr(const std::string& hostname, const PRNetAddr& addr) {
    addrs_[hostname] = addr;
  }

  nr_resolver *AllocateResolver();

  void DestroyResolver();

private:
  
  static int destroy(void **objp);
  static int resolve(void *obj,
                     nr_resolver_resource *resource,
                     int (*cb)(void *cb_arg,
                               nr_transport_addr *addr),
                     void *cb_arg,
                     void **handle);
  static void resolve_cb(NR_SOCKET s, int how, void *cb_arg);
  static int cancel(void *obj, void *handle);

  
  const PRNetAddr *Resolve(const std::string& hostname) {
    if (!addrs_.count(hostname))
      return nullptr;

    return &addrs_[hostname];
  }


  struct PendingResolution {
    PendingResolution(NrIceResolverFake *resolver,
                      const std::string& hostname,
                      uint16_t port,
                      int (*cb)(void *cb_arg, nr_transport_addr *addr),
                      void *cb_arg) :
        resolver_(resolver),
        hostname_(hostname),
        port_(port),
        cb_(cb), cb_arg_(cb_arg) {}

    NrIceResolverFake *resolver_;
    std::string hostname_;
    uint16_t port_;
    int (*cb_)(void *cb_arg, nr_transport_addr *addr);
    void *cb_arg_;
    void *timer_handle_;
  };

  nr_resolver_vtbl* vtbl_;
  std::map<std::string, PRNetAddr> addrs_;
  uint32_t delay_ms_;
  int allocated_resolvers_;
};

}  

#endif
