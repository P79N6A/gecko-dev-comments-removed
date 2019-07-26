


























#ifndef DOUBLE_CONVERSION_UTILS_H_
#define DOUBLE_CONVERSION_UTILS_H_

#include <stdlib.h>
#include <string.h>

#include "mozilla/Assertions.h"
#ifndef ASSERT
#define ASSERT(condition)      MOZ_ASSERT(condition)
#endif
#ifndef UNIMPLEMENTED
#define UNIMPLEMENTED() MOZ_CRASH()
#endif
#ifndef UNREACHABLE
#define UNREACHABLE()   MOZ_CRASH()
#endif











#if defined(_M_X64) || defined(__x86_64__) || \
    defined(__ARMEL__) || defined(__avr32__) || \
    defined(__hppa__) || defined(__ia64__) || \
    defined(__mips__) || \
    defined(__powerpc__) || defined(__ppc__) || defined(__ppc64__) || \
    defined(__sparc__) || defined(__sparc) || defined(__s390__) || \
    defined(__SH4__) || defined(__alpha__) || \
    defined(_MIPS_ARCH_MIPS32R2) || \
    defined(_AARCH64EL_)
#define DOUBLE_CONVERSION_CORRECT_DOUBLE_OPERATIONS 1
#elif defined(_M_IX86) || defined(__i386__) || defined(__i386)
#if defined(_WIN32)

#define DOUBLE_CONVERSION_CORRECT_DOUBLE_OPERATIONS 1
#else
#undef DOUBLE_CONVERSION_CORRECT_DOUBLE_OPERATIONS
#endif  
#else
#error Target architecture was not detected as supported by Double-Conversion.
#endif


#include <stdint.h>




#define UINT64_2PART_C(a, b) (((static_cast<uint64_t>(a) << 32) + 0x##b##u))






#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)                                   \
  ((sizeof(a) / sizeof(*(a))) /                         \
  static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))
#endif



#ifndef DISALLOW_COPY_AND_ASSIGN
#define DISALLOW_COPY_AND_ASSIGN(TypeName)      \
  TypeName(const TypeName&);                    \
  void operator=(const TypeName&)
#endif







#ifndef DISALLOW_IMPLICIT_CONSTRUCTORS
#define DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName();                                    \
  DISALLOW_COPY_AND_ASSIGN(TypeName)
#endif

namespace double_conversion {

static const int kCharSize = sizeof(char);


template <typename T>
static T Max(T a, T b) {
  return a < b ? b : a;
}



template <typename T>
static T Min(T a, T b) {
  return a < b ? a : b;
}


inline int StrLength(const char* string) {
  size_t length = strlen(string);
  ASSERT(length == static_cast<size_t>(static_cast<int>(length)));
  return static_cast<int>(length);
}


template <typename T>
class Vector {
 public:
  Vector() : start_(NULL), length_(0) {}
  Vector(T* data, int length) : start_(data), length_(length) {
    ASSERT(length == 0 || (length > 0 && data != NULL));
  }

  
  
  Vector<T> SubVector(int from, int to) {
    ASSERT(to <= length_);
    ASSERT(from < to);
    ASSERT(0 <= from);
    return Vector<T>(start() + from, to - from);
  }

  
  int length() const { return length_; }

  
  bool is_empty() const { return length_ == 0; }

  
  T* start() const { return start_; }

  
  T& operator[](int index) const {
    ASSERT(0 <= index && index < length_);
    return start_[index];
  }

  T& first() { return start_[0]; }

  T& last() { return start_[length_ - 1]; }

 private:
  T* start_;
  int length_;
};





class StringBuilder {
 public:
  StringBuilder(char* buffer, int size)
      : buffer_(buffer, size), position_(0) { }

  ~StringBuilder() { if (!is_finalized()) Finalize(); }

  int size() const { return buffer_.length(); }

  
  int position() const {
    ASSERT(!is_finalized());
    return position_;
  }

  
  void Reset() { position_ = 0; }

  
  
  
  void AddCharacter(char c) {
    ASSERT(c != '\0');
    ASSERT(!is_finalized() && position_ < buffer_.length());
    buffer_[position_++] = c;
  }

  
  
  void AddString(const char* s) {
    AddSubstring(s, StrLength(s));
  }

  
  
  void AddSubstring(const char* s, int n) {
    ASSERT(!is_finalized() && position_ + n < buffer_.length());
    ASSERT(static_cast<size_t>(n) <= strlen(s));
    memmove(&buffer_[position_], s, n * kCharSize);
    position_ += n;
  }


  
  
  void AddPadding(char c, int count) {
    for (int i = 0; i < count; i++) {
      AddCharacter(c);
    }
  }

  
  char* Finalize() {
    ASSERT(!is_finalized() && position_ < buffer_.length());
    buffer_[position_] = '\0';
    
    
    ASSERT(strlen(buffer_.start()) == static_cast<size_t>(position_));
    position_ = -1;
    ASSERT(is_finalized());
    return buffer_.start();
  }

 private:
  Vector<char> buffer_;
  int position_;

  bool is_finalized() const { return position_ < 0; }

  DISALLOW_IMPLICIT_CONSTRUCTORS(StringBuilder);
};

























template <class Dest, class Source>
inline Dest BitCast(const Source& source) {
  static_assert(sizeof(Dest) == sizeof(Source),
                "BitCast's source and destination types must be the same size");

  Dest dest;
  memmove(&dest, &source, sizeof(dest));
  return dest;
}

template <class Dest, class Source>
inline Dest BitCast(Source* source) {
  return BitCast<Dest>(reinterpret_cast<uintptr_t>(source));
}

}  

#endif  
