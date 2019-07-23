



#include "base/string_util.h"

#include "build/build_config.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>

#include <algorithm>
#include <vector>

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/singleton.h"
#include "base/third_party/dmg_fp/dmg_fp.h"

namespace {




struct EmptyStrings {
  EmptyStrings() {}
  const std::string s;
  const std::wstring ws;
  const string16 s16;
};




template<typename T>
struct ToUnsigned {
  typedef T Unsigned;
};

template<>
struct ToUnsigned<char> {
  typedef unsigned char Unsigned;
};
template<>
struct ToUnsigned<signed char> {
  typedef unsigned char Unsigned;
};
template<>
struct ToUnsigned<wchar_t> {
#if defined(WCHAR_T_IS_UTF16)
  typedef unsigned short Unsigned;
#elif defined(WCHAR_T_IS_UTF32)
  typedef uint32 Unsigned;
#endif
};
template<>
struct ToUnsigned<short> {
  typedef unsigned short Unsigned;
};



struct ReplacementOffset {
  ReplacementOffset(int parameter, size_t offset)
      : parameter(parameter),
        offset(offset) {}

  
  int parameter;

  
  size_t offset;
};

static bool CompareParameter(const ReplacementOffset& elem1,
                             const ReplacementOffset& elem2) {
  return elem1.parameter < elem2.parameter;
}













template<typename StringToNumberTraits>
bool StringToNumber(const typename StringToNumberTraits::string_type& input,
                    typename StringToNumberTraits::value_type* output) {
  typedef StringToNumberTraits traits;

  errno = 0;  
  typename traits::string_type::value_type* endptr = NULL;
  typename traits::value_type value = traits::convert_func(input.c_str(),
                                                           &endptr);
  *output = value;

  
  
  
  
  
  
  
  
  
  return errno == 0 &&
         !input.empty() &&
         input.c_str() + input.length() == endptr &&
         traits::valid_func(input);
}

class StringToLongTraits {
 public:
  typedef std::string string_type;
  typedef long value_type;
  static const int kBase = 10;
  static inline value_type convert_func(const string_type::value_type* str,
                                        string_type::value_type** endptr) {
    return strtol(str, endptr, kBase);
  }
  static inline bool valid_func(const string_type& str) {
    return !str.empty() && !isspace(str[0]);
  }
};

class String16ToLongTraits {
 public:
  typedef string16 string_type;
  typedef long value_type;
  static const int kBase = 10;
  static inline value_type convert_func(const string_type::value_type* str,
                                        string_type::value_type** endptr) {
#if defined(WCHAR_T_IS_UTF16)
    return wcstol(str, endptr, kBase);
#elif defined(WCHAR_T_IS_UTF32)
    std::string ascii_string = UTF16ToASCII(string16(str));
    char* ascii_end = NULL;
    value_type ret = strtol(ascii_string.c_str(), &ascii_end, kBase);
    if (ascii_string.c_str() + ascii_string.length() == ascii_end) {
      *endptr =
          const_cast<string_type::value_type*>(str) + ascii_string.length();
    }
    return ret;
#endif
  }
  static inline bool valid_func(const string_type& str) {
    return !str.empty() && !iswspace(str[0]);
  }
};

class StringToInt64Traits {
 public:
  typedef std::string string_type;
  typedef int64 value_type;
  static const int kBase = 10;
  static inline value_type convert_func(const string_type::value_type* str,
                                        string_type::value_type** endptr) {
#ifdef OS_WIN
    return _strtoi64(str, endptr, kBase);
#else  
    return strtoll(str, endptr, kBase);
#endif
  }
  static inline bool valid_func(const string_type& str) {
    return !str.empty() && !isspace(str[0]);
  }
};

class String16ToInt64Traits {
 public:
  typedef string16 string_type;
  typedef int64 value_type;
  static const int kBase = 10;
  static inline value_type convert_func(const string_type::value_type* str,
                                        string_type::value_type** endptr) {
#ifdef OS_WIN
    return _wcstoi64(str, endptr, kBase);
#else  
    std::string ascii_string = UTF16ToASCII(string16(str));
    char* ascii_end = NULL;
    value_type ret = strtoll(ascii_string.c_str(), &ascii_end, kBase);
    if (ascii_string.c_str() + ascii_string.length() == ascii_end) {
      *endptr =
          const_cast<string_type::value_type*>(str) + ascii_string.length();
    }
    return ret;
#endif
  }
  static inline bool valid_func(const string_type& str) {
    return !str.empty() && !iswspace(str[0]);
  }
};




class HexStringToLongTraits {
 public:
  typedef std::string string_type;
  typedef long value_type;
  static const int kBase = 16;
  static inline value_type convert_func(const string_type::value_type* str,
                                        string_type::value_type** endptr) {
    return strtoul(str, endptr, kBase);
  }
  static inline bool valid_func(const string_type& str) {
    return !str.empty() && !isspace(str[0]);
  }
};

class HexString16ToLongTraits {
 public:
  typedef string16 string_type;
  typedef long value_type;
  static const int kBase = 16;
  static inline value_type convert_func(const string_type::value_type* str,
                                        string_type::value_type** endptr) {
#if defined(WCHAR_T_IS_UTF16)
    return wcstoul(str, endptr, kBase);
#elif defined(WCHAR_T_IS_UTF32)
    std::string ascii_string = UTF16ToASCII(string16(str));
    char* ascii_end = NULL;
    value_type ret = strtoul(ascii_string.c_str(), &ascii_end, kBase);
    if (ascii_string.c_str() + ascii_string.length() == ascii_end) {
      *endptr =
          const_cast<string_type::value_type*>(str) + ascii_string.length();
    }
    return ret;
#endif
  }
  static inline bool valid_func(const string_type& str) {
    return !str.empty() && !iswspace(str[0]);
  }
};

class StringToDoubleTraits {
 public:
  typedef std::string string_type;
  typedef double value_type;
  static inline value_type convert_func(const string_type::value_type* str,
                                        string_type::value_type** endptr) {
    return dmg_fp::strtod(str, endptr);
  }
  static inline bool valid_func(const string_type& str) {
    return !str.empty() && !isspace(str[0]);
  }
};

class String16ToDoubleTraits {
 public:
  typedef string16 string_type;
  typedef double value_type;
  static inline value_type convert_func(const string_type::value_type* str,
                                        string_type::value_type** endptr) {
    
    
    
    
    std::string ascii_string = UTF16ToASCII(string16(str));
    char* ascii_end = NULL;
    value_type ret = dmg_fp::strtod(ascii_string.c_str(), &ascii_end);
    if (ascii_string.c_str() + ascii_string.length() == ascii_end) {
      
      *endptr =
          const_cast<string_type::value_type*>(str) + ascii_string.length();
    }

    return ret;
  }
  static inline bool valid_func(const string_type& str) {
    return !str.empty() && !iswspace(str[0]);
  }
};

}  


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

