



#ifndef BASE_STRINGS_UTF_STRING_CONVERSION_UTILS_H_
#define BASE_STRINGS_UTF_STRING_CONVERSION_UTILS_H_



#include "base/base_export.h"
#include "base/strings/string16.h"

namespace base {

inline bool IsValidCodepoint(uint32 code_point) {
  
  
  
  return code_point < 0xD800u ||
         (code_point >= 0xE000u && code_point <= 0x10FFFFu);
}

inline bool IsValidCharacter(uint32 code_point) {
  
  
  return code_point < 0xD800u || (code_point >= 0xE000u &&
      code_point < 0xFDD0u) || (code_point > 0xFDEFu &&
      code_point <= 0x10FFFFu && (code_point & 0xFFFEu) != 0xFFFEu);
}










BASE_EXPORT bool ReadUnicodeCharacter(const char* src,
                                      int32 src_len,
                                      int32* char_index,
                                      uint32* code_point_out);


BASE_EXPORT bool ReadUnicodeCharacter(const char16* src,
                                      int32 src_len,
                                      int32* char_index,
                                      uint32* code_point);

#if defined(WCHAR_T_IS_UTF32)

BASE_EXPORT bool ReadUnicodeCharacter(const wchar_t* src,
                                      int32 src_len,
                                      int32* char_index,
                                      uint32* code_point);
#endif  






BASE_EXPORT size_t WriteUnicodeCharacter(uint32 code_point,
                                         std::string* output);



BASE_EXPORT size_t WriteUnicodeCharacter(uint32 code_point, string16* output);

#if defined(WCHAR_T_IS_UTF32)


inline size_t WriteUnicodeCharacter(uint32 code_point, std::wstring* output) {
  
  output->push_back(code_point);
  return 1;
}
#endif  







template<typename CHAR>
void PrepareForUTF8Output(const CHAR* src, size_t src_len, std::string* output);



template<typename STRING>
void PrepareForUTF16Or32Output(const char* src, size_t src_len, STRING* output);

}  

#endif  
