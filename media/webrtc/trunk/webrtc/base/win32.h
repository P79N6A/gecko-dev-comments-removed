









#ifndef WEBRTC_BASE_WIN32_H_
#define WEBRTC_BASE_WIN32_H_

#if defined(WEBRTC_WIN)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif


#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <winsock2.h>
#include <windows.h>

#ifndef SECURITY_MANDATORY_LABEL_AUTHORITY

#define SECURITY_MANDATORY_MEDIUM_RID               (0x00002000L)
#define TokenIntegrityLevel static_cast<TOKEN_INFORMATION_CLASS>(0x19)
typedef struct _TOKEN_MANDATORY_LABEL {
    SID_AND_ATTRIBUTES Label;
} TOKEN_MANDATORY_LABEL, *PTOKEN_MANDATORY_LABEL;
#endif  

#undef SetPort

#include <string>

#include "webrtc/base/stringutils.h"
#include "webrtc/base/basictypes.h"

namespace rtc {

const char* win32_inet_ntop(int af, const void *src, char* dst, socklen_t size);
int win32_inet_pton(int af, const char* src, void *dst);



inline std::wstring ToUtf16(const char* utf8, size_t len) {
  int len16 = ::MultiByteToWideChar(CP_UTF8, 0, utf8, static_cast<int>(len),
                                    NULL, 0);
  wchar_t* ws = STACK_ARRAY(wchar_t, len16);
  ::MultiByteToWideChar(CP_UTF8, 0, utf8, static_cast<int>(len), ws, len16);
  return std::wstring(ws, len16);
}

inline std::wstring ToUtf16(const std::string& str) {
  return ToUtf16(str.data(), str.length());
}

inline std::string ToUtf8(const wchar_t* wide, size_t len) {
  int len8 = ::WideCharToMultiByte(CP_UTF8, 0, wide, static_cast<int>(len),
                                   NULL, 0, NULL, NULL);
  char* ns = STACK_ARRAY(char, len8);
  ::WideCharToMultiByte(CP_UTF8, 0, wide, static_cast<int>(len), ns, len8,
                        NULL, NULL);
  return std::string(ns, len8);
}

inline std::string ToUtf8(const wchar_t* wide) {
  return ToUtf8(wide, wcslen(wide));
}

inline std::string ToUtf8(const std::wstring& wstr) {
  return ToUtf8(wstr.data(), wstr.length());
}


void FileTimeToUnixTime(const FILETIME& ft, time_t* ut);


void UnixTimeToFileTime(const time_t& ut, FILETIME * ft);


bool Utf8ToWindowsFilename(const std::string& utf8, std::wstring* filename);


inline uint64 ToUInt64(const FILETIME& ft) {
  ULARGE_INTEGER r = {ft.dwLowDateTime, ft.dwHighDateTime};
  return r.QuadPart;
}

enum WindowsMajorVersions {
  kWindows2000 = 5,
  kWindowsVista = 6,
};
bool GetOsVersion(int* major, int* minor, int* build);

inline bool IsWindowsVistaOrLater() {
  int major;
  return (GetOsVersion(&major, NULL, NULL) && major >= kWindowsVista);
}

inline bool IsWindowsXpOrLater() {
  int major, minor;
  return (GetOsVersion(&major, &minor, NULL) &&
          (major >= kWindowsVista ||
          (major == kWindows2000 && minor >= 1)));
}


bool GetCurrentProcessIntegrityLevel(int* level);

inline bool IsCurrentProcessLowIntegrity() {
  int level;
  return (GetCurrentProcessIntegrityLevel(&level) &&
      level < SECURITY_MANDATORY_MEDIUM_RID);
}

bool AdjustCurrentProcessPrivilege(const TCHAR* privilege, bool to_enable);



}  

#endif  
#endif  
