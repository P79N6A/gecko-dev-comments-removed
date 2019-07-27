









#ifndef WEBRTC_BASE_SOCKETSTREAM_H_
#define WEBRTC_BASE_SOCKETSTREAM_H_

#include "webrtc/base/asyncsocket.h"
#include "webrtc/base/common.h"
#include "webrtc/base/stream.h"

namespace rtc {



class SocketStream : public StreamInterface, public sigslot::has_slots<> {
 public:
  explicit SocketStream(AsyncSocket* socket);
  virtual ~SocketStream();

  void Attach(AsyncSocket* socket);
  AsyncSocket* Detach();

  AsyncSocket* GetSocket() { return socket_; }

  virtual StreamState GetState() const;

  virtual StreamResult Read(void* buffer, size_t buffer_len,
                            size_t* read, int* error);

  virtual StreamResult Write(const void* data, size_t data_len,
                             size_t* written, int* error);

  virtual void Close();

 private:
  void OnConnectEvent(AsyncSocket* socket);
  void OnReadEvent(AsyncSocket* socket);
  void OnWriteEvent(AsyncSocket* socket);
  void OnCloseEvent(AsyncSocket* socket, int err);

  AsyncSocket* socket_;

  DISALLOW_EVIL_CONSTRUCTORS(SocketStream);
};



}  

#endif  