#ifdef CHROMIUM_MOZILLA_BUILD
namespace base {
#endif

const std::string& EmptyString() {
  return Singleton<EmptyStrings>::get()->s;
}

const std::wstring& EmptyWString() {
  return Singleton<EmptyStrings>::get()->ws;
}

const string16& EmptyString16() {
  return Singleton<EmptyStrings>::get()->s16;
}

#ifdef CHROMIUM_MOZILLA_BUILD
}
#endif

const wchar_t kWhitespaceWide[] = {
  0x0009,  
  0x000A,
  0x000B,
  0x000C,
  0x000D,
  0x0020,  
  0x0085,  
  0x00A0,  
  0x1680,  
  0x180E,  
  0x2000,  
  0x2001,
  0x2002,
  0x2003,
  0x2004,
  0x2005,
  0x2006,
  0x2007,
  0x2008,
  0x2009,
  0x200A,
  0x200C,  
  0x2028,  
  0x2029,  
  0x202F,  
  0x205F,  
  0x3000,  
  0
};
const char kWhitespaceASCII[] = {
  0x09,    
  0x0A,
  0x0B,
  0x0C,
  0x0D,
  0x20,    
  0
};
const char* const kCodepageUTF8 = "UTF-8";

template<typename STR>
TrimPositions TrimStringT(const STR& input,
                          const typename STR::value_type trim_chars[],
                          TrimPositions positions,
                          STR* output) {
  
  const typename STR::size_type last_char = input.length() - 1;
  const typename STR::size_type first_good_char = (positions & TRIM_LEADING) ?
      input.find_first_not_of(trim_chars) : 0;
  const typename STR::size_type last_good_char = (positions & TRIM_TRAILING) ?
      input.find_last_not_of(trim_chars) : last_char;

  
  
  
  if (input.empty() ||
      (first_good_char == STR::npos) || (last_good_char == STR::npos)) {
    bool input_was_empty = input.empty();  
    output->clear();
    return input_was_empty ? TRIM_NONE : positions;
  }

  
  *output =
      input.substr(first_good_char, last_good_char - first_good_char + 1);

  
  return static_cast<TrimPositions>(
      ((first_good_char == 0) ? TRIM_NONE : TRIM_LEADING) |
      ((last_good_char == last_char) ? TRIM_NONE : TRIM_TRAILING));
}

bool TrimString(const std::wstring& input,
                const wchar_t trim_chars[],
                std::wstring* output) {
  return TrimStringT(input, trim_chars, TRIM_ALL, output) != TRIM_NONE;
}

bool TrimString(const std::string& input,
                const char trim_chars[],
                std::string* output) {
  return TrimStringT(input, trim_chars, TRIM_ALL, output) != TRIM_NONE;
}

TrimPositions TrimWhitespace(const std::wstring& input,
                             TrimPositions positions,
                             std::wstring* output) {
  return TrimStringT(input, kWhitespaceWide, positions, output);
}

TrimPositions TrimWhitespaceASCII(const std::string& input,
                                  TrimPositions positions,
                                  std::string* output) {
  return TrimStringT(input, kWhitespaceASCII, positions, output);
}



TrimPositions TrimWhitespace(const std::string& input,
                             TrimPositions positions,
                             std::string* output) {
  return TrimWhitespaceASCII(input, positions, output);
}

