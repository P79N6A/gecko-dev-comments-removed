








#ifndef WEBRTC_BASE_MACSOCKETSERVER_H__
#define WEBRTC_BASE_MACSOCKETSERVER_H__

#include <set>
#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS) 
#include <Carbon/Carbon.h>
#endif
#include "webrtc/base/physicalsocketserver.h"

namespace rtc {




class MacAsyncSocket;

class MacBaseSocketServer : public PhysicalSocketServer {
 public:
  MacBaseSocketServer();
  virtual ~MacBaseSocketServer();

  
  virtual Socket* CreateSocket(int type) { return NULL; }
  virtual Socket* CreateSocket(int family, int type) { return NULL; }

  virtual AsyncSocket* CreateAsyncSocket(int type);
  virtual AsyncSocket* CreateAsyncSocket(int family, int type);

  virtual bool Wait(int cms, bool process_io) = 0;
  virtual void WakeUp() = 0;

  void RegisterSocket(MacAsyncSocket* socket);
  void UnregisterSocket(MacAsyncSocket* socket);

  
  virtual bool SetPosixSignalHandler(int signum, void (*handler)(int));

 protected:
  void EnableSocketCallbacks(bool enable);
  const std::set<MacAsyncSocket*>& sockets() {
    return sockets_;
  }

 private:
  static void FileDescriptorCallback(CFFileDescriptorRef ref,
                                     CFOptionFlags flags,
                                     void* context);

  std::set<MacAsyncSocket*> sockets_;
};





class MacCFSocketServer : public MacBaseSocketServer {
 public:
  MacCFSocketServer();
  virtual ~MacCFSocketServer();

  
  virtual bool Wait(int cms, bool process_io);
  virtual void WakeUp();
  void OnWakeUpCallback();

 private:
  CFRunLoopRef run_loop_;
  CFRunLoopSourceRef wake_up_;
};

#ifndef CARBON_DEPRECATED










class MacCarbonSocketServer : public MacBaseSocketServer {
 public:
  MacCarbonSocketServer();
  virtual ~MacCarbonSocketServer();

  
  virtual bool Wait(int cms, bool process_io);
  virtual void WakeUp();

 private:
  EventQueueRef event_queue_;
  EventRef wake_up_;
};










class MacCarbonAppSocketServer : public MacBaseSocketServer {
 public:
  MacCarbonAppSocketServer();
  virtual ~MacCarbonAppSocketServer();

  
  virtual bool Wait(int cms, bool process_io);
  virtual void WakeUp();

 private:
  static OSStatus WakeUpEventHandler(EventHandlerCallRef next, EventRef event,
                                     void *data);
  static void TimerHandler(EventLoopTimerRef timer, void *data);

  EventQueueRef event_queue_;
  EventHandlerRef event_handler_;
  EventLoopTimerRef timer_;
};

#endif
} 

#endif  
