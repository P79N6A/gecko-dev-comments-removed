




























#include <cassert>
#include <vector>

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
  assert(wcs_length > 0);
#else  
  if ((wcs_length = mbstowcs(NULL, mbs.c_str(), mbs.length())) == (size_t)-1) {
    return false;
  }

  
  ++wcs_length;
#endif  

  std::vector<wchar_t> wcs_v(wcs_length);

  
#if _MSC_VER >= 1400  
  if ((err = mbstowcs_s(NULL, &wcs_v[0], wcs_length, mbs.c_str(),
                        _TRUNCATE)) != 0) {
    return false;
  }
#else  
  if (mbstowcs(&wcs_v[0], mbs.c_str(), mbs.length()) == (size_t)-1) {
    return false;
  }

  
  wcs_v[wcs_length - 1] = '\0';
#endif  

  *wcs = &wcs_v[0];
  return true;
}


bool WindowsStringUtils::safe_wcstombs(const wstring &wcs, string *mbs) {
  assert(mbs);

  
  size_t mbs_length;

#if _MSC_VER >= 1400  
  errno_t err;
  if ((err = wcstombs_s(&mbs_length, NULL, 0, wcs.c_str(), _TRUNCATE)) != 0) {
    return false;
  }
  assert(mbs_length > 0);
#else  
  if ((mbs_length = wcstombs(NULL, wcs.c_str(), wcs.length())) == (size_t)-1) {
    return false;
  }

  
  ++mbs_length;
#endif  

  std::vector<char> mbs_v(mbs_length);

  
#if _MSC_VER >= 1400  
  if ((err = wcstombs_s(NULL, &mbs_v[0], mbs_length, wcs.c_str(),
                        _TRUNCATE)) != 0) {
    return false;
  }
#else  
  if (wcstombs(&mbs_v[0], wcs.c_str(), wcs.length()) == (size_t)-1) {
    return false;
  }

  
  mbs_v[mbs_length - 1] = '\0';
#endif  

  *mbs = &mbs_v[0];
  return true;
}

}  
