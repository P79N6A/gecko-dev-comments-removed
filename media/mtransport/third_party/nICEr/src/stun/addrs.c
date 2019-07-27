
































static char *RCSSTRING __UNUSED__="$Id: addrs.c,v 1.2 2008/04/28 18:21:30 ekr Exp $";

#include <csi_platform.h>
#include <assert.h>
#include <string.h>

#ifdef WIN32
#include <winsock2.h>
#include <iphlpapi.h>
#include <tchar.h>
#else   

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <errno.h>

#ifndef ANDROID

#include <sys/types.h> 
#include <ifaddrs.h> 
#else
#include "ifaddrs-android.h"
#define getifaddrs android_getifaddrs
#define freeifaddrs android_freeifaddrs
#endif

#ifdef LINUX

#ifdef ANDROID

#undef __unused
#else
#include <linux/if.h> 
#include <linux/wireless.h> 
#include <linux/ethtool.h> 
#include <linux/sockios.h> 
#endif 

#endif 

#endif  

#include "stun.h"
#include "addrs.h"

#if defined(WIN32)

#define WIN32_MAX_NUM_INTERFACES  20


#define _NR_MAX_KEY_LENGTH 256
#define _NR_MAX_NAME_LENGTH 512

#define _ADAPTERS_BASE_REG "SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}"

static int nr_win32_get_adapter_friendly_name(char *adapter_GUID, char **friendly_name)
{
    int r,_status;
    HKEY adapter_reg;
    TCHAR adapter_key[_NR_MAX_KEY_LENGTH];
    TCHAR keyval_buf[_NR_MAX_KEY_LENGTH];
    TCHAR adapter_GUID_tchar[_NR_MAX_NAME_LENGTH];
    DWORD keyval_len, key_type;
    size_t converted_chars, newlen;
    char *my_fn = 0;

#ifdef _UNICODE
    mbstowcs_s(&converted_chars, adapter_GUID_tchar, strlen(adapter_GUID)+1,
               adapter_GUID, _TRUNCATE);
#else
    strlcpy(adapter_GUID_tchar, adapter_GUID, _NR_MAX_NAME_LENGTH);
#endif

    _tcscpy_s(adapter_key, _NR_MAX_KEY_LENGTH, TEXT(_ADAPTERS_BASE_REG));
    _tcscat_s(adapter_key, _NR_MAX_KEY_LENGTH, TEXT("\\"));
    _tcscat_s(adapter_key, _NR_MAX_KEY_LENGTH, adapter_GUID_tchar);
    _tcscat_s(adapter_key, _NR_MAX_KEY_LENGTH, TEXT("\\Connection"));

    r = RegOpenKeyEx(HKEY_LOCAL_MACHINE, adapter_key, 0, KEY_READ, &adapter_reg);

    if (r != ERROR_SUCCESS) {
      r_log(NR_LOG_STUN, LOG_ERR, "Got error %d opening adapter reg key\n", r);
      ABORT(R_INTERNAL);
    }

    keyval_len = sizeof(keyval_buf);
    r = RegQueryValueEx(adapter_reg, TEXT("Name"), NULL, &key_type,
                        (BYTE *)keyval_buf, &keyval_len);

    RegCloseKey(adapter_reg);

#ifdef UNICODE
    newlen = wcslen(keyval_buf)+1;
    my_fn = (char *) RCALLOC(newlen);
    if (!my_fn) {
      ABORT(R_NO_MEMORY);
    }
    wcstombs_s(&converted_chars, my_fn, newlen, keyval_buf, _TRUNCATE);
#else
    my_fn = r_strdup(keyval_buf);
#endif

    *friendly_name = my_fn;
    _status=0;

abort:
    if (_status) {
      if (my_fn) free(my_fn);
    }
    return(_status);
}

