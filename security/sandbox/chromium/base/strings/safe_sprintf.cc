



#include "base/strings/safe_sprintf.h"

#include <limits>

#if !defined(NDEBUG)





















#include "base/logging.h"
#define DEBUG_CHECK RAW_CHECK
#else
#define DEBUG_CHECK(x) do { if (x) { } } while (0)
#endif

namespace base {
namespace strings {























namespace {
const size_t kSSizeMaxConst = ((size_t)(ssize_t)-1) >> 1;

const char kUpCaseHexDigits[]   = "0123456789ABCDEF";
const char kDownCaseHexDigits[] = "0123456789abcdef";
}

#if defined(NDEBUG)




namespace {
const size_t kSSizeMax = kSSizeMaxConst;
}
#else  





namespace {
static size_t kSSizeMax = kSSizeMaxConst;
}

namespace internal {
void SetSafeSPrintfSSizeMaxForTest(size_t max) {
  kSSizeMax = max;
}

size_t GetSafeSPrintfSSizeMaxForTest() {
  return kSSizeMax;
}
}
#endif  

namespace {
class Buffer {
 public:
  
  
  
  
  
  Buffer(char* buffer, size_t size)
      : buffer_(buffer),
        size_(size - 1),  
        count_(0) {







#if __cplusplus >= 201103 && !defined(OS_ANDROID) && !defined(OS_MACOSX) && \
    !defined(OS_IOS) && !(defined(__clang__) && defined(OS_WIN))
    COMPILE_ASSERT(kSSizeMaxConst == \
                   static_cast<size_t>(std::numeric_limits<ssize_t>::max()),
                   kSSizeMax_is_the_max_value_of_an_ssize_t);
#endif
    DEBUG_CHECK(size > 0);
    DEBUG_CHECK(size <= kSSizeMax);
  }

  ~Buffer() {
    
    
    
    
    
    
    *GetInsertionPoint() = '\000';
  }

  
  
  
  inline bool OutOfAddressableSpace() const {
    return count_ == static_cast<size_t>(kSSizeMax - 1);
  }

  
  
  
  
  inline ssize_t GetCount() const {
    DEBUG_CHECK(count_ < kSSizeMax);
    return static_cast<ssize_t>(count_);
  }

  
  
  
  
  
  
  inline bool Out(char ch) {
    if (size_ >= 1 && count_ < size_) {
      buffer_[count_] = ch;
      return IncrementCountByOne();
    }
    
    
    
    IncrementCountByOne();
    return false;
  }

  
  
  
  
  
  
