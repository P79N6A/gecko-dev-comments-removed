











#ifndef WEBRTC_BASE_MACCOCOASOCKETSERVER_H_
#define WEBRTC_BASE_MACCOCOASOCKETSERVER_H_

#include "webrtc/base/macsocketserver.h"

#ifdef __OBJC__
@class NSTimer, MacCocoaSocketServerHelperRtc;
#else
class NSTimer;
class MacCocoaSocketServerHelperRtc;
#endif

namespace rtc {



class MacCocoaSocketServer : public MacBaseSocketServer {
 public:
  explicit MacCocoaSocketServer();
  virtual ~MacCocoaSocketServer();

  virtual bool Wait(int cms, bool process_io);
  virtual void WakeUp();

 private:
  MacCocoaSocketServerHelperRtc* helper_;
  NSTimer* timer_;  
  
  int run_count_;

  DISALLOW_EVIL_CONSTRUCTORS(MacCocoaSocketServer);
};

}  

#endif  
