



#include "base/string_util.h"

#include <string.h>
#include <vector>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/singleton.h"
#include "unicode/ucnv.h"
#include "unicode/numfmt.h"
#include "unicode/ustring.h"

namespace {










bool ReadUnicodeCharacter(const char* src, int32 src_len,
                          int32* char_index, uint32* code_point_out) {
  
  
  
  int32 code_point;
  U8_NEXT(src, *char_index, src_len, code_point);
  *code_point_out = static_cast<uint32>(code_point);

  
  
  (*char_index)--;

  
  return U_IS_UNICODE_CHAR(code_point);
}


bool ReadUnicodeCharacter(const char16* src, int32 src_len,
                          int32* char_index, uint32* code_point) {
  if (U16_IS_SURROGATE(src[*char_index])) {
    if (!U16_IS_SURROGATE_LEAD(src[*char_index]) ||
        *char_index + 1 >= src_len ||
        !U16_IS_TRAIL(src[*char_index + 1])) {
      
      return false;
    }

    
    *code_point = U16_GET_SUPPLEMENTARY(src[*char_index],
                                        src[*char_index + 1]);
    (*char_index)++;
  } else {
    
    *code_point = src[*char_index];
  }

  return U_IS_UNICODE_CHAR(*code_point);
}

#if defined(WCHAR_T_IS_UTF32)

bool ReadUnicodeCharacter(const wchar_t* src, int32 src_len,
                        int32* char_index, uint32* code_point) {
  
  *code_point = src[*char_index];

  
  return U_IS_UNICODE_CHAR(*code_point);
}
#endif  




void WriteUnicodeCharacter(uint32 code_point, std::string* output) {
  if (code_point <= 0x7f) {
    
    output->push_back(code_point);
    return;
  }

  
  int32 char_offset = static_cast<int32>(output->length());
  output->resize(char_offset + U8_MAX_LENGTH);

  U8_APPEND_UNSAFE(&(*output)[0], char_offset, code_point);

  
  
  output->resize(char_offset);
}


void WriteUnicodeCharacter(uint32 code_point, string16* output) {
  if (U16_LENGTH(code_point) == 1) {
    
    output->push_back(static_cast<char16>(code_point));
  } else {
    
    int32 char_offset = static_cast<int32>(output->length());
    output->resize(char_offset + U16_MAX_LENGTH);
    U16_APPEND_UNSAFE(&(*output)[0], char_offset, code_point);
  }
}

#if defined(WCHAR_T_IS_UTF32)

inline void WriteUnicodeCharacter(uint32 code_point, std::wstring* output) {
  
  output->push_back(code_point);
}
#endif  







template<typename SRC_CHAR, typename DEST_STRING>
bool ConvertUnicode(const SRC_CHAR* src, size_t src_len, DEST_STRING* output) {
  output->clear();

  
  bool success = true;
  int32 src_len32 = static_cast<int32>(src_len);
  for (int32 i = 0; i < src_len32; i++) {
    uint32 code_point;
    if (ReadUnicodeCharacter(src, src_len32, &i, &code_point))
      WriteUnicodeCharacter(code_point, output);
    else
      success = false;
  }
  return success;
}






template<typename CHAR>
void ReserveUTF8Output(const CHAR* src, size_t src_len, std::string* output) {
  if (src[0] < 0x80) {
    
    output->reserve(src_len);
  } else {
    
    output->reserve(src_len * 3);
  }
}




template<typename STRING>
void ReserveUTF16Or32Output(const char* src, size_t src_len, STRING* output) {
  if (static_cast<unsigned char>(src[0]) < 0x80) {
    
    output->reserve(src_len);
  } else {
    
    
    output->reserve(src_len / 2);
  }
}

}  



std::string WideToUTF8(const std::wstring& wide) {
  std::string ret;
  if (wide.empty())
    return ret;

  
  
  WideToUTF8(wide.data(), wide.length(), &ret);
  return ret;
}

bool WideToUTF8(const wchar_t* src, size_t src_len, std::string* output) {
  if (src_len == 0) {
    output->clear();
    return true;
  }

  ReserveUTF8Output(src, src_len, output);
  return ConvertUnicode<wchar_t, std::string>(src, src_len, output);
}

std::wstring UTF8ToWide(const StringPiece& utf8) {
  std::wstring ret;
  if (utf8.empty())
    return ret;

  UTF8ToWide(utf8.data(), utf8.length(), &ret);
  return ret;
}

