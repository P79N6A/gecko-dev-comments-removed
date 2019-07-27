














#ifndef WEBRTC_BASE_MACASYNCSOCKET_H__
#define WEBRTC_BASE_MACASYNCSOCKET_H__

#include <CoreFoundation/CoreFoundation.h>

#include "webrtc/base/asyncsocket.h"
#include "webrtc/base/nethelpers.h"

namespace rtc {

class MacBaseSocketServer;

class MacAsyncSocket : public AsyncSocket, public sigslot::has_slots<> {
 public:
  MacAsyncSocket(MacBaseSocketServer* ss, int family);
  virtual ~MacAsyncSocket();

  bool valid() const { return source_ != NULL; }

  
  virtual SocketAddress GetLocalAddress() const;
  virtual SocketAddress GetRemoteAddress() const;
  virtual int Bind(const SocketAddress& addr);
  virtual int Connect(const SocketAddress& addr);
  virtual int Send(const void* buffer, size_t length);
  virtual int SendTo(const void* buffer, size_t length,
                     const SocketAddress& addr);
  virtual int Recv(void* buffer, size_t length);
  virtual int RecvFrom(void* buffer, size_t length, SocketAddress* out_addr);
  virtual int Listen(int backlog);
  virtual MacAsyncSocket* Accept(SocketAddress* out_addr);
  virtual int Close();
  virtual int GetError() const;
  virtual void SetError(int error);
  virtual ConnState GetState() const;
  virtual int EstimateMTU(uint16* mtu);
  virtual int GetOption(Option opt, int* value);
  virtual int SetOption(Option opt, int value);

  
  void EnableCallbacks();
  void DisableCallbacks();

 protected:
  void OnResolveResult(SignalThread* thread);
  int DoConnect(const SocketAddress& addr);

 private:
  
  MacAsyncSocket(MacBaseSocketServer* ss, int family, int native_socket);

   
  
  void Initialize(int family);

  
  
  static CFDataRef CopyCFAddress(const SocketAddress& address);

  
  static void MacAsyncSocketCallBack(CFSocketRef s,
                                     CFSocketCallBackType callbackType,
                                     CFDataRef address,
                                     const void* data,
                                     void* info);

  MacBaseSocketServer* ss_;
  CFSocketRef socket_;
  int native_socket_;
  CFRunLoopSourceRef source_;
  int current_callbacks_;
  bool disabled_;
  int error_;
  ConnState state_;
  AsyncResolver* resolver_;

  DISALLOW_EVIL_CONSTRUCTORS(MacAsyncSocket);
};

}  

#endif  
