




























#include <cassert>
#include <curl/curl.h>
#include <curl/easy.h>
#include <curl/types.h>

#include "common/linux/http_upload.h"

namespace {


static size_t WriteCallback(void *ptr, size_t size,
                            size_t nmemb, void *userp) {
  if (!userp)
    return 0;

  std::string *response = reinterpret_cast<std::string *>(userp);
  size_t real_size = size * nmemb;
  response->append(reinterpret_cast<char *>(ptr), real_size);
  return real_size;
}

}  

namespace google_breakpad {

static const char kUserAgent[] = "Breakpad/1.0 (Linux)";


bool HTTPUpload::SendRequest(const string &url,
                             const map<string, string> &parameters,
                             const string &upload_file,
                             const string &file_part_name,
                             const string &proxy,
                             const string &proxy_user_pwd,
                             string *response_body) {
  if (!CheckParameters(parameters))
    return false;

  CURL *curl = curl_easy_init();
  CURLcode err_code = CURLE_OK;

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, kUserAgent);
    
    if (!proxy.empty())
      curl_easy_setopt(curl, CURLOPT_PROXY, proxy.c_str());
    if (!proxy_user_pwd.empty())
      curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, proxy_user_pwd.c_str());

    struct curl_httppost *formpost = NULL;
    struct curl_httppost *lastptr = NULL;
    
    map<string, string>::const_iterator iter = parameters.begin();
    for (; iter != parameters.end(); ++iter)
      curl_formadd(&formpost, &lastptr,
                   CURLFORM_COPYNAME, iter->first.c_str(),
                   CURLFORM_COPYCONTENTS, iter->second.c_str(),
                   CURLFORM_END);

    
    curl_formadd(&formpost, &lastptr,
                 CURLFORM_COPYNAME, file_part_name.c_str(),
                 CURLFORM_FILE, upload_file.c_str(),
                 CURLFORM_END);

    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

    
    struct curl_slist *headerlist = NULL;
    char buf[] = "Expect:";
    headerlist = curl_slist_append(headerlist, buf);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

    if (response_body != NULL) {
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA,
                       reinterpret_cast<void *>(response_body));
    }

    err_code = curl_easy_perform(curl);
#ifndef NDEBUG
    if (err_code != CURLE_OK)
      fprintf(stderr, "Failed to send http request to %s, error: %s\n",
              url.c_str(),
              curl_easy_strerror(err_code));
#endif

    if (curl != NULL)
      curl_easy_cleanup(curl);
    if (formpost != NULL)
      curl_formfree(formpost);
    if (headerlist != NULL)
      curl_slist_free_all(headerlist);
    return err_code == CURLE_OK;
  }
  return false;
}


bool HTTPUpload::CheckParameters(const map<string, string> &parameters) {
  for (map<string, string>::const_iterator pos = parameters.begin();
       pos != parameters.end(); ++pos) {
    const string &str = pos->first;
    if (str.size() == 0)
      return false;  
    for (unsigned int i = 0; i < str.size(); ++i) {
      int c = str[i];
      if (c < 32 || c == '"' || c > 127) {
        return false;
      }
    }
  }
  return true;
}

}  
