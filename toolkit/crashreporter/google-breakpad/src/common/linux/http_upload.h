
































#ifndef COMMON_LINUX_HTTP_UPLOAD_H__
#define COMMON_LINUX_HTTP_UPLOAD_H__

#include <map>
#include <string>

#include "common/using_std_string.h"

namespace google_breakpad {

using std::map;

class HTTPUpload {
 public:
  
  
  
  
  
  
  
  
  
  
  
  
  
  static bool SendRequest(const string &url,
                          const map<string, string> &parameters,
                          const map<string, string> &files,
                          const string &proxy,
                          const string &proxy_user_pwd,
                          const string &ca_certificate_file,
                          string *response_body,
                          long *response_code,
                          string *error_description);

 private:
  
  
  
  static bool CheckParameters(const map<string, string> &parameters);

  
  
  HTTPUpload();
  explicit HTTPUpload(const HTTPUpload &);
  void operator=(const HTTPUpload &);
  ~HTTPUpload();
};

}  

#endif  