static int
stun_get_win32_addrs(nr_local_addr addrs[], int maxaddrs, int *count)
{
    int r,_status;
    PIP_ADAPTER_ADDRESSES AdapterAddresses = NULL, tmpAddress = NULL;
    ULONG buflen;
    char munged_ifname[IFNAMSIZ];
    int n = 0;

    *count = 0;

    if (maxaddrs <= 0)
      ABORT(R_INTERNAL);

    

    buflen = 0;

    r = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, AdapterAddresses, &buflen);
    if (r != ERROR_BUFFER_OVERFLOW) {
      r_log(NR_LOG_STUN, LOG_ERR, "Error getting buf len from GetAdaptersAddresses()");
      ABORT(R_INTERNAL);
    }

    AdapterAddresses = (PIP_ADAPTER_ADDRESSES) RMALLOC(buflen);
    if (AdapterAddresses == NULL) {
      r_log(NR_LOG_STUN, LOG_ERR, "Error allocating buf for GetAdaptersAddresses()");
      ABORT(R_NO_MEMORY);
    }

    

    r = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, AdapterAddresses, &buflen);
    if (r != NO_ERROR) {
      r_log(NR_LOG_STUN, LOG_ERR, "Error getting addresses from GetAdaptersAddresses()");
      ABORT(R_INTERNAL);
    }

    

    for (tmpAddress = AdapterAddresses; tmpAddress != NULL; tmpAddress = tmpAddress->Next) {
      char *c;

      if (tmpAddress->OperStatus != IfOperStatusUp)
        continue;

      snprintf(munged_ifname, IFNAMSIZ, "%S%c", tmpAddress->FriendlyName, 0);
      
      c = strchr(munged_ifname, ' ');
      while (c != NULL) {
        *c = '_';
         c = strchr(munged_ifname, ' ');
      }
      c = strchr(munged_ifname, '.');
      while (c != NULL) {
        *c = '+';
         c = strchr(munged_ifname, '+');
      }

      if ((tmpAddress->IfIndex != 0) || (tmpAddress->Ipv6IfIndex != 0)) {
        IP_ADAPTER_UNICAST_ADDRESS *u = 0;

        for (u = tmpAddress->FirstUnicastAddress; u != 0; u = u->Next) {
          SOCKET_ADDRESS *sa_addr = &u->Address;

          if ((sa_addr->lpSockaddr->sa_family == AF_INET) ||
              (sa_addr->lpSockaddr->sa_family == AF_INET6)) {
            if ((r=nr_sockaddr_to_transport_addr((struct sockaddr*)sa_addr->lpSockaddr, IPPROTO_UDP, 0, &(addrs[n].addr))))
                ABORT(r);
          }
          else {
            r_log(NR_LOG_STUN, LOG_DEBUG, "Unrecognized sa_family for adapteraddress %s",munged_ifname);
            continue;
          }

          strlcpy(addrs[n].addr.ifname, munged_ifname, sizeof(addrs[n].addr.ifname));
          
          addrs[n].interface.type = NR_INTERFACE_TYPE_UNKNOWN;
          addrs[n].interface.estimated_speed = 0;
          if (++n >= maxaddrs)
            goto done;
        }
      }
    }

   done:
    *count = n;
    _status = 0;

  abort:
    RFREE(AdapterAddresses);
    return _status;
}

#else 

static int
nr_stun_is_duplicate_addr(nr_local_addr addrs[], int count, nr_local_addr *addr);

