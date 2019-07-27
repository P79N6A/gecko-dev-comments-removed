



#ifndef BASE_STRINGS_UTF_STRING_CONVERSIONS_H_
#define BASE_STRINGS_UTF_STRING_CONVERSIONS_H_

#include <string>

#include "base/base_export.h"
#include "base/strings/string16.h"
#include "base/strings/string_piece.h"

namespace base {







BASE_EXPORT bool WideToUTF8(const wchar_t* src, size_t src_len,
                            std::string* output);
BASE_EXPORT std::string WideToUTF8(const std::wstring& wide);
BASE_EXPORT bool UTF8ToWide(const char* src, size_t src_len,
                            std::wstring* output);
BASE_EXPORT std::wstring UTF8ToWide(const StringPiece& utf8);

BASE_EXPORT bool WideToUTF16(const wchar_t* src, size_t src_len,
                             string16* output);
BASE_EXPORT string16 WideToUTF16(const std::wstring& wide);
BASE_EXPORT bool UTF16ToWide(const char16* src, size_t src_len,
                             std::wstring* output);
BASE_EXPORT std::wstring UTF16ToWide(const string16& utf16);

BASE_EXPORT bool UTF8ToUTF16(const char* src, size_t src_len, string16* output);
BASE_EXPORT string16 UTF8ToUTF16(const StringPiece& utf8);
BASE_EXPORT bool UTF16ToUTF8(const char16* src, size_t src_len,
                             std::string* output);
BASE_EXPORT std::string UTF16ToUTF8(const string16& utf16);



BASE_EXPORT std::wstring ASCIIToWide(const StringPiece& ascii);
BASE_EXPORT string16 ASCIIToUTF16(const StringPiece& ascii);



BASE_EXPORT std::string UTF16ToASCII(const string16& utf16);

}  

#endif  