std::wstring CollapseWhitespace(const std::wstring& text,
                                bool trim_sequences_with_line_breaks) {
  std::wstring result;
  result.resize(text.size());

  
  
  bool in_whitespace = true;
  bool already_trimmed = true;

  int chars_written = 0;
  for (std::wstring::const_iterator i(text.begin()); i != text.end(); ++i) {
    if (IsWhitespace(*i)) {
      if (!in_whitespace) {
        
        in_whitespace = true;
        result[chars_written++] = L' ';
      }
      if (trim_sequences_with_line_breaks && !already_trimmed &&
          ((*i == '\n') || (*i == '\r'))) {
        
        already_trimmed = true;
        --chars_written;
      }
    } else {
      
      in_whitespace = false;
      already_trimmed = false;
      result[chars_written++] = *i;
    }
  }

  if (in_whitespace && !already_trimmed) {
    
    --chars_written;
  }

  result.resize(chars_written);
  return result;
}

std::string WideToASCII(const std::wstring& wide) {
  DCHECK(IsStringASCII(wide));
  return std::string(wide.begin(), wide.end());
}

std::wstring ASCIIToWide(const std::string& ascii) {
  DCHECK(IsStringASCII(ascii));
  return std::wstring(ascii.begin(), ascii.end());
}

std::string UTF16ToASCII(const string16& utf16) {
  DCHECK(IsStringASCII(utf16));
  return std::string(utf16.begin(), utf16.end());
}

string16 ASCIIToUTF16(const std::string& ascii) {
  DCHECK(IsStringASCII(ascii));
  return string16(ascii.begin(), ascii.end());
}


bool WideToLatin1(const std::wstring& wide, std::string* latin1) {
  std::string output;
  output.resize(wide.size());
  latin1->clear();
  for (size_t i = 0; i < wide.size(); i++) {
    if (wide[i] > 255)
      return false;
    output[i] = static_cast<char>(wide[i]);
  }
  latin1->swap(output);
  return true;
}

bool IsString8Bit(const std::wstring& str) {
  for (size_t i = 0; i < str.length(); i++) {
    if (str[i] > 255)
      return false;
  }
  return true;
}

template<class STR>
static bool DoIsStringASCII(const STR& str) {
  for (size_t i = 0; i < str.length(); i++) {
    typename ToUnsigned<typename STR::value_type>::Unsigned c = str[i];
    if (c > 0x7F)
      return false;
  }
  return true;
}

bool IsStringASCII(const std::wstring& str) {
  return DoIsStringASCII(str);
}

#if !defined(WCHAR_T_IS_UTF16)
bool IsStringASCII(const string16& str) {
  return DoIsStringASCII(str);
}
#endif

bool IsStringASCII(const std::string& str) {
  return DoIsStringASCII(str);
}





static inline bool IsBegin2ByteUTF8(int c) {
  return (c & 0xE0) == 0xC0;
}
static inline bool IsBegin3ByteUTF8(int c) {
  return (c & 0xF0) == 0xE0;
}
static inline bool IsBegin4ByteUTF8(int c) {
  return (c & 0xF8) == 0xF0;
}
static inline bool IsInUTF8Sequence(int c) {
  return (c & 0xC0) == 0x80;
}




























template<typename CHAR>
static bool IsStringUTF8T(const CHAR* str, int length) {
  bool overlong = false;
  bool surrogate = false;
  bool nonchar = false;

  
  typename ToUnsigned<CHAR>::Unsigned olupper = 0;

  
  typename ToUnsigned<CHAR>::Unsigned slower = 0;

  
  
  int positions_left = 0;

  for (int i = 0; i < length; i++) {
    
    
    typename ToUnsigned<CHAR>::Unsigned c = str[i];
    if (c < 0x80)
      continue;  

    if (c <= 0xC1) {
      
      return false;
    } else if (IsBegin2ByteUTF8(c)) {
      positions_left = 1;
    } else if (IsBegin3ByteUTF8(c)) {
      positions_left = 2;
      if (c == 0xE0) {
        
        overlong = true;
        olupper = 0x9F;
      } else if (c == 0xED) {
        
        surrogate = true;
        slower = 0xA0;
      } else if (c == 0xEF) {
        
        
        nonchar = true;
      }
    } else if (c <= 0xF4) {
      positions_left = 3;
      nonchar = true;
      if (c == 0xF0) {
        
        overlong = true;
        olupper = 0x8F;
      } else if (c == 0xF4) {
        
        
        surrogate = true;
        slower = 0x90;
      }
    } else {
      return false;
    }

    
    while (positions_left) {
      positions_left--;
      i++;
      c = str[i];
      if (!c)
        return false;  

      
      if (nonchar && ((!positions_left && c < 0xBE) ||
                      (positions_left == 1 && c != 0xBF) ||
                      (positions_left == 2 && 0x0F != (0x0F & c) ))) {
        nonchar = false;
      }
      if (!IsInUTF8Sequence(c) || (overlong && c <= olupper) ||
          (surrogate && slower <= c) || (nonchar && !positions_left) ) {
        return false;
      }
      overlong = surrogate = false;
    }
  }
  return true;
}

bool IsStringUTF8(const std::string& str) {
  return IsStringUTF8T(str.data(), str.length());
}

