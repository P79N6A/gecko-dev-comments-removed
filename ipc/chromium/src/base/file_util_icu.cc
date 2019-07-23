











#include "base/file_util.h"

#include "base/string_util.h"
#include "unicode/uniset.h"

namespace file_util {

void ReplaceIllegalCharacters(std::wstring* file_name, int replace_char) {
  DCHECK(file_name);

  
  
  
  
  
  
  
  
  
  

  UErrorCode status = U_ZERO_ERROR;
#if defined(WCHAR_T_IS_UTF16)
  UnicodeSet illegal_characters(UnicodeString(
      L"[[\"*/:<>?\\\\|][:Cc:][:Cf:] - [\u200c\u200d]]"), status);
#else
  UnicodeSet illegal_characters(UNICODE_STRING_SIMPLE(
      "[[\"*/:<>?\\\\|][:Cc:][:Cf:] - [\\u200c\\u200d]]").unescape(), status);
#endif
  DCHECK(U_SUCCESS(status));
  
  
  illegal_characters.add(0xFDD0, 0xFDEF);
  for (int i = 0; i <= 0x10; ++i) {
    int plane_base = 0x10000 * i;
    illegal_characters.add(plane_base + 0xFFFE, plane_base + 0xFFFF);
  }
  illegal_characters.freeze();
  DCHECK(!illegal_characters.contains(replace_char) && replace_char < 0x10000);

  
  TrimWhitespace(*file_name, TRIM_ALL, file_name);

  std::wstring::size_type i = 0;
  std::wstring::size_type length = file_name->size();
  const wchar_t* wstr = file_name->data();
#if defined(WCHAR_T_IS_UTF16)
  
  
  std::wstring temp;
  temp.reserve(length);
  while (i < length) {
    UChar32 ucs4;
    std::wstring::size_type prev = i;
    U16_NEXT(wstr, i, length, ucs4);
    if (illegal_characters.contains(ucs4)) {
      temp.push_back(replace_char);
    } else if (ucs4 < 0x10000) {
      temp.push_back(ucs4);
    } else {
      temp.push_back(wstr[prev]);
      temp.push_back(wstr[prev + 1]);
    }
  }
  file_name->swap(temp);
#elif defined(WCHAR_T_IS_UTF32)
  while (i < length) {
    if (illegal_characters.contains(wstr[i])) {
      (*file_name)[i] = replace_char;
    }
    ++i;
  }
#else
#error wchar_t* should be either UTF-16 or UTF-32
#endif
}

}  
