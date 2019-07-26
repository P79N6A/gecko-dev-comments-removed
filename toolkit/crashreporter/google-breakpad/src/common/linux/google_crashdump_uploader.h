





























#include <string>
#include <map>

#include "common/using_std_string.h"

namespace google_breakpad {

class LibcurlWrapper;

class GoogleCrashdumpUploader {
 public:
  GoogleCrashdumpUploader(const string& product,
                          const string& version,
                          const string& guid,
                          const string& ptime,
                          const string& ctime,
                          const string& email,
                          const string& comments,
                          const string& minidump_pathname,
                          const string& crash_server,
                          const string& proxy_host,
                          const string& proxy_userpassword);

  GoogleCrashdumpUploader(const string& product,
                          const string& version,
                          const string& guid,
                          const string& ptime,
                          const string& ctime,
                          const string& email,
                          const string& comments,
                          const string& minidump_pathname,
                          const string& crash_server,
                          const string& proxy_host,
                          const string& proxy_userpassword,
                          LibcurlWrapper* http_layer);

  void Init(const string& product,
            const string& version,
            const string& guid,
            const string& ptime,
            const string& ctime,
            const string& email,
            const string& comments,
            const string& minidump_pathname,
            const string& crash_server,
            const string& proxy_host,
            const string& proxy_userpassword,
            LibcurlWrapper* http_layer);
  bool Upload();

 private:
  bool CheckRequiredParametersArePresent();

  LibcurlWrapper* http_layer_;
  string product_;
  string version_;
  string guid_;
  string ptime_;
  string ctime_;
  string email_;
  string comments_;
  string minidump_pathname_;

  string crash_server_;
  string proxy_host_;
  string proxy_userpassword_;

  std::map<string, string> parameters_;
};
}