bool IsStringWideUTF8(const std::wstring& str) {
  return IsStringUTF8T(str.data(), str.length());
}

template<typename Iter>
static inline bool DoLowerCaseEqualsASCII(Iter a_begin,
                                          Iter a_end,
                                          const char* b) {
  for (Iter it = a_begin; it != a_end; ++it, ++b) {
    if (!*b || ToLowerASCII(*it) != *b)
      return false;
  }
  return *b == 0;
}


bool LowerCaseEqualsASCII(const std::string& a, const char* b) {
  return DoLowerCaseEqualsASCII(a.begin(), a.end(), b);
}

bool LowerCaseEqualsASCII(const std::wstring& a, const char* b) {
  return DoLowerCaseEqualsASCII(a.begin(), a.end(), b);
}

bool LowerCaseEqualsASCII(std::string::const_iterator a_begin,
                          std::string::const_iterator a_end,
                          const char* b) {
  return DoLowerCaseEqualsASCII(a_begin, a_end, b);
}

bool LowerCaseEqualsASCII(std::wstring::const_iterator a_begin,
                          std::wstring::const_iterator a_end,
                          const char* b) {
  return DoLowerCaseEqualsASCII(a_begin, a_end, b);
}
bool LowerCaseEqualsASCII(const char* a_begin,
                          const char* a_end,
                          const char* b) {
  return DoLowerCaseEqualsASCII(a_begin, a_end, b);
}
bool LowerCaseEqualsASCII(const wchar_t* a_begin,
                          const wchar_t* a_end,
                          const char* b) {
  return DoLowerCaseEqualsASCII(a_begin, a_end, b);
}

bool StartsWithASCII(const std::string& str,
                     const std::string& search,
                     bool case_sensitive) {
  if (case_sensitive)
    return str.compare(0, search.length(), search) == 0;
  else
    return base::strncasecmp(str.c_str(), search.c_str(), search.length()) == 0;
}

bool StartsWith(const std::wstring& str,
                const std::wstring& search,
                bool case_sensitive) {
  if (case_sensitive)
    return str.compare(0, search.length(), search) == 0;
  else {
    if (search.size() > str.size())
      return false;
    return std::equal(search.begin(), search.end(), str.begin(),
                      CaseInsensitiveCompare<wchar_t>());
  }
}

DataUnits GetByteDisplayUnits(int64 bytes) {
  
  
  
  static const int64 kUnitThresholds[] = {
    0,              
    3*1024,         
    2*1024*1024,    
    1024*1024*1024  
  };

  if (bytes < 0) {
    NOTREACHED() << "Negative bytes value";
    return DATA_UNITS_BYTE;
  }

  int unit_index = arraysize(kUnitThresholds);
  while (--unit_index > 0) {
    if (bytes >= kUnitThresholds[unit_index])
      break;
  }

  DCHECK(unit_index >= DATA_UNITS_BYTE && unit_index <= DATA_UNITS_GIGABYTE);
  return DataUnits(unit_index);
}



static const wchar_t* const kByteStrings[] = {
  L"B",
  L"kB",
  L"MB",
  L"GB"
};

static const wchar_t* const kSpeedStrings[] = {
  L"B/s",
  L"kB/s",
  L"MB/s",
  L"GB/s"
};

std::wstring FormatBytesInternal(int64 bytes,
                                 DataUnits units,
                                 bool show_units,
                                 const wchar_t* const* suffix) {
  if (bytes < 0) {
    NOTREACHED() << "Negative bytes value";
    return std::wstring();
  }

  DCHECK(units >= DATA_UNITS_BYTE && units <= DATA_UNITS_GIGABYTE);

  
  double unit_amount = static_cast<double>(bytes);
  for (int i = 0; i < units; ++i)
    unit_amount /= 1024.0;

  wchar_t tmp[64];
  
  double int_part;
  double fractional_part = modf(unit_amount, &int_part);
  modf(fractional_part * 10, &int_part);
  if (int_part == 0) {
    base::swprintf(tmp, arraysize(tmp),
                   L"%lld", static_cast<int64>(unit_amount));
  } else {
    base::swprintf(tmp, arraysize(tmp), L"%.1lf", unit_amount);
  }

  std::wstring ret(tmp);
  if (show_units) {
    ret += L" ";
    ret += suffix[units];
  }

  return ret;
}

std::wstring FormatBytes(int64 bytes, DataUnits units, bool show_units) {
  return FormatBytesInternal(bytes, units, show_units, kByteStrings);
}

std::wstring FormatSpeed(int64 bytes, DataUnits units, bool show_units) {
  return FormatBytesInternal(bytes, units, show_units, kSpeedStrings);
}

template<class StringType>
void DoReplaceSubstringsAfterOffset(StringType* str,
                                    typename StringType::size_type start_offset,
                                    const StringType& find_this,
                                    const StringType& replace_with,
                                    bool replace_all) {
  if ((start_offset == StringType::npos) || (start_offset >= str->length()))
    return;

  DCHECK(!find_this.empty());
  for (typename StringType::size_type offs(str->find(find_this, start_offset));
      offs != StringType::npos; offs = str->find(find_this, offs)) {
    str->replace(offs, find_this.length(), replace_with);
    offs += replace_with.length();

    if (!replace_all)
      break;
  }
}