static int
stun_getifaddrs(nr_local_addr addrs[], int maxaddrs, int *count)
{
  int r,_status;
  struct ifaddrs* if_addrs_head=NULL;
  struct ifaddrs* if_addr;

  *count=0;

  if (getifaddrs(&if_addrs_head) == -1) {
    r_log(NR_LOG_STUN, LOG_ERR, "getifaddrs error e = %d", errno);
    ABORT(R_INTERNAL);
  }

  if_addr = if_addrs_head;

  while (if_addr && *count < maxaddrs) {
    switch (if_addr->ifa_addr->sa_family) {
      case AF_INET:
      case AF_INET6:
        if (r=nr_sockaddr_to_transport_addr(if_addr->ifa_addr, IPPROTO_UDP, 0, &(addrs[*count].addr))) {
          r_log(NR_LOG_STUN, LOG_ERR, "nr_sockaddr_to_transport_addr error r = %d", r);
        } else {
#if defined(LINUX) && !defined(ANDROID)
          struct ethtool_cmd ecmd;
          struct ifreq ifr;
          struct iwreq wrq;
          int e;
          int s = socket(AF_INET, SOCK_DGRAM, 0);

          strncpy(ifr.ifr_name, if_addr->ifa_name, sizeof(ifr.ifr_name));
          
          
          ecmd.cmd = ETHTOOL_GSET;
          
          ifr.ifr_data = (void*)&ecmd;

          e = ioctl(s, SIOCETHTOOL, &ifr);
          if (e == 0)
          {
             

             addrs[*count].interface.type = NR_INTERFACE_TYPE_WIRED;
#ifdef DONT_HAVE_ETHTOOL_SPEED_HI
             addrs[*count].interface.estimated_speed = ecmd.speed;
#else
             addrs[*count].interface.estimated_speed = ((ecmd.speed_hi << 16) | ecmd.speed) * 1000;
#endif
          }

          strncpy(wrq.ifr_name, if_addr->ifa_name, sizeof(wrq.ifr_name));
          e = ioctl(s, SIOCGIWRATE, &wrq);
          if (e == 0)
          {
             addrs[*count].interface.type = NR_INTERFACE_TYPE_WIFI;
             addrs[*count].interface.estimated_speed = wrq.u.bitrate.value / 1000;
          }

          if (if_addr->ifa_flags & IFF_POINTOPOINT)
          {
             addrs[*count].interface.type = NR_INTERFACE_TYPE_UNKNOWN | NR_INTERFACE_TYPE_VPN;
             
          }
#else
          addrs[*count].interface.type = NR_INTERFACE_TYPE_UNKNOWN;
          addrs[*count].interface.estimated_speed = 0;
#endif
          strlcpy(addrs[*count].addr.ifname, if_addr->ifa_name, sizeof(addrs[*count].addr.ifname));
          ++(*count);
        }
        break;
      default:
        ;
    }

    if_addr = if_addr->ifa_next;
  }

  _status=0;
abort:
  if (if_addrs_head) {
    freeifaddrs(if_addrs_head);
  }
  return(_status);
}

#endif

static int
nr_stun_is_duplicate_addr(nr_local_addr addrs[], int count, nr_local_addr *addr)
{
    int i;
    int different;

    for (i = 0; i < count; ++i) {
        different = nr_transport_addr_cmp(&addrs[i].addr, &(addr->addr),
          NR_TRANSPORT_ADDR_CMP_MODE_ALL);
        if (!different)
            return 1;  
    }

    return 0;
}

int
nr_stun_remove_duplicate_addrs(nr_local_addr addrs[], int remove_loopback, int remove_link_local, int *count)
{
    int r, _status;
    nr_local_addr *tmp = 0;
    int i;
    int n;

    tmp = RMALLOC(*count * sizeof(*tmp));
    if (!tmp)
        ABORT(R_NO_MEMORY);

    n = 0;
    for (i = 0; i < *count; ++i) {
        if (nr_stun_is_duplicate_addr(tmp, n, &addrs[i])) {
            
        }
        else if (remove_loopback && nr_transport_addr_is_loopback(&addrs[i].addr)) {
            
        }
        else if (remove_link_local &&
                 addrs[i].addr.ip_version == NR_IPV6 &&
                 nr_transport_addr_is_link_local(&addrs[i].addr)) {
            
        }
        else {
            
            if ((r=nr_local_addr_copy(&tmp[n], &addrs[i])))
                ABORT(r);
            ++n;
        }
    }

    *count = n;

    
    for (i = 0; i < *count; ++i) {
        if ((r=nr_local_addr_copy(&addrs[i], &tmp[i])))
            ABORT(r);
    }

    _status = 0;
  abort:
    RFREE(tmp);
    return _status;
}

#ifndef USE_PLATFORM_NR_STUN_GET_ADDRS

int
nr_stun_get_addrs(nr_local_addr addrs[], int maxaddrs, int drop_loopback, int drop_link_local, int *count)
{
    int _status=0;
    int i;
    char typestr[100];

#ifdef WIN32
    _status = stun_get_win32_addrs(addrs, maxaddrs, count);
#else
    _status = stun_getifaddrs(addrs, maxaddrs, count);
#endif

    nr_stun_remove_duplicate_addrs(addrs, drop_loopback, drop_link_local, count);

    for (i = 0; i < *count; ++i) {
    nr_local_addr_fmt_info_string(addrs+i,typestr,sizeof(typestr));
        r_log(NR_LOG_STUN, LOG_DEBUG, "Address %d: %s on %s, type: %s\n",
            i,addrs[i].addr.as_string,addrs[i].addr.ifname,typestr);
    }

    return _status;
}

#endif
