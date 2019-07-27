









#ifndef WEBRTC_BASE_SOCKETADDRESS_H_
#define WEBRTC_BASE_SOCKETADDRESS_H_

#include <string>
#include <vector>
#include <iosfwd>
#include "webrtc/base/basictypes.h"
#include "webrtc/base/ipaddress.h"

#undef SetPort

struct sockaddr_in;
struct sockaddr_storage;

namespace rtc {


class SocketAddress {
 public:
  
  SocketAddress();

  
  
  SocketAddress(const std::string& hostname, int port);

  
  
  SocketAddress(uint32 ip_as_host_order_integer, int port);

  
  SocketAddress(const IPAddress& ip, int port);

  
  SocketAddress(const SocketAddress& addr);

  
  void Clear();

  
  bool IsNil() const;

  
  bool IsComplete() const;

  
  SocketAddress& operator=(const SocketAddress& addr);

  
  
  void SetIP(uint32 ip_as_host_order_integer);

  
  void SetIP(const IPAddress& ip);

  
  
  void SetIP(const std::string& hostname);

  
  
  
  void SetResolvedIP(uint32 ip_as_host_order_integer);

  
  
  void SetResolvedIP(const IPAddress& ip);

  
  void SetPort(int port);

  
  const std::string& hostname() const { return hostname_; }

  
  
  uint32 ip() const;

  const IPAddress& ipaddr() const;

  int family() const {return ip_.family(); }

  
  uint16 port() const;

  
  
  
  
  
  int scope_id() const {return scope_id_; }
  void SetScopeID(int id) { scope_id_ = id; }

  
  
  
  std::string HostAsURIString() const;

  
  
  std::string HostAsSensitiveURIString() const;

  
  std::string PortAsString() const;

  
  std::string ToString() const;

  
  std::string ToSensitiveString() const;

  
  bool FromString(const std::string& str);

  friend std::ostream& operator<<(std::ostream& os, const SocketAddress& addr);

  
  
  
  bool IsAnyIP() const;
  inline bool IsAny() const { return IsAnyIP(); }  

  
  
  
  bool IsLoopbackIP() const;

  
  
  
  bool IsPrivateIP() const;

  
  bool IsUnresolvedIP() const;
  inline bool IsUnresolved() const { return IsUnresolvedIP(); }  

  
  bool operator ==(const SocketAddress& addr) const;
  inline bool operator !=(const SocketAddress& addr) const {
    return !this->operator ==(addr);
  }

  
  bool operator <(const SocketAddress& addr) const;

  
  bool EqualIPs(const SocketAddress& addr) const;

  
  bool EqualPorts(const SocketAddress& addr) const;

  
  size_t Hash() const;

  
  
  void ToSockAddr(sockaddr_in* saddr) const;

  
  bool FromSockAddr(const sockaddr_in& saddr);

  
  
  
  
  
  
  size_t ToDualStackSockAddrStorage(sockaddr_storage* saddr) const;
  size_t ToSockAddrStorage(sockaddr_storage* saddr) const;

  
  
  
  static std::string IPToString(uint32 ip_as_host_order_integer);

  
  
  static std::string IPToSensitiveString(uint32 ip_as_host_order_integer);

  
  
  
  
  static bool StringToIP(const std::string& str, uint32* ip);
  static uint32 StringToIP(const std::string& str);

  
  static bool StringToIP(const std::string& str, IPAddress* ip);

 private:
  std::string hostname_;
  IPAddress ip_;
  uint16 port_;
  int scope_id_;
  bool literal_;  
};

bool SocketAddressFromSockAddrStorage(const sockaddr_storage& saddr,
                                      SocketAddress* out);
SocketAddress EmptySocketAddressWithFamily(int family);

}  

#endif  