void ReplaceFirstSubstringAfterOffset(string16* str,
                                      string16::size_type start_offset,
                                      const string16& find_this,
                                      const string16& replace_with) {
  DoReplaceSubstringsAfterOffset(str, start_offset, find_this, replace_with,
                                 false);  
}

void ReplaceFirstSubstringAfterOffset(std::string* str,
                                      std::string::size_type start_offset,
                                      const std::string& find_this,
                                      const std::string& replace_with) {
  DoReplaceSubstringsAfterOffset(str, start_offset, find_this, replace_with,
                                 false);  
}

void ReplaceSubstringsAfterOffset(string16* str,
                                  string16::size_type start_offset,
                                  const string16& find_this,
                                  const string16& replace_with) {
  DoReplaceSubstringsAfterOffset(str, start_offset, find_this, replace_with,
                                 true);  
}

void ReplaceSubstringsAfterOffset(std::string* str,
                                  std::string::size_type start_offset,
                                  const std::string& find_this,
                                  const std::string& replace_with) {
  DoReplaceSubstringsAfterOffset(str, start_offset, find_this, replace_with,
                                 true);  
}







inline int vsnprintfT(char* buffer,
                      size_t buf_size,
                      const char* format,
                      va_list argptr) {
  return base::vsnprintf(buffer, buf_size, format, argptr);
}

inline int vsnprintfT(wchar_t* buffer,
                      size_t buf_size,
                      const wchar_t* format,
                      va_list argptr) {
  return base::vswprintf(buffer, buf_size, format, argptr);
}



template <class StringType>
static void StringAppendVT(StringType* dst,
                           const typename StringType::value_type* format,
                           va_list ap) {
  
  
  
  typename StringType::value_type stack_buf[1024];

  va_list backup_ap;
  base::va_copy(backup_ap, ap);

#if !defined(OS_WIN)
  errno = 0;
#endif
  int result = vsnprintfT(stack_buf, arraysize(stack_buf), format, backup_ap);
  va_end(backup_ap);

  if (result >= 0 && result < static_cast<int>(arraysize(stack_buf))) {
    
    dst->append(stack_buf, result);
    return;
  }

  
  int mem_length = arraysize(stack_buf);
  while (true) {
    if (result < 0) {
#if !defined(OS_WIN)
      
      
      
      if (errno != 0 && errno != EOVERFLOW)
#endif
      {
        
        DLOG(WARNING) << "Unable to printf the requested string due to error.";
        return;
      }
      
      mem_length *= 2;
    } else {
      
      mem_length = result + 1;
    }

    if (mem_length > 32 * 1024 * 1024) {
      
      
      
      DLOG(WARNING) << "Unable to printf the requested string due to size.";
      return;
    }

    std::vector<typename StringType::value_type> mem_buf(mem_length);

    
    base::va_copy(backup_ap, ap);

    result = vsnprintfT(&mem_buf[0], mem_length, format, ap);
    va_end(backup_ap);

    if ((result >= 0) && (result < mem_length)) {
      
      dst->append(&mem_buf[0], result);
      return;
    }
  }
}

namespace {

template <typename STR, typename INT, typename UINT, bool NEG>
struct IntToStringT {

  
  
  
  
  
  
  
  template <typename INT2, typename UINT2, bool NEG2>
  struct ToUnsignedT { };

  template <typename INT2, typename UINT2>
  struct ToUnsignedT<INT2, UINT2, false> {
    static UINT2 ToUnsigned(INT2 value) {
      return static_cast<UINT2>(value);
    }
  };

  template <typename INT2, typename UINT2>
  struct ToUnsignedT<INT2, UINT2, true> {
    static UINT2 ToUnsigned(INT2 value) {
      return static_cast<UINT2>(value < 0 ? -value : value);
    }
  };

  static STR IntToString(INT value) {
    
    
    const int kOutputBufSize = 3 * sizeof(INT) + 1;

    
    
    STR outbuf(kOutputBufSize, 0);

    bool is_neg = value < 0;
    
    
    UINT res = ToUnsignedT<INT, UINT, NEG>::ToUnsigned(value);

    for (typename STR::iterator it = outbuf.end();;) {
      --it;
      DCHECK(it != outbuf.begin());
      *it = static_cast<typename STR::value_type>((res % 10) + '0');
      res /= 10;

      
      if (res == 0) {
        if (is_neg) {
          --it;
          DCHECK(it != outbuf.begin());
          *it = static_cast<typename STR::value_type>('-');
        }
        return STR(it, outbuf.end());
      }
    }
    NOTREACHED();
    return STR();
  }
};

}

