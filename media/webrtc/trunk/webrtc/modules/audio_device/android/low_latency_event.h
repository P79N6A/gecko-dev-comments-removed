









#ifndef WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_LOW_LATENCY_EVENT_H_
#define WEBRTC_MODULES_AUDIO_DEVICE_ANDROID_LOW_LATENCY_EVENT_H_

#include <errno.h>
#include <limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

namespace webrtc {



class LowLatencyEvent {
 public:
  LowLatencyEvent();
  ~LowLatencyEvent();

  
  
  bool Start();
  
  
  
  
  bool Stop();

  
  void SignalEvent(int event_id, int event_msg);
  
  void WaitOnEvent(int* event_id, int* event_msg);

 private:
  typedef int Handle;
  static const Handle kInvalidHandle;
  static const int kReadHandle;
  static const int kWriteHandle;

  
  static bool Close(Handle* handle);

  
  
  void WriteFd(int message_id, int message);
  
  void ReadFd(int* message_id, int* message);

  Handle handles_[2];
};

}  

#endif  
