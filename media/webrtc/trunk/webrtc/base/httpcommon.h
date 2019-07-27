









#ifndef WEBRTC_BASE_HTTPCOMMON_H__
#define WEBRTC_BASE_HTTPCOMMON_H__

#include <map>
#include <string>
#include <vector>
#include "webrtc/base/basictypes.h"
#include "webrtc/base/common.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/stringutils.h"
#include "webrtc/base/stream.h"

namespace rtc {

class CryptString;
class SocketAddress;





enum HttpCode {
  HC_OK = 200,
  HC_NON_AUTHORITATIVE = 203,
  HC_NO_CONTENT = 204,
  HC_PARTIAL_CONTENT = 206,

  HC_MULTIPLE_CHOICES = 300,
  HC_MOVED_PERMANENTLY = 301,
  HC_FOUND = 302,
  HC_SEE_OTHER = 303,
  HC_NOT_MODIFIED = 304,
  HC_MOVED_TEMPORARILY = 307,

  HC_BAD_REQUEST = 400,
  HC_UNAUTHORIZED = 401,
  HC_FORBIDDEN = 403,
  HC_NOT_FOUND = 404,
  HC_PROXY_AUTHENTICATION_REQUIRED = 407,
  HC_GONE = 410,

  HC_INTERNAL_SERVER_ERROR = 500,
  HC_NOT_IMPLEMENTED = 501,
  HC_SERVICE_UNAVAILABLE = 503,
};

enum HttpVersion {
  HVER_1_0, HVER_1_1, HVER_UNKNOWN,
  HVER_LAST = HVER_UNKNOWN
};

enum HttpVerb {
  HV_GET, HV_POST, HV_PUT, HV_DELETE, HV_CONNECT, HV_HEAD,
  HV_LAST = HV_HEAD
};

enum HttpError {
  HE_NONE,
  HE_PROTOCOL,            
  HE_DISCONNECTED,        
  HE_OVERFLOW,            
  HE_CONNECT_FAILED,      
  HE_SOCKET_ERROR,        
  HE_SHUTDOWN,            
  HE_OPERATION_CANCELLED, 
  HE_AUTH,                
  HE_CERTIFICATE_EXPIRED, 
  HE_STREAM,              
  HE_CACHE,               
  HE_DEFAULT
};

enum HttpHeader {
  HH_AGE,
  HH_CACHE_CONTROL,
  HH_CONNECTION,
  HH_CONTENT_DISPOSITION,
  HH_CONTENT_LENGTH,
  HH_CONTENT_RANGE,
  HH_CONTENT_TYPE,
  HH_COOKIE,
  HH_DATE,
  HH_ETAG,
  HH_EXPIRES,
  HH_HOST,
  HH_IF_MODIFIED_SINCE,
  HH_IF_NONE_MATCH,
  HH_KEEP_ALIVE,
  HH_LAST_MODIFIED,
  HH_LOCATION,
  HH_PROXY_AUTHENTICATE,
  HH_PROXY_AUTHORIZATION,
  HH_PROXY_CONNECTION,
  HH_RANGE,
  HH_SET_COOKIE,
  HH_TE,
  HH_TRAILERS,
  HH_TRANSFER_ENCODING,
  HH_UPGRADE,
  HH_USER_AGENT,
  HH_WWW_AUTHENTICATE,
  HH_LAST = HH_WWW_AUTHENTICATE
};

const uint16 HTTP_DEFAULT_PORT = 80;
const uint16 HTTP_SECURE_PORT = 443;





inline HttpError mkerr(HttpError err, HttpError def_err = HE_DEFAULT) {
  return (err != HE_NONE) ? err : def_err;
}

const char* ToString(HttpVersion version);
bool FromString(HttpVersion& version, const std::string& str);

const char* ToString(HttpVerb verb);
bool FromString(HttpVerb& verb, const std::string& str);

const char* ToString(HttpHeader header);
bool FromString(HttpHeader& header, const std::string& str);

inline bool HttpCodeIsInformational(uint32 code) { return ((code / 100) == 1); }
inline bool HttpCodeIsSuccessful(uint32 code)    { return ((code / 100) == 2); }
inline bool HttpCodeIsRedirection(uint32 code)   { return ((code / 100) == 3); }
inline bool HttpCodeIsClientError(uint32 code)   { return ((code / 100) == 4); }
inline bool HttpCodeIsServerError(uint32 code)   { return ((code / 100) == 5); }

bool HttpCodeHasBody(uint32 code);
bool HttpCodeIsCacheable(uint32 code);
bool HttpHeaderIsEndToEnd(HttpHeader header);
bool HttpHeaderIsCollapsible(HttpHeader header);

struct HttpData;
bool HttpShouldKeepAlive(const HttpData& data);

typedef std::pair<std::string, std::string> HttpAttribute;
typedef std::vector<HttpAttribute> HttpAttributeList;
void HttpComposeAttributes(const HttpAttributeList& attributes, char separator,
                           std::string* composed);
void HttpParseAttributes(const char * data, size_t len,
                         HttpAttributeList& attributes);
bool HttpHasAttribute(const HttpAttributeList& attributes,
                      const std::string& name,
                      std::string* value);
bool HttpHasNthAttribute(HttpAttributeList& attributes,
                         size_t index,
                         std::string* name,
                         std::string* value);


bool HttpDateToSeconds(const std::string& date, time_t* seconds);

inline uint16 HttpDefaultPort(bool secure) {
  return secure ? HTTP_SECURE_PORT : HTTP_DEFAULT_PORT;
}


std::string HttpAddress(const SocketAddress& address, bool secure);


struct iless {
  bool operator()(const std::string& lhs, const std::string& rhs) const {
    return (::_stricmp(lhs.c_str(), rhs.c_str()) < 0);
  }
};


std::string quote(const std::string& str);





template<class CTYPE>
class Url {
public:
  typedef typename Traits<CTYPE>::string string;

  
  static int Encode(const CTYPE* source, CTYPE* destination, size_t len);
  static int Encode(const string& source, string& destination);
  static int Decode(const CTYPE* source, CTYPE* destination, size_t len);
  static int Decode(const string& source, string& destination);

