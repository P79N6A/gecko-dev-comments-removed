































#include <string>
#include <map>

#include "common/using_std_string.h"
#include "third_party/curl/curl.h"

namespace google_breakpad {
class LibcurlWrapper {
 public:
  LibcurlWrapper();
  virtual bool Init();
  virtual bool SetProxy(const string& proxy_host,
                        const string& proxy_userpwd);
  virtual bool AddFile(const string& upload_file_path,
                       const string& basename);
  virtual bool SendRequest(const string& url,
                           const std::map<string, string>& parameters,
                           string* server_response);
 private:
  
  
  bool SetFunctionPointers();

  bool init_ok_;                 
  void* curl_lib_;               
                                 
  string last_curl_error_;  
                                 
  

  CURL *curl_;                   

  CURL* (*easy_init_)(void);

  
  struct curl_httppost *formpost_;
  struct curl_httppost *lastptr_;
  struct curl_slist *headerlist_;

  
  CURLcode (*easy_setopt_)(CURL *, CURLoption, ...);
  CURLFORMcode (*formadd_)(struct curl_httppost **,
                           struct curl_httppost **, ...);
  struct curl_slist* (*slist_append_)(struct curl_slist *, const char *);
  void (*slist_free_all_)(struct curl_slist *);
  CURLcode (*easy_perform_)(CURL *);
  const char* (*easy_strerror_)(CURLcode);
  void (*easy_cleanup_)(CURL *);
  void (*formfree_)(struct curl_httppost *);

};
}