bool UTF8ToWide(const char* src, size_t src_len, std::wstring* output) {
  if (src_len == 0) {
    output->clear();
    return true;
  }

  ReserveUTF16Or32Output(src, src_len, output);
  return ConvertUnicode<char, std::wstring>(src, src_len, output);
}



#if defined(WCHAR_T_IS_UTF16)


string16 WideToUTF16(const std::wstring& wide) {
  return wide;
}

bool WideToUTF16(const wchar_t* src, size_t src_len, string16* output) {
  output->assign(src, src_len);
  return true;
}

std::wstring UTF16ToWide(const string16& utf16) {
  return utf16;
}

bool UTF16ToWide(const char16* src, size_t src_len, std::wstring* output) {
  output->assign(src, src_len);
  return true;
}

#elif defined(WCHAR_T_IS_UTF32)

string16 WideToUTF16(const std::wstring& wide) {
  string16 ret;
  if (wide.empty())
    return ret;

  WideToUTF16(wide.data(), wide.length(), &ret);
  return ret;
}

bool WideToUTF16(const wchar_t* src, size_t src_len, string16* output) {
  if (src_len == 0) {
    output->clear();
    return true;
  }

  
  
  output->reserve(src_len);
  return ConvertUnicode<wchar_t, string16>(src, src_len, output);
}

std::wstring UTF16ToWide(const string16& utf16) {
  std::wstring ret;
  if (utf16.empty())
    return ret;

  UTF16ToWide(utf16.data(), utf16.length(), &ret);
  return ret;
}

bool UTF16ToWide(const char16* src, size_t src_len, std::wstring* output) {
  if (src_len == 0) {
    output->clear();
    return true;
  }

  
  
  output->reserve(src_len);
  return ConvertUnicode<char16, std::wstring>(src, src_len, output);
}

#endif  



#if defined(WCHAR_T_IS_UTF32)

bool UTF8ToUTF16(const char* src, size_t src_len, string16* output) {
  if (src_len == 0) {
    output->clear();
    return true;
  }

  ReserveUTF16Or32Output(src, src_len, output);
  return ConvertUnicode<char, string16>(src, src_len, output);
}

string16 UTF8ToUTF16(const std::string& utf8) {
  string16 ret;
  if (utf8.empty())
    return ret;

  
  
  UTF8ToUTF16(utf8.data(), utf8.length(), &ret);
  return ret;
}

bool UTF16ToUTF8(const char16* src, size_t src_len, std::string* output) {
  if (src_len == 0) {
    output->clear();
    return true;
  }

  ReserveUTF8Output(src, src_len, output);
  return ConvertUnicode<char16, std::string>(src, src_len, output);
}

std::string UTF16ToUTF8(const string16& utf16) {
  std::string ret;
  if (utf16.empty())
    return ret;

  
  
  UTF16ToUTF8(utf16.data(), utf16.length(), &ret);
  return ret;
}

#elif defined(WCHAR_T_IS_UTF16)


bool UTF8ToUTF16(const char* src, size_t src_len, string16* output) {
  return UTF8ToWide(src, src_len, output);
}

string16 UTF8ToUTF16(const std::string& utf8) {
  return UTF8ToWide(utf8);
}

bool UTF16ToUTF8(const char16* src, size_t src_len, std::string* output) {
  return WideToUTF8(src, src_len, output);
}

std::string UTF16ToUTF8(const string16& utf16) {
  return WideToUTF8(utf16);
}

#endif





bool WideToCodepage(const std::wstring& wide,
                    const char* codepage_name,
                    OnStringUtilConversionError::Type on_error,
                    std::string* encoded) {
  encoded->clear();

  UErrorCode status = U_ZERO_ERROR;
  UConverter* converter = ucnv_open(codepage_name, &status);
  if (!U_SUCCESS(status))
    return false;

  const UChar* uchar_src;
  int uchar_len;
#if defined(WCHAR_T_IS_UTF16)
  uchar_src = wide.c_str();
  uchar_len = static_cast<int>(wide.length());
#elif defined(WCHAR_T_IS_UTF32)
  
  
  
  
  
  std::vector<UChar> wide_uchar(wide.length() * 2 + 1);
  u_strFromWCS(&wide_uchar[0], wide_uchar.size(), &uchar_len,
               wide.c_str(), wide.length(), &status);
  uchar_src = &wide_uchar[0];
  DCHECK(U_SUCCESS(status)) << "failed to convert wstring to UChar*";
#endif  

  int encoded_max_length = UCNV_GET_MAX_BYTES_FOR_STRING(uchar_len,
    ucnv_getMaxCharSize(converter));
  encoded->resize(encoded_max_length);

  
  switch (on_error) {
    case OnStringUtilConversionError::FAIL:
      ucnv_setFromUCallBack(converter, UCNV_FROM_U_CALLBACK_STOP, 0,
                            NULL, NULL, &status);
      break;
    case OnStringUtilConversionError::SKIP:
      ucnv_setFromUCallBack(converter, UCNV_FROM_U_CALLBACK_SKIP, 0,
                            NULL, NULL, &status);
      break;
    default:
      NOTREACHED();
  }

  
  int actual_size = ucnv_fromUChars(converter, &(*encoded)[0],
    encoded_max_length, uchar_src, uchar_len, &status);
  encoded->resize(actual_size);
  ucnv_close(converter);
  if (U_SUCCESS(status))
    return true;
  encoded->clear();  
  return false;
}