  Url(const string& url) { do_set_url(url.c_str(), url.size()); }
  Url(const string& path, const string& host, uint16 port = HTTP_DEFAULT_PORT)
  : host_(host), port_(port), secure_(HTTP_SECURE_PORT == port)
  { set_full_path(path); }

  bool valid() const { return !host_.empty(); }
  void clear() {
    host_.clear();
    port_ = HTTP_DEFAULT_PORT;
    secure_ = false;
    path_.assign(1, static_cast<CTYPE>('/'));
    query_.clear();
  }

  void set_url(const string& val) {
    do_set_url(val.c_str(), val.size());
  }
  string url() const {
    string val; do_get_url(&val); return val;
  }

  void set_address(const string& val) {
    do_set_address(val.c_str(), val.size());
  }
  string address() const {
    string val; do_get_address(&val); return val;
  }

  void set_full_path(const string& val) {
    do_set_full_path(val.c_str(), val.size());
  }
  string full_path() const {
    string val; do_get_full_path(&val); return val;
  }

  void set_host(const string& val) { host_ = val; }
  const string& host() const { return host_; }

  void set_port(uint16 val) { port_ = val; }
  uint16 port() const { return port_; }

  void set_secure(bool val) { secure_ = val; }
  bool secure() const { return secure_; }

  void set_path(const string& val) {
    if (val.empty()) {
      path_.assign(1, static_cast<CTYPE>('/'));
    } else {
      ASSERT(val[0] == static_cast<CTYPE>('/'));
      path_ = val;
    }
  }
  const string& path() const { return path_; }

  void set_query(const string& val) {
    ASSERT(val.empty() || (val[0] == static_cast<CTYPE>('?')));
    query_ = val;
  }
  const string& query() const { return query_; }

  bool get_attribute(const string& name, string* value) const;

private:
  void do_set_url(const CTYPE* val, size_t len);
  void do_set_address(const CTYPE* val, size_t len);
  void do_set_full_path(const CTYPE* val, size_t len);

  void do_get_url(string* val) const;
  void do_get_address(string* val) const;
  void do_get_full_path(string* val) const;

  string host_, path_, query_;
  uint16 port_;
  bool secure_;
};





struct HttpData {
  typedef std::multimap<std::string, std::string, iless> HeaderMap;
  typedef HeaderMap::const_iterator const_iterator;
  typedef HeaderMap::iterator iterator;

  HttpVersion version;
  scoped_ptr<StreamInterface> document;

  HttpData() : version(HVER_1_1) { }

