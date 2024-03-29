










#ifndef NetUtils_h
#define NetUtils_h

#include "arpa/inet.h"


#define RESET_IPV4_ADDRESSES 0x01
#define RESET_IPV6_ADDRESSES 0x02
#define RESET_ALL_ADDRESSES  (RESET_IPV4_ADDRESSES | RESET_IPV6_ADDRESSES)



class NetUtils
{
public:
  static void* GetSharedLibrary();

  NetUtils();

  int32_t do_ifc_enable(const char *ifname);
  int32_t do_ifc_disable(const char *ifname);
  int32_t do_ifc_configure(const char *ifname,
                           in_addr_t address,
                           uint32_t prefixLength,
                           in_addr_t gateway,
                           in_addr_t dns1,
                           in_addr_t dns2);
  int32_t do_ifc_reset_connections(const char *ifname, const int32_t resetMask);
  int32_t do_ifc_set_default_route(const char *ifname, in_addr_t gateway);
  int32_t do_ifc_add_route(const char *ifname,
                           const char *dst,
                           uint32_t prefixLength,
                           const char *gateway);
  int32_t do_ifc_remove_route(const char *ifname,
                              const char *dst,
                              uint32_t prefixLength,
                              const char *gateway);
  int32_t do_ifc_remove_host_routes(const char *ifname);
  int32_t do_ifc_remove_default_route(const char *ifname);
  int32_t do_dhcp_stop(const char *ifname);
  int32_t do_dhcp_do_request(const char *ifname,
                             char *ipaddr,
                             char *gateway,
                             uint32_t *prefixLength,
                             char *dns1,
                             char *dns2,
                             char *server,
                             uint32_t  *lease,
                             char* vendorinfo);

  static int32_t SdkVersion();
};


#define DEFINE_DLFUNC(name, ret, args...) typedef ret (*FUNC##name)(args);


#define USE_DLFUNC(name)                                                      \
  FUNC##name name = (FUNC##name) dlsym(GetSharedLibrary(), #name);            \
  if (!name) {                                                                \
    MOZ_CRASH("Symbol not found in shared library : " #name);                 \
  }

#endif 
