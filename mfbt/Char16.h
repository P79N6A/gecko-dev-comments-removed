







#ifndef mozilla_Char16_h
#define mozilla_Char16_h








#ifdef _MSC_VER
   












#  define MOZ_UTF16_HELPER(s) L##s
#  define _CHAR16T
#  ifdef __cplusplus
     typedef wchar_t char16_t;
#  else
     typedef unsigned short char16_t;
#  endif
   typedef unsigned int char32_t;
#elif defined(__cplusplus) && \
      (__cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__))
   
#  define MOZ_UTF16_HELPER(s) u##s
   



#  define MOZ_CHAR16_IS_NOT_WCHAR
#  ifdef WIN32
#    define MOZ_USE_CHAR16_WRAPPER
#  endif
#elif !defined(__cplusplus)
#  if defined(WIN32)
#    include <yvals.h>
     typedef wchar_t char16_t;
#  else
     




     typedef unsigned short char16_t;
#  endif
#else
#  error "Char16.h requires C++11 (or something like it) for UTF-16 support."
#endif

#ifdef MOZ_USE_CHAR16_WRAPPER
# include <string>
  









class char16ptr_t
{
  private:
    const char16_t* ptr;
    static_assert(sizeof(char16_t) == sizeof(wchar_t), "char16_t and wchar_t sizes differ");

  public:
    char16ptr_t(const char16_t* ptr) : ptr(ptr) {}
    char16ptr_t(const wchar_t* ptr) : ptr(reinterpret_cast<const char16_t*>(ptr)) {}

    
    constexpr char16ptr_t(decltype(nullptr)) : ptr(nullptr) {}

    operator const char16_t*() const {
      return ptr;
    }
    operator const wchar_t*() const {
      return reinterpret_cast<const wchar_t*>(ptr);
    }
    operator const void*() const {
      return ptr;
    }
    operator bool() const {
      return ptr != nullptr;
    }
    operator std::wstring() const {
      return std::wstring(static_cast<const wchar_t*>(*this));
    }

    
    explicit operator char16_t*() const {
      return const_cast<char16_t*>(ptr);
    }
    explicit operator wchar_t*() const {
      return const_cast<wchar_t*>(static_cast<const wchar_t*>(*this));
    }

    




    explicit operator const char*() const {
      return reinterpret_cast<const char*>(ptr);
    }
    explicit operator const unsigned char*() const {
      return reinterpret_cast<const unsigned char*>(ptr);
    }
    explicit operator unsigned char*() const {
      return const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(ptr));
    }
    explicit operator void*() const {
      return const_cast<char16_t*>(ptr);
    }

    
    char16_t operator[](size_t i) const {
      return ptr[i];
    }
    bool operator==(const char16ptr_t &x) const {
      return ptr == x.ptr;
    }
    bool operator==(decltype(nullptr)) const {
      return ptr == nullptr;
    }
    bool operator!=(const char16ptr_t &x) const {
      return ptr != x.ptr;
    }
    bool operator!=(decltype(nullptr)) const {
      return ptr != nullptr;
    }
    char16ptr_t operator+(size_t add) const {
      return char16ptr_t(ptr + add);
    }
    ptrdiff_t operator-(const char16ptr_t &other) const {
      return ptr - other.ptr;
    }
};

inline decltype((char*)0-(char*)0)
operator-(const char16_t* x, const char16ptr_t y) {
  return x - static_cast<const char16_t*>(y);
}

#else

typedef const char16_t* char16ptr_t;

#endif


#define __PRUNICHAR__
typedef char16_t PRUnichar;









#define MOZ_UTF16(s) MOZ_UTF16_HELPER(s)

#if defined(__cplusplus) && \
    (__cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__))
static_assert(sizeof(char16_t) == 2, "Is char16_t type 16 bits?");
static_assert(char16_t(-1) > char16_t(0), "Is char16_t type unsigned?");
static_assert(sizeof(MOZ_UTF16('A')) == 2, "Is char literal 16 bits?");
static_assert(sizeof(MOZ_UTF16("")[0]) == 2, "Is string char 16 bits?");
#endif

#endif 
