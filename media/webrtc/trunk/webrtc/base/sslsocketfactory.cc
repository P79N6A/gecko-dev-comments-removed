









#include "webrtc/base/autodetectproxy.h"
#include "webrtc/base/httpcommon.h"
#include "webrtc/base/httpcommon-inl.h"
#include "webrtc/base/socketadapters.h"
#include "webrtc/base/ssladapter.h"
#include "webrtc/base/sslsocketfactory.h"

namespace rtc {








class ProxySocketAdapter : public AsyncSocketAdapter {
 public:
  ProxySocketAdapter(SslSocketFactory* factory, int family, int type)
      : AsyncSocketAdapter(NULL), factory_(factory), family_(family),
        type_(type), detect_(NULL) {
  }
  virtual ~ProxySocketAdapter() {
    Close();
  }

  virtual int Connect(const SocketAddress& addr) {
    ASSERT(NULL == detect_);
    ASSERT(NULL == socket_);
    remote_ = addr;
    if (remote_.IsAnyIP() && remote_.hostname().empty()) {
      LOG_F(LS_ERROR) << "Empty address";
      return SOCKET_ERROR;
    }
    Url<char> url("/", remote_.HostAsURIString(), remote_.port());
    detect_ = new AutoDetectProxy(factory_->agent_);
    detect_->set_server_url(url.url());
    detect_->SignalWorkDone.connect(this,
        &ProxySocketAdapter::OnProxyDetectionComplete);
    detect_->Start();
    return SOCKET_ERROR;
  }
  virtual int GetError() const {
    if (socket_) {
      return socket_->GetError();
    }
    return detect_ ? EWOULDBLOCK : EADDRNOTAVAIL;
  }
  virtual int Close() {
    if (socket_) {
      return socket_->Close();
    }
    if (detect_) {
      detect_->Destroy(false);
      detect_ = NULL;
    }
    return 0;
  }
  virtual ConnState GetState() const {
    if (socket_) {
      return socket_->GetState();
    }
    return detect_ ? CS_CONNECTING : CS_CLOSED;
  }

private:
  
  void OnProxyDetectionComplete(SignalThread* thread) {
    ASSERT(detect_ == thread);
    Attach(factory_->CreateProxySocket(detect_->proxy(), family_, type_));
    detect_->Release();
    detect_ = NULL;
    if (0 == AsyncSocketAdapter::Connect(remote_)) {
      SignalConnectEvent(this);
    } else if (!IsBlockingError(socket_->GetError())) {
      SignalCloseEvent(this, socket_->GetError());
    }
  }

  SslSocketFactory* factory_;
  int family_;
  int type_;
  SocketAddress remote_;
  AutoDetectProxy* detect_;
};





Socket* SslSocketFactory::CreateSocket(int type) {
  return CreateSocket(AF_INET, type);
}

Socket* SslSocketFactory::CreateSocket(int family, int type) {
  return factory_->CreateSocket(family, type);
}

AsyncSocket* SslSocketFactory::CreateAsyncSocket(int type) {
  return CreateAsyncSocket(AF_INET, type);
}

AsyncSocket* SslSocketFactory::CreateAsyncSocket(int family, int type) {
  if (autodetect_proxy_) {
    return new ProxySocketAdapter(this, family, type);
  } else {
    return CreateProxySocket(proxy_, family, type);
  }
}


AsyncSocket* SslSocketFactory::CreateProxySocket(const ProxyInfo& proxy,
                                                 int family,
                                                 int type) {
  AsyncSocket* socket = factory_->CreateAsyncSocket(family, type);
  if (!socket)
    return NULL;

  
  if (!logging_label_.empty() && binary_mode_) {
    socket = new LoggingSocketAdapter(socket, logging_level_,
                                      logging_label_.c_str(), binary_mode_);
  }

  if (proxy.type) {
    AsyncSocket* proxy_socket = 0;
    if (proxy_.type == PROXY_SOCKS5) {
      proxy_socket = new AsyncSocksProxySocket(socket, proxy.address,
                                               proxy.username, proxy.password);
    } else {
      
      AsyncHttpsProxySocket* http_proxy =
          new AsyncHttpsProxySocket(socket, agent_, proxy.address,
                                    proxy.username, proxy.password);
      http_proxy->SetForceConnect(force_connect_ || !hostname_.empty());
      proxy_socket = http_proxy;
    }
    if (!proxy_socket) {
      delete socket;
      return NULL;
    }
    socket = proxy_socket;  
  }

  if (!hostname_.empty()) {
    if (SSLAdapter* ssl_adapter = SSLAdapter::Create(socket)) {
      ssl_adapter->set_ignore_bad_cert(ignore_bad_cert_);
      ssl_adapter->StartSSL(hostname_.c_str(), true);
      socket = ssl_adapter;
    } else {
      LOG_F(LS_ERROR) << "SSL unavailable";
    }
  }

  
  if (!logging_label_.empty() && !binary_mode_) {
    socket = new LoggingSocketAdapter(socket, logging_level_,
                                      logging_label_.c_str(), binary_mode_);
  }
  return socket;
}



}  
