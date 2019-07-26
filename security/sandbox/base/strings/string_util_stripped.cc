#include <wchar.h>

namespace base {

bool IsWprintfFormatPortable(const wchar_t* format) {
  for (const wchar_t* position = format; *position != '\0'; ++position) {
    if (*position == '%') {
      bool in_specification = true;
      bool modifier_l = false;
      while (in_specification) {
        
        if (*++position == '\0') {
          
          
          
          return true;
        }

        if (*position == 'l') {
          
          modifier_l = true;
        } else if (((*position == 's' || *position == 'c') && !modifier_l) ||
                   *position == 'S' || *position == 'C' || *position == 'F' ||
                   *position == 'D' || *position == 'O' || *position == 'U') {
          
          return false;
        }

        if (wcschr(L"diouxXeEfgGaAcspn%", *position)) {
          
          in_specification = false;
        }
      }
    }
  }

  return true;
}

}  

