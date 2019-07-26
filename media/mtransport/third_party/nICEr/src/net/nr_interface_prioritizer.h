


































#ifndef _nr_interface_prioritizer
#define _nr_interface_prioritizer

#include "transport_addr.h"
#include "local_addr.h"

typedef struct nr_interface_prioritizer_vtbl_ {
  int (*add_interface)(void *obj, nr_local_addr *iface);
  int (*get_priority)(void *obj, const char *key, UCHAR *pref);
  int (*sort_preference)(void *obj);
  int (*destroy)(void **obj);
} nr_interface_prioritizer_vtbl;

typedef struct nr_interface_prioritizer_ {
  void *obj;
  nr_interface_prioritizer_vtbl *vtbl;
} nr_interface_prioritizer;

int nr_interface_prioritizer_create_int(void *obj, nr_interface_prioritizer_vtbl *vtbl,
                                        nr_interface_prioritizer **prioritizer);

int nr_interface_prioritizer_destroy(nr_interface_prioritizer **prioritizer);

int nr_interface_prioritizer_add_interface(nr_interface_prioritizer *prioritizer,
                                           nr_local_addr *addr);

int nr_interface_prioritizer_get_priority(nr_interface_prioritizer *prioritizer,
                                          const char *key, UCHAR *interface_preference);

int nr_interface_prioritizer_sort_preference(nr_interface_prioritizer *prioritizer);
#endif
