









#ifndef WEBRTC_BASE_ASYNCUDPSOCKET_H_
#define WEBRTC_BASE_ASYNCUDPSOCKET_H_

#include "webrtc/base/asyncpacketsocket.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/socketfactory.h"

namespace rtc {



class AsyncUDPSocket : public AsyncPacketSocket {
 public:
  
  
  
  static AsyncUDPSocket* Create(AsyncSocket* socket,
                                const SocketAddress& bind_address);
  
  
  static AsyncUDPSocket* Create(SocketFactory* factory,
                                const SocketAddress& bind_address);
  explicit AsyncUDPSocket(AsyncSocket* socket);
  virtual ~AsyncUDPSocket();

  virtual SocketAddress GetLocalAddress() const;
  virtual SocketAddress GetRemoteAddress() const;
  virtual int Send(const void *pv, size_t cb,
                   const rtc::PacketOptions& options);
  virtual int SendTo(const void *pv, size_t cb, const SocketAddress& addr,
                     const rtc::PacketOptions& options);
  virtual int Close();

  virtual State GetState() const;
  virtual int GetOption(Socket::Option opt, int* value);
  virtual int SetOption(Socket::Option opt, int value);
  virtual int GetError() const;
  virtual void SetError(int error);

 private:
  
  void OnReadEvent(AsyncSocket* socket);
  
  void OnWriteEvent(AsyncSocket* socket);

  scoped_ptr<AsyncSocket> socket_;
  char* buf_;
  size_t size_;
};

}  

#endif  
