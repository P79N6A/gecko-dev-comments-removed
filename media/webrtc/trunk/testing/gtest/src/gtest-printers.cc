











































#include "gtest/gtest-printers.h"
#include <ctype.h>
#include <stdio.h>
#include <ostream>  
#include <string>
#include "gtest/internal/gtest-port.h"

namespace testing {

namespace {

using ::std::ostream;

#if GTEST_OS_WINDOWS_MOBILE  
# define snprintf _snprintf
#elif _MSC_VER >= 1400  
# define snprintf _snprintf_s
#elif _MSC_VER
# define snprintf _snprintf
#endif  


void PrintByteSegmentInObjectTo(const unsigned char* obj_bytes, size_t start,
                                size_t count, ostream* os) {
  char text[5] = "";
  for (size_t i = 0; i != count; i++) {
    const size_t j = start + i;
    if (i != 0) {
      
      
      if ((j % 2) == 0)
        *os << ' ';
      else
        *os << '-';
    }
    snprintf(text, sizeof(text), "%02X", obj_bytes[j]);
    *os << text;
  }
}


void PrintBytesInObjectToImpl(const unsigned char* obj_bytes, size_t count,
                              ostream* os) {
  
  *os << count << "-byte object <";

  const size_t kThreshold = 132;
  const size_t kChunkSize = 64;
  
  
  
  
  if (count < kThreshold) {
    PrintByteSegmentInObjectTo(obj_bytes, 0, count, os);
  } else {
    PrintByteSegmentInObjectTo(obj_bytes, 0, kChunkSize, os);
    *os << " ... ";
    
    const size_t resume_pos = (count - kChunkSize + 1)/2*2;
    PrintByteSegmentInObjectTo(obj_bytes, resume_pos, count - resume_pos, os);
  }
  *os << ">";
}

}  

namespace internal2 {






void PrintBytesInObjectTo(const unsigned char* obj_bytes, size_t count,
                          ostream* os) {
  PrintBytesInObjectToImpl(obj_bytes, count, os);
}

}  

namespace internal {






enum CharFormat {
  kAsIs,
  kHexEscape,
  kSpecialEscape
};




inline bool IsPrintableAscii(wchar_t c) {
  return 0x20 <= c && c <= 0x7E;
}





template <typename UnsignedChar, typename Char>
static CharFormat PrintAsCharLiteralTo(Char c, ostream* os) {
  switch (static_cast<wchar_t>(c)) {
    case L'\0':
      *os << "\\0";
      break;
    case L'\'':
      *os << "\\'";
      break;
    case L'\\':
      *os << "\\\\";
      break;
    case L'\a':
      *os << "\\a";
      break;
    case L'\b':
      *os << "\\b";
      break;
    case L'\f':
      *os << "\\f";
      break;
    case L'\n':
      *os << "\\n";
      break;
    case L'\r':
      *os << "\\r";
      break;
    case L'\t':
      *os << "\\t";
      break;
    case L'\v':
      *os << "\\v";
      break;
    default:
      if (IsPrintableAscii(c)) {
        *os << static_cast<char>(c);
        return kAsIs;
      } else {
        *os << String::Format("\\x%X", static_cast<UnsignedChar>(c));
        return kHexEscape;
      }
  }
  return kSpecialEscape;
}



static CharFormat PrintAsWideStringLiteralTo(wchar_t c, ostream* os) {
  switch (c) {
    case L'\'':
      *os << "'";
      return kAsIs;
    case L'"':
      *os << "\\\"";
      return kSpecialEscape;
    default:
      return PrintAsCharLiteralTo<wchar_t>(c, os);
  }
}



static CharFormat PrintAsNarrowStringLiteralTo(char c, ostream* os) {
  return PrintAsWideStringLiteralTo(static_cast<unsigned char>(c), os);
}





template <typename UnsignedChar, typename Char>
void PrintCharAndCodeTo(Char c, ostream* os) {
  
  *os << ((sizeof(c) > 1) ? "L'" : "'");
  const CharFormat format = PrintAsCharLiteralTo<UnsignedChar>(c, os);
  *os << "'";

  
  
  
  if (c == 0)
    return;
  *os << " (" << String::Format("%d", c).c_str();

  
  
  
  if (format == kHexEscape || (1 <= c && c <= 9)) {
    
  } else {
    *os << String::Format(", 0x%X",
                          static_cast<UnsignedChar>(c)).c_str();
  }
  *os << ")";
}

void PrintTo(unsigned char c, ::std::ostream* os) {
  PrintCharAndCodeTo<unsigned char>(c, os);
}
void PrintTo(signed char c, ::std::ostream* os) {
  PrintCharAndCodeTo<unsigned char>(c, os);
}



void PrintTo(wchar_t wc, ostream* os) {
  PrintCharAndCodeTo<wchar_t>(wc, os);
}




static void PrintCharsAsStringTo(const char* begin, size_t len, ostream* os) {
  *os << "\"";
  bool is_previous_hex = false;
  for (size_t index = 0; index < len; ++index) {
    const char cur = begin[index];
    if (is_previous_hex && IsXDigit(cur)) {
      
      
      
      *os << "\" \"";
    }
    is_previous_hex = PrintAsNarrowStringLiteralTo(cur, os) == kHexEscape;
  }
  *os << "\"";
}


void UniversalPrintArray(const char* begin, size_t len, ostream* os) {
  PrintCharsAsStringTo(begin, len, os);
}




static void PrintWideCharsAsStringTo(const wchar_t* begin, size_t len,
                                     ostream* os) {
  *os << "L\"";
  bool is_previous_hex = false;
  for (size_t index = 0; index < len; ++index) {
    const wchar_t cur = begin[index];
    if (is_previous_hex && isascii(cur) && IsXDigit(static_cast<char>(cur))) {
      
      
      
      *os << "\" L\"";
    }
    is_previous_hex = PrintAsWideStringLiteralTo(cur, os) == kHexEscape;
  }
  *os << "\"";
}


void PrintTo(const char* s, ostream* os) {
  if (s == NULL) {
    *os << "NULL";
  } else {
    *os << ImplicitCast_<const void*>(s) << " pointing to ";
    PrintCharsAsStringTo(s, strlen(s), os);
  }
}







#if !defined(_MSC_VER) || defined(_NATIVE_WCHAR_T_DEFINED)

void PrintTo(const wchar_t* s, ostream* os) {
  if (s == NULL) {
    *os << "NULL";
  } else {
    *os << ImplicitCast_<const void*>(s) << " pointing to ";
    PrintWideCharsAsStringTo(s, wcslen(s), os);
  }
}
#endif  


#if GTEST_HAS_GLOBAL_STRING
void PrintStringTo(const ::string& s, ostream* os) {
  PrintCharsAsStringTo(s.data(), s.size(), os);
}
#endif  

void PrintStringTo(const ::std::string& s, ostream* os) {
  PrintCharsAsStringTo(s.data(), s.size(), os);
}


#if GTEST_HAS_GLOBAL_WSTRING
void PrintWideStringTo(const ::wstring& s, ostream* os) {
  PrintWideCharsAsStringTo(s.data(), s.size(), os);
}
#endif  

#if GTEST_HAS_STD_WSTRING
void PrintWideStringTo(const ::std::wstring& s, ostream* os) {
  PrintWideCharsAsStringTo(s.data(), s.size(), os);
}
#endif  

}  

}  
