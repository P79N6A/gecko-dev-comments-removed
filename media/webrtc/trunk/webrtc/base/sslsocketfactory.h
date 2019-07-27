









#ifndef WEBRTC_BASE_SSLSOCKETFACTORY_H__
#define WEBRTC_BASE_SSLSOCKETFACTORY_H__

#include "webrtc/base/proxyinfo.h"
#include "webrtc/base/socketserver.h"

namespace rtc {





class SslSocketFactory : public SocketFactory {
 public:
  SslSocketFactory(SocketFactory* factory, const std::string& user_agent)
     : factory_(factory), agent_(user_agent), autodetect_proxy_(true),
       force_connect_(false), logging_level_(LS_VERBOSE), binary_mode_(false),
       ignore_bad_cert_(false) {
  }

  void SetAutoDetectProxy() {
    autodetect_proxy_ = true;
  }
  void SetForceConnect(bool force) {
    force_connect_ = force;
  }
  void SetProxy(const ProxyInfo& proxy) {
    autodetect_proxy_ = false;
    proxy_ = proxy;
  }
  bool autodetect_proxy() const { return autodetect_proxy_; }
  const ProxyInfo& proxy() const { return proxy_; }

  void UseSSL(const char* hostname) { hostname_ = hostname; }
  void DisableSSL() { hostname_.clear(); }
  void SetIgnoreBadCert(bool ignore) { ignore_bad_cert_ = ignore; }
  bool ignore_bad_cert() const { return ignore_bad_cert_; }

  void SetLogging(LoggingSeverity level, const std::string& label, 
                  bool binary_mode = false) {
    logging_level_ = level;
    logging_label_ = label;
    binary_mode_ = binary_mode;
  }

  
  virtual Socket* CreateSocket(int type);
  virtual Socket* CreateSocket(int family, int type);

  virtual AsyncSocket* CreateAsyncSocket(int type);
  virtual AsyncSocket* CreateAsyncSocket(int family, int type);

 private:
  friend class ProxySocketAdapter;
  AsyncSocket* CreateProxySocket(const ProxyInfo& proxy, int family, int type);

  SocketFactory* factory_;
  std::string agent_;
  bool autodetect_proxy_, force_connect_;
  ProxyInfo proxy_;
  std::string hostname_, logging_label_;
  LoggingSeverity logging_level_;
  bool binary_mode_;
  bool ignore_bad_cert_;
};



}  

#endif  
