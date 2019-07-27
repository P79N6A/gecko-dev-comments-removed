









#include "webrtc/base/win32.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <algorithm>

#include "webrtc/base/basictypes.h"
#include "webrtc/base/byteorder.h"
#include "webrtc/base/common.h"
#include "webrtc/base/logging.h"

namespace rtc {


static const char* inet_ntop_v4(const void* src, char* dst, socklen_t size);
static const char* inet_ntop_v6(const void* src, char* dst, socklen_t size);
static int inet_pton_v4(const char* src, void* dst);
static int inet_pton_v6(const char* src, void* dst);





const char* win32_inet_ntop(int af, const void *src,
                            char* dst, socklen_t size) {
  if (!src || !dst) {
    return NULL;
  }
  switch (af) {
    case AF_INET: {
      return inet_ntop_v4(src, dst, size);
    }
    case AF_INET6: {
      return inet_ntop_v6(src, dst, size);
    }
  }
  return NULL;
}



int win32_inet_pton(int af, const char* src, void* dst) {
  if (!src || !dst) {
    return 0;
  }
  if (af == AF_INET) {
    return inet_pton_v4(src, dst);
  } else if (af == AF_INET6) {
    return inet_pton_v6(src, dst);
  }
  return -1;
}



const char* inet_ntop_v4(const void* src, char* dst, socklen_t size) {
  if (size < INET_ADDRSTRLEN) {
    return NULL;
  }
  const struct in_addr* as_in_addr =
      reinterpret_cast<const struct in_addr*>(src);
  rtc::sprintfn(dst, size, "%d.%d.%d.%d",
                      as_in_addr->S_un.S_un_b.s_b1,
                      as_in_addr->S_un.S_un_b.s_b2,
                      as_in_addr->S_un.S_un_b.s_b3,
                      as_in_addr->S_un.S_un_b.s_b4);
  return dst;
}


const char* inet_ntop_v6(const void* src, char* dst, socklen_t size) {
  if (size < INET6_ADDRSTRLEN) {
    return NULL;
  }
  const uint16* as_shorts =
      reinterpret_cast<const uint16*>(src);
  int runpos[8];
  int current = 1;
  int max = 1;
  int maxpos = -1;
  int run_array_size = ARRAY_SIZE(runpos);
  
  for (int i = 0; i < run_array_size; ++i) {
    if (as_shorts[i] == 0) {
      runpos[i] = current;
      if (current > max) {
        maxpos = i;
        max = current;
      }
      ++current;
    } else {
      runpos[i] = -1;
      current =1;
    }
  }

  if (max > 1) {
    int tmpmax = maxpos;
    
    for (int i = run_array_size - 1; i >= 0; i--) {
      if (i > tmpmax) {
        runpos[i] = -1;
      } else if (runpos[i] == -1) {
        
        
        tmpmax = -1;
      }
    }
  }

  char* cursor = dst;
  
  
  
  if (runpos[0] == 1 && (maxpos == 5 ||
                         (maxpos == 4 && as_shorts[5] == 0xFFFF))) {
    *cursor++ = ':';
    *cursor++ = ':';
    if (maxpos == 4) {
      cursor += rtc::sprintfn(cursor, INET6_ADDRSTRLEN - 2, "ffff:");
    }
    const struct in_addr* as_v4 =
        reinterpret_cast<const struct in_addr*>(&(as_shorts[6]));
    inet_ntop_v4(as_v4, cursor,
                 static_cast<socklen_t>(INET6_ADDRSTRLEN - (cursor - dst)));
  } else {
    for (int i = 0; i < run_array_size; ++i) {
      if (runpos[i] == -1) {
        cursor += rtc::sprintfn(cursor,
                                      INET6_ADDRSTRLEN - (cursor - dst),
                                      "%x", NetworkToHost16(as_shorts[i]));
        if (i != 7 && runpos[i + 1] != 1) {
          *cursor++ = ':';
        }
      } else if (runpos[i] == 1) {
        
        *cursor++ = ':';
        *cursor++ = ':';
        i += (max - 1);
      }
    }
  }
  return dst;
}







int inet_pton_v4(const char* src, void* dst) {
  const int kIpv4AddressSize = 4;
  int found = 0;
  const char* src_pos = src;
  unsigned char result[kIpv4AddressSize] = {0};

  while (*src_pos != '\0') {
    
    
    if (!isdigit(*src_pos)) {
      return 0;
    }
    char* end_pos;
    long value = strtol(src_pos, &end_pos, 10);
    if (value < 0 || value > 255 || src_pos == end_pos) {
      return 0;
    }
    ++found;
    if (found > kIpv4AddressSize) {
      return 0;
    }
    result[found - 1] = static_cast<unsigned char>(value);
    src_pos = end_pos;
    if (*src_pos == '.') {
      
      ++src_pos;
    } else if (*src_pos != '\0') {
      
      return 0;
    }
  }
  if (found != kIpv4AddressSize) {
    return 0;
  }
  memcpy(dst, result, sizeof(result));
  return 1;
}


int inet_pton_v6(const char* src, void* dst) {
  
  
  const char* readcursor = src;
  char c = *readcursor++;
  while (c) {
    if (c == 'x') {
      return 0;
    }
    c = *readcursor++;
  }
  readcursor = src;

  struct in6_addr an_addr;
  memset(&an_addr, 0, sizeof(an_addr));

  uint16* addr_cursor = reinterpret_cast<uint16*>(&an_addr.s6_addr[0]);
  uint16* addr_end = reinterpret_cast<uint16*>(&an_addr.s6_addr[16]);
  bool seencompressed = false;

  
  
  
  if (*readcursor == ':' && *(readcursor+1) == ':' &&
      *(readcursor + 2) != 0) {
    
    const char* addrstart = readcursor + 2;
    if (rtc::strchr(addrstart, ".")) {
      const char* colon = rtc::strchr(addrstart, "::");
      if (colon) {
        uint16 a_short;
        int bytesread = 0;
        if (sscanf(addrstart, "%hx%n", &a_short, &bytesread) != 1 ||
            a_short != 0xFFFF || bytesread != 4) {
          
          return 0;
        } else {
          an_addr.s6_addr[10] = 0xFF;
          an_addr.s6_addr[11] = 0xFF;
          addrstart = colon + 1;
        }
      }
      struct in_addr v4;
      if (inet_pton_v4(addrstart, &v4.s_addr)) {
        memcpy(&an_addr.s6_addr[12], &v4, sizeof(v4));
        memcpy(dst, &an_addr, sizeof(an_addr));
        return 1;
      } else {
        
        return 0;
      }
    }
  }

  
  while (*readcursor != 0 && addr_cursor < addr_end) {
    if (*readcursor == ':') {
      if (*(readcursor + 1) == ':') {
        if (seencompressed) {
          
          return 0;
        }
        
        
        readcursor += 2;
        const char* coloncounter = readcursor;
        int coloncount = 0;
        if (*coloncounter == 0) {
          
          addr_cursor = addr_end;
        } else {
          while (*coloncounter) {
            if (*coloncounter == ':') {
              ++coloncount;
            }
            ++coloncounter;
          }
          
          addr_cursor = addr_end - (coloncount + 1);
          seencompressed = true;
        }
      } else {
        ++readcursor;
      }
    } else {
      uint16 word;
      int bytesread = 0;
      if (sscanf(readcursor, "%hx%n", &word, &bytesread) != 1) {
        return 0;
      } else {
        *addr_cursor = HostToNetwork16(word);
        ++addr_cursor;
        readcursor += bytesread;
        if (*readcursor != ':' && *readcursor != '\0') {
          return 0;
        }
      }
    }
  }

  if (*readcursor != '\0' || addr_cursor < addr_end) {
    
    return 0;
  }
  memcpy(dst, &an_addr, sizeof(an_addr));
  return 1;
}












void FileTimeToUnixTime(const FILETIME& ft, time_t* ut) {
  ASSERT(NULL != ut);

  
  
  SYSTEMTIME base_st;
  memset(&base_st, 0, sizeof(base_st));
  base_st.wDay = 1;
  base_st.wMonth = 1;
  base_st.wYear = 1970;

  FILETIME base_ft;
  SystemTimeToFileTime(&base_st, &base_ft);

  ULARGE_INTEGER base_ul, current_ul;
  memcpy(&base_ul, &base_ft, sizeof(FILETIME));
  memcpy(&current_ul, &ft, sizeof(FILETIME));

  
  
  const ULONGLONG RATIO = 10000000;
  *ut = static_cast<time_t>((current_ul.QuadPart - base_ul.QuadPart) / RATIO);
}

void UnixTimeToFileTime(const time_t& ut, FILETIME* ft) {
  ASSERT(NULL != ft);

  
  
  SYSTEMTIME base_st;
  memset(&base_st, 0, sizeof(base_st));
  base_st.wDay = 1;
  base_st.wMonth = 1;
  base_st.wYear = 1970;

  FILETIME base_ft;
  SystemTimeToFileTime(&base_st, &base_ft);

  ULARGE_INTEGER base_ul;
  memcpy(&base_ul, &base_ft, sizeof(FILETIME));

  
  
  const ULONGLONG RATIO = 10000000;
  ULARGE_INTEGER current_ul;
  current_ul.QuadPart = base_ul.QuadPart + static_cast<int64>(ut) * RATIO;
  memcpy(ft, &current_ul, sizeof(FILETIME));
}

bool Utf8ToWindowsFilename(const std::string& utf8, std::wstring* filename) {
  
  
  
  

  
  int wlen = ::MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                                   static_cast<int>(utf8.length() + 1), NULL,
                                   0);
  if (0 == wlen) {
    return false;
  }
  wchar_t* wfilename = STACK_ARRAY(wchar_t, wlen);
  if (0 == ::MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                                 static_cast<int>(utf8.length() + 1),
                                 wfilename, wlen)) {
    return false;
  }
  
  std::replace(wfilename, wfilename + wlen, L'/', L'\\');
  
  DWORD full_len = ::GetFullPathName(wfilename, 0, NULL, NULL);
  if (0 == full_len) {
    return false;
  }
  wchar_t* filepart = NULL;
  wchar_t* full_filename = STACK_ARRAY(wchar_t, full_len + 6);
  wchar_t* start = full_filename + 6;
  if (0 == ::GetFullPathName(wfilename, full_len, start, &filepart)) {
    return false;
  }
  
  const wchar_t kLongPathPrefix[] = L"\\\\?\\UNC";
  if ((start[0] != L'\\') || (start[1] != L'\\')) {
    
    
    start -= 4;
    ASSERT(start >= full_filename);
    memcpy(start, kLongPathPrefix, 4 * sizeof(wchar_t));
  } else if (start[2] != L'?') {
    
    
    start -= 6;
    ASSERT(start >= full_filename);
    memcpy(start, kLongPathPrefix, 7 * sizeof(wchar_t));
  } else {
    
  }
  filename->assign(start);
  return true;
}

