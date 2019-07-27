


#include <map>
#include <set>
#include <string>
#include "logging.h"
#include "nrinterfaceprioritizer.h"
#include "nsCOMPtr.h"

MOZ_MTLOG_MODULE("mtransport")

namespace {

class LocalAddress {
public:
  LocalAddress()
    : key_(),
      is_vpn_(-1),
      estimated_speed_(-1),
      type_preference_(-1),
      ip_version_(-1) {}

  bool Init(const nr_local_addr& local_addr) {
    char buf[MAXIFNAME + 41];
    int r = nr_transport_addr_fmt_ifname_addr_string(&local_addr.addr, buf, sizeof(buf));
    if (r) {
      MOZ_MTLOG(ML_ERROR, "Error formatting interface address string.");
      return false;
    }
    key_ = buf;
    is_vpn_ = (local_addr.interface.type & NR_INTERFACE_TYPE_VPN) != 0 ? 1 : 0;
    estimated_speed_ = local_addr.interface.estimated_speed;
    type_preference_ = GetNetworkTypePreference(local_addr.interface.type);
    ip_version_ = local_addr.addr.ip_version;
    return true;
  }

  bool operator<(const LocalAddress& rhs) const {
    
    
    
    if (type_preference_ != rhs.type_preference_) {
      return type_preference_ < rhs.type_preference_;
    }

    
    
    
    if (is_vpn_ != rhs.is_vpn_) {
      return is_vpn_ < rhs.is_vpn_;
    }

    
    if (estimated_speed_ != rhs.estimated_speed_) {
      return estimated_speed_ > rhs.estimated_speed_;
    }

    
    if (ip_version_ != rhs.ip_version_) {
      return ip_version_ > rhs.ip_version_;
    }

    
    return key_ < rhs.key_;
  }

  const std::string& GetKey() const {
    return key_;
  }

private:
  
  
  static inline int GetNetworkTypePreference(int type) {
    if (type & NR_INTERFACE_TYPE_WIRED) {
      return 1;
    }
    if (type & NR_INTERFACE_TYPE_WIFI) {
      return 2;
    }
    if (type & NR_INTERFACE_TYPE_MOBILE) {
      return 3;
    }
    return 4;
  }

  std::string key_;
  int is_vpn_;
  int estimated_speed_;
  int type_preference_;
  int ip_version_;
};

class InterfacePrioritizer {
public:
  InterfacePrioritizer()
    : local_addrs_(),
      preference_map_(),
      sorted_(false) {}

  int add(const nr_local_addr *iface) {
    LocalAddress addr;
    if (!addr.Init(*iface)) {
      return R_FAILED;
    }
    std::pair<std::set<LocalAddress>::iterator, bool> r =
      local_addrs_.insert(addr);
    if (!r.second) {
      return R_ALREADY; 
    }
    sorted_ = false;
    return 0;
  }

  int sort() {
    UCHAR tmp_pref = 127;
    preference_map_.clear();
    for (std::set<LocalAddress>::iterator i = local_addrs_.begin();
         i != local_addrs_.end(); ++i) {
      if (tmp_pref == 0) {
        return R_FAILED;
      }
      preference_map_.insert(make_pair(i->GetKey(), tmp_pref--));
    }
    sorted_ = true;
    return 0;
  }

  int getPreference(const char *key, UCHAR *pref) {
    if (!sorted_) {
      return R_FAILED;
    }
    std::map<std::string, UCHAR>::iterator i = preference_map_.find(key);
    if (i == preference_map_.end()) {
      return R_NOT_FOUND;
    }
    *pref = i->second;
    return 0;
  }

private:
  std::set<LocalAddress> local_addrs_;
  std::map<std::string, UCHAR> preference_map_;
  bool sorted_;
};

} 

static int add_interface(void *obj, nr_local_addr *iface) {
  InterfacePrioritizer *ip = static_cast<InterfacePrioritizer*>(obj);
  return ip->add(iface);
}

static int get_priority(void *obj, const char *key, UCHAR *pref) {
  InterfacePrioritizer *ip = static_cast<InterfacePrioritizer*>(obj);
  return ip->getPreference(key, pref);
}

static int sort_preference(void *obj) {
  InterfacePrioritizer *ip = static_cast<InterfacePrioritizer*>(obj);
  return ip->sort();
}

static int destroy(void **objp) {
  if (!objp || !*objp) {
    return 0;
  }

  InterfacePrioritizer *ip = static_cast<InterfacePrioritizer*>(*objp);
  *objp = 0;
  delete ip;

  return 0;
}

static nr_interface_prioritizer_vtbl priorizer_vtbl = {
  add_interface,
  get_priority,
  sort_preference,
  destroy
};

namespace mozilla {

nr_interface_prioritizer* CreateInterfacePrioritizer() {
  nr_interface_prioritizer *ip;
  int r = nr_interface_prioritizer_create_int(new InterfacePrioritizer(),
                                              &priorizer_vtbl,
                                              &ip);
  if (r != 0) {
    return nullptr;
  }
  return ip;
}

} 
