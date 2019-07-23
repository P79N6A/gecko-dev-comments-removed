































#ifndef COMMON_WINDOWS_STRING_UTILS_INL_H__
#define COMMON_WINDOWS_STRING_UTILS_INL_H__

#include <stdarg.h>
#include <wchar.h>






#if _MSC_VER >= 1400  
#define WIN_STRING_FORMAT_LL "ll"
#else  
#define WIN_STRING_FORMAT_LL "I64"
#endif  

namespace google_airbag {

class WindowsStringUtils {
 public:
  
  static void safe_swprintf(wchar_t *buffer, size_t count,
                            const wchar_t *format, ...);

  
  
  
  static void safe_wcscpy(wchar_t *destination, size_t destination_size,
                          const wchar_t *source);

  
  
  
  
  static void safe_wcsncpy(wchar_t *destination, size_t destination_size,
                           const wchar_t *source, size_t count);

 private:
  
  WindowsStringUtils();
  WindowsStringUtils(const WindowsStringUtils&);
  ~WindowsStringUtils();
  void operator=(const WindowsStringUtils&);
};


inline void WindowsStringUtils::safe_swprintf(wchar_t *buffer, size_t count,
                                              const wchar_t *format, ...) {
  va_list args;
  va_start(args, format);
  vswprintf(buffer, count, format, args);

#if _MSC_VER < 1400  
  
  
  if (buffer && count)
    buffer[count - 1] = 0;
#endif  
}


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