bool GetOsVersion(int* major, int* minor, int* build) {
  OSVERSIONINFO info = {0};
  info.dwOSVersionInfoSize = sizeof(info);
  if (GetVersionEx(&info)) {
    if (major) *major = info.dwMajorVersion;
    if (minor) *minor = info.dwMinorVersion;
    if (build) *build = info.dwBuildNumber;
    return true;
  }
  return false;
}

bool GetCurrentProcessIntegrityLevel(int* level) {
  bool ret = false;
  HANDLE process = ::GetCurrentProcess(), token;
  if (OpenProcessToken(process, TOKEN_QUERY | TOKEN_QUERY_SOURCE, &token)) {
    DWORD size;
    if (!GetTokenInformation(token, TokenIntegrityLevel, NULL, 0, &size) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER) {

      char* buf = STACK_ARRAY(char, size);
      TOKEN_MANDATORY_LABEL* til =
          reinterpret_cast<TOKEN_MANDATORY_LABEL*>(buf);
      if (GetTokenInformation(token, TokenIntegrityLevel, til, size, &size)) {

        DWORD count = *GetSidSubAuthorityCount(til->Label.Sid);
        *level = *GetSidSubAuthority(til->Label.Sid, count - 1);
        ret = true;
      }
    }
    CloseHandle(token);
  }
  return ret;
}
}  
