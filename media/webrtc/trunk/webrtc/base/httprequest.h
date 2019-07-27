









#ifndef _HTTPREQUEST_H_
#define _HTTPREQUEST_H_

#include "webrtc/base/httpclient.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/proxyinfo.h"
#include "webrtc/base/socketserver.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/sslsocketfactory.h"  

namespace rtc {





class FirewallManager;
class MemoryStream;

class HttpRequest {
public:
  HttpRequest(const std::string &user_agent);

  void Send();

  void set_proxy(const ProxyInfo& proxy) {
    proxy_ = proxy;
  }
  void set_firewall(FirewallManager * firewall) {
    firewall_ = firewall;
  }

  
  const std::string& host() { return host_; }
  void set_host(const std::string& host) { host_ = host; }

  
  int port() { return port_; }
  void set_port(int port) { port_ = port; }

   
  bool secure() { return secure_; }
  void set_secure(bool secure) { secure_ = secure; }

  
  const std::string& response_redirect() { return response_redirect_; }

  
  int timeout() { return timeout_; }
  void set_timeout(int timeout) { timeout_ = timeout; }

  
  bool fail_redirect() const { return fail_redirect_; }
  void set_fail_redirect(bool fail_redirect) { fail_redirect_ = fail_redirect; }

  HttpRequestData& request() { return client_.request(); }
  HttpResponseData& response() { return client_.response(); }
  HttpErrorType error() { return error_; }

protected:
  void set_error(HttpErrorType error) { error_ = error; }

private:
  ProxyInfo proxy_;
  FirewallManager * firewall_;
  std::string host_;
  int port_;
  bool secure_;
  int timeout_;
  bool fail_redirect_;
  HttpClient client_;
  HttpErrorType error_;
  std::string response_redirect_;
};





class HttpMonitor : public sigslot::has_slots<> {
public:
  HttpMonitor(SocketServer *ss);

  void reset() {
    complete_ = false;
    error_ = HE_DEFAULT;
  }

  bool done() const { return complete_; }
  HttpErrorType error() const { return error_; }

  void Connect(HttpClient* http);
  void OnHttpClientComplete(HttpClient * http, HttpErrorType error);

private:
  bool complete_;
  HttpErrorType error_;
  SocketServer *ss_;
};



}  

#endif  
