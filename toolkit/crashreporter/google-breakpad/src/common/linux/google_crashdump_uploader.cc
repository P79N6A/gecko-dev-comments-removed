





























#include "common/linux/google_crashdump_uploader.h"
#include "common/linux/libcurl_wrapper.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>

#include "common/using_std_string.h"

namespace google_breakpad {

GoogleCrashdumpUploader::GoogleCrashdumpUploader(const string& product,
                                                 const string& version,
                                                 const string& guid,
                                                 const string& ptime,
                                                 const string& ctime,
                                                 const string& email,
                                                 const string& comments,
                                                 const string& minidump_pathname,
                                                 const string& crash_server,
                                                 const string& proxy_host,
                                                 const string& proxy_userpassword) {
  LibcurlWrapper* http_layer = new LibcurlWrapper();
  Init(product,
       version,
       guid,
       ptime,
       ctime,
       email,
       comments,
       minidump_pathname,
       crash_server,
       proxy_host,
       proxy_userpassword,
       http_layer);
}

GoogleCrashdumpUploader::GoogleCrashdumpUploader(const string& product,
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
                                                 LibcurlWrapper* http_layer) {
  Init(product,
       version,
       guid,
       ptime,
       ctime,
       email,
       comments,
       minidump_pathname,
       crash_server,
       proxy_host,
       proxy_userpassword,
       http_layer);
}

void GoogleCrashdumpUploader::Init(const string& product,
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
                                   LibcurlWrapper* http_layer) {
  product_ = product;
  version_ = version;
  guid_ = guid;
  ptime_ = ptime;
  ctime_ = ctime;
  email_ = email;
  comments_ = comments;
  http_layer_ = http_layer;

  crash_server_ = crash_server;
  proxy_host_ = proxy_host;
  proxy_userpassword_ = proxy_userpassword;
  minidump_pathname_ = minidump_pathname;
  std::cout << "Uploader initializing";
  std::cout << "\tProduct: " << product_;
  std::cout << "\tVersion: " << version_;
  std::cout << "\tGUID: " << guid_;
  if (!ptime_.empty()) {
    std::cout << "\tProcess uptime: " << ptime_;
  }
  if (!ctime_.empty()) {
    std::cout << "\tCumulative Process uptime: " << ctime_;
  }
  if (!email_.empty()) {
    std::cout << "\tEmail: " << email_;
  }
  if (!comments_.empty()) {
    std::cout << "\tComments: " << comments_;
  }
}

bool GoogleCrashdumpUploader::CheckRequiredParametersArePresent() {
  string error_text;
  if (product_.empty()) {
    error_text.append("\nProduct name must be specified.");
  }

  if (version_.empty()) {
    error_text.append("\nProduct version must be specified.");
  }

  if (guid_.empty()) {
    error_text.append("\nClient ID must be specified.");
  }

  if (minidump_pathname_.empty()) {
    error_text.append("\nMinidump pathname must be specified.");
  }

  if (!error_text.empty()) {
    std::cout << error_text;
    return false;
  }
  return true;

}

bool GoogleCrashdumpUploader::Upload() {
  bool ok = http_layer_->Init();
  if (!ok) {
    std::cout << "http layer init failed";
    return ok;
  }

  if (!CheckRequiredParametersArePresent()) {
    return false;
  }

  struct stat st;
  int err = stat(minidump_pathname_.c_str(), &st);
  if (err) {
    std::cout << minidump_pathname_ << " could not be found";
    return false;
  }

  parameters_["prod"] = product_;
  parameters_["ver"] = version_;
  parameters_["guid"] = guid_;
  parameters_["ptime"] = ptime_;
  parameters_["ctime"] = ctime_;
  parameters_["email"] = email_;
  parameters_["comments_"] = comments_;
  if (!http_layer_->AddFile(minidump_pathname_,
                            "upload_file_minidump")) {
    return false;
  }
  std::cout << "Sending request to " << crash_server_;
  return http_layer_->SendRequest(crash_server_,
                                  parameters_,
                                  NULL);
}
}
