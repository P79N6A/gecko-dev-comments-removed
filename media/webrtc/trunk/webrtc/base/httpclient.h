









#ifndef WEBRTC_BASE_HTTPCLIENT_H__
#define WEBRTC_BASE_HTTPCLIENT_H__

#include "webrtc/base/common.h"
#include "webrtc/base/httpbase.h"
#include "webrtc/base/nethelpers.h"
#include "webrtc/base/proxyinfo.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/sigslot.h"
#include "webrtc/base/socketaddress.h"
#include "webrtc/base/socketpool.h"

namespace rtc {








bool HttpWriteCacheHeaders(const HttpResponseData* response,
                           StreamInterface* output, size_t* size);


bool HttpReadCacheHeaders(StreamInterface* input,
                          HttpResponseData* response,
                          HttpData::HeaderCombine combine);






class DiskCache;
class HttpClient;
class IPNetPool;

class SignalThread;



#if STRICT_HTTP_ERROR
typedef HttpError HttpErrorType;
#else  
typedef int HttpErrorType;
#endif  

class HttpClient : private IHttpNotify, public sigslot::has_slots<> {
public:
  
  
  HttpClient(const std::string& agent, StreamPool* pool,
             HttpTransaction* transaction = NULL);
  virtual ~HttpClient();

  void set_pool(StreamPool* pool) { pool_ = pool; }

  void set_agent(const std::string& agent) { agent_ = agent; }
  const std::string& agent() const { return agent_; }
  
  void set_proxy(const ProxyInfo& proxy) { proxy_ = proxy; }
  const ProxyInfo& proxy() const { return proxy_; }

  
  
  
  
  
  void set_request_retries(size_t retries) { retries_ = retries; }
  size_t request_retries() const { return retries_; }

  enum RedirectAction { REDIRECT_DEFAULT, REDIRECT_ALWAYS, REDIRECT_NEVER };
  void set_redirect_action(RedirectAction action) { redirect_action_ = action; }
  RedirectAction redirect_action() const { return redirect_action_; }
  
  void set_fail_redirect(bool fail_redirect) {
    redirect_action_ = REDIRECT_NEVER;
  }
  bool fail_redirect() const { return (REDIRECT_NEVER == redirect_action_); }

  enum UriForm { URI_DEFAULT, URI_ABSOLUTE, URI_RELATIVE };
  void set_uri_form(UriForm form) { uri_form_ = form; }
  UriForm uri_form() const { return uri_form_; }

  void set_cache(DiskCache* cache) { ASSERT(!IsCacheActive()); cache_ = cache; }
  bool cache_enabled() const { return (NULL != cache_); }

  
  
  void reset();
  
  void set_server(const SocketAddress& address);
  const SocketAddress& server() const { return server_; }

  
  
  
  
  
  HttpTransaction* transaction() { return transaction_; }
  const HttpTransaction* transaction() const { return transaction_; }
  HttpRequestData& request() { return transaction_->request; }
  const HttpRequestData& request() const { return transaction_->request; }
  HttpResponseData& response() { return transaction_->response; }
  const HttpResponseData& response() const { return transaction_->response; }
  
  
  void prepare_get(const std::string& url);
  void prepare_post(const std::string& url, const std::string& content_type,
                    StreamInterface* request_doc);

  
  StreamInterface* GetDocumentStream();

  
  void start();
  
  
  
  
  
  
  
  
  
  sigslot::signal3<HttpClient*,bool,size_t> SignalHeaderAvailable;
  
  sigslot::signal2<HttpClient*,HttpErrorType> SignalHttpClientComplete;

protected:
  void connect();
  void release();

  bool ShouldRedirect(std::string* location) const;

  bool BeginCacheFile();
  HttpError WriteCacheHeaders(const std::string& id);
  void CompleteCacheFile();

  bool CheckCache();
  HttpError ReadCacheHeaders(const std::string& id, bool override);
  HttpError ReadCacheBody(const std::string& id);

  bool PrepareValidate();
  HttpError CompleteValidate();

  HttpError OnHeaderAvailable(bool ignore_data, bool chunked, size_t data_size);

  void StartDNSLookup();
  void OnResolveResult(AsyncResolverInterface* resolver);

  
  virtual HttpError onHttpHeaderComplete(bool chunked, size_t& data_size);
  virtual void onHttpComplete(HttpMode mode, HttpError err);
  virtual void onHttpClosed(HttpError err);
  
private:
  enum CacheState { CS_READY, CS_WRITING, CS_READING, CS_VALIDATING };
  bool IsCacheActive() const { return (cache_state_ > CS_READY); }

  std::string agent_;
  StreamPool* pool_;
  HttpBase base_;
  SocketAddress server_;
  ProxyInfo proxy_;
  HttpTransaction* transaction_;
  bool free_transaction_;
  size_t retries_, attempt_, redirects_;
  RedirectAction redirect_action_;
  UriForm uri_form_;
  scoped_ptr<HttpAuthContext> context_;
  DiskCache* cache_;
  CacheState cache_state_;
  AsyncResolverInterface* resolver_;
};





class HttpClientDefault : public ReuseSocketPool, public HttpClient {
public:
  HttpClientDefault(SocketFactory* factory, const std::string& agent,
                    HttpTransaction* transaction = NULL);
};



}  

#endif
