









#include "webrtc/base/win32socketinit.h"

#include "webrtc/base/win32.h"

namespace rtc {


void EnsureWinsockInit() {
  
  
  
  
}

#if defined(WEBRTC_WIN)
class WinsockInitializer {
 public:
  WinsockInitializer() {
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(1, 0);
    err_ = WSAStartup(wVersionRequested, &wsaData);
  }
  ~WinsockInitializer() {
    if (!err_)
      WSACleanup();
  }
  int error() {
    return err_;
  }
 private:
  int err_;
};
WinsockInitializer g_winsockinit;
#endif

}  