  enum HeaderCombine { HC_YES, HC_NO, HC_AUTO, HC_REPLACE, HC_NEW };
  void changeHeader(const std::string& name, const std::string& value,
                    HeaderCombine combine);
  inline void addHeader(const std::string& name, const std::string& value,
                        bool append = true) {
    changeHeader(name, value, append ? HC_AUTO : HC_NO);
  }
  inline void setHeader(const std::string& name, const std::string& value,
                        bool overwrite = true) {
    changeHeader(name, value, overwrite ? HC_REPLACE : HC_NEW);
  }
  
  size_t clearHeader(const std::string& name);
  
  iterator clearHeader(iterator header);

  
  bool hasHeader(const std::string& name, std::string* value) const;

  inline const_iterator begin() const {
    return headers_.begin();
  }
  inline const_iterator end() const {
    return headers_.end();
  }
  inline iterator begin() {
    return headers_.begin();
  }
  inline iterator end() {
    return headers_.end();
  }
  inline const_iterator begin(const std::string& name) const {
    return headers_.lower_bound(name);
  }
  inline const_iterator end(const std::string& name) const {
    return headers_.upper_bound(name);
  }
  inline iterator begin(const std::string& name) {
    return headers_.lower_bound(name);
  }
  inline iterator end(const std::string& name) {
    return headers_.upper_bound(name);
  }

  
  inline void changeHeader(HttpHeader header, const std::string& value,
                           HeaderCombine combine) {
    changeHeader(ToString(header), value, combine);
  }
  inline void addHeader(HttpHeader header, const std::string& value,
                        bool append = true) {
    addHeader(ToString(header), value, append);
  }
  inline void setHeader(HttpHeader header, const std::string& value,
                        bool overwrite = true) {
    setHeader(ToString(header), value, overwrite);
  }
  inline void clearHeader(HttpHeader header) {
    clearHeader(ToString(header));
  }
  inline bool hasHeader(HttpHeader header, std::string* value) const {
    return hasHeader(ToString(header), value);
  }
  inline const_iterator begin(HttpHeader header) const {
    return headers_.lower_bound(ToString(header));
  }
  inline const_iterator end(HttpHeader header) const {
    return headers_.upper_bound(ToString(header));
  }
  inline iterator begin(HttpHeader header) {
    return headers_.lower_bound(ToString(header));
  }
  inline iterator end(HttpHeader header) {
    return headers_.upper_bound(ToString(header));
  }

  void setContent(const std::string& content_type, StreamInterface* document);
  void setDocumentAndLength(StreamInterface* document);

  virtual size_t formatLeader(char* buffer, size_t size) const = 0;
  virtual HttpError parseLeader(const char* line, size_t len) = 0;

protected:
  virtual ~HttpData() { }
  void clear(bool release_document);
  void copy(const HttpData& src);

private:
  HeaderMap headers_;
};

struct HttpRequestData : public HttpData {
  HttpVerb verb;
  std::string path;

  HttpRequestData() : verb(HV_GET) { }

  void clear(bool release_document);
  void copy(const HttpRequestData& src);

  virtual size_t formatLeader(char* buffer, size_t size) const;
  virtual HttpError parseLeader(const char* line, size_t len);

  bool getAbsoluteUri(std::string* uri) const;
  bool getRelativeUri(std::string* host, std::string* path) const;
};

struct HttpResponseData : public HttpData {
  uint32 scode;
  std::string message;

  HttpResponseData() : scode(HC_INTERNAL_SERVER_ERROR) { }
  void clear(bool release_document);
  void copy(const HttpResponseData& src);

  
  void set_success(uint32 scode = HC_OK);
  void set_success(const std::string& content_type, StreamInterface* document,
                   uint32 scode = HC_OK);
  void set_redirect(const std::string& location,
                    uint32 scode = HC_MOVED_TEMPORARILY);
  void set_error(uint32 scode);

  virtual size_t formatLeader(char* buffer, size_t size) const;
  virtual HttpError parseLeader(const char* line, size_t len);
};

struct HttpTransaction {
  HttpRequestData request;
  HttpResponseData response;
};





struct HttpAuthContext {
  std::string auth_method;
  HttpAuthContext(const std::string& auth) : auth_method(auth) { }
  virtual ~HttpAuthContext() { }
};

enum HttpAuthResult { HAR_RESPONSE, HAR_IGNORE, HAR_CREDENTIALS, HAR_ERROR };




HttpAuthResult HttpAuthenticate(
  const char * challenge, size_t len,
  const SocketAddress& server,
  const std::string& method, const std::string& uri,
  const std::string& username, const CryptString& password,
  HttpAuthContext *& context, std::string& response, std::string& auth_method);



} 

#endif 
