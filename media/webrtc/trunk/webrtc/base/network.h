









#ifndef WEBRTC_BASE_NETWORK_H_
#define WEBRTC_BASE_NETWORK_H_

#include <deque>
#include <map>
#include <string>
#include <vector>

#include "webrtc/base/basictypes.h"
#include "webrtc/base/ipaddress.h"
#include "webrtc/base/messagehandler.h"
#include "webrtc/base/sigslot.h"

#if defined(WEBRTC_POSIX)
struct ifaddrs;
#endif  

namespace rtc {

class Network;
class Thread;

enum AdapterType {
  
  ADAPTER_TYPE_UNKNOWN = 0,
  ADAPTER_TYPE_ETHERNET = 1,
  ADAPTER_TYPE_WIFI = 2,
  ADAPTER_TYPE_CELLULAR = 3,
  ADAPTER_TYPE_VPN = 4
};




std::string MakeNetworkKey(const std::string& name, const IPAddress& prefix,
                           int prefix_length);



class NetworkManager {
 public:
  typedef std::vector<Network*> NetworkList;

  NetworkManager();
  virtual ~NetworkManager();

  
  sigslot::signal0<> SignalNetworksChanged;

  
  sigslot::signal0<> SignalError;

  
  
  
  
  virtual void StartUpdating() = 0;
  virtual void StopUpdating() = 0;

  
  
  
  
  
  virtual void GetNetworks(NetworkList* networks) const = 0;

  
  virtual void DumpNetworks(bool include_ignored) {}
};


class NetworkManagerBase : public NetworkManager {
 public:
  NetworkManagerBase();
  virtual ~NetworkManagerBase();

  virtual void GetNetworks(std::vector<Network*>* networks) const;
  bool ipv6_enabled() const { return ipv6_enabled_; }
  void set_ipv6_enabled(bool enabled) { ipv6_enabled_ = enabled; }

 protected:
  typedef std::map<std::string, Network*> NetworkMap;
  
  
  
  
  
  void MergeNetworkList(const NetworkList& list, bool* changed);

 private:
  friend class NetworkTest;
  void DoUpdateNetworks();

  NetworkList networks_;
  NetworkMap networks_map_;
  bool ipv6_enabled_;
};



class BasicNetworkManager : public NetworkManagerBase,
                            public MessageHandler {
 public:
  BasicNetworkManager();
  virtual ~BasicNetworkManager();

  virtual void StartUpdating();
  virtual void StopUpdating();

  
  virtual void DumpNetworks(bool include_ignored);

  
  virtual void OnMessage(Message* msg);
  bool started() { return start_count_ > 0; }

  
  
  void set_network_ignore_list(const std::vector<std::string>& list) {
    network_ignore_list_ = list;
  }
#if defined(WEBRTC_LINUX)
  
  void set_ignore_non_default_routes(bool value) {
    ignore_non_default_routes_ = true;
  }
#endif

 protected:
#if defined(WEBRTC_POSIX)
  
  void ConvertIfAddrs(ifaddrs* interfaces,
                      bool include_ignored,
                      NetworkList* networks) const;
#endif  

  
  bool CreateNetworks(bool include_ignored, NetworkList* networks) const;

  
  bool IsIgnoredNetwork(const Network& network) const;

 private:
  friend class NetworkTest;

  void DoUpdateNetworks();

  Thread* thread_;
  bool sent_first_update_;
  int start_count_;
  std::vector<std::string> network_ignore_list_;
  bool ignore_non_default_routes_;
};


class Network {
 public:
  Network(const std::string& name, const std::string& description,
          const IPAddress& prefix, int prefix_length);

  Network(const std::string& name, const std::string& description,
          const IPAddress& prefix, int prefix_length, AdapterType type);

  
  const std::string& name() const { return name_; }

  
  
  const std::string& description() const { return description_; }

  
  const IPAddress& prefix() const { return prefix_; }
  
  int prefix_length() const { return prefix_length_; }

  
  
  std::string key() const { return key_; }

  
  
  
  
  
  
  
  

  
  
  
  
  
  

  
  
  IPAddress GetBestIP() const;

  
  
  IPAddress ip() const { return GetBestIP(); }

  
  void AddIP(const InterfaceAddress& ip) { ips_.push_back(ip); }

  
  
  bool SetIPs(const std::vector<InterfaceAddress>& ips, bool already_changed);
  
  const std::vector<InterfaceAddress>& GetIPs() const { return ips_;}
  
  void ClearIPs() { ips_.clear(); }

  
  
  int scope_id() const { return scope_id_; }
  void set_scope_id(int id) { scope_id_ = id; }

  
  
  bool ignored() const { return ignored_; }
  void set_ignored(bool ignored) { ignored_ = ignored; }

  AdapterType type() const { return type_; }
  int preference() const { return preference_; }
  void set_preference(int preference) { preference_ = preference; }

  
  std::string ToString() const;

 private:
  std::string name_;
  std::string description_;
  IPAddress prefix_;
  int prefix_length_;
  std::string key_;
  std::vector<InterfaceAddress> ips_;
  int scope_id_;
  bool ignored_;
  AdapterType type_;
  int preference_;

  friend class NetworkManager;
};

}  

#endif  
