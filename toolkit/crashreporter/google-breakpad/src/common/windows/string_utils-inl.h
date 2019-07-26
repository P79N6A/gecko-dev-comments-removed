































#ifndef COMMON_WINDOWS_STRING_UTILS_INL_H__
#define COMMON_WINDOWS_STRING_UTILS_INL_H__

#include <stdarg.h>
#include <wchar.h>

#include <string>






#if _MSC_VER >= 1400  
#define WIN_STRING_FORMAT_LL "ll"
#else  
#define WIN_STRING_FORMAT_LL "I64"
#endif  






#if _MSC_VER < 1400  
#define swprintf _snwprintf
#else


#define swprintf swprintf_s
#endif  

namespace google_breakpad {

using std::string;
using std::wstring;

class WindowsStringUtils {
 public:
  
  
  
  static void safe_wcscpy(wchar_t *destination, size_t destination_size,
                          const wchar_t *source);

  
  
  
  
  static void safe_wcsncpy(wchar_t *destination, size_t destination_size,
                           const wchar_t *source, size_t count);

  
  
  
  static bool safe_mbstowcs(const string &mbs, wstring *wcs);

  
  static bool safe_wcstombs(const wstring &wcs, string *mbs);

  
  static wstring GetBaseName(const wstring &filename);

 private:
  
  WindowsStringUtils();
  WindowsStringUtils(const WindowsStringUtils&);
  ~WindowsStringUtils();
  void operator=(const WindowsStringUtils&);
};


inline void WindowsStringUtils::safe_wcscpy(wchar_t *destination,
                                            size_t destination_size,
                                            const wchar_t *source) {
#if _MSC_VER >= 1400  
  wcscpy_s(destination, destination_size, source);
#else  
  
  
  
  wcsncpy(destination, source, destination_size);
  if (destination && destination_size)
    destination[destination_size - 1] = 0;
#endif  
}


inline void WindowsStringUtils::safe_wcsncpy(wchar_t *destination,
                                             size_t destination_size,
                                             const wchar_t *source,
                                             size_t count) {
#if _MSC_VER >= 1400  
  wcsncpy_s(destination, destination_size, source, count);
#else  
  
  
  
  if (destination_size < count)
    count = destination_size;

  wcsncpy(destination, source, count);
  if (destination && count)
    destination[count - 1] = 0;
#endif  
}

}  

#endif  
