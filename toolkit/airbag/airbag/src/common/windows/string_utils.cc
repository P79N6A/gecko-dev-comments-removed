




























#include <cassert>

#include "common/windows/string_utils-inl.h"

namespace google_breakpad {


wstring WindowsStringUtils::GetBaseName(const wstring &filename) {
  wstring base_name(filename);
  size_t slash_pos = base_name.find_last_of(L"/\\");
  if (slash_pos != wstring::npos) {
    base_name.erase(0, slash_pos + 1);
  }
  return base_name;
}


bool WindowsStringUtils::safe_mbstowcs(const string &mbs, wstring *wcs) {
  assert(wcs);

  
  size_t wcs_length;

#if _MSC_VER >= 1400  
  errno_t err;
  if ((err = mbstowcs_s(&wcs_length, NULL, 0, mbs.c_str(), _TRUNCATE)) != 0) {
    return false;
  }
#else  
  if ((wcs_length = mbstowcs(NULL, mbs.c_str(), mbs.length())) < 0) {
    return false;
  }

  
  ++wcs_length;
#endif  

  
  wchar_t *wcs_c = new wchar_t[wcs_length];

  
#if _MSC_VER >= 1400  
  if ((err = mbstowcs_s(NULL, wcs_c, wcs_length, mbs.c_str(),
                        _TRUNCATE)) != 0) {
    delete[] wcs_c;
    return false;
  }
#else  
  if (mbstowcs(wcs_c, mbs.c_str(), mbs.length()) < 0) {
    delete[] wcs_c;
    return false;
  }

  
  wcs_c[wcs_length - 1] = '\0';
#endif  

  *wcs = wcs_c;
  delete[] wcs_c;
  return true;
}

}  
