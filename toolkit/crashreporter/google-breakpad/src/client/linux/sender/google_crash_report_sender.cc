




























#include "common/linux/google_crashdump_uploader.h"
#include "third_party/linux/include/gflags/gflags.h"
#include <string>
#include <iostream>

#include "common/using_std_string.h"

DEFINE_string(crash_server, "https://clients2.google.com/cr",
              "The crash server to upload minidumps to.");
DEFINE_string(product_name, "",
              "The product name that the minidump corresponds to.");
DEFINE_string(product_version, "",
              "The version of the product that produced the minidump.");
DEFINE_string(client_id, "",
              "The client GUID");
DEFINE_string(minidump_path, "",
              "The path of the minidump file.");
DEFINE_string(ptime, "",
              "The process uptime in milliseconds.");
DEFINE_string(ctime, "",
              "The cumulative process uptime in milliseconds.");
DEFINE_string(email, "",
              "The user's email address.");
DEFINE_string(comments, "",
              "Extra user comments");
DEFINE_string(proxy_host, "",
              "Proxy host");
DEFINE_string(proxy_userpasswd, "",
              "Proxy username/password in user:pass format.");


bool CheckForRequiredFlagsOrDie() {
  string error_text = "";
  if (FLAGS_product_name.empty()) {
    error_text.append("\nProduct name must be specified.");
  }

  if (FLAGS_product_version.empty()) {
    error_text.append("\nProduct version must be specified.");
  }

  if (FLAGS_client_id.empty()) {
    error_text.append("\nClient ID must be specified.");
  }

  if (FLAGS_minidump_path.empty()) {
    error_text.append("\nMinidump pathname must be specified.");
  }

  if (!error_text.empty()) {
    std::cout << error_text;
    return false;
  }
  return true;
}

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);
  if (!CheckForRequiredFlagsOrDie()) {
    return 1;
  }
  google_breakpad::GoogleCrashdumpUploader g(FLAGS_product_name,
                                             FLAGS_product_version,
                                             FLAGS_client_id,
                                             FLAGS_ptime,
                                             FLAGS_ctime,
                                             FLAGS_email,
                                             FLAGS_comments,
                                             FLAGS_minidump_path,
                                             FLAGS_crash_server,
                                             FLAGS_proxy_host,
                                             FLAGS_proxy_userpasswd);
  g.Upload();
}
