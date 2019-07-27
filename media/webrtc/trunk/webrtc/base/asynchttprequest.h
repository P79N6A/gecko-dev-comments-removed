









#ifndef WEBRTC_BASE_ASYNCHTTPREQUEST_H_
#define WEBRTC_BASE_ASYNCHTTPREQUEST_H_

#include <string>
#include "webrtc/base/event.h"
#include "webrtc/base/httpclient.h"
#include "webrtc/base/signalthread.h"
#include "webrtc/base/socketpool.h"
#include "webrtc/base/sslsocketfactory.h"

namespace rtc {

class FirewallManager;







class AsyncHttpRequest : public SignalThread {
 public:
  explicit AsyncHttpRequest(const std::string &user_agent);
  ~AsyncHttpRequest();

  
  
  int start_delay() const { return start_delay_; }
  void set_start_delay(int delay) { start_delay_ = delay; }

  const ProxyInfo& proxy() const { return proxy_; }
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

  
  int timeout() { return timeout_; }
  void set_timeout(int timeout) { timeout_ = timeout; }

  
  bool fail_redirect() const { return fail_redirect_; }
  void set_fail_redirect(bool redirect) { fail_redirect_ = redirect; }

  
  const std::string& response_redirect() { return response_redirect_; }

  HttpRequestData& request() { return client_.request(); }
  HttpResponseData& response() { return client_.response(); }
  HttpErrorType error() { return error_; }

 protected:
  void set_error(HttpErrorType error) { error_ = error; }
  virtual void OnWorkStart();
  virtual void OnWorkStop();
  void OnComplete(HttpClient* client, HttpErrorType error);
  virtual void OnMessage(Message* message);
  virtual void DoWork();

 private:
  void LaunchRequest();

  int start_delay_;
  ProxyInfo proxy_;
  FirewallManager* firewall_;
  std::string host_;
  int port_;
  bool secure_;
  int timeout_;
  bool fail_redirect_;
  SslSocketFactory factory_;
  ReuseSocketPool pool_;
  HttpClient client_;
  HttpErrorType error_;
  std::string response_redirect_;
};

}  

#endif  
