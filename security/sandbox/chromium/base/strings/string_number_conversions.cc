



#include "base/strings/string_number_conversions.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <wctype.h>

#include <limits>

#include "base/logging.h"
#include "base/scoped_clear_errno.h"
#include "base/strings/utf_string_conversions.h"
#include "base/third_party/dmg_fp/dmg_fp.h"

namespace base {

namespace {

template <typename STR, typename INT, typename UINT, bool NEG>
struct IntToStringT {
  
  
  
  
  
  
  
  template <typename INT2, typename UINT2, bool NEG2>
  struct ToUnsignedT {};

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

  
  
  template <typename INT2, bool NEG2>
  struct TestNegT {};
  template <typename INT2>
  struct TestNegT<INT2, false> {
    static bool TestNeg(INT2 value) {
      
      return false;
    }
  };
  template <typename INT2>
  struct TestNegT<INT2, true> {
    static bool TestNeg(INT2 value) {
      return value < 0;
    }
  };

  static STR IntToString(INT value) {
    
    
    const int kOutputBufSize = 3 * sizeof(INT) + 1;

    
    
    STR outbuf(kOutputBufSize, 0);

    bool is_neg = TestNegT<INT, NEG>::TestNeg(value);
    
    
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


template<typename CHAR, int BASE, bool BASE_LTE_10> class BaseCharToDigit {
};


template<typename CHAR, int BASE> class BaseCharToDigit<CHAR, BASE, true> {
 public:
  static bool Convert(CHAR c, uint8* digit) {
    if (c >= '0' && c < '0' + BASE) {
      *digit = c - '0';
      return true;
    }
    return false;
  }
};


template<typename CHAR, int BASE> class BaseCharToDigit<CHAR, BASE, false> {
 public:
  static bool Convert(CHAR c, uint8* digit) {
    if (c >= '0' && c <= '9') {
      *digit = c - '0';
    } else if (c >= 'a' && c < 'a' + BASE - 10) {
      *digit = c - 'a' + 10;
    } else if (c >= 'A' && c < 'A' + BASE - 10) {
      *digit = c - 'A' + 10;
    } else {
      return false;
    }
    return true;
  }
};

template<int BASE, typename CHAR> bool CharToDigit(CHAR c, uint8* digit) {
  return BaseCharToDigit<CHAR, BASE, BASE <= 10>::Convert(c, digit);
}





template<typename CHAR> class WhitespaceHelper {
};

template<> class WhitespaceHelper<char> {
 public:
  static bool Invoke(char c) {
    return 0 != isspace(static_cast<unsigned char>(c));
  }
};

template<> class WhitespaceHelper<char16> {
 public:
  static bool Invoke(char16 c) {
    return 0 != iswspace(c);
  }
};

template<typename CHAR> bool LocalIsWhitespace(CHAR c) {
  return WhitespaceHelper<CHAR>::Invoke(c);
}







template<typename IteratorRangeToNumberTraits>
class IteratorRangeToNumber {
 public:
  typedef IteratorRangeToNumberTraits traits;
  typedef typename traits::iterator_type const_iterator;
  typedef typename traits::value_type value_type;

  
  
  static bool Invoke(const_iterator begin,
                     const_iterator end,
                     value_type* output) {
    bool valid = true;

    while (begin != end && LocalIsWhitespace(*begin)) {
      valid = false;
      ++begin;
    }

    if (begin != end && *begin == '-') {
      if (!std::numeric_limits<value_type>::is_signed) {
        valid = false;
      } else if (!Negative::Invoke(begin + 1, end, output)) {
        valid = false;
      }
    } else {
      if (begin != end && *begin == '+') {
        ++begin;
      }
      if (!Positive::Invoke(begin, end, output)) {
        valid = false;
      }
    }

    return valid;
  }

 private:
  
  
  
  
  
  template<typename Sign>
  class Base {
   public:
    static bool Invoke(const_iterator begin, const_iterator end,
                       typename traits::value_type* output) {
      *output = 0;

      if (begin == end) {
        return false;
      }

      
      
      if (traits::kBase == 16 && end - begin > 2 && *begin == '0' &&
          (*(begin + 1) == 'x' || *(begin + 1) == 'X')) {
        begin += 2;
      }

      for (const_iterator current = begin; current != end; ++current) {
        uint8 new_digit = 0;

        if (!CharToDigit<traits::kBase>(*current, &new_digit)) {
          return false;
        }

        if (current != begin) {
          if (!Sign::CheckBounds(output, new_digit)) {
            return false;
          }
          *output *= traits::kBase;
        }

        Sign::Increment(new_digit, output);
      }
      return true;
    }
  };

  class Positive : public Base<Positive> {
   public:
    static bool CheckBounds(value_type* output, uint8 new_digit) {
      if (*output > static_cast<value_type>(traits::max() / traits::kBase) ||
          (*output == static_cast<value_type>(traits::max() / traits::kBase) &&
           new_digit > traits::max() % traits::kBase)) {
        *output = traits::max();
        return false;
      }
      return true;
    }
    static void Increment(uint8 increment, value_type* output) {
      *output += increment;
    }
  };

  class Negative : public Base<Negative> {
   public:
    static bool CheckBounds(value_type* output, uint8 new_digit) {
      if (*output < traits::min() / traits::kBase ||
          (*output == traits::min() / traits::kBase &&
           new_digit > 0 - traits::min() % traits::kBase)) {
        *output = traits::min();
        return false;
      }
      return true;
    }
    static void Increment(uint8 increment, value_type* output) {
      *output -= increment;
    }
  };
};

template<typename ITERATOR, typename VALUE, int BASE>
class BaseIteratorRangeToNumberTraits {
 public:
  typedef ITERATOR iterator_type;
  typedef VALUE value_type;
  static value_type min() {
    return std::numeric_limits<value_type>::min();
  }
  static value_type max() {
    return std::numeric_limits<value_type>::max();
  }
  static const int kBase = BASE;
};

template<typename ITERATOR>
class BaseHexIteratorRangeToIntTraits
    : public BaseIteratorRangeToNumberTraits<ITERATOR, int, 16> {
};

template<typename ITERATOR>
class BaseHexIteratorRangeToInt64Traits
    : public BaseIteratorRangeToNumberTraits<ITERATOR, int64, 16> {
};

template<typename ITERATOR>
class BaseHexIteratorRangeToUInt64Traits
    : public BaseIteratorRangeToNumberTraits<ITERATOR, uint64, 16> {
};

typedef BaseHexIteratorRangeToIntTraits<StringPiece::const_iterator>
    HexIteratorRangeToIntTraits;

typedef BaseHexIteratorRangeToInt64Traits<StringPiece::const_iterator>
    HexIteratorRangeToInt64Traits;

typedef BaseHexIteratorRangeToUInt64Traits<StringPiece::const_iterator>
    HexIteratorRangeToUInt64Traits;

template<typename STR>
bool HexStringToBytesT(const STR& input, std::vector<uint8>* output) {
  DCHECK_EQ(output->size(), 0u);
  size_t count = input.size();
  if (count == 0 || (count % 2) != 0)
    return false;
  for (uintptr_t i = 0; i < count / 2; ++i) {
    uint8 msb = 0;  
    uint8 lsb = 0;  
    if (!CharToDigit<16>(input[i * 2], &msb) ||
        !CharToDigit<16>(input[i * 2 + 1], &lsb))
      return false;
    output->push_back((msb << 4) | lsb);
  }
  return true;
}

template <typename VALUE, int BASE>
class StringPieceToNumberTraits
    : public BaseIteratorRangeToNumberTraits<StringPiece::const_iterator,
                                             VALUE,
                                             BASE> {
};

template <typename VALUE>
bool StringToIntImpl(const StringPiece& input, VALUE* output) {
  return IteratorRangeToNumber<StringPieceToNumberTraits<VALUE, 10> >::Invoke(
      input.begin(), input.end(), output);
}

template <typename VALUE, int BASE>
class StringPiece16ToNumberTraits
    : public BaseIteratorRangeToNumberTraits<StringPiece16::const_iterator,
                                             VALUE,
                                             BASE> {
};

template <typename VALUE>
bool String16ToIntImpl(const StringPiece16& input, VALUE* output) {
  return IteratorRangeToNumber<StringPiece16ToNumberTraits<VALUE, 10> >::Invoke(
      input.begin(), input.end(), output);
}

}  

std::string IntToString(int value) {
  return IntToStringT<std::string, int, unsigned int, true>::
      IntToString(value);
}

string16 IntToString16(int value) {
  return IntToStringT<string16, int, unsigned int, true>::
      IntToString(value);
}

std::string UintToString(unsigned int value) {
  return IntToStringT<std::string, unsigned int, unsigned int, false>::
      IntToString(value);
}

string16 UintToString16(unsigned int value) {
  return IntToStringT<string16, unsigned int, unsigned int, false>::
      IntToString(value);
}

std::string Int64ToString(int64 value) {
  return IntToStringT<std::string, int64, uint64, true>::
      IntToString(value);
}

string16 Int64ToString16(int64 value) {
  return IntToStringT<string16, int64, uint64, true>::IntToString(value);
}

std::string Uint64ToString(uint64 value) {
  return IntToStringT<std::string, uint64, uint64, false>::
      IntToString(value);
}

string16 Uint64ToString16(uint64 value) {
  return IntToStringT<string16, uint64, uint64, false>::
      IntToString(value);
}

std::string DoubleToString(double value) {
  
  char buffer[32];
  dmg_fp::g_fmt(buffer, value);
  return std::string(buffer);
}

bool StringToInt(const StringPiece& input, int* output) {
  return StringToIntImpl(input, output);
}

bool StringToInt(const StringPiece16& input, int* output) {
  return String16ToIntImpl(input, output);
}

bool StringToUint(const StringPiece& input, unsigned* output) {
  return StringToIntImpl(input, output);
}

bool StringToUint(const StringPiece16& input, unsigned* output) {
  return String16ToIntImpl(input, output);
}

bool StringToInt64(const StringPiece& input, int64* output) {
  return StringToIntImpl(input, output);
}

bool StringToInt64(const StringPiece16& input, int64* output) {
  return String16ToIntImpl(input, output);
}

bool StringToUint64(const StringPiece& input, uint64* output) {
  return StringToIntImpl(input, output);
}

bool StringToUint64(const StringPiece16& input, uint64* output) {
  return String16ToIntImpl(input, output);
}

bool StringToSizeT(const StringPiece& input, size_t* output) {
  return StringToIntImpl(input, output);
}

bool StringToSizeT(const StringPiece16& input, size_t* output) {
  return String16ToIntImpl(input, output);
}

bool StringToDouble(const std::string& input, double* output) {
  
  ScopedClearErrno clear_errno;

  char* endptr = NULL;
  *output = dmg_fp::strtod(input.c_str(), &endptr);

  
  
  
  
  
  
  
  
  
  return errno == 0 &&
         !input.empty() &&
         input.c_str() + input.length() == endptr &&
         !isspace(input[0]);
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

bool HexStringToInt(const StringPiece& input, int* output) {
  return IteratorRangeToNumber<HexIteratorRangeToIntTraits>::Invoke(
    input.begin(), input.end(), output);
}

bool HexStringToInt64(const StringPiece& input, int64* output) {
  return IteratorRangeToNumber<HexIteratorRangeToInt64Traits>::Invoke(
    input.begin(), input.end(), output);
}

bool HexStringToUInt64(const StringPiece& input, uint64* output) {
  return IteratorRangeToNumber<HexIteratorRangeToUInt64Traits>::Invoke(
      input.begin(), input.end(), output);
}

bool HexStringToBytes(const std::string& input, std::vector<uint8>* output) {
  return HexStringToBytesT(input, output);
}

}  
