
































#ifndef COMMON_WINDOWS_HTTP_UPLOAD_H__
#define COMMON_WINDOWS_HTTP_UPLOAD_H__

#pragma warning( push )

#pragma warning( disable : 4530 ) 

#include <Windows.h>
#include <WinInet.h>

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
                          const wstring &upload_file,
                          const wstring &file_part_name,
                          int *timeout,
                          wstring *response_body,
                          int *response_code);

 private:
  class AutoInternetHandle;

  
  
  
  
  static bool HTTPUpload::ReadResponse(HINTERNET request, wstring* response);

  
  static wstring GenerateMultipartBoundary();

  
  static wstring GenerateRequestHeader(const wstring &boundary);

  
  
  
  static bool GenerateRequestBody(const map<wstring, wstring> &parameters,
                                  const wstring &upload_file,
                                  const wstring &file_part_name,
                                  const wstring &boundary,
                                  string *request_body);

  
  static void GetFileContents(const wstring &filename, vector<char> *contents);

  
  static wstring UTF8ToWide(const string &utf8);

  
  static string WideToUTF8(const wstring &wide);

  
  
  
  static bool CheckParameters(const map<wstring, wstring> &parameters);

  
  
  HTTPUpload();
  explicit HTTPUpload(const HTTPUpload &);
  void operator=(const HTTPUpload &);
  ~HTTPUpload();
};

}  

#pragma warning( pop )

#endif  
