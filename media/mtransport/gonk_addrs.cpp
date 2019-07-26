




extern "C" {
#include <arpa/inet.h>
#include "r_types.h"
#include "stun.h"
#include "addrs.h"
}

#include <vector>
#include <string>
#include "nsINetworkManager.h"
#include "nsINetworkInterfaceListService.h"
#include "runnable_utils.h"
#include "nsCOMPtr.h"
#include "nsThreadUtils.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/SyncRunnable.h"

namespace {
struct NetworkInterface {
  struct sockaddr_in addr;
  std::string name;
  
  int type;
};

nsresult
GetInterfaces(std::vector<NetworkInterface>* aInterfaces)
{
  MOZ_ASSERT(aInterfaces);

  
  nsresult rv;
  nsCOMPtr<nsINetworkInterfaceListService> listService =
    do_GetService("@mozilla.org/network/interface-list-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t flags =
    nsINetworkInterfaceListService::LIST_NOT_INCLUDE_SUPL_INTERFACES |
    nsINetworkInterfaceListService::LIST_NOT_INCLUDE_MMS_INTERFACES |
    nsINetworkInterfaceListService::LIST_NOT_INCLUDE_IMS_INTERFACES |
    nsINetworkInterfaceListService::LIST_NOT_INCLUDE_DUN_INTERFACES;
  nsCOMPtr<nsINetworkInterfaceList> networkList;
  NS_ENSURE_SUCCESS(listService->GetDataInterfaceList(flags,
                                                      getter_AddRefs(networkList)),
                    NS_ERROR_FAILURE);

  
  int32_t listLength;
  NS_ENSURE_SUCCESS(networkList->GetNumberOfInterface(&listLength),
                    NS_ERROR_FAILURE);
  aInterfaces->clear();

  nsAutoString ip;
  nsAutoString ifaceName;
  for (int32_t i = 0; i < listLength; i++) {
    nsCOMPtr<nsINetworkInterface> iface;
    if (NS_FAILED(networkList->GetInterface(i, getter_AddRefs(iface)))) {
      continue;
    }

    NetworkInterface interface;
    memset(&(interface.addr), 0, sizeof(interface.addr));
    interface.addr.sin_family = AF_INET;
    if (NS_FAILED(iface->GetIp(ip))) {
      continue;
    }
    if (inet_pton(AF_INET, NS_ConvertUTF16toUTF8(ip).get(),
                  &(interface.addr.sin_addr.s_addr)) != 1) {
      continue;
    }

    if (NS_FAILED(iface->GetName(ifaceName))) {
      continue;
    }
    interface.name = NS_ConvertUTF16toUTF8(ifaceName).get();

    int32_t type;
    if (NS_FAILED(iface->GetType(&type))) {
      continue;
    }
    switch (type) {
    case nsINetworkInterface::NETWORK_TYPE_WIFI:
      interface.type = NR_INTERFACE_TYPE_WIFI;
      break;
    case nsINetworkInterface::NETWORK_TYPE_MOBILE:
      interface.type = NR_INTERFACE_TYPE_MOBILE;
      break;
    }

    aInterfaces->push_back(interface);
  }
  return NS_OK;
}
} 

int
nr_stun_get_addrs(nr_local_addr aAddrs[], int aMaxAddrs,
                  int aDropLoopback, int* aCount)
{
  nsresult rv;
  int r;

  
  std::vector<NetworkInterface> interfaces;
  nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
  mozilla::SyncRunnable::DispatchToThread(
    mainThread.get(),
    mozilla::WrapRunnableNMRet(&GetInterfaces, &interfaces, &rv),
    false);
  if (NS_FAILED(rv)) {
    return R_FAILED;
  }

  
  int32_t n = 0;
  size_t num_interface = std::min(interfaces.size(), (size_t)aMaxAddrs);
  for (size_t i = 0; i < num_interface; ++i) {
    NetworkInterface &interface = interfaces[i];
    if (nr_sockaddr_to_transport_addr((sockaddr*)&(interface.addr),
                                      sizeof(struct sockaddr_in),
                                      IPPROTO_UDP, 0, &(aAddrs[n].addr))) {
      r_log(NR_LOG_STUN, LOG_WARNING, "Problem transforming address");
      return R_FAILED;
    }
    strlcpy(aAddrs[n].addr.ifname, interface.name.c_str(),
            sizeof(aAddrs[n].addr.ifname));
    aAddrs[n].interface.type = interface.type;
    aAddrs[n].interface.estimated_speed = 0;
    n++;
  }

  *aCount = n;
  r = nr_stun_remove_duplicate_addrs(aAddrs, aDropLoopback, aCount);
  if (r != 0) {
    return r;
  }

  for (int i = 0; i < *aCount; ++i) {
    char typestr[100];
    nr_local_addr_fmt_info_string(aAddrs + i, typestr, sizeof(typestr));
    r_log(NR_LOG_STUN, LOG_DEBUG, "Address %d: %s on %s, type: %s\n",
          i, aAddrs[i].addr.as_string, aAddrs[i].addr.ifname, typestr);
  }

  return 0;
}
