









#include "webrtc/base/asyncsocket.h"

namespace rtc {

AsyncSocket::AsyncSocket() {
}

AsyncSocket::~AsyncSocket() {
}

AsyncSocketAdapter::AsyncSocketAdapter(AsyncSocket* socket) : socket_(NULL) {
  Attach(socket);
}

AsyncSocketAdapter::~AsyncSocketAdapter() {
  delete socket_;
}

void AsyncSocketAdapter::Attach(AsyncSocket* socket) {
  ASSERT(!socket_);
  socket_ = socket;
  if (socket_) {
    socket_->SignalConnectEvent.connect(this,
        &AsyncSocketAdapter::OnConnectEvent);
    socket_->SignalReadEvent.connect(this,
        &AsyncSocketAdapter::OnReadEvent);
    socket_->SignalWriteEvent.connect(this,
        &AsyncSocketAdapter::OnWriteEvent);
    socket_->SignalCloseEvent.connect(this,
        &AsyncSocketAdapter::OnCloseEvent);
  }
}

}  
