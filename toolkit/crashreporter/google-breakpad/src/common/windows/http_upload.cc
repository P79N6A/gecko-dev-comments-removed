




























#include <assert.h>


#pragma warning( disable : 4530 )

#include <fstream>

#include "common/windows/string_utils-inl.h"

#include "common/windows/http_upload.h"

namespace google_breakpad {

using std::ifstream;
using std::ios;

static const wchar_t kUserAgent[] = L"Breakpad/1.0 (Windows)";


class HTTPUpload::AutoInternetHandle {
 public:
  explicit AutoInternetHandle(HINTERNET handle) : handle_(handle) {}
  ~AutoInternetHandle() {
    if (handle_) {
      InternetCloseHandle(handle_);
    }
  }

  HINTERNET get() { return handle_; }

 private:
  HINTERNET handle_;
};


bool HTTPUpload::SendRequest(const wstring &url,
                             const map<wstring, wstring> &parameters,
                             const map<wstring, wstring> &files,
                             int *timeout,
                             wstring *response_body,
                             int *response_code) {
  if (response_code) {
    *response_code = 0;
  }

  
  if (!CheckParameters(parameters)) {
    return false;
  }

  
  wchar_t scheme[16], host[256], path[256];
  URL_COMPONENTS components;
  memset(&components, 0, sizeof(components));
  components.dwStructSize = sizeof(components);
  components.lpszScheme = scheme;
  components.dwSchemeLength = sizeof(scheme) / sizeof(scheme[0]);
  components.lpszHostName = host;
  components.dwHostNameLength = sizeof(host) / sizeof(host[0]);
  components.lpszUrlPath = path;
  components.dwUrlPathLength = sizeof(path) / sizeof(path[0]);
  if (!InternetCrackUrl(url.c_str(), static_cast<DWORD>(url.size()),
                        0, &components)) {
    return false;
  }
  bool secure = false;
  if (wcscmp(scheme, L"https") == 0) {
    secure = true;
  } else if (wcscmp(scheme, L"http") != 0) {
    return false;
  }

  AutoInternetHandle internet(InternetOpen(kUserAgent,
                                           INTERNET_OPEN_TYPE_PRECONFIG,
                                           NULL,  
                                           NULL,  
                                           0));   
  if (!internet.get()) {
    return false;
  }

  AutoInternetHandle connection(InternetConnect(internet.get(),
                                                host,
                                                components.nPort,
                                                NULL,    
                                                NULL,    
                                                INTERNET_SERVICE_HTTP,
                                                0,       
                                                NULL));  
  if (!connection.get()) {
    return false;
  }

  DWORD http_open_flags = secure ? INTERNET_FLAG_SECURE : 0;
  http_open_flags |= INTERNET_FLAG_NO_COOKIES;
  AutoInternetHandle request(HttpOpenRequest(connection.get(),
                                             L"POST",
                                             path,
                                             NULL,    
                                             NULL,    
                                             NULL,    
                                             http_open_flags,
                                             NULL));  
  if (!request.get()) {
    return false;
  }

  wstring boundary = GenerateMultipartBoundary();
  wstring content_type_header = GenerateRequestHeader(boundary);
  HttpAddRequestHeaders(request.get(),
                        content_type_header.c_str(),
                        static_cast<DWORD>(-1),
                        HTTP_ADDREQ_FLAG_ADD);

  string request_body;
  if (!GenerateRequestBody(parameters, files, boundary, &request_body)) {
    return false;
  }

  if (timeout) {
    if (!InternetSetOption(request.get(),
                           INTERNET_OPTION_SEND_TIMEOUT,
                           timeout,
                           sizeof(*timeout))) {
      fwprintf(stderr, L"Could not unset send timeout, continuing...\n");
    }

    if (!InternetSetOption(request.get(),
                           INTERNET_OPTION_RECEIVE_TIMEOUT,
                           timeout,
                           sizeof(*timeout))) {
      fwprintf(stderr, L"Could not unset receive timeout, continuing...\n");
    }
  }
  
  if (!HttpSendRequest(request.get(), NULL, 0,
                       const_cast<char *>(request_body.data()),
                       static_cast<DWORD>(request_body.size()))) {
    return false;
  }

  
  wchar_t http_status[4];
  DWORD http_status_size = sizeof(http_status);
  if (!HttpQueryInfo(request.get(), HTTP_QUERY_STATUS_CODE,
                     static_cast<LPVOID>(&http_status), &http_status_size,
                     0)) {
    return false;
  }

  int http_response = wcstol(http_status, NULL, 10);
  if (response_code) {
    *response_code = http_response;
  }

  bool result = (http_response == 200);

  if (result) {
    result = ReadResponse(request.get(), response_body);
  }

  return result;
}


bool HTTPUpload::ReadResponse(HINTERNET request, wstring *response) {
  bool has_content_length_header = false;
  wchar_t content_length[32];
  DWORD content_length_size = sizeof(content_length);
  DWORD claimed_size = 0;
  string response_body;

  if (HttpQueryInfo(request, HTTP_QUERY_CONTENT_LENGTH,
                    static_cast<LPVOID>(&content_length),
                    &content_length_size, 0)) {
    has_content_length_header = true;
    claimed_size = wcstol(content_length, NULL, 10);
    response_body.reserve(claimed_size);
  }


  DWORD bytes_available;
  DWORD total_read = 0;
  BOOL return_code;

  while (((return_code = InternetQueryDataAvailable(request, &bytes_available,
	  0, 0)) != 0) && bytes_available > 0) {

    vector<char> response_buffer(bytes_available);
    DWORD size_read;

    return_code = InternetReadFile(request,
                                   &response_buffer[0],
                                   bytes_available, &size_read);

    if (return_code && size_read > 0) {
      total_read += size_read;
      response_body.append(&response_buffer[0], size_read);
    } else {
      break;
    }
  }

  bool succeeded = return_code && (!has_content_length_header ||
                                   (total_read == claimed_size));
  if (succeeded && response) {
    *response = UTF8ToWide(response_body);
  }

  return succeeded;
}


wstring HTTPUpload::GenerateMultipartBoundary() {
  
  static const wchar_t kBoundaryPrefix[] = L"---------------------------";
  static const int kBoundaryLength = 27 + 16 + 1;

  
  int r0 = rand();
  int r1 = rand();

  wchar_t temp[kBoundaryLength];
  swprintf(temp, kBoundaryLength, L"%s%08X%08X", kBoundaryPrefix, r0, r1);

  
  temp[kBoundaryLength - 1] = L'\0';

  return wstring(temp);
}


wstring HTTPUpload::GenerateRequestHeader(const wstring &boundary) {
  wstring header = L"Content-Type: multipart/form-data; boundary=";
  header += boundary;
  return header;
}


bool HTTPUpload::GenerateRequestBody(const map<wstring, wstring> &parameters,
                                     const map<wstring, wstring> &files,
                                     const wstring &boundary,
                                     string *request_body) {
  string boundary_str = WideToUTF8(boundary);
  if (boundary_str.empty()) {
    return false;
  }

  request_body->clear();

  
  for (map<wstring, wstring>::const_iterator pos = parameters.begin();
       pos != parameters.end(); ++pos) {
    request_body->append("--" + boundary_str + "\r\n");
    request_body->append("Content-Disposition: form-data; name=\"" +
                         WideToUTF8(pos->first) + "\"\r\n\r\n" +
                         WideToUTF8(pos->second) + "\r\n");
  }

  for (map<wstring, wstring>::const_iterator pos = files.begin();
       pos != files.end(); ++pos) {
    vector<char> contents;
    if (!GetFileContents(pos->second, &contents)) {
      return false;
    }

    
    string filename_utf8 = WideToUTF8(pos->second);
    if (filename_utf8.empty()) {
      return false;
    }

    string file_part_name_utf8 = WideToUTF8(pos->first);
    if (file_part_name_utf8.empty()) {
      return false;
    }

    request_body->append("--" + boundary_str + "\r\n");
    request_body->append("Content-Disposition: form-data; "
      "name=\"" + file_part_name_utf8 + "\"; "
      "filename=\"" + filename_utf8 + "\"\r\n");
    request_body->append("Content-Type: application/octet-stream\r\n");
    request_body->append("\r\n");

    if (!contents.empty()) {
      request_body->append(&(contents[0]), contents.size());
    }
    request_body->append("\r\n");
  }
  request_body->append("--" + boundary_str + "--\r\n");
  return true;
}


bool HTTPUpload::GetFileContents(const wstring &filename,
                                 vector<char> *contents) {
  
  
  
  
  
  
#ifdef _MSC_VER
  ifstream file;
  file.open(filename.c_str(), ios::binary);
#else 
  ifstream file(WideToMBCP(filename, CP_ACP).c_str(), ios::binary);
#endif  
  if (file.is_open()) {
    file.seekg(0, ios::end);
    std::streamoff length = file.tellg();
    contents->resize(length);
    if (length != 0) {
      file.seekg(0, ios::beg);
      file.read(&((*contents)[0]), length);
    }
    file.close();
    return true;
  }
  return false;
}


wstring HTTPUpload::UTF8ToWide(const string &utf8) {
  if (utf8.length() == 0) {
    return wstring();
  }

  
  int charcount = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, NULL, 0);

  if (charcount == 0) {
    return wstring();
  }

  
  wchar_t* buf = new wchar_t[charcount];
  MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, buf, charcount);
  wstring result(buf);
  delete[] buf;
  return result;
}


string HTTPUpload::WideToMBCP(const wstring &wide, unsigned int cp) {
  if (wide.length() == 0) {
    return string();
  }

  
  int charcount = WideCharToMultiByte(cp, 0, wide.c_str(), -1,
                                      NULL, 0, NULL, NULL);
  if (charcount == 0) {
    return string();
  }

  
  char *buf = new char[charcount];
  WideCharToMultiByte(cp, 0, wide.c_str(), -1, buf, charcount,
                      NULL, NULL);

  string result(buf);
  delete[] buf;
  return result;
}


bool HTTPUpload::CheckParameters(const map<wstring, wstring> &parameters) {
  for (map<wstring, wstring>::const_iterator pos = parameters.begin();
       pos != parameters.end(); ++pos) {
    const wstring &str = pos->first;
    if (str.size() == 0) {
      return false;  
    }
    for (unsigned int i = 0; i < str.size(); ++i) {
      wchar_t c = str[i];
      if (c < 32 || c == '"' || c > 127) {
        return false;
      }
    }
  }
  return true;
}

}  