std::string IntToString(int value) {
  return IntToStringT<std::string, int, unsigned int, true>::
      IntToString(value);
}
std::wstring IntToWString(int value) {
  return IntToStringT<std::wstring, int, unsigned int, true>::
      IntToString(value);
}
std::string UintToString(unsigned int value) {
  return IntToStringT<std::string, unsigned int, unsigned int, false>::
      IntToString(value);
}
std::wstring UintToWString(unsigned int value) {
  return IntToStringT<std::wstring, unsigned int, unsigned int, false>::
      IntToString(value);
}
std::string Int64ToString(int64 value) {
  return IntToStringT<std::string, int64, uint64, true>::
      IntToString(value);
}
std::wstring Int64ToWString(int64 value) {
  return IntToStringT<std::wstring, int64, uint64, true>::
      IntToString(value);
}
std::string Uint64ToString(uint64 value) {
  return IntToStringT<std::string, uint64, uint64, false>::
      IntToString(value);
}
std::wstring Uint64ToWString(uint64 value) {
  return IntToStringT<std::wstring, uint64, uint64, false>::
      IntToString(value);
}

std::string DoubleToString(double value) {
  
  char buffer[32];
  dmg_fp::g_fmt(buffer, value);
  return std::string(buffer);
}

std::wstring DoubleToWString(double value) {
  return ASCIIToWide(DoubleToString(value));
}

void StringAppendV(std::string* dst, const char* format, va_list ap) {
  StringAppendVT(dst, format, ap);
}

void StringAppendV(std::wstring* dst, const wchar_t* format, va_list ap) {
  StringAppendVT(dst, format, ap);
}

std::string StringPrintf(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  std::string result;
  StringAppendV(&result, format, ap);
  va_end(ap);
  return result;
}

std::wstring StringPrintf(const wchar_t* format, ...) {
  va_list ap;
  va_start(ap, format);
  std::wstring result;
  StringAppendV(&result, format, ap);
  va_end(ap);
  return result;
}

const std::string& SStringPrintf(std::string* dst, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  dst->clear();
  StringAppendV(dst, format, ap);
  va_end(ap);
  return *dst;
}

const std::wstring& SStringPrintf(std::wstring* dst,
                                  const wchar_t* format, ...) {
  va_list ap;
  va_start(ap, format);
  dst->clear();
  StringAppendV(dst, format, ap);
  va_end(ap);
  return *dst;
}

void StringAppendF(std::string* dst, const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  StringAppendV(dst, format, ap);
  va_end(ap);
}

void StringAppendF(std::wstring* dst, const wchar_t* format, ...) {
  va_list ap;
  va_start(ap, format);
  StringAppendV(dst, format, ap);
  va_end(ap);
}

template<typename STR>
static void SplitStringT(const STR& str,
                         const typename STR::value_type s,
                         bool trim_whitespace,
                         std::vector<STR>* r) {
  size_t last = 0;
  size_t i;
  size_t c = str.size();
  for (i = 0; i <= c; ++i) {
    if (i == c || str[i] == s) {
      size_t len = i - last;
      STR tmp = str.substr(last, len);
      if (trim_whitespace) {
        STR t_tmp;
        TrimWhitespace(tmp, TRIM_ALL, &t_tmp);
        r->push_back(t_tmp);
      } else {
        r->push_back(tmp);
      }
      last = i + 1;
    }
  }
}

void SplitString(const std::wstring& str,
                 wchar_t s,
                 std::vector<std::wstring>* r) {
  SplitStringT(str, s, true, r);
}

void SplitString(const std::string& str,
                 char s,
                 std::vector<std::string>* r) {
  SplitStringT(str, s, true, r);
}

void SplitStringDontTrim(const std::wstring& str,
                         wchar_t s,
                         std::vector<std::wstring>* r) {
  SplitStringT(str, s, false, r);
}

void SplitStringDontTrim(const std::string& str,
                         char s,
                         std::vector<std::string>* r) {
  SplitStringT(str, s, false, r);
}

template<typename STR>
static STR JoinStringT(const std::vector<STR>& parts,
                       typename STR::value_type sep) {
  if (parts.size() == 0) return STR();

  STR result(parts[0]);
  typename std::vector<STR>::const_iterator iter = parts.begin();
  ++iter;

  for (; iter != parts.end(); ++iter) {
    result += sep;
    result += *iter;
  }

  return result;
}

std::string JoinString(const std::vector<std::string>& parts, char sep) {
  return JoinStringT(parts, sep);
}

std::wstring JoinString(const std::vector<std::wstring>& parts, wchar_t sep) {
  return JoinStringT(parts, sep);
}

void SplitStringAlongWhitespace(const std::wstring& str,
                                std::vector<std::wstring>* result) {
  const size_t length = str.length();
  if (!length)
    return;

  bool last_was_ws = false;
  size_t last_non_ws_start = 0;
  for (size_t i = 0; i < length; ++i) {
    switch(str[i]) {
      
      case L' ':
      case L'\t':
      case L'\xA':
      case L'\xB':
      case L'\xC':
      case L'\xD':
        if (!last_was_ws) {
          if (i > 0) {
            result->push_back(
                str.substr(last_non_ws_start, i - last_non_ws_start));
          }
          last_was_ws = true;
        }
        break;

      default:  
        if (last_was_ws) {
          last_was_ws = false;
          last_non_ws_start = i;
        }
        break;
    }
  }
  if (!last_was_ws) {
    result->push_back(
              str.substr(last_non_ws_start, length - last_non_ws_start));
  }
}

