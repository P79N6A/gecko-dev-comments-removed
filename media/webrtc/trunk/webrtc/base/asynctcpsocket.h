









#ifndef WEBRTC_BASE_ASYNCTCPSOCKET_H_
#define WEBRTC_BASE_ASYNCTCPSOCKET_H_

#include "webrtc/base/asyncpacketsocket.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/socketfactory.h"

namespace rtc {




class AsyncTCPSocketBase : public AsyncPacketSocket {
 public:
  AsyncTCPSocketBase(AsyncSocket* socket, bool listen, size_t max_packet_size);
  virtual ~AsyncTCPSocketBase();

  
  virtual int Send(const void *pv, size_t cb,
                   const rtc::PacketOptions& options) = 0;
  virtual void ProcessInput(char* data, size_t* len) = 0;
  
  virtual void HandleIncomingConnection(AsyncSocket* socket) = 0;

  virtual SocketAddress GetLocalAddress() const;
  virtual SocketAddress GetRemoteAddress() const;
  virtual int SendTo(const void *pv, size_t cb, const SocketAddress& addr,
                     const rtc::PacketOptions& options);
  virtual int Close();

  virtual State GetState() const;
  virtual int GetOption(Socket::Option opt, int* value);
  virtual int SetOption(Socket::Option opt, int value);
  virtual int GetError() const;
  virtual void SetError(int error);

 protected:
  
  
  
  static AsyncSocket* ConnectSocket(AsyncSocket* socket,
                                    const SocketAddress& bind_address,
                                    const SocketAddress& remote_address);
  virtual int SendRaw(const void* pv, size_t cb);
  int FlushOutBuffer();
  
  void AppendToOutBuffer(const void* pv, size_t cb);

  
  bool IsOutBufferEmpty() const { return outpos_ == 0; }
  void ClearOutBuffer() { outpos_ = 0; }

 private:
  
  void OnConnectEvent(AsyncSocket* socket);
  void OnReadEvent(AsyncSocket* socket);
  void OnWriteEvent(AsyncSocket* socket);
  void OnCloseEvent(AsyncSocket* socket, int error);

  scoped_ptr<AsyncSocket> socket_;
  bool listen_;
  char* inbuf_, * outbuf_;
  size_t insize_, inpos_, outsize_, outpos_;

  DISALLOW_EVIL_CONSTRUCTORS(AsyncTCPSocketBase);
};

class AsyncTCPSocket : public AsyncTCPSocketBase {
 public:
  
  
  
  static AsyncTCPSocket* Create(AsyncSocket* socket,
                                const SocketAddress& bind_address,
                                const SocketAddress& remote_address);
  AsyncTCPSocket(AsyncSocket* socket, bool listen);
  virtual ~AsyncTCPSocket() {}

  virtual int Send(const void* pv, size_t cb,
                   const rtc::PacketOptions& options);
  virtual void ProcessInput(char* data, size_t* len);
  virtual void HandleIncomingConnection(AsyncSocket* socket);

 private:
  DISALLOW_EVIL_CONSTRUCTORS(AsyncTCPSocket);
};

}  

#endif  
