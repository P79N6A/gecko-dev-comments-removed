



#include "NetUtils.h"
#include <dlfcn.h>
#include <errno.h>
#include <cutils/properties.h>
#include "prinit.h"
#include "mozilla/Assertions.h"
#include "nsDebug.h"

static void* sNetUtilsLib;
static PRCallOnceType sInitNetUtilsLib;

static PRStatus
InitNetUtilsLib()
{
  sNetUtilsLib = dlopen("/system/lib/libnetutils.so", RTLD_LAZY);
  
  return PR_SUCCESS;
}

static void*
GetNetUtilsLibHandle()
{
  PR_CallOnce(&sInitNetUtilsLib, InitNetUtilsLib);
  return sNetUtilsLib;
}


void*
NetUtils::GetSharedLibrary()
{
  void* netLib = GetNetUtilsLibHandle();
  if (!netLib) {
    NS_WARNING("No /system/lib/libnetutils.so");
  }
  return netLib;
}


int32_t
NetUtils::SdkVersion()
{
  char propVersion[PROPERTY_VALUE_MAX];
  property_get("ro.build.version.sdk", propVersion, "0");
  int32_t version = strtol(propVersion, nullptr, 10);
  return version;
}

DEFINE_DLFUNC(ifc_enable, int32_t, const char*)
DEFINE_DLFUNC(ifc_disable, int32_t, const char*)
DEFINE_DLFUNC(ifc_configure, int32_t, const char*, in_addr_t, uint32_t,
              in_addr_t, in_addr_t, in_addr_t)
DEFINE_DLFUNC(ifc_reset_connections, int32_t, const char*, const int32_t)
DEFINE_DLFUNC(dhcp_stop, int32_t, const char*)

int32_t NetUtils::do_ifc_enable(const char *ifname)
{
  USE_DLFUNC(ifc_enable)
  return ifc_enable(ifname);
}

int32_t NetUtils::do_ifc_disable(const char *ifname)
{
  USE_DLFUNC(ifc_disable)
  return ifc_disable(ifname);
}

int32_t NetUtils::do_ifc_configure(const char *ifname,
                                       in_addr_t address,
                                       uint32_t prefixLength,
                                       in_addr_t gateway,
                                       in_addr_t dns1,
                                       in_addr_t dns2)
{
  USE_DLFUNC(ifc_configure)
  int32_t ret = ifc_configure(ifname, address, prefixLength, gateway, dns1, dns2);
  return ret;
}

int32_t NetUtils::do_ifc_reset_connections(const char *ifname,
                                               const int32_t resetMask)
{
  USE_DLFUNC(ifc_reset_connections)
  return ifc_reset_connections(ifname, resetMask);
}

int32_t NetUtils::do_dhcp_stop(const char *ifname)
{
  USE_DLFUNC(dhcp_stop)
  return dhcp_stop(ifname);
}

int32_t NetUtils::do_dhcp_do_request(const char *ifname,
                                         char *ipaddr,
                                         char *gateway,
                                         uint32_t *prefixLength,
                                         char *dns1,
                                         char *dns2,
                                         char *server,
                                         uint32_t  *lease,
                                         char* vendorinfo)
{
  int32_t ret = -1;
  uint32_t sdkVersion = SdkVersion();

  if (sdkVersion == 15) {
    
    
    DEFINE_DLFUNC(dhcp_do_request, int32_t, const char*, char*, char*,  uint32_t*, char*, char*, char*, uint32_t*)
    USE_DLFUNC(dhcp_do_request)
    vendorinfo[0] = '\0';

    ret = dhcp_do_request(ifname, ipaddr, gateway, prefixLength, dns1, dns2,
                          server, lease);
  } else if (sdkVersion == 16 || sdkVersion == 17) {
    
    
    
    DEFINE_DLFUNC(dhcp_do_request, int32_t, const char*, char*, char*,  uint32_t*, char*, char*, char*, uint32_t*, char*)
    USE_DLFUNC(dhcp_do_request)
    ret = dhcp_do_request(ifname, ipaddr, gateway, prefixLength, dns1, dns2,
                          server, lease, vendorinfo);
  } else if (sdkVersion == 18) {
    
    
    DEFINE_DLFUNC(dhcp_do_request, int32_t, const char*, char*, char*,  uint32_t*, char**, char*, uint32_t*, char*, char*)
    USE_DLFUNC(dhcp_do_request)
    char *dns[3] = {dns1, dns2, nullptr};
    char domains[PROPERTY_VALUE_MAX];
    ret = dhcp_do_request(ifname, ipaddr, gateway, prefixLength, dns,
                          server, lease, vendorinfo, domains);
  } else {
    NS_WARNING("Unable to perform do_dhcp_request: unsupported sdk version!");
  }
  return ret;
}
