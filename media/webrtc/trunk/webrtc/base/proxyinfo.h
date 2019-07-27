









#ifndef WEBRTC_BASE_PROXYINFO_H__
#define WEBRTC_BASE_PROXYINFO_H__

#include <string>
#include "webrtc/base/socketaddress.h"
#include "webrtc/base/cryptstring.h"

namespace rtc {

enum ProxyType {
  PROXY_NONE,
  PROXY_HTTPS,
  PROXY_SOCKS5,
  PROXY_UNKNOWN
};
const char * ProxyToString(ProxyType proxy);

struct ProxyInfo {
  ProxyType type;
  SocketAddress address;
  std::string autoconfig_url;
  bool autodetect;
  std::string bypass_list;
  std::string username;
  CryptString password;

  ProxyInfo() : type(PROXY_NONE), autodetect(false) { }
};

} 

#endif
