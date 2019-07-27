









#include "webrtc/base/autodetectproxy.h"
#include "webrtc/base/httpcommon.h"
#include "webrtc/base/httpcommon-inl.h"
#include "webrtc/base/nethelpers.h"

namespace rtc {

static const ProxyType TEST_ORDER[] = {
  PROXY_HTTPS, PROXY_SOCKS5, PROXY_UNKNOWN
};

static const int kSavedStringLimit = 128;

static void SaveStringToStack(char *dst,
                              const std::string &src,
                              size_t dst_size) {
  strncpy(dst, src.c_str(), dst_size - 1);
  dst[dst_size - 1] = '\0';
}

AutoDetectProxy::AutoDetectProxy(const std::string& user_agent)
    : agent_(user_agent), resolver_(NULL), socket_(NULL), next_(0) {
}

AutoDetectProxy::~AutoDetectProxy() {
  if (resolver_) {
    resolver_->Destroy(false);
  }
}

void AutoDetectProxy::DoWork() {
  
  if (!server_url_.empty()) {
    LOG(LS_INFO) << "GetProxySettingsForUrl(" << server_url_ << ") - start";
    GetProxyForUrl(agent_.c_str(), server_url_.c_str(), &proxy_);
    LOG(LS_INFO) << "GetProxySettingsForUrl - stop";
  }
  Url<char> url(proxy_.address.HostAsURIString());
  if (url.valid()) {
    LOG(LS_WARNING) << "AutoDetectProxy removing http prefix on proxy host";
    proxy_.address.SetIP(url.host());
  }
  LOG(LS_INFO) << "AutoDetectProxy found proxy at " << proxy_.address;
  if (proxy_.type == PROXY_UNKNOWN) {
    LOG(LS_INFO) << "AutoDetectProxy initiating proxy classification";
    Next();
    
    Thread::Current()->ProcessMessages(kForever);
    
    delete socket_;
  }
  
  
  
}

void AutoDetectProxy::OnMessage(Message *msg) {
  if (MSG_UNRESOLVABLE == msg->message_id) {
    
    Complete(PROXY_UNKNOWN);
  } else if (MSG_TIMEOUT == msg->message_id) {
    OnCloseEvent(socket_, ETIMEDOUT);
  } else {
    
    
    
    
    
    

    
    
    char agent[kSavedStringLimit];
    SaveStringToStack(agent, agent_, sizeof agent);

    int next = next_;

    
    
    ProxyType type = proxy().type;

    char address_hostname[kSavedStringLimit];
    SaveStringToStack(address_hostname,
                      proxy().address.hostname(),
                      sizeof address_hostname);

    IPAddress address_ip = proxy().address.ipaddr();

    uint16 address_port = proxy().address.port();

    char autoconfig_url[kSavedStringLimit];
    SaveStringToStack(autoconfig_url,
                      proxy().autoconfig_url,
                      sizeof autoconfig_url);

    bool autodetect = proxy().autodetect;

    char bypass_list[kSavedStringLimit];
    SaveStringToStack(bypass_list, proxy().bypass_list, sizeof bypass_list);

    char username[kSavedStringLimit];
    SaveStringToStack(username, proxy().username, sizeof username);

    SignalThread::OnMessage(msg);

    
    
    LOG(LS_SENSITIVE) << agent << " " << next << " " << type << " "
                      << address_hostname << " " << address_ip << " "
                      << address_port << " " << autoconfig_url << " "
                      << autodetect << " " << bypass_list << " " << username;
  }
}

void AutoDetectProxy::OnResolveResult(AsyncResolverInterface* resolver) {
  if (resolver != resolver_) {
    return;
  }
  int error = resolver_->GetError();
  if (error == 0) {
    LOG(LS_VERBOSE) << "Resolved " << proxy_.address << " to "
                    << resolver_->address();
    proxy_.address = resolver_->address();
    if (!DoConnect()) {
      Thread::Current()->Post(this, MSG_TIMEOUT);
    }
  } else {
    LOG(LS_INFO) << "Failed to resolve " << resolver_->address();
    resolver_->Destroy(false);
    resolver_ = NULL;
    proxy_.address = SocketAddress();
    Thread::Current()->Post(this, MSG_UNRESOLVABLE);
  }
}

void AutoDetectProxy::Next() {
  if (TEST_ORDER[next_] >= PROXY_UNKNOWN) {
    Complete(PROXY_UNKNOWN);
    return;
  }

  LOG(LS_VERBOSE) << "AutoDetectProxy connecting to "
                  << proxy_.address.ToSensitiveString();

  if (socket_) {
    Thread::Current()->Clear(this, MSG_TIMEOUT);
    Thread::Current()->Clear(this, MSG_UNRESOLVABLE);
    socket_->Close();
    Thread::Current()->Dispose(socket_);
    socket_ = NULL;
  }
  int timeout = 2000;
  if (proxy_.address.IsUnresolvedIP()) {
    
    timeout += 2000;
    if (!resolver_) {
      resolver_ = new AsyncResolver();
    }
    resolver_->SignalDone.connect(this, &AutoDetectProxy::OnResolveResult);
    resolver_->Start(proxy_.address);
  } else {
    if (!DoConnect()) {
      Thread::Current()->Post(this, MSG_TIMEOUT);
      return;
    }
  }
  Thread::Current()->PostDelayed(timeout, this, MSG_TIMEOUT);
}

bool AutoDetectProxy::DoConnect() {
  if (resolver_) {
    resolver_->Destroy(false);
    resolver_ = NULL;
  }
  socket_ =
      Thread::Current()->socketserver()->CreateAsyncSocket(
          proxy_.address.family(), SOCK_STREAM);
  if (!socket_) {
    LOG(LS_VERBOSE) << "Unable to create socket for " << proxy_.address;
    return false;
  }
  socket_->SignalConnectEvent.connect(this, &AutoDetectProxy::OnConnectEvent);
  socket_->SignalReadEvent.connect(this, &AutoDetectProxy::OnReadEvent);
  socket_->SignalCloseEvent.connect(this, &AutoDetectProxy::OnCloseEvent);
  socket_->Connect(proxy_.address);
  return true;
}

void AutoDetectProxy::Complete(ProxyType type) {
  Thread::Current()->Clear(this, MSG_TIMEOUT);
  Thread::Current()->Clear(this, MSG_UNRESOLVABLE);
  if (socket_) {
    socket_->Close();
  }

  proxy_.type = type;
  LoggingSeverity sev = (proxy_.type == PROXY_UNKNOWN) ? LS_ERROR : LS_INFO;
  LOG_V(sev) << "AutoDetectProxy detected "
             << proxy_.address.ToSensitiveString()
             << " as type " << proxy_.type;

  Thread::Current()->Quit();
}

void AutoDetectProxy::OnConnectEvent(AsyncSocket * socket) {
  std::string probe;

  switch (TEST_ORDER[next_]) {
    case PROXY_HTTPS:
      probe.assign("CONNECT www.google.com:443 HTTP/1.0\r\n"
                   "User-Agent: ");
      probe.append(agent_);
      probe.append("\r\n"
                   "Host: www.google.com\r\n"
                   "Content-Length: 0\r\n"
                   "Proxy-Connection: Keep-Alive\r\n"
                   "\r\n");
      break;
    case PROXY_SOCKS5:
      probe.assign("\005\001\000", 3);
      break;
    default:
      ASSERT(false);
      return;
  }

  LOG(LS_VERBOSE) << "AutoDetectProxy probing type " << TEST_ORDER[next_]
                  << " sending " << probe.size() << " bytes";
  socket_->Send(probe.data(), probe.size());
}

void AutoDetectProxy::OnReadEvent(AsyncSocket * socket) {
  char data[257];
  int len = socket_->Recv(data, 256);
  if (len > 0) {
    data[len] = 0;
    LOG(LS_VERBOSE) << "AutoDetectProxy read " << len << " bytes";
  }

  switch (TEST_ORDER[next_]) {
    case PROXY_HTTPS:
      if ((len >= 2) && (data[0] == '\x05')) {
        Complete(PROXY_SOCKS5);
        return;
      }
      if ((len >= 5) && (strncmp(data, "HTTP/", 5) == 0)) {
        Complete(PROXY_HTTPS);
        return;
      }
      break;
    case PROXY_SOCKS5:
      if ((len >= 2) && (data[0] == '\x05')) {
        Complete(PROXY_SOCKS5);
        return;
      }
      break;
    default:
      ASSERT(false);
      return;
  }

  ++next_;
  Next();
}

void AutoDetectProxy::OnCloseEvent(AsyncSocket * socket, int error) {
  LOG(LS_VERBOSE) << "AutoDetectProxy closed with error: " << error;
  ++next_;
  Next();
}

}  
