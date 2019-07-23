





























#include <string>
#include <map>

namespace google_breakpad {

class LibcurlWrapper;

class GoogleCrashdumpUploader {
 public:
  GoogleCrashdumpUploader(const std::string& product,
                          const std::string& version,
                          const std::string& guid,
                          const std::string& ptime,
                          const std::string& ctime,
                          const std::string& email,
                          const std::string& comments,
                          const std::string& minidump_pathname,
                          const std::string& crash_server,
                          const std::string& proxy_host,
                          const std::string& proxy_userpassword);

  GoogleCrashdumpUploader(const std::string& product,
                          const std::string& version,
                          const std::string& guid,
                          const std::string& ptime,
                          const std::string& ctime,
                          const std::string& email,
                          const std::string& comments,
                          const std::string& minidump_pathname,
                          const std::string& crash_server,
                          const std::string& proxy_host,
                          const std::string& proxy_userpassword,
                          LibcurlWrapper* http_layer);

  void Init(const std::string& product,
            const std::string& version,
            const std::string& guid,
            const std::string& ptime,
            const std::string& ctime,
            const std::string& email,
            const std::string& comments,
            const std::string& minidump_pathname,
            const std::string& crash_server,
            const std::string& proxy_host,
            const std::string& proxy_userpassword,
            LibcurlWrapper* http_layer);
  bool Upload();

 private:
  bool CheckRequiredParametersArePresent();

  LibcurlWrapper* http_layer_;
  std::string product_;
  std::string version_;
  std::string guid_;
  std::string ptime_;
  std::string ctime_;
  std::string email_;
  std::string comments_;
  std::string minidump_pathname_;

  std::string crash_server_;
  std::string proxy_host_;
  std::string proxy_userpassword_;

  std::map<std::string, std::string> parameters_;
};
}
