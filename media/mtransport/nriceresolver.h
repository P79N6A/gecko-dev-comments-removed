










































#ifndef nriceresolver_h__
#define nriceresolver_h__

#include <map>
#include <string>
#include "nspr.h"
#include "prnetdb.h"
#include "nsIDNSService.h"
#include "nsIDNSListener.h"
#include "nsICancelable.h"

typedef struct nr_resolver_ nr_resolver;
typedef struct nr_resolver_vtbl_ nr_resolver_vtbl;
typedef struct nr_transport_addr_ nr_transport_addr;
typedef struct nr_resolver_resource_ nr_resolver_resource;

namespace mozilla {

class NrIceResolver
{
 private:
  ~NrIceResolver();
 public:
  NrIceResolver();

  nsresult Init();
  nr_resolver *AllocateResolver();
  void DestroyResolver();
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(NrIceResolver)

 private:
  
  static int destroy(void **objp);
  static int resolve(void *obj, nr_resolver_resource *resource,
                     int (*cb)(void *cb_arg, nr_transport_addr *addr),
                     void *cb_arg, void **handle);
  static void resolve_cb(NR_SOCKET s, int how, void *cb_arg);
  static int cancel(void *obj, void *handle);

  int resolve(nr_resolver_resource *resource,
              int (*cb)(void *cb_arg, nr_transport_addr *addr),
              void *cb_arg, void **handle);

  class PendingResolution : public nsIDNSListener
  {
   public:
    PendingResolution(nsIEventTarget *thread,
                      uint16_t port,
                      int transport,
                      int (*cb)(void *cb_arg, nr_transport_addr *addr),
                      void *cb_arg) :
        thread_(thread),
        port_(port),
        transport_(transport),
        cb_(cb), cb_arg_(cb_arg),
        canceled_ (false) {}
    NS_IMETHOD OnLookupComplete(nsICancelable *request, nsIDNSRecord *record,
                                nsresult status) override;
    int cancel();
    nsCOMPtr<nsICancelable> request_;
    NS_DECL_THREADSAFE_ISUPPORTS

   private:
    virtual ~PendingResolution(){};
    nsCOMPtr<nsIEventTarget> thread_;
    uint16_t port_;
    int transport_;
    int (*cb_)(void *cb_arg, nr_transport_addr *addr);
    void *cb_arg_;
    bool canceled_;
  };

  nr_resolver_vtbl* vtbl_;
  nsCOMPtr<nsIEventTarget> sts_thread_;
  nsCOMPtr<nsIDNSService> dns_;
#ifdef DEBUG
  int allocated_resolvers_;
#endif
};

}  
#endif
