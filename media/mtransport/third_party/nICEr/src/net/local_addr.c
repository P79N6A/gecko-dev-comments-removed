


































#include <nr_api.h>
#include <string.h>
#include "local_addr.h"

int nr_local_addr_copy(nr_local_addr *to, nr_local_addr *from)
  {
    nr_transport_addr_copy(&(to->addr), &(from->addr));
    to->interface = from->interface;
    return(0);
  }

int nr_local_addr_fmt_info_string(nr_local_addr *addr, char *buf, int len)
  {
    int addr_type = addr->interface.type;
    const char *vpn = (addr_type & NR_INTERFACE_TYPE_VPN) ? "VPN on " : "";

    const char *type = (addr_type & NR_INTERFACE_TYPE_WIRED) ? "wired" :
                       (addr_type & NR_INTERFACE_TYPE_WIFI) ? "wifi" :
                       (addr_type & NR_INTERFACE_TYPE_MOBILE) ? "mobile" :
                       "unknown";

    snprintf(buf, len, "%s%s, estimated speed: %d kbps",
             vpn, type, addr->interface.estimated_speed);
    return (0);
  }
