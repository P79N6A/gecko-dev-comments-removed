


































#include <nr_api.h>
#include "nr_socket_wrapper.h"

int nr_socket_wrapper_factory_create_int(void *obj, nr_socket_wrapper_factory_vtbl *vtbl,
                                         nr_socket_wrapper_factory **wrapperp)
{
  int _status;
  nr_socket_wrapper_factory *wrapper=0;

  if (!(wrapper=RCALLOC(sizeof(nr_socket_wrapper_factory))))
    ABORT(R_NO_MEMORY);

  wrapper->obj=obj;
  wrapper->vtbl=vtbl;

  *wrapperp=wrapper;
  _status=0;
abort:
  return(_status);
}

int nr_socket_wrapper_factory_wrap(nr_socket_wrapper_factory *wrapper,
                                   nr_socket *inner,
                                   nr_socket **socketp)
{
  return wrapper->vtbl->wrap(wrapper->obj, inner, socketp);
}

int nr_socket_wrapper_factory_destroy(nr_socket_wrapper_factory **wrapperp)
{
  nr_socket_wrapper_factory *wrapper;

  if (!wrapperp || !*wrapperp)
    return 0;

  wrapper = *wrapperp;
  *wrapperp = 0;

  assert(wrapper->vtbl);
  if (wrapper->vtbl)
    wrapper->vtbl->destroy(&wrapper->obj);

  RFREE(wrapper);

  return 0;
}

