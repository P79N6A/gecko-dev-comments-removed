
































#ifndef _addrs_h_
#define _addrs_h_

#include "transport_addr.h"
#include "local_addr.h"

int nr_stun_get_addrs(nr_local_addr addrs[], int maxaddrs, int remove_loopback, int remove_link_local, int *count);
int nr_stun_remove_duplicate_addrs(nr_local_addr addrs[], int remove_loopback, int remove_link_local, int *count);

#endif
