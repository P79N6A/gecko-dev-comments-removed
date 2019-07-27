




























#include "common/linux/http_upload.h"

#include <assert.h>
#include <dlfcn.h>
#include "third_party/curl/curl.h"

namespace {


static size_t WriteCallback(void *ptr, size_t size,
                            size_t nmemb, void *userp) {
  if (!userp)
    return 0;

  string *response = reinterpret_cast<string *>(userp);
  size_t real_size = size * nmemb;
  response->append(reinterpret_cast<char *>(ptr), real_size);
  return real_size;
}

}  

namespace google_breakpad {

static const char kUserAgent[] = "Breakpad/1.0 (Linux)";


bool HTTPUpload::SendRequest(const string &url,
                             const map<string, string> &parameters,
                             const map<string, string> &files,
                             const string &proxy,
                             const string &proxy_user_pwd,
                             const string &ca_certificate_file,
                             string *response_body,
                             long *response_code,
                             string *error_description) {
  if (response_code != NULL)
    *response_code = 0;

  if (!CheckParameters(parameters))
    return false;

  void *curl_lib = dlopen("libcurl.so", RTLD_NOW);
  if (!curl_lib) {
    if (error_description != NULL)
      *error_description = dlerror();
    curl_lib = dlopen("libcurl.so.4", RTLD_NOW);
  }
  if (!curl_lib) {
    
    
    curl_lib = dlopen("libcurl-gnutls.so.4", RTLD_NOW);
  }
  if (!curl_lib) {
    curl_lib = dlopen("libcurl.so.3", RTLD_NOW);
  }
  if (!curl_lib) {
    return false;
  }

  CURL* (*curl_easy_init)(void);
  *(void**) (&curl_easy_init) = dlsym(curl_lib, "curl_easy_init");
  CURL *curl = (*curl_easy_init)();
  if (error_description != NULL)
    *error_description = "No Error";

  if (!curl) {
    dlclose(curl_lib);
    return false;
  }

  CURLcode err_code = CURLE_OK;
  CURLcode (*curl_easy_setopt)(CURL *, CURLoption, ...);
  *(void**) (&curl_easy_setopt) = dlsym(curl_lib, "curl_easy_setopt");
  (*curl_easy_setopt)(curl, CURLOPT_URL, url.c_str());
  (*curl_easy_setopt)(curl, CURLOPT_USERAGENT, kUserAgent);
  
  if (!proxy.empty())
    (*curl_easy_setopt)(curl, CURLOPT_PROXY, proxy.c_str());
  if (!proxy_user_pwd.empty())
    (*curl_easy_setopt)(curl, CURLOPT_PROXYUSERPWD, proxy_user_pwd.c_str());

  if (!ca_certificate_file.empty())
    (*curl_easy_setopt)(curl, CURLOPT_CAINFO, ca_certificate_file.c_str());

  struct curl_httppost *formpost = NULL;
  struct curl_httppost *lastptr = NULL;
  
  CURLFORMcode (*curl_formadd)(struct curl_httppost **, struct curl_httppost **, ...);
  *(void**) (&curl_formadd) = dlsym(curl_lib, "curl_formadd");
  map<string, string>::const_iterator iter = parameters.begin();
  for (; iter != parameters.end(); ++iter)
    (*curl_formadd)(&formpost, &lastptr,
                 CURLFORM_COPYNAME, iter->first.c_str(),
                 CURLFORM_COPYCONTENTS, iter->second.c_str(),
                 CURLFORM_END);

  
  for (iter = files.begin(); iter != files.end(); ++iter) {
    (*curl_formadd)(&formpost, &lastptr,
                 CURLFORM_COPYNAME, iter->first.c_str(),
                 CURLFORM_FILE, iter->second.c_str(),
                 CURLFORM_END);
  }

  (*curl_easy_setopt)(curl, CURLOPT_HTTPPOST, formpost);

  
  struct curl_slist *headerlist = NULL;
  char buf[] = "Expect:";
  struct curl_slist* (*curl_slist_append)(struct curl_slist *, const char *);
  *(void**) (&curl_slist_append) = dlsym(curl_lib, "curl_slist_append");
  headerlist = (*curl_slist_append)(headerlist, buf);
  (*curl_easy_setopt)(curl, CURLOPT_HTTPHEADER, headerlist);

  if (response_body != NULL) {
    (*curl_easy_setopt)(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    (*curl_easy_setopt)(curl, CURLOPT_WRITEDATA,
                     reinterpret_cast<void *>(response_body));
  }

  
  (*curl_easy_setopt)(curl, CURLOPT_FAILONERROR, 1);

  CURLcode (*curl_easy_perform)(CURL *);
  *(void**) (&curl_easy_perform) = dlsym(curl_lib, "curl_easy_perform");
  err_code = (*curl_easy_perform)(curl);
  if (response_code != NULL) {
    CURLcode (*curl_easy_getinfo)(CURL *, CURLINFO, ...);
    *(void**) (&curl_easy_getinfo) = dlsym(curl_lib, "curl_easy_getinfo");
    (*curl_easy_getinfo)(curl, CURLINFO_RESPONSE_CODE, response_code);
  }
  const char* (*curl_easy_strerror)(CURLcode);
  *(void**) (&curl_easy_strerror) = dlsym(curl_lib, "curl_easy_strerror");
#ifndef NDEBUG
  if (err_code != CURLE_OK)
    fprintf(stderr, "Failed to send http request to %s, error: %s\n",
            url.c_str(),
            (*curl_easy_strerror)(err_code));
#endif
  if (error_description != NULL)
    *error_description = (*curl_easy_strerror)(err_code);

  void (*curl_easy_cleanup)(CURL *);
  *(void**) (&curl_easy_cleanup) = dlsym(curl_lib, "curl_easy_cleanup");
  (*curl_easy_cleanup)(curl);
  if (formpost != NULL) {
    void (*curl_formfree)(struct curl_httppost *);
    *(void**) (&curl_formfree) = dlsym(curl_lib, "curl_formfree");
    (*curl_formfree)(formpost);
  }
  if (headerlist != NULL) {
    void (*curl_slist_free_all)(struct curl_slist *);
    *(void**) (&curl_slist_free_all) = dlsym(curl_lib, "curl_slist_free_all");
    (*curl_slist_free_all)(headerlist);
  }
  dlclose(curl_lib);
  return err_code == CURLE_OK;
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
