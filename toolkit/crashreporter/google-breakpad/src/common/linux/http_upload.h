
































#ifndef COMMON_LINUX_HTTP_UPLOAD_H__
#define COMMON_LINUX_HTTP_UPLOAD_H__

#include <map>
#include <string>

namespace google_breakpad {

using std::string;
using std::map;

class HTTPUpload {
 public:
  
  
  
  
  
  
  
  
  
  static bool SendRequest(const string &url,
                          const map<string, string> &parameters,
                          const string &upload_file,
                          const string &file_part_name,
                          const string &proxy,
                          const string &proxy_user_pwd,
                          string *response_body);

 private:
  
  
  
  static bool CheckParameters(const map<string, string> &parameters);

  
  
  HTTPUpload();
  explicit HTTPUpload(const HTTPUpload &);
  void operator=(const HTTPUpload &);
  ~HTTPUpload();
};

}  

#endif  
