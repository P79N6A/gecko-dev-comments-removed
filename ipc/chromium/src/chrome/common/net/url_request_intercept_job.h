



#ifndef CHROME_COMMON_NET_URL_REQUEST_INTERCEPT_JOB_H_
#define CHROME_COMMON_NET_URL_REQUEST_INTERCEPT_JOB_H_

#include <string>

#include "base/scoped_ptr.h"
#include "net/url_request/url_request_job.h"
#include "chrome/browser/chrome_plugin_host.h"
#include "chrome/common/chrome_plugin_api.h"
#include "chrome/common/chrome_plugin_util.h"
#include "chrome/common/notification_observer.h"

class ChromePluginLib;
class URLRequest;


class URLRequestInterceptJob
    : public URLRequestJob, public NotificationObserver {
 public:
  static URLRequestInterceptJob* FromCPRequest(CPRequest* request) {
    return ScopableCPRequest::GetData<URLRequestInterceptJob*>(request);
  }

  URLRequestInterceptJob(URLRequest* request, ChromePluginLib* plugin,
                         ScopableCPRequest* cprequest);
  virtual ~URLRequestInterceptJob();

  
  void OnStartCompleted(int result);
  void OnReadCompleted(int bytes_read);

  
  virtual void Start();
  virtual void Kill();
  virtual bool ReadRawData(net::IOBuffer* buf, int buf_size, int* bytes_read);
  virtual bool GetMimeType(std::string* mime_type) const;
  virtual bool GetCharset(std::string* charset);
  virtual void GetResponseInfo(net::HttpResponseInfo* info);
  virtual int GetResponseCode() const;
  virtual bool GetContentEncoding(std::string* encoding_type);
  virtual bool IsRedirectResponse(GURL* location, int* http_status_code);

  
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details);
 private:
  void StartAsync();
  void DetachPlugin();

  scoped_ptr<ScopableCPRequest> cprequest_;
  ChromePluginLib* plugin_;
  bool got_headers_;
  net::IOBuffer* read_buffer_;
  int read_buffer_size_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestInterceptJob);
};

#endif  
