

































#ifndef _transport_addr_reg_h
#define _transport_addr_reg_h

#include "registry.h"

int nr_reg_get_transport_addr(NR_registry prefix, int keep, nr_transport_addr *addr);
int nr_reg_set_transport_addr(NR_registry prefix, int keep, nr_transport_addr *addr);
int nr_reg_get_transport_addr2(NR_registry prefix, char *name, int keep, nr_transport_addr *addr);
int nr_reg_set_transport_addr2(NR_registry prefix, char *name, int keep, nr_transport_addr *addr);

#endif

