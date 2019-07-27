



#include "base/strings/utf_string_conversions.h"

#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversion_utils.h"

namespace base {

namespace {







template<typename SRC_CHAR, typename DEST_STRING>
bool ConvertUnicode(const SRC_CHAR* src,
                    size_t src_len,
                    DEST_STRING* output) {
  
  bool success = true;
  int32 src_len32 = static_cast<int32>(src_len);
  for (int32 i = 0; i < src_len32; i++) {
    uint32 code_point;
    if (ReadUnicodeCharacter(src, src_len32, &i, &code_point)) {
      WriteUnicodeCharacter(code_point, output);
    } else {
      WriteUnicodeCharacter(0xFFFD, output);
      success = false;
    }
  }

  return success;
}

}  



bool WideToUTF8(const wchar_t* src, size_t src_len, std::string* output) {
  PrepareForUTF8Output(src, src_len, output);
  return ConvertUnicode(src, src_len, output);
}

std::string WideToUTF8(const std::wstring& wide) {
  std::string ret;
  
  
  WideToUTF8(wide.data(), wide.length(), &ret);
  return ret;
}

bool UTF8ToWide(const char* src, size_t src_len, std::wstring* output) {
  PrepareForUTF16Or32Output(src, src_len, output);
  return ConvertUnicode(src, src_len, output);
}

std::wstring UTF8ToWide(const StringPiece& utf8) {
  std::wstring ret;
  UTF8ToWide(utf8.data(), utf8.length(), &ret);
  return ret;
}



#if defined(WCHAR_T_IS_UTF16)


bool WideToUTF16(const wchar_t* src, size_t src_len, string16* output) {
  output->assign(src, src_len);
  return true;
}

string16 WideToUTF16(const std::wstring& wide) {
  return wide;
}

bool UTF16ToWide(const char16* src, size_t src_len, std::wstring* output) {
  output->assign(src, src_len);
  return true;
}

std::wstring UTF16ToWide(const string16& utf16) {
  return utf16;
}

#elif defined(WCHAR_T_IS_UTF32)

bool WideToUTF16(const wchar_t* src, size_t src_len, string16* output) {
  output->clear();
  
  
  output->reserve(src_len);
  return ConvertUnicode(src, src_len, output);
}

string16 WideToUTF16(const std::wstring& wide) {
  string16 ret;
  WideToUTF16(wide.data(), wide.length(), &ret);
  return ret;
}

bool UTF16ToWide(const char16* src, size_t src_len, std::wstring* output) {
  output->clear();
  
  
  output->reserve(src_len);
  return ConvertUnicode(src, src_len, output);
}

std::wstring UTF16ToWide(const string16& utf16) {
  std::wstring ret;
  UTF16ToWide(utf16.data(), utf16.length(), &ret);
  return ret;
}

#endif  



#if defined(WCHAR_T_IS_UTF32)

bool UTF8ToUTF16(const char* src, size_t src_len, string16* output) {
  PrepareForUTF16Or32Output(src, src_len, output);
  return ConvertUnicode(src, src_len, output);
}

string16 UTF8ToUTF16(const StringPiece& utf8) {
  string16 ret;
  
  
  UTF8ToUTF16(utf8.data(), utf8.length(), &ret);
  return ret;
}

bool UTF16ToUTF8(const char16* src, size_t src_len, std::string* output) {
  PrepareForUTF8Output(src, src_len, output);
  return ConvertUnicode(src, src_len, output);
}

std::string UTF16ToUTF8(const string16& utf16) {
  std::string ret;
  
  
  UTF16ToUTF8(utf16.data(), utf16.length(), &ret);
  return ret;
}

#elif defined(WCHAR_T_IS_UTF16)


bool UTF8ToUTF16(const char* src, size_t src_len, string16* output) {
  return UTF8ToWide(src, src_len, output);
}

string16 UTF8ToUTF16(const StringPiece& utf8) {
  return UTF8ToWide(utf8);
}

bool UTF16ToUTF8(const char16* src, size_t src_len, std::string* output) {
  return WideToUTF8(src, src_len, output);
}

std::string UTF16ToUTF8(const string16& utf16) {
  return WideToUTF8(utf16);
}

#endif

std::wstring ASCIIToWide(const StringPiece& ascii) {
  DCHECK(IsStringASCII(ascii)) << ascii;
  return std::wstring(ascii.begin(), ascii.end());
}

string16 ASCIIToUTF16(const StringPiece& ascii) {
  DCHECK(IsStringASCII(ascii)) << ascii;
  return string16(ascii.begin(), ascii.end());
}

std::string UTF16ToASCII(const string16& utf16) {
  DCHECK(IsStringASCII(utf16)) << UTF16ToUTF8(utf16);
  return std::string(utf16.begin(), utf16.end());
}

}  
