












#include "webrtc/base/win32socketinit.h"

#include "net/base/winsock_init.h"

#if !defined(WEBRTC_WIN)
#error "Only compile this on Windows"
#endif

namespace rtc {

void EnsureWinsockInit() {
  net::EnsureWinsockInit();
}

}  