string16 ReplaceStringPlaceholders(const string16& format_string,
                                   const string16& a,
                                   size_t* offset) {
  std::vector<size_t> offsets;
  string16 result = ReplaceStringPlaceholders(format_string, a,
                                              string16(),
                                              string16(),
                                              string16(), &offsets);
  DCHECK(offsets.size() == 1);
  if (offset) {
    *offset = offsets[0];
  }
  return result;
}

string16 ReplaceStringPlaceholders(const string16& format_string,
                                   const string16& a,
                                   const string16& b,
                                   std::vector<size_t>* offsets) {
  return ReplaceStringPlaceholders(format_string, a, b, string16(),
                                   string16(), offsets);
}

string16 ReplaceStringPlaceholders(const string16& format_string,
                                   const string16& a,
                                   const string16& b,
                                   const string16& c,
                                   std::vector<size_t>* offsets) {
  return ReplaceStringPlaceholders(format_string, a, b, c, string16(),
                                   offsets);
}

string16 ReplaceStringPlaceholders(const string16& format_string,
                                       const string16& a,
                                       const string16& b,
                                       const string16& c,
                                       const string16& d,
                                       std::vector<size_t>* offsets) {
  
  
  const string16* subst_texts[] = { &a, &b, &c, &d };

  string16 formatted;
  formatted.reserve(format_string.length() + a.length() +
      b.length() + c.length() + d.length());

  std::vector<ReplacementOffset> r_offsets;

  
  for (string16::const_iterator i = format_string.begin();
       i != format_string.end(); ++i) {
    if ('$' == *i) {
      if (i + 1 != format_string.end()) {
        ++i;
        DCHECK('$' == *i || ('1' <= *i && *i <= '4')) <<
            "Invalid placeholder: " << *i;
        if ('$' == *i) {
          formatted.push_back('$');
        } else {
          int index = *i - '1';
          if (offsets) {
            ReplacementOffset r_offset(index,
                                       static_cast<int>(formatted.size()));
            r_offsets.insert(std::lower_bound(r_offsets.begin(),
                                              r_offsets.end(), r_offset,
                                              &CompareParameter),
                             r_offset);
          }
          formatted.append(*subst_texts[index]);
        }
      }
    } else {
      formatted.push_back(*i);
    }
  }
  if (offsets) {
    for (std::vector<ReplacementOffset>::const_iterator i = r_offsets.begin();
         i != r_offsets.end(); ++i) {
      offsets->push_back(i->offset);
    }
  }
  return formatted;
}

template <class CHAR>
static bool IsWildcard(CHAR character) {
  return character == '*' || character == '?';
}


template <class CHAR>
static void EatSameChars(const CHAR** pattern, const CHAR** string) {
  bool escaped = false;
  while (**pattern && **string) {
    if (!escaped && IsWildcard(**pattern)) {
      
      return;
    }

    
    
    if (!escaped && **pattern == L'\\') {
      escaped = true;
      (*pattern)++;
      continue;
    }

    
    if (**pattern == **string) {
      (*pattern)++;
      (*string)++;
    } else {
      
      
      
      
      
      if (escaped) {
        (*pattern)--;
      }
      return;
    }

    escaped = false;
  }
}

template <class CHAR>
static void EatWildcard(const CHAR** pattern) {
  while(**pattern) {
    if (!IsWildcard(**pattern))
      return;
    (*pattern)++;
  }
}

template <class CHAR>
static bool MatchPatternT(const CHAR* eval, const CHAR* pattern) {
  
  EatSameChars(&pattern, &eval);

  
  
  if (*eval == 0) {
    EatWildcard(&pattern);
    if (*pattern)
      return false;
    return true;
  }

  
  if (*pattern == 0)
    return false;

  
  
  if (pattern[0] == '?') {
    if (MatchPatternT(eval, pattern + 1) ||
        MatchPatternT(eval + 1, pattern + 1))
      return true;
  }

  
  
  if (pattern[0] == '*') {
    while (*eval) {
      if (MatchPatternT(eval, pattern + 1))
        return true;
      eval++;
    }

    
    
    if (*eval == 0) {
      EatWildcard(&pattern);
      if (*pattern)
        return false;
      return true;
    }
  }

  return false;
}

bool MatchPattern(const std::wstring& eval, const std::wstring& pattern) {
  return MatchPatternT(eval.c_str(), pattern.c_str());
}

bool MatchPattern(const std::string& eval, const std::string& pattern) {
  return MatchPatternT(eval.c_str(), pattern.c_str());
}






bool StringToInt(const std::string& input, int* output) {
  COMPILE_ASSERT(sizeof(int) == sizeof(long), cannot_strtol_to_int);
  return StringToNumber<StringToLongTraits>(input,
                                            reinterpret_cast<long*>(output));
}

bool StringToInt(const string16& input, int* output) {
  COMPILE_ASSERT(sizeof(int) == sizeof(long), cannot_wcstol_to_int);
  return StringToNumber<String16ToLongTraits>(input,
                                              reinterpret_cast<long*>(output));
}

bool StringToInt64(const std::string& input, int64* output) {
  return StringToNumber<StringToInt64Traits>(input, output);
}

