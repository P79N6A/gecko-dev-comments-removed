









#include "webrtc/base/httprequest.h"

#include "webrtc/base/common.h"
#include "webrtc/base/firewallsocketserver.h"
#include "webrtc/base/httpclient.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/physicalsocketserver.h"
#include "webrtc/base/socketadapters.h"
#include "webrtc/base/socketpool.h"
#include "webrtc/base/ssladapter.h"

using namespace rtc;





HttpMonitor::HttpMonitor(SocketServer *ss) {
  ASSERT(Thread::Current() != NULL);
  ss_ = ss;
  reset();
}

void HttpMonitor::Connect(HttpClient *http) {
  http->SignalHttpClientComplete.connect(this,
    &HttpMonitor::OnHttpClientComplete);
}

void HttpMonitor::OnHttpClientComplete(HttpClient * http, HttpErrorType error) {
  complete_ = true;
  error_ = error;
  ss_->WakeUp();
}





const int kDefaultHTTPTimeout = 30 * 1000; 

HttpRequest::HttpRequest(const std::string &user_agent)
    : firewall_(0), port_(80), secure_(false),
      timeout_(kDefaultHTTPTimeout), fail_redirect_(false),
      client_(user_agent.c_str(), NULL), error_(HE_NONE) {
}

void HttpRequest::Send() {
  
  

  PhysicalSocketServer physical;
  SocketServer * ss = &physical;
  if (firewall_) {
    ss = new FirewallSocketServer(ss, firewall_);
  }

  SslSocketFactory factory(ss, client_.agent());
  factory.SetProxy(proxy_);
  if (secure_)
    factory.UseSSL(host_.c_str());

  

  ReuseSocketPool pool(&factory);
  client_.set_pool(&pool);

  bool transparent_proxy = (port_ == 80) && ((proxy_.type == PROXY_HTTPS) ||
                           (proxy_.type == PROXY_UNKNOWN));

  if (transparent_proxy) {
    client_.set_proxy(proxy_);
  }
  client_.set_fail_redirect(fail_redirect_);

  SocketAddress server(host_, port_);
  client_.set_server(server);

  LOG(LS_INFO) << "HttpRequest start: " << host_ + client_.request().path;

  HttpMonitor monitor(ss);
  monitor.Connect(&client_);
  client_.start();
  ss->Wait(timeout_, true);
  if (!monitor.done()) {
    LOG(LS_INFO) << "HttpRequest request timed out";
    client_.reset();
    return;
  }

  set_error(monitor.error());
  if (error_) {
    LOG(LS_INFO) << "HttpRequest request error: " << error_;
    return;
  }

  std::string value;
  if (client_.response().hasHeader(HH_LOCATION, &value)) {
    response_redirect_ = value.c_str();
  }
}
