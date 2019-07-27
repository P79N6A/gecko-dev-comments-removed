









#include "webrtc/base/proxyinfo.h"

namespace rtc {

const char * ProxyToString(ProxyType proxy) {
  const char * const PROXY_NAMES[] = { "none", "https", "socks5", "unknown" };
  return PROXY_NAMES[proxy];
}

} 
