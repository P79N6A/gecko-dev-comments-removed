


































#include "nr_api.h"
#include "nr_interface_prioritizer.h"
#include "transport_addr.h"

int nr_interface_prioritizer_create_int(void *obj,
  nr_interface_prioritizer_vtbl *vtbl,nr_interface_prioritizer **ifpp)
  {
    int _status;
    nr_interface_prioritizer *ifp=0;

    if(!(ifp=RCALLOC(sizeof(nr_interface_prioritizer))))
      ABORT(R_NO_MEMORY);

    ifp->obj = obj;
    ifp->vtbl = vtbl;

    *ifpp = ifp;

    _status=0;
  abort:
    return(_status);
  }

int nr_interface_prioritizer_destroy(nr_interface_prioritizer **ifpp)
  {
    nr_interface_prioritizer *ifp;

    if (!ifpp || !*ifpp)
      return(0);

    ifp = *ifpp;
    *ifpp = 0;
    ifp->vtbl->destroy(&ifp->obj);
    RFREE(ifp);
    return(0);
  }

int nr_interface_prioritizer_add_interface(nr_interface_prioritizer *ifp,
  nr_local_addr *addr)
  {
    return ifp->vtbl->add_interface(ifp->obj, addr);
  }

int nr_interface_prioritizer_get_priority(nr_interface_prioritizer *ifp,
  const char *key, UCHAR *interface_preference)
  {
    return ifp->vtbl->get_priority(ifp->obj,key,interface_preference);
  }

int nr_interface_prioritizer_sort_preference(nr_interface_prioritizer *ifp)
  {
    return ifp->vtbl->sort_preference(ifp->obj);
  }