  inline bool Pad(char pad, size_t padding, size_t len) {
    DEBUG_CHECK(pad);
    DEBUG_CHECK(padding <= kSSizeMax);
    for (; padding > len; --padding) {
      if (!Out(pad)) {
        if (--padding) {
          IncrementCount(padding-len);
        }
        return false;
      }
    }
    return true;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool IToASCII(bool sign, bool upcase, int64_t i, int base,
                char pad, size_t padding, const char* prefix);

 private:
  
  
  
  inline bool IncrementCount(size_t inc) {
    
    
    
    
    
    DEBUG_CHECK(inc <= kSSizeMax - 1);
    if (count_ > kSSizeMax - 1 - inc) {
      count_ = kSSizeMax - 1;
      return false;
    } else {
      count_ += inc;
      return true;
    }
  }

  
  inline bool IncrementCountByOne() {
    return IncrementCount(1);
  }

  
  
  
  
  inline char* GetInsertionPoint() const {
    size_t idx = count_;
    if (idx > size_) {
      idx = size_;
    }
    return buffer_ + idx;
  }

  
  char* buffer_;

  
  
  const size_t size_;

  
  
  
  size_t count_;

  DISALLOW_COPY_AND_ASSIGN(Buffer);
};


bool Buffer::IToASCII(bool sign, bool upcase, int64_t i, int base,
                      char pad, size_t padding, const char* prefix) {
  
  
  DEBUG_CHECK(base >= 2);
  DEBUG_CHECK(base <= 16);
  DEBUG_CHECK(!sign || base == 10);
  DEBUG_CHECK(pad == '0' || pad == ' ');
  DEBUG_CHECK(padding <= kSSizeMax);
  DEBUG_CHECK(!(sign && prefix && *prefix));

  
  
  
  
  
  
  
  
  int minint = 0;
  uint64_t num;
  if (sign && i < 0) {
    prefix = "-";

    
    if (i == std::numeric_limits<int64_t>::min()) {
      
      minint = 1;
      num = static_cast<uint64_t>(-(i + 1));
    } else {
      
      num = static_cast<uint64_t>(-i);
    }
  } else {
    num = static_cast<uint64_t>(i);
  }

  
  
  
  
  
  
  const char* reverse_prefix = NULL;
  if (prefix && *prefix) {
    if (pad == '0') {
      while (*prefix) {
        if (padding) {
          --padding;
        }
        Out(*prefix++);
      }
      prefix = NULL;
    } else {
      for (reverse_prefix = prefix; *reverse_prefix; ++reverse_prefix) {
      }
    }
  } else
    prefix = NULL;
  const size_t prefix_length = reverse_prefix - prefix;

  
  
  size_t start = count_;
  size_t discarded = 0;
  bool started = false;
  do {
    
    if (count_ >= size_) {
      if (start < size_) {
        
        
        
        
        
        
        
        for (char* move = buffer_ + start, *end = buffer_ + size_ - 1;
             move < end;
             ++move) {
          *move = move[1];
        }
        ++discarded;
        --count_;
      } else if (count_ - size_ > 1) {
        
        
        
        
        
        
        --count_;
        ++discarded;
      }
    }

    
    
    
    
    if (!num && started) {
      if (reverse_prefix > prefix) {
        Out(*--reverse_prefix);
      } else {
        Out(pad);
      }
    } else {
      started = true;
      Out((upcase ? kUpCaseHexDigits : kDownCaseHexDigits)[num%base + minint]);
    }

    minint = 0;
    num /= base;

    
    if (padding > 0) {
      --padding;

      
      
      
      
      
      
      
      if (discarded > 8*sizeof(num) + prefix_length) {
        IncrementCount(padding);
        padding = 0;
      }
    }
  } while (num || padding || (reverse_prefix > prefix));

  
  
  
  
  char* front = buffer_ + start;
  char* back = GetInsertionPoint();
  while (--back > front) {
    char ch = *back;
    *back = *front;
    *front++ = ch;
  }

  IncrementCount(discarded);
  return !discarded;
}

}  

namespace internal {

ssize_t SafeSNPrintf(char* buf, size_t sz, const char* fmt, const Arg* args,
                     const size_t max_args) {
  
  
  
  
  if (static_cast<ssize_t>(sz) < 1) {
    return -1;
  } else if (sz > kSSizeMax) {
    sz = kSSizeMax;
  }

  
  
  Buffer buffer(buf, sz);
  size_t padding;
  char pad;
  for (unsigned int cur_arg = 0; *fmt && !buffer.OutOfAddressableSpace(); ) {
    if (*fmt++ == '%') {
      padding = 0;
      pad = ' ';
      char ch = *fmt++;
    format_character_found:
      switch (ch) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        
        
        
        pad = ch == '0' ? '0' : ' ';
        for (;;) {
          
          
          const size_t max_padding = kSSizeMax - 1;
          if (padding > max_padding/10 ||
              10*padding > max_padding - (ch - '0')) {
            DEBUG_CHECK(padding <= max_padding/10 &&
                        10*padding <= max_padding - (ch - '0'));
            
            
          padding_overflow:
            padding = max_padding;
            while ((ch = *fmt++) >= '0' && ch <= '9') {
            }
            if (cur_arg < max_args) {
              ++cur_arg;
            }
            goto fail_to_expand;
          }
          padding = 10*padding + ch - '0';
          if (padding > max_padding) {
            
            
            
            
            DEBUG_CHECK(padding <= max_padding);
            goto padding_overflow;
          }
          ch = *fmt++;
          if (ch < '0' || ch > '9') {
            
            
            goto format_character_found;
          }
        }
        break;
      case 'c': {  
        
        if (cur_arg >= max_args) {
          DEBUG_CHECK(cur_arg < max_args);
          goto fail_to_expand;
        }

        
        const Arg& arg = args[cur_arg++];
        if (arg.type != Arg::INT && arg.type != Arg::UINT) {
          DEBUG_CHECK(arg.type == Arg::INT || arg.type == Arg::UINT);
          goto fail_to_expand;
        }

        
        buffer.Pad(' ', padding, 1);

        
        char ch = static_cast<char>(arg.integer.i);
        if (!ch) {
          goto end_of_output_buffer;
        }
        buffer.Out(ch);
        break; }
      case 'd':    
      case 'o':    
      case 'x':    
      case 'X':
      case 'p': {  
        
        if (cur_arg >= max_args) {
          DEBUG_CHECK(cur_arg < max_args);
          goto fail_to_expand;
        }

        const Arg& arg = args[cur_arg++];
        int64_t i;
        const char* prefix = NULL;
        if (ch != 'p') {
          
          if (arg.type != Arg::INT && arg.type != Arg::UINT) {
            DEBUG_CHECK(arg.type == Arg::INT || arg.type == Arg::UINT);
            goto fail_to_expand;
          }
          i = arg.integer.i;

          if (ch != 'd') {
            
            
            
            
            
            
            
            if (arg.integer.width < sizeof(int64_t)) {
              i &= (1LL << (8*arg.integer.width)) - 1;
            }
          }
        } else {
          
          if (arg.type == Arg::POINTER) {
            i = reinterpret_cast<uintptr_t>(arg.ptr);
          } else if (arg.type == Arg::STRING) {
            i = reinterpret_cast<uintptr_t>(arg.str);
          } else if (arg.type == Arg::INT &&
                     arg.integer.width == sizeof(NULL) &&
                     arg.integer.i == 0) {  
            i = 0;
          } else {
            DEBUG_CHECK(arg.type == Arg::POINTER || arg.type == Arg::STRING);
            goto fail_to_expand;
          }

          
          prefix = "0x";
        }

        
        
        
        
        
        buffer.IToASCII(ch == 'd' && arg.type == Arg::INT,
                        ch != 'x', i,
                        ch == 'o' ? 8 : ch == 'd' ? 10 : 16,
                        pad, padding, prefix);
        break; }
      case 's': {
        
        if (cur_arg >= max_args) {
          DEBUG_CHECK(cur_arg < max_args);
          goto fail_to_expand;
        }

        
        const Arg& arg = args[cur_arg++];
        const char *s;
        if (arg.type == Arg::STRING) {
          s = arg.str ? arg.str : "<NULL>";
        } else if (arg.type == Arg::INT && arg.integer.width == sizeof(NULL) &&
                   arg.integer.i == 0) {  
          s = "<NULL>";
        } else {
          DEBUG_CHECK(arg.type == Arg::STRING);
          goto fail_to_expand;
        }

        
        
        if (padding) {
          size_t len = 0;
          for (const char* src = s; *src++; ) {
            ++len;
          }
          buffer.Pad(' ', padding, len);
        }

        
        
        
        for (const char* src = s; *src; ) {
          buffer.Out(*src++);
        }
        break; }
      case '%':
        
        goto copy_verbatim;
      fail_to_expand:
        
        
        
        
        
        
        
        
        
        
      default:
        
        
        buffer.Out('%');
        DEBUG_CHECK(ch);
        if (!ch) {
          goto end_of_format_string;
        }
        buffer.Out(ch);
        break;
      }
    } else {
  copy_verbatim:
    buffer.Out(fmt[-1]);
    }
  }
 end_of_format_string:
 end_of_output_buffer:
  return buffer.GetCount();
}

}  

ssize_t SafeSNPrintf(char* buf, size_t sz, const char* fmt) {
  
  
  
  
  if (static_cast<ssize_t>(sz) < 1) {
    return -1;
  } else if (sz > kSSizeMax) {
    sz = kSSizeMax;
  }

  Buffer buffer(buf, sz);

  
  
  
  
  const char* src = fmt;
  for (; *src; ++src) {
    buffer.Out(*src);
    DEBUG_CHECK(src[0] != '%' || src[1] == '%');
    if (src[0] == '%' && src[1] == '%') {
      ++src;
    }
  }
  return buffer.GetCount();
}

}  
}  
