
































#ifndef COMMON_WINDOWS_HTTP_UPLOAD_H__
#define COMMON_WINDOWS_HTTP_UPLOAD_H__

#pragma warning( push )

#pragma warning( disable : 4530 ) 

#include <windows.h>
#include <wininet.h>

#include <map>
#include <string>
#include <vector>

namespace google_breakpad {

using std::string;
using std::wstring;
using std::map;
using std::vector;

class HTTPUpload {
 public:
  
  
  
  
  
  
  
  
  
  
  
  static bool SendRequest(const wstring &url,
                          const map<wstring, wstring> &parameters,
                          const map<wstring, wstring> &files,
                          int *timeout,
                          wstring *response_body,
                          int *response_code);

 private:
  class AutoInternetHandle;

  
  
  
  
  static bool ReadResponse(HINTERNET request, wstring* response);

  
  static wstring GenerateMultipartBoundary();

  
  static wstring GenerateRequestHeader(const wstring &boundary);

  
  
  
  static bool GenerateRequestBody(const map<wstring, wstring> &parameters,
                                  const map<wstring, wstring> &files,
                                  const wstring &boundary,
                                  string *request_body);

  
  static bool GetFileContents(const wstring &filename, vector<char> *contents);

  
  static wstring UTF8ToWide(const string &utf8);

  
  static string WideToUTF8(const wstring &wide) {
      return WideToMBCP(wide, CP_UTF8);
  }

  
  static string WideToMBCP(const wstring &wide, unsigned int cp);

  
  
  
  static bool CheckParameters(const map<wstring, wstring> &parameters);

  
  
  HTTPUpload();
  explicit HTTPUpload(const HTTPUpload &);
  void operator=(const HTTPUpload &);
  ~HTTPUpload();
};

}  

#pragma warning( pop )

#endif  
