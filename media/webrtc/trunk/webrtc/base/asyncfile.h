









#ifndef WEBRTC_BASE_ASYNCFILE_H__
#define WEBRTC_BASE_ASYNCFILE_H__

#include "webrtc/base/sigslot.h"

namespace rtc {



class AsyncFile {
 public:
  AsyncFile();
  virtual ~AsyncFile();

  
  virtual bool readable() = 0;
  virtual void set_readable(bool value) = 0;

  
  virtual bool writable() = 0;
  virtual void set_writable(bool value) = 0;

  sigslot::signal1<AsyncFile*> SignalReadEvent;
  sigslot::signal1<AsyncFile*> SignalWriteEvent;
  sigslot::signal2<AsyncFile*, int> SignalCloseEvent;
};

}  

#endif  
