


































#ifndef _local_addr_h
#define _local_addr_h

#include "transport_addr.h"

typedef struct nr_interface_ {
  int type;
#define NR_INTERFACE_TYPE_UNKNOWN 0x0
#define NR_INTERFACE_TYPE_WIRED   0x1
#define NR_INTERFACE_TYPE_WIFI    0x2
#define NR_INTERFACE_TYPE_MOBILE  0x4
#define NR_INTERFACE_TYPE_VPN     0x8
  int estimated_speed; 
} nr_interface;

typedef struct nr_local_addr_ {
  nr_transport_addr addr;
  nr_interface interface;
} nr_local_addr;

int nr_local_addr_copy(nr_local_addr *to, nr_local_addr *from);
int nr_local_addr_fmt_info_string(nr_local_addr *addr, char *buf, int len);

#endif
