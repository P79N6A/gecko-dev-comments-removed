


































#include <nr_api.h>
#include "nr_resolver.h"

int nr_resolver_create_int(void *obj, nr_resolver_vtbl *vtbl, nr_resolver **resolverp)
{
  int _status;
  nr_resolver *resolver=0;

  if (!(resolver=RCALLOC(sizeof(nr_resolver))))
    ABORT(R_NO_MEMORY);

  resolver->obj=obj;
  resolver->vtbl=vtbl;

  *resolverp=resolver;
  _status=0;
abort:
  return(_status);
}

int nr_resolver_destroy(nr_resolver **resolverp)
{
  nr_resolver *resolver;

  if(!resolverp || !*resolverp)
    return(0);

  resolver=*resolverp;
  *resolverp=0;

  resolver->vtbl->destroy(&resolver->obj);

  RFREE(resolver);

  return(0);
}

int nr_resolver_resolve(nr_resolver *resolver,
                        nr_resolver_resource *resource,
                        int (*cb)(void *cb_arg, nr_transport_addr *addr),
                        void *cb_arg,
                        void **handle)
{
  return resolver->vtbl->resolve(resolver->obj, resource, cb, cb_arg, handle);
}

int nr_resolver_cancel(nr_resolver *resolver, void *handle)
{
  return resolver->vtbl->cancel(resolver->obj, handle);
}