bool StringToInt64(const string16& input, int64* output) {
  return StringToNumber<String16ToInt64Traits>(input, output);
}

bool HexStringToInt(const std::string& input, int* output) {
  COMPILE_ASSERT(sizeof(int) == sizeof(long), cannot_strtol_to_int);
  return StringToNumber<HexStringToLongTraits>(input,
                                               reinterpret_cast<long*>(output));
}

bool HexStringToInt(const string16& input, int* output) {
  COMPILE_ASSERT(sizeof(int) == sizeof(long), cannot_wcstol_to_int);
  return StringToNumber<HexString16ToLongTraits>(
      input, reinterpret_cast<long*>(output));
}

namespace {

template<class CHAR>
bool HexDigitToIntT(const CHAR digit, uint8* val) {
  if (digit >= '0' && digit <= '9')
    *val = digit - '0';
  else if (digit >= 'a' && digit <= 'f')
    *val = 10 + digit - 'a';
  else if (digit >= 'A' && digit <= 'F')
    *val = 10 + digit - 'A';
  else
    return false;
  return true;
}

template<typename STR>
bool HexStringToBytesT(const STR& input, std::vector<uint8>* output) {
  DCHECK(output->size() == 0);
  int count = input.size();
  if (count == 0 || (count % 2) != 0)
    return false;
  for (int i = 0; i < count / 2; ++i) {
    uint8 msb = 0;  
    uint8 lsb = 0;  
    if (!HexDigitToIntT(input[i * 2], &msb) ||
        !HexDigitToIntT(input[i * 2 + 1], &lsb))
      return false;
    output->push_back((msb << 4) | lsb);
  }
  return true;
}

}  

bool HexStringToBytes(const std::string& input, std::vector<uint8>* output) {
  return HexStringToBytesT(input, output);
}

bool HexStringToBytes(const string16& input, std::vector<uint8>* output) {
  return HexStringToBytesT(input, output);
}

int StringToInt(const std::string& value) {
  int result;
  StringToInt(value, &result);
  return result;
}

int StringToInt(const string16& value) {
  int result;
  StringToInt(value, &result);
  return result;
}

int64 StringToInt64(const std::string& value) {
  int64 result;
  StringToInt64(value, &result);
  return result;
}

int64 StringToInt64(const string16& value) {
  int64 result;
  StringToInt64(value, &result);
  return result;
}

int HexStringToInt(const std::string& value) {
  int result;
  HexStringToInt(value, &result);
  return result;
}

int HexStringToInt(const string16& value) {
  int result;
  HexStringToInt(value, &result);
  return result;
}

bool StringToDouble(const std::string& input, double* output) {
  return StringToNumber<StringToDoubleTraits>(input, output);
}

bool StringToDouble(const string16& input, double* output) {
  return StringToNumber<String16ToDoubleTraits>(input, output);
}

double StringToDouble(const std::string& value) {
  double result;
  StringToDouble(value, &result);
  return result;
}

double StringToDouble(const string16& value) {
  double result;
  StringToDouble(value, &result);
  return result;
}





namespace {

template <typename CHAR>
size_t lcpyT(CHAR* dst, const CHAR* src, size_t dst_size) {
  for (size_t i = 0; i < dst_size; ++i) {
    if ((dst[i] = src[i]) == 0)  
      return i;
  }

  
  if (dst_size != 0)
    dst[dst_size - 1] = 0;

  
  while (src[dst_size]) ++dst_size;
  return dst_size;
}

}  

size_t base::strlcpy(char* dst, const char* src, size_t dst_size) {
  return lcpyT<char>(dst, src, dst_size);
}
size_t base::wcslcpy(wchar_t* dst, const wchar_t* src, size_t dst_size) {
  return lcpyT<wchar_t>(dst, src, dst_size);
}

bool ElideString(const std::wstring& input, int max_len, std::wstring* output) {
  DCHECK(max_len >= 0);
  if (static_cast<int>(input.length()) <= max_len) {
    output->assign(input);
    return false;
  }

  switch (max_len) {
    case 0:
      output->clear();
      break;
    case 1:
      output->assign(input.substr(0, 1));
      break;
    case 2:
      output->assign(input.substr(0, 2));
      break;
    case 3:
      output->assign(input.substr(0, 1) + L"." +
                     input.substr(input.length() - 1));
      break;
    case 4:
      output->assign(input.substr(0, 1) + L".." +
                     input.substr(input.length() - 1));
      break;
    default: {
      int rstr_len = (max_len - 3) / 2;
      int lstr_len = rstr_len + ((max_len - 3) % 2);
      output->assign(input.substr(0, lstr_len) + L"..." +
                     input.substr(input.length() - rstr_len));
      break;
    }
  }

  return true;
}

std::string HexEncode(const void* bytes, size_t size) {
  static const char kHexChars[] = "0123456789ABCDEF";

  
  std::string ret(size * 2, '\0');

  for (size_t i = 0; i < size; ++i) {
    char b = reinterpret_cast<const char*>(bytes)[i];
    ret[(i * 2)] = kHexChars[(b >> 4) & 0xf];
    ret[(i * 2) + 1] = kHexChars[b & 0xf];
  }
  return ret;
}