bool CodepageToWide(const std::string& encoded,
                    const char* codepage_name,
                    OnStringUtilConversionError::Type on_error,
                    std::wstring* wide) {
  wide->clear();

  UErrorCode status = U_ZERO_ERROR;
  UConverter* converter = ucnv_open(codepage_name, &status);
  if (!U_SUCCESS(status))
    return false;

  
  size_t uchar_max_length = encoded.length() * 2 + 1;

  UChar* uchar_dst;
#if defined(WCHAR_T_IS_UTF16)
  uchar_dst = WriteInto(wide, uchar_max_length);
#elif defined(WCHAR_T_IS_UTF32)
  
  
  std::vector<UChar> wide_uchar(uchar_max_length);
  uchar_dst = &wide_uchar[0];
#endif  

  
  switch (on_error) {
    case OnStringUtilConversionError::FAIL:
      ucnv_setToUCallBack(converter, UCNV_TO_U_CALLBACK_STOP, 0,
                          NULL, NULL, &status);
      break;
    case OnStringUtilConversionError::SKIP:
      ucnv_setToUCallBack(converter, UCNV_TO_U_CALLBACK_SKIP, 0,
                          NULL, NULL, &status);
      break;
    default:
      NOTREACHED();
  }

  int actual_size = ucnv_toUChars(converter,
                                  uchar_dst,
                                  static_cast<int>(uchar_max_length),
                                  encoded.data(),
                                  static_cast<int>(encoded.length()),
                                  &status);
  ucnv_close(converter);
  if (!U_SUCCESS(status)) {
    wide->clear();  
    return false;
  }

#ifdef WCHAR_T_IS_UTF32
  
  
  
  
  u_strToWCS(WriteInto(wide, actual_size + 1), actual_size + 1, &actual_size,
             uchar_dst, actual_size, &status);
  DCHECK(U_SUCCESS(status)) << "failed to convert UChar* to wstring";
#endif  

  wide->resize(actual_size);
  return true;
}



namespace {

struct NumberFormatSingletonTraits
    : public DefaultSingletonTraits<NumberFormat> {
  static NumberFormat* New() {
    UErrorCode status = U_ZERO_ERROR;
    NumberFormat* formatter = NumberFormat::createInstance(status);
    DCHECK(U_SUCCESS(status));
    return formatter;
  }
  
  
  
  
};

}  

std::wstring FormatNumber(int64 number) {
  NumberFormat* number_format =
      Singleton<NumberFormat, NumberFormatSingletonTraits>::get();

  if (!number_format) {
    
    return StringPrintf(L"%lld", number);
  }
  UnicodeString ustr;
  number_format->format(number, ustr);

#if defined(WCHAR_T_IS_UTF16)
  return std::wstring(ustr.getBuffer(),
                      static_cast<std::wstring::size_type>(ustr.length()));
#elif defined(WCHAR_T_IS_UTF32)
  wchar_t buffer[64];  
                       
  int length = 0;
  UErrorCode error = U_ZERO_ERROR;
  u_strToWCS(buffer, 64, &length, ustr.getBuffer(), ustr.length() , &error);
  if (U_FAILURE(error)) {
    NOTREACHED();
    
    return StringPrintf(L"%lld", number);
  }
  return std::wstring(buffer, static_cast<std::wstring::size_type>(length));
#endif  
}

TrimPositions TrimWhitespaceUTF8(const std::string& input,
                                 TrimPositions positions,
                                 std::string* output) {
  
  
  
  DCHECK(IsStringUTF8(input));
  std::wstring input_wide = UTF8ToWide(input);
  std::wstring output_wide;
  TrimPositions result = TrimWhitespace(input_wide, positions, &output_wide);
  *output = WideToUTF8(output_wide);
  return result;
}
